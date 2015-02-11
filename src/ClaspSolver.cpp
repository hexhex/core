/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * Copyright (C) 2011, 2012, 2013, 2014 Christoph Redl
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with dlvhex; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */

/**
 * @file   ClaspSolver.cpp
 * @author Christoph Redl
 * @author Peter Schueller <peterschueller@sabanciuniv.edu> (performance improvements, incremental model update)
 *
 * @brief Interface to genuine clasp 3.0.0-based Solver.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_LIBCLASP

#include "dlvhex2/ClaspSolver.h"

#include <iostream>
#include <sstream>
#include "dlvhex2/Logger.h"
#include "dlvhex2/GenuineSolver.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Set.h"
#include "dlvhex2/UnfoundedSetChecker.h"
#include "dlvhex2/AnnotatedGroundProgram.h"
#include "dlvhex2/Benchmarking.h"

#include <boost/foreach.hpp>
#include <boost/graph/strong_components.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/tokenizer.hpp>

#include "clasp/solver.h"
#include "clasp/clause.h"
#include "clasp/literal.h"
#include "program_opts/program_options.h"
#include "program_opts/application.h"


// activate this for detailed benchmarking in this file 
#undef DLVHEX_CLASPSOLVER_PROGRAMINIT_BENCHMARKING


// @clasp examples: Run the following command to use the Makefile in clasp/libclasp/examples:
//   export CXXFLAGS="-DWITH_THREADS=0 -I $PWD/../ -I $PWD/../../libprogram_opts/"

/*

Variable Namespaces
===================

The ClaspSolver class uses three different name spaces for variables:
 (i)   HEX-IDs
 (ii)  clasp program variables
 (iii) clasp solver variables

There is a one-to-one correlation between (i) and (ii)
(except for an additional clasp variable which is permantly set to false to express empty rule heads),
while the relation between (ii) and (iii) is many-to-zero/one. This is because
program variables can be eliminated due to optimization, or multiple program variables can be identified
to be equivalent and are thus mapped to the same internal solver variable.

That is:
	(i) <1--1> (ii) <N--0/1> (iii)

It is important to know when to use which namespace. All classes of the HEX-solver other than this one use only (i).
When sending an ASP program to clasp or calling clasp functions related to ASP program variables (such as defining programs or freezing external variables),
it expects (ii). When sending clauses/nogoods to clasp, it expects the literals to use (iii).
Also when retrieving models from clasp, the result is represented using (iii).
Note that (ii) is only relevant in ASP mode, whereas SAT mode uses only (i) and (iii).

We have the following conversion options:

(i) ---> (ii)   Translating a HEX-ID "id" (i) to a clasp program variable (ii) is via convertHexToClaspProgramLit(id.address).

                Example usages: sending programs to clasp, adding new rules

(i) ---> (iii)  Translating a HEX-ID "id" (i) to a clasp solver variable (iii) is via convertHexToClaspSolverLit(id.address).

                Example usages: sending nogoods to clasp, external learning

(ii) -/-> (i)   Unsupported/not needed (addition would be easy)

(ii) -/-> (iii) Unsupported/not needed (addition would be easy)

(iii) ---> (i)  Translating a positive or negative clasp solver variable "lit" (iii) to the list of address parts of a HEX-ID (i) of type ground atom
                is via convertClaspSolverLitToHex(lit.index()); this returns a pointer to a std::vector<IDAddress>.

                Example usages: assignment extraction

(iii) -/-> (ii) Unsupported/not needed, but indirectly possible via (iii) --> (i) --> (ii)


*/

DLVHEX_NAMESPACE_BEGIN

// ============================== ExternalPropagator ==============================

ClaspSolver::ExternalPropagator::ExternalPropagator(ClaspSolver& cs) : cs(cs){

	cs.claspctx.master()->addPost(this);
	startAssignmentExtraction();

	// initialize propagation deferring
	lastPropagation = boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time());
	int deferMS = cs.ctx.config.getOption("ClaspDeferMaxTMilliseconds");
	skipMaxDuration = boost::posix_time::microseconds(deferMS * 1000);

	skipAmount = cs.ctx.config.getOption("ClaspDeferNPropagations");
	skipCounter = 0;
}

ClaspSolver::ExternalPropagator::~ExternalPropagator(){
	stopAssignmentExtraction();
	cs.claspctx.master()->removePost(this);
}

void ClaspSolver::ExternalPropagator::startAssignmentExtraction(){

	DBGLOG(DBG, "Initializing assignment extraction");
	currentIntr = InterpretationPtr(new Interpretation(cs.reg));
	currentAssigned = InterpretationPtr(new Interpretation(cs.reg));
	currentChanged = InterpretationPtr(new Interpretation(cs.reg));

	// we need to extract the full assignment (only this time), to make sure that we did not miss any updates before initialization of this extractor
	DBGLOG(DBG, "Extracting full interpretation from clasp");
	cs.extractClaspInterpretation(*cs.claspctx.master(), currentIntr, currentAssigned, currentChanged);

	// add watches for all literals and decision levels
	for (Clasp::SymbolTable::const_iterator it = cs.claspctx.master()->symbolTable().begin(); it != cs.claspctx.master()->symbolTable().end(); ++it) {
		// skip eliminated variables
		if (cs.claspctx.eliminated(it->second.lit.var())) continue;

		uint32_t level = cs.claspctx.master()->level(it->second.lit.var());
		if (cs.claspToHex.size() > it->second.lit.index()){
			BOOST_FOREACH (IDAddress adr, *cs.convertClaspSolverLitToHex(it->second.lit.index())){
				// add the variable to the undo watch for the decision level on which it was assigned
				while (assignmentsOnDecisionLevel.size() < level + 1){
					assignmentsOnDecisionLevel.push_back(std::vector<IDAddress>());
					DBGLOG(DBG, "Adding undo watch to level " << level);
					if (level > 0) cs.claspctx.master()->addUndoWatch(level, this);
				}
			}
		}

		DBGLOG(DBG, "Adding watch for literal C:" << it->second.lit.index() << "/" << (it->second.lit.sign() ? "!" : "") << it->second.lit.var() << " and its negation");
		cs.claspctx.master()->addWatch(it->second.lit, this);
		cs.claspctx.master()->addWatch(Clasp::Literal(it->second.lit.var(), !it->second.lit.sign()), this);
	}
}

void ClaspSolver::ExternalPropagator::stopAssignmentExtraction(){

	// remove watches for all literals
	for (Clasp::SymbolTable::const_iterator it = cs.claspctx.master()->symbolTable().begin(); it != cs.claspctx.master()->symbolTable().end(); ++it) {
		// skip eliminated variables
		if (cs.claspctx.eliminated(it->second.lit.var())) continue;

		DBGLOG(DBG, "Removing watch for literal C:" << it->second.lit.index() << "/" << (it->second.lit.sign() ? "!" : "") << it->second.lit.var() << " and its negation");
		cs.claspctx.master()->removeWatch(it->second.lit, this);
		cs.claspctx.master()->removeWatch(Clasp::Literal(it->second.lit.var(), !it->second.lit.sign()), this);
	}

	// remove watches for all decision levels
	for (uint32_t i = 1; i < assignmentsOnDecisionLevel.size(); i++){
		//if (cs.claspctx.master()->validLevel(i)){
			DBGLOG(DBG, "Removing watch for decision level " << i);
			cs.claspctx.master()->removeUndoWatch(i, this);
		//}
	}

	currentIntr.reset();
	currentAssigned.reset();
	currentChanged.reset();
}

void ClaspSolver::ExternalPropagator::callHexPropagators(Clasp::Solver& s){

	DBGLOG(DBG, "ExternalPropagator: Calling HEX-Propagator");
#ifndef NDEBUG
	// extract model and compare with the incrementally extracted one
	InterpretationPtr fullyExtractedCurrentIntr = InterpretationPtr(new Interpretation(cs.reg));
	InterpretationPtr fullyExtractedCurrentAssigned = InterpretationPtr(new Interpretation(cs.reg));
	cs.extractClaspInterpretation(s, fullyExtractedCurrentIntr, fullyExtractedCurrentAssigned, InterpretationPtr());

	std::stringstream ss;
	ss << "Partial assignment: " << *fullyExtractedCurrentIntr << " (assigned: " << *fullyExtractedCurrentAssigned << ")" << std::endl;
	ss << "Incrementally extracted partial assignment: " << *currentIntr << " (assigned: " << *currentAssigned << ")" << std::endl;
	ss << "Changed atoms: " << *currentChanged;
	DBGLOG(DBG, ss.str());
	assert (fullyExtractedCurrentIntr->getStorage() == currentIntr->getStorage() && fullyExtractedCurrentAssigned->getStorage() == currentAssigned->getStorage());

	int propNr = 0;
#endif

	// call HEX propagators
	DBGLOG(DBG, "Need to call " << cs.propagators.size() << " propagators");
	BOOST_FOREACH (PropagatorCallback* propagator, cs.propagators){
		DBGLOG(DBG, "ExternalPropagator: Calling HEX-Propagator #" << (++propNr));
		propagator->propagate(currentIntr, currentAssigned, currentChanged);
	}
	currentChanged->clear();
}

bool ClaspSolver::ExternalPropagator::addNewNogoodsToClasp(Clasp::Solver& s){

	assert (!s.hasConflict() && "tried to add new nogoods while solver is in conflict");

	// add new clauses to clasp
	DBGLOG(DBG, "ExternalPropagator: Adding new clauses to clasp (" << cs.nogoods.size() << " were prepared)");
	bool inconsistent = false;
	Clasp::ClauseCreator cc(cs.claspctx.master());
	std::list<Nogood>::iterator ngIt = cs.nogoods.begin();
	while (ngIt != cs.nogoods.end()){
		const Nogood& ng = *ngIt;

		TransformNogoodToClaspResult ngClasp = cs.nogoodToClaspClause(ng);
		if (!ngClasp.tautological && !ngClasp.outOfDomain){
			DBGLOG(DBG, "Adding learned nogood " << ng.getStringRepresentation(cs.reg) << " to clasp");
			cc.start(Clasp::Constraint_t::learnt_other);
			BOOST_FOREACH (Clasp::Literal lit, ngClasp.clause){
				cc.add(lit);
			}

			Clasp::ClauseInfo ci(Clasp::Constraint_t::learnt_other);
			assert (!s.hasConflict() && (s.decisionLevel() == 0 || ci.learnt()));
			Clasp::ClauseCreator::Result res = cc.create(s, cc.lits(), 0, ci);
			DBGLOG(DBG, "Assignment is " << (res.ok() ? "not " : "") << "conflicting, new clause is " << (res.unit() ? "" : "not ") << "unit");

			if (!res.ok()){
				inconsistent = true;
				break;
			}
		}
		ngIt++;
	}

	// remove all processed nogoods
	cs.nogoods.erase(cs.nogoods.begin(), ngIt);
	assert(!inconsistent || s.hasConflict());
	return inconsistent;
}

bool ClaspSolver::ExternalPropagator::propagateFixpoint(Clasp::Solver& s, Clasp::PostPropagator* ctx){

	DBGLOG(DBG, "ExternalPropagator::propagateFixpoint");

	// check if we shall propagate
	bool hexPropagate = false;
	boost::posix_time::ptime now(boost::posix_time::microsec_clock::local_time());
	if(now - lastPropagation > skipMaxDuration){
		DBGLOG(DBG,"we shall propagate to HEX because skipMaxDuration=" << skipMaxDuration <<
		           " < time of last propgation, now=" << now << ", lastPropagation=" << lastPropagation);
		lastPropagation = now;
		skipCounter = 0;
		hexPropagate = true;
	}else{
		if(skipCounter >= skipAmount){
			DBGLOG(DBG,"we shall propagate to HEX because skipCounter=" << skipCounter <<
				   " >= skipAmount=" << skipAmount);
			lastPropagation = now;
			skipCounter = 0;
			hexPropagate = true;
		}
	}
	DBGLOG(DBG, "Will " << (hexPropagate ? "" : "not ") << "propagate to HEX");

	for (;;){
		if (hexPropagate) callHexPropagators(s);
		if (addNewNogoodsToClasp(s)){
			DBGLOG(DBG, "Propagation led to conflict");
			assert(s.queueSize() == 0 || s.hasConflict());
			return false;
		}
		if (s.queueSize() == 0){
			DBGLOG(DBG, "Nothing more to propagate");
			assert(s.queueSize() == 0 || s.hasConflict());
			return true;
		}
		if (!s.propagateUntil(this)){
			DBGLOG(DBG, "Propagated something, rescheduling previous propagators");
			assert(s.queueSize() == 0 || s.hasConflict());
			return false;
		}
	}
}

bool ClaspSolver::ExternalPropagator::isModel(Clasp::Solver& s){

	DBGLOG(DBG, "ExternalPropagator::isModel");

	// here we must call the HEX propagators to make sure that the verification status of external atoms is correct
	// after this method returns
	DBGLOG(DBG, "Must propagate to HEX");
	callHexPropagators(s);
	if (addNewNogoodsToClasp(s)) return false;
	return s.numFreeVars() == 0 && !s.hasConflict();
}

uint32_t ClaspSolver::ExternalPropagator::priority() const{
	return Clasp::PostPropagator::priority_class_general;
}

Clasp::Constraint::PropResult ClaspSolver::ExternalPropagator::propagate(Clasp::Solver& s, Clasp::Literal p, uint32& data){

	DBGLOG(DBG, "ExternalPropagator::propagate");
	Clasp::Literal pneg(p.var(), !p.sign());

	assert(s.isTrue(p) && s.isFalse(pneg));

	uint32_t level = s.level(p.var());

	DBGLOG(DBG, "Clasp notified about literal C:" << p.index() << "/" << (p.sign() ? "!" : "") << p.var() << " becoming true on dl " << level);
	if (cs.claspToHex.size() > p.index()){
		BOOST_FOREACH (IDAddress adr, *cs.convertClaspSolverLitToHex(p.index())){
			DBGLOG(DBG, "Assigning H:" << adr << " to true");
			currentIntr->setFact(adr);
			currentAssigned->setFact(adr);
			currentChanged->setFact(adr);

			// add the variable to the undo watch for the decision level on which it was assigned
			while (assignmentsOnDecisionLevel.size() < level + 1){
				if(assignmentsOnDecisionLevel.size() > 0){
					DBGLOG(DBG, "Adding undo watch to level " << assignmentsOnDecisionLevel.size());
					cs.claspctx.master()->addUndoWatch(assignmentsOnDecisionLevel.size(), this);
				}
				assignmentsOnDecisionLevel.push_back(std::vector<IDAddress>());
			}
			assignmentsOnDecisionLevel[level].push_back(adr);
		}
	}

	DBGLOG(DBG, "This implies that literal C:" << pneg.index() << "/" << (pneg.sign() ? "!" : "") << p.var() << " becomes false on dl " << level);
	if (cs.claspToHex.size() > pneg.index()){
		BOOST_FOREACH (IDAddress adr, *cs.convertClaspSolverLitToHex(pneg.index())){
			DBGLOG(DBG, "Assigning H:" << adr << " to false");
			currentIntr->clearFact(adr);
			currentAssigned->setFact(adr);
			currentChanged->setFact(adr);

			// add the variable to the undo watch for the decision level on which it was assigned
			while (assignmentsOnDecisionLevel.size() < level + 1){
				if(assignmentsOnDecisionLevel.size() > 0) cs.claspctx.master()->addUndoWatch(assignmentsOnDecisionLevel.size(), this);
				assignmentsOnDecisionLevel.push_back(std::vector<IDAddress>());
			}
			assignmentsOnDecisionLevel[level].push_back(adr);
		}
	}
	DBGLOG(DBG, "ExternalPropagator::propagate finished");
	return PropResult();
}

void ClaspSolver::ExternalPropagator::undoLevel(Clasp::Solver& s){

	DBGLOG(DBG, "Backtracking to decision level " << s.decisionLevel());
	for (uint32_t i = s.decisionLevel(); i < assignmentsOnDecisionLevel.size(); i++){
		DBGLOG(DBG, "Undoing decision level " << i);
		BOOST_FOREACH (IDAddress adr, assignmentsOnDecisionLevel[i]){
			DBGLOG(DBG, "Unassigning H:" << adr);
			currentIntr->clearFact(adr);
			currentAssigned->clearFact(adr);
			currentChanged->setFact(adr);
		}
	}
	assignmentsOnDecisionLevel.erase(assignmentsOnDecisionLevel.begin() + s.decisionLevel(), assignmentsOnDecisionLevel.end());
}

// ============================== ClaspSolver ==============================

std::string ClaspSolver::idAddressToString(IDAddress adr){
	std::stringstream ss;
	ss << adr;
	return ss.str();
}

IDAddress ClaspSolver::stringToIDAddress(std::string str){
#ifndef NDEBUG
	if (str.find(":") != std::string::npos){
		str = str.substr(0, str.find(":"));
	}
#endif
	return atoi(str.c_str());
}

void ClaspSolver::extractClaspInterpretation(Clasp::Solver& solver, InterpretationPtr extractCurrentIntr, InterpretationPtr extractCurrentAssigned, InterpretationPtr extractCurrentChanged){

	DBGLOG(DBG, "Extracting clasp interpretation");
	if (!!extractCurrentIntr) extractCurrentIntr->clear();
	if (!!extractCurrentAssigned) extractCurrentAssigned->clear();
	for (Clasp::SymbolTable::const_iterator it = solver.symbolTable().begin(); it != solver.symbolTable().end(); ++it) {
		// skip eliminated variables
		if (claspctx.eliminated(it->second.lit.var())) continue;

		if (solver.isTrue(it->second.lit) && !it->second.name.empty()) {
			DBGLOG(DBG, "Literal C:" << it->second.lit.index() << "/" << (it->second.lit.sign() ? "!" : "") << it->second.lit.var() << "@" <<
			             solver.level(it->second.lit.var()) << (claspctx.eliminated(it->second.lit.var()) ? "(elim)" : "") << ",H:" << it->second.name.c_str() << " is true");
			BOOST_FOREACH (IDAddress adr, *convertClaspSolverLitToHex(it->second.lit.index())){
				if (!!extractCurrentIntr) extractCurrentIntr->setFact(adr);
				if (!!extractCurrentAssigned) extractCurrentAssigned->setFact(adr);
				if (!!extractCurrentChanged) extractCurrentChanged->setFact(adr);
			}
		}
		if (solver.isFalse(it->second.lit) && !it->second.name.empty()) {
			DBGLOG(DBG, "Literal C:" << it->second.lit.index() << "/" << (it->second.lit.sign() ? "!" : "") << it->second.lit.var() << "@" <<
			             solver.level(it->second.lit.var()) << (claspctx.eliminated(it->second.lit.var()) ? "(elim)" : "") << ",H:" << it->second.name.c_str() << " is false");
			BOOST_FOREACH (IDAddress adr, *convertClaspSolverLitToHex(it->second.lit.index())){
				if (!!extractCurrentAssigned) extractCurrentAssigned->setFact(adr);
				if (!!extractCurrentChanged) extractCurrentChanged->setFact(adr);
			}
		}
	}
}

// freezes all variables in "frozen"; if the pointer is 0, then all variables are frozen
void ClaspSolver::freezeVariables(InterpretationConstPtr frozen, bool freezeByDefault){

	if (!!frozen){
		DBGLOG(DBG, "Setting selected variables to frozen: " << *frozen);

#ifndef NDEBUG
		int cntFrozen = 0;
		std::set<int> alreadyFrozen;
#endif
		bm::bvector<>::enumerator en = frozen->getStorage().first();
		bm::bvector<>::enumerator en_end = frozen->getStorage().end();
		while (en < en_end){
			if (isMappedToClaspLiteral(*en)){
#ifndef NDEBUG
				if (alreadyFrozen.count(hexToClaspSolver[*en].var()) == 0){
					cntFrozen++;
					alreadyFrozen.insert(hexToClaspSolver[*en].var());
				}
#endif
				switch (problemType) {
					case ASP:
						// Note: (1) asp.free() does more than (2) claspctx.setFrozen().
						// (2) prevents the internal solver variable from being eliminated due to optimization
						// (1) does in addition prevent Clark's completion an loop clauses for these variables from being added (now), which
						//     allows for defining them in later incremental steps
						asp.freeze(convertHexToClaspProgramLit(*en).var(), Clasp::value_false);
						break;
					case SAT:
						claspctx.setFrozen(convertHexToClaspSolverLit(*en).var(), true);
						break;
					default:
						assert(false && "unknown problem type");
				}
			}
			en++;
		}
#ifndef NDEBUG
		DBGLOG(DBG, "Setting " << cntFrozen << " out of " << claspctx.numVars() << " variables to frozen");
#endif
	}else{
		if (freezeByDefault){
			DBGLOG(DBG, "Setting all " << claspctx.numVars() << " variables to frozen");
			for (uint32_t i = 1; i <= claspctx.numVars(); i++){

				switch (problemType) {
					case ASP:
						asp.freeze(i, Clasp::value_false);
						break;
					case SAT:
						claspctx.setFrozen(i, true);
						break;
					default:
						assert(false && "unknown problem type");
				}
			}
		}
	}
}

void ClaspSolver::sendWeightRuleToClasp(Clasp::Asp::LogicProgram& asp, ID ruleId){
	#ifdef DLVHEX_CLASPSOLVER_PROGRAMINIT_BENCHMARKING
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::sendWeightRuleTC");
	#endif

	const Rule& rule = reg->rules.getByID(ruleId);
	asp.startRule(Clasp::Asp::WEIGHTRULE, rule.bound.address);
	assert(rule.head.size() != 0);
	BOOST_FOREACH (ID h, rule.head){
		// add literal to head
		DBGLOG(DBG, "Adding to head: " << convertHexToClaspProgramLit(h.address).var());
		asp.addHead(convertHexToClaspProgramLit(h.address).var());
	}
	for (uint32_t i = 0; i < rule.body.size(); ++i){
		// add literal to body
		DBGLOG(DBG, "Adding to body: " << (!(convertHexToClaspProgramLit(rule.body[i].address).sign() ^ rule.body[i].isNaf()) ? "" : "!") << convertHexToClaspProgramLit(rule.body[i].address).var());
		asp.addToBody(convertHexToClaspProgramLit(rule.body[i].address).var(), !(convertHexToClaspProgramLit(rule.body[i].address).sign() ^ rule.body[i].isNaf()), rule.bodyWeightVector[i].address);
	}
	asp.endRule();
}

void ClaspSolver::sendOrdinaryRuleToClasp(Clasp::Asp::LogicProgram& asp, ID ruleId){
	#ifdef DLVHEX_CLASPSOLVER_PROGRAMINIT_BENCHMARKING
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::sendOrdinaryRuleTC");
	#endif

	const Rule& rule = reg->rules.getByID(ruleId);
	asp.startRule(rule.head.size() > 1 ? Clasp::Asp::DISJUNCTIVERULE : Clasp::Asp::BASICRULE);
	if (rule.head.size() == 0){
		asp.addHead(false_);
	}
	BOOST_FOREACH (ID h, rule.head){
		// add literal to head
		DBGLOG(DBG, "Adding to head: " << convertHexToClaspProgramLit(h.address).var());
		asp.addHead(convertHexToClaspProgramLit(h.address).var());
	}
	BOOST_FOREACH (ID b, rule.body){
		// add literal to body
		DBGLOG(DBG, "Adding to body: " << (!(convertHexToClaspProgramLit(b.address).sign() ^ b.isNaf()) ? "" : "!") << convertHexToClaspProgramLit(b.address).var());
		asp.addToBody(convertHexToClaspProgramLit(b.address).var(), !(convertHexToClaspProgramLit(b.address).sign() ^ b.isNaf()));
	}
	asp.endRule();
}

void ClaspSolver::sendRuleToClasp(Clasp::Asp::LogicProgram& asp, ID ruleId){
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::sendRuleTC");
	const Rule& rule = reg->rules.getByID(ruleId);

	if (ID(rule.kind, 0).isWeakConstraint()) throw GeneralError("clasp-based solver does not support weak constraints");

	DBGLOG(DBG, "sendRuleToClasp " << printToString<RawPrinter>(ruleId, reg));

	// distinct by the type of the rule
	if (ID(rule.kind, 0).isWeightRule()){
		sendWeightRuleToClasp(asp, ruleId);
	}else{
		sendOrdinaryRuleToClasp(asp, ruleId);
	}
}

void ClaspSolver::sendProgramToClasp(const AnnotatedGroundProgram& p, InterpretationConstPtr frozen){
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::sendProgramTC");

	asp.start(claspctx, config.asp);
	asp.setNonHcfConfiguration(config.testerConfig());

	// The following two lines allow for defining the program incrementally
	// Note: If the program is to be defined incrementally, then updateProgram() must be called before each extension, even for the first one
	asp.updateProgram();

	false_ = nextVar++;
	asp.setCompute(false_, false);

	// transfer edb
	DBGLOG(DBG, "Sending EDB to clasp: " << *p.getGroundProgram().edb);
	bm::bvector<>::enumerator en = p.getGroundProgram().edb->getStorage().first();
	bm::bvector<>::enumerator en_end = p.getGroundProgram().edb->getStorage().end();
	while (en < en_end){
		// add fact
		asp.startRule(Clasp::Asp::BASICRULE).addHead(convertHexToClaspProgramLit(*en).var()).endRule();
		en++;
	}

	// transfer idb
	DBGLOG(DBG, "Sending IDB to clasp");
	BOOST_FOREACH (ID ruleId, p.getGroundProgram().idb){

/*
//Incremental Solving:

inconsistent = !asp.endProgram();

DBGLOG(DBG, "SAT instance has " << claspctx.numVars() << " variables");
DBGLOG(DBG, "Instance is now " << (inconsistent ? "" : "not ") << "inconsistent");

if (inconsistent){
	DBGLOG(DBG, "Program is inconsistent, aborting initialization");
	inconsistent = true;
	return;
}

DBGLOG(DBG, "Prepare new model enumerator");
modelEnumerator.reset(config.solve.createEnumerator(config.solve));
modelEnumerator->init(claspctx, 0, config.solve.numModels);

DBGLOG(DBG, "Finalizing reinitialization");
if (!claspctx.endInit()){
	DBGLOG(DBG, "Program is inconsistent, aborting initialization");
	inconsistent = true;
	return;
}

DBGLOG(DBG, "Resetting solver object");
solve.reset(new Clasp::BasicSolve(*claspctx.master()));
enumerationStarted = false;

asp.update();
*/

		sendRuleToClasp(asp, ruleId);
	}

	freezeVariables(frozen, false /* do not freeze variables by default */);

	inconsistent = !asp.endProgram();

	DBGLOG(DBG, "ASP instance has " << claspctx.numVars() << " variables");

	DBGLOG(DBG, "Instance is " << (inconsistent ? "" : "not ") << "inconsistent");

#ifndef NDEBUG
	std::stringstream ss;
	asp.write(ss);
	DBGLOG(DBG, "Program in LParse format: " << ss.str());
#endif
}

void ClaspSolver::sendNogoodSetToClasp(const NogoodSet& ns, InterpretationConstPtr frozen){

	#ifdef DLVHEX_CLASPSOLVER_PROGRAMINIT_BENCHMARKING
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::sendNogoodSetTC");
	#endif

	DBGLOG(DBG, "Sending NogoodSet to clasp: " << ns);

	sat.startProgram(claspctx);

	prepareProblem(sat, ns);
	updateSymbolTable();

	for (int i = 0; i < ns.getNogoodCount(); i++){
		const Nogood& ng = ns.getNogood(i);
		TransformNogoodToClaspResult ngClasp = nogoodToClaspClause(ng, true);
		assert (!ngClasp.outOfDomain && "literals were not properly mapped to clasp");
		if (!ngClasp.tautological){
			DBGLOG(DBG, "Adding nogood " << ng << " as clasp-clause");

			sat.addClause(ngClasp.clause);
		}else{
			DBGLOG(DBG, "Skipping tautological nogood");
		}
	}

	inconsistent = !sat.endProgram();
	DBGLOG(DBG, "SAT instance has " << claspctx.numVars() << " variables");

	DBGLOG(DBG, "Instance is " << (inconsistent ? "" : "not ") << "inconsistent");

	freezeVariables(frozen, true /* freeze variables by default */);
}

void ClaspSolver::interpretClaspCommandline(Clasp::Problem_t::Type type){

	DBGLOG(DBG, "Interpreting clasp command-line");

	std::string claspconfigstr = ctx.config.getStringOption("ClaspConfiguration");
	if( claspconfigstr == "none" ) claspconfigstr = "";
	if( claspconfigstr == "frumpy"
	 || claspconfigstr == "jumpy"
	 || claspconfigstr == "handy"
	 || claspconfigstr == "crafty"
	 || claspconfigstr == "trendy") claspconfigstr = "--configuration=" + claspconfigstr;
	// otherwise let the config string itself be parsed by clasp

	DBGLOG(DBG, "Found configuration: " << claspconfigstr);
	try{
		// options found in command-line
		allOpts = std::auto_ptr<ProgramOptions::OptionContext>(new ProgramOptions::OptionContext("<clasp_dlvhex>"));
		DBGLOG(DBG, "Configuring options");
		config.reset();
		config.addOptions(*allOpts);
		DBGLOG(DBG, "Parsing command-line");
		parsedValues = std::auto_ptr<ProgramOptions::ParsedValues>(new ProgramOptions::ParsedValues(*allOpts));
		*parsedValues = ProgramOptions::parseCommandString(claspconfigstr, *allOpts);
		DBGLOG(DBG, "Assigning specified values");
		parsedOptions.assign(*parsedValues);
		DBGLOG(DBG, "Assigning defaults");
		allOpts->assignDefaults(parsedOptions);

		DBGLOG(DBG, "Applying options");
		config.finalize(parsedOptions, type, true);
		config.solve.numModels = 0;
		claspctx.setConfiguration(&config, false);

		DBGLOG(DBG, "Finished option parsing");
	}catch(...){
		DBGLOG(DBG, "Could not parse clasp options: " + claspconfigstr);
		throw;
	}
}

void ClaspSolver::shutdownClasp(){
	if (!!ep.get()) ep.reset();
	resetAndResizeClaspToHex(0);
}

ClaspSolver::TransformNogoodToClaspResult ClaspSolver::nogoodToClaspClause(const Nogood& ng, bool extendDomainIfNecessary) {

#ifndef NDEBUG
	DBGLOG(DBG, "Translating nogood " << ng.getStringRepresentation(reg) << " to clasp");
	std::stringstream ss;
	ss << "{ ";
	bool first = true;
#endif

	// translate dlvhex::Nogood to clasp clause
	Set<uint32> pos;
	Set<uint32> neg;
	Clasp::LitVec clause;
	bool taut = false;
	BOOST_FOREACH (ID lit, ng){

		// only nogoods are relevant where all variables occur in this clasp instance
		if (!isMappedToClaspLiteral(lit.address) && !extendDomainIfNecessary){
			DBGLOG(DBG, "some literal was not properly mapped to clasp");
			return TransformNogoodToClaspResult(Clasp::LitVec(), false, true);
		}

		// mclit = mapped clasp literal
		const Clasp::Literal mclit = convertHexToClaspSolverLit(lit.address, extendDomainIfNecessary);
		if (claspctx.eliminated(mclit.var())){
			DBGLOG(DBG, "some literal was eliminated");
			return TransformNogoodToClaspResult(clause, false, true);
		}

		// avoid duplicate literals
		// if the literal was already added with the same sign, skip it
		// if it was added with different sign, cancel adding the clause
		if (!(mclit.sign() ^ lit.isNaf())){
			if (pos.contains(mclit.var())){
				continue;
			}else if (neg.contains(mclit.var())){
				taut = true;
			}
			pos.insert(mclit.var());
		}else{
			if (neg.contains(mclit.var())){
				continue;
			}else if (pos.contains(mclit.var())){
				taut = true;
			}
			neg.insert(mclit.var());
		}

		// 1. cs.hexToClaspSolver maps hex-atoms to clasp-literals
		// 2. the sign must be changed if the hex-atom was default-negated (xor ^)
		// 3. the overall sign must be changed (negation !) because we work with nogoods and clasp works with clauses
		Clasp::Literal clit = Clasp::Literal(mclit.var(), !(mclit.sign() ^ lit.isNaf()));
		clause.push_back(clit);
#ifndef NDEBUG
		if (!first) ss << ", ";
		first = false;
		ss << (clit.sign() ? "!" : "") << clit.var();
#endif
	}

#ifndef NDEBUG
	ss << " } (" << (taut ? "" : "not ") << "tautological)";
	DBGLOG(DBG, "Clasp clause is: " << ss.str());
#endif

	return TransformNogoodToClaspResult(clause, taut, false);
}

void ClaspSolver::prepareProblem(Clasp::Asp::LogicProgram& asp, const OrdinaryASPProgram& p){

	assert(false && "Deprecated. Due to use of Clasp::LogicProgram for initialization, this method does not need to be called anymore");

	#ifdef DLVHEX_CLASPSOLVER_PROGRAMINIT_BENCHMARKING
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::prepareProblem P pb");
	#endif

	DBGLOG(DBG, "Building atom index");

	// edb
	bm::bvector<>::enumerator en = p.edb->getStorage().first();
	bm::bvector<>::enumerator en_end = p.edb->getStorage().end();
	while (en < en_end){
		if (!isMappedToClaspLiteral(*en)){
			uint32_t c = *en + 2;
			DBGLOG(DBG, "Clasp index of atom H:" << *en << " is " << c);

			// create positive literal -> false
			storeHexToClasp(*en, Clasp::Literal(c, false));

			std::string str = idAddressToString(*en);
			asp.setAtomName(c, str.c_str());
		}
		en++;
	}

	// idb
	BOOST_FOREACH (ID ruleId, p.idb){
		const Rule& rule = reg->rules.getByID(ruleId);
		BOOST_FOREACH (ID h, rule.head){
			if (!isMappedToClaspLiteral(h.address)){
				uint32_t c = h.address + 2;
				DBGLOG(DBG, "Clasp index of atom H:" << h.address << " is C:" << c);

				// create positive literal -> false
				storeHexToClasp(h.address, Clasp::Literal(c, false));

				std::string str = idAddressToString(h.address);
				asp.setAtomName(c, str.c_str());
			}
		}
		BOOST_FOREACH (ID b, rule.body){
			if (!isMappedToClaspLiteral(b.address)){
				uint32_t c = b.address + 2;
				DBGLOG(DBG, "Clasp index of atom H:" << b.address << " is C:" << c);

				// create positive literal -> false
				storeHexToClasp(b.address, Clasp::Literal(c, false));

				std::string str = idAddressToString(b.address);
				asp.setAtomName(c, str.c_str());
			}
		}
	}
}

void ClaspSolver::prepareProblem(Clasp::SatBuilder& sat, const NogoodSet& ns){
	#ifdef DLVHEX_CLASPSOLVER_PROGRAMINIT_BENCHMARKING
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::prepareProblem ns");
	#endif

	DBGLOG(DBG, "Building atom index");
	DBGLOG(DBG, "symbol table has " << claspctx.symbolTable().size() << " entries");

	bool inverselits = (ctx.config.getOption("ClaspInverseLiterals") != 0);

	assert(hexToClaspSolver.empty());
	hexToClaspSolver.reserve(reg->ogatoms.getSize());

	// build symbol table and hexToClaspSolver
	claspctx.symbolTable().startInit(Clasp::SymbolTable::map_indirect);
	unsigned largestIdx = 0;
	unsigned varCnt = 0;
	for (int i = 0; i < ns.getNogoodCount(); i++){
		const Nogood& ng = ns.getNogood(i);
		BOOST_FOREACH (ID lit, ng){
			Clasp::Literal clasplit = convertHexToClaspSolverLit(lit.address, true, inverselits);
			if (clasplit.index() > largestIdx) largestIdx = clasplit.index();
		}
	}
	claspctx.symbolTable().endInit();
	sat.prepareProblem(claspctx.numVars());
	updateSymbolTable();
}

void ClaspSolver::updateSymbolTable(){

	#ifdef DLVHEX_CLASPSOLVER_PROGRAMINIT_BENCHMARKING
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::updateSymbolTable");
	#endif

	hexToClaspSolver.clear();
	hexToClaspSolver.reserve(reg->ogatoms.getSize());

	// go through clasp symbol table
	const Clasp::SymbolTable& symTab = claspctx.symbolTable();

	// each variable can be a positive or negative literal, literals are (var << 1 | sign)
	// build empty set of NULL-pointers to vectors
	// (literals which are internal variables and have no HEX equivalent do not show up in symbol table)
#ifndef NDEBUG
	if (problemType == ASP){
		DBGLOG(DBG, "ASP problem has " << claspctx.numVars() << " variables");
	}else if (problemType == SAT){
		DBGLOG(DBG, "SAT problem has " << claspctx.numVars() << " variables");
	}
#endif
	resetAndResizeClaspToHex(claspctx.numVars() * 2 + 1 + 1);	// the largest possible index is "claspctx.numVars() * 2 + 1", thus we allocate one element more

	LOG(DBG, "Symbol table of optimized program:");
	for (Clasp::SymbolTable::const_iterator it = symTab.begin(); it != symTab.end(); ++it) {
		std::string ss(it->second.name.c_str());
		IDAddress hexAdr = stringToIDAddress(it->second.name.c_str());
		storeHexToClasp(hexAdr, it->second.lit);
		DBGLOG(DBG, "H:" << hexAdr << " (" << reg->ogatoms.getByAddress(hexAdr).text <<  ") <--> "
		            "C:" << it->second.lit.index() << "/" << (it->second.lit.sign() ? "!" : "") << it->second.lit.var());
		assert(it->second.lit.index() < claspToHex.size());
		AddressVector* &c2h = claspToHex[it->second.lit.index()];
		c2h->push_back(hexAdr);
	}

	DBGLOG(DBG, "hexToClaspSolver.size()=" << hexToClaspSolver.size() << ", symTab.size()=" << symTab.size());
}

void ClaspSolver::storeHexToClasp(IDAddress addr, Clasp::Literal lit, bool alsoStoreNonoptimized){

	if(addr >= hexToClaspSolver.size()){
		hexToClaspSolver.resize(addr + 1, noLiteral);
	}
	hexToClaspSolver[addr] = lit;

	if (alsoStoreNonoptimized){
		if(addr >= hexToClaspProgram.size()){
			hexToClaspProgram.resize(addr + 1, noLiteral);
		}
		hexToClaspProgram[addr] = lit;
	}
}

void ClaspSolver::resetAndResizeClaspToHex(unsigned size)
{
	DBGLOG(DBG, "resetAndResizeClaspToHex: current size is " << claspToHex.size());
	for(unsigned u = 0; u < claspToHex.size(); ++u)
	{
		if(claspToHex[u]){
			delete claspToHex[u];
			claspToHex[u] = NULL;
		}
	}
	DBGLOG(DBG, "resetAndResizeClaspToHex: resizing to " << size);
	claspToHex.resize(size, NULL);
	for(unsigned u = 0; u < claspToHex.size(); ++u) {
		claspToHex[u] = new AddressVector;
	}
}

Clasp::Literal ClaspSolver::convertHexToClaspSolverLit(IDAddress addr, bool registerVar, bool inverseLits) {
//	DBGLOG(DBG, "Translating HEX address " << addr << " to clasp");
	if (!isMappedToClaspLiteral(addr)){
		uint32_t c = (registerVar ? claspctx.addVar(Clasp::Var_t::atom_var) : nextVar++);
		Clasp::Literal clasplit(c, inverseLits);
		storeHexToClasp(addr, clasplit, true);
		std::string str = idAddressToString(addr);
#ifndef NDEBUG
		str = str + ":" + RawPrinter::toString(reg, reg->ogatoms.getIDByAddress(addr));
#endif
		claspctx.symbolTable().addUnique(c, str.c_str()).lit = clasplit;
//		DBGLOG(DBG, "Creating new clasp variable " << hexToClaspSolver[addr].var() << " for HEX address " << addr);
//	}else{
//		DBGLOG(DBG, "Mapping to " << hexToClaspSolver[addr].var() << " already exists");
	}
	assert(addr < hexToClaspSolver.size());
	assert(hexToClaspSolver[addr] != noLiteral);
	return hexToClaspSolver[addr];
}

Clasp::Literal ClaspSolver::convertHexToClaspProgramLit(IDAddress addr, bool registerVar, bool inverseLits) {
	if (!isMappedToClaspLiteral(addr)){
		uint32_t c = (registerVar ? claspctx.addVar(Clasp::Var_t::atom_var) : nextVar++);
		Clasp::Literal clasplit(c, inverseLits);
		storeHexToClasp(addr, clasplit, true);
		std::string str = idAddressToString(addr);
#ifndef NDEBUG
		str = str + ":" + RawPrinter::toString(reg, reg->ogatoms.getIDByAddress(addr));
#endif
		claspctx.symbolTable().addUnique(c, str.c_str()).lit = clasplit;
	}
	assert(addr < hexToClaspSolver.size());
	assert(hexToClaspSolver[addr] != noLiteral);
	return hexToClaspProgram[addr];
}

const ClaspSolver::AddressVector* ClaspSolver::convertClaspSolverLitToHex(int index){
	return claspToHex[index];
}

void ClaspSolver::outputProject(InterpretationPtr intr){
	if( !!intr && !!projectionMask )
	{
		DBGLOG(DBG, "Projecting " << *intr);
		intr->getStorage() -= projectionMask->getStorage();
		DBGLOG(DBG, "Projected to " << *intr);
	}
}

ClaspSolver::ClaspSolver(ProgramCtx& ctx, const AnnotatedGroundProgram& p, InterpretationConstPtr frozen)
 : ctx(ctx), solve(0), ep(0), modelCount(0), nextVar(2), projectionMask(p.getGroundProgram().mask), noLiteral(Clasp::Literal::fromRep(~0x0)){
	reg = ctx.registry();

	DBGLOG(DBG, "Configure clasp in ASP mode");
	problemType = ASP;
	config.reset();
	interpretClaspCommandline(Clasp::Problem_t::ASP);
	nextSolveStep = Restart;

	claspctx.requestStepVar();
	sendProgramToClasp(p, frozen);

	if (inconsistent){
		DBGLOG(DBG, "Program is inconsistent, aborting initialization");
		inconsistent = true;
		return;
	}

	DBGLOG(DBG, "Prepare model enumerator");
	modelEnumerator.reset(config.solve.createEnumerator(config.solve));
	modelEnumerator->init(claspctx, 0, config.solve.numModels);

	DBGLOG(DBG, "Finalizing initialization");
	if (!claspctx.endInit()){
		DBGLOG(DBG, "Program is inconsistent, aborting initialization");
		inconsistent = true;
		return;
	}

	updateSymbolTable();

	DBGLOG(DBG, "Prepare solver object");
	solve.reset(new Clasp::BasicSolve(*claspctx.master()));
	enumerationStarted = false;

	DBGLOG(DBG, "Adding post propagator");
	ep.reset(new ExternalPropagator(*this));
}

ClaspSolver::ClaspSolver(ProgramCtx& ctx, const NogoodSet& ns, InterpretationConstPtr frozen)
 : ctx(ctx), solve(0), ep(0), modelCount(0), nextVar(2), noLiteral(Clasp::Literal::fromRep(~0x0)){
	reg = ctx.registry();

	DBGLOG(DBG, "Configure clasp in SAT mode");
	problemType = SAT;
	config.reset();
	interpretClaspCommandline(Clasp::Problem_t::SAT);
	nextSolveStep = Restart;

	claspctx.requestStepVar();
	sendNogoodSetToClasp(ns, frozen);

	if (inconsistent){
		DBGLOG(DBG, "Program is inconsistent, aborting initialization");
		inconsistent = true;
		return;
	}

	DBGLOG(DBG, "Prepare model enumerator");
	modelEnumerator.reset(config.solve.createEnumerator(config.solve));
	modelEnumerator->init(claspctx, 0, config.solve.numModels);

	DBGLOG(DBG, "Finalizing initialization");
	if (!claspctx.endInit()){
		DBGLOG(DBG, "SAT instance is unsatisfiable");
		inconsistent = true;
		return;
	}

	updateSymbolTable();

#ifndef NDEBUG
	std::stringstream ss;
	InterpretationPtr vars = InterpretationPtr(new Interpretation(reg));
	for (int ngi = 0; ngi < ns.getNogoodCount(); ++ngi){
		const Nogood& ng = ns.getNogood(ngi);
		TransformNogoodToClaspResult ngClasp = nogoodToClaspClause(ng);		
		bool first = true;
		ss << ":- ";
		for (uint32_t i = 0; i < ngClasp.clause.size(); ++i){
			Clasp::Literal lit = ngClasp.clause[i];
			if (!first) ss << ", ";
			first = false;
			ss << (lit.sign() ? "-" : "") << "a" << lit.var();
			vars->setFact(lit.var());
		}
		if (!first) ss << "." << std::endl;
	}
	bm::bvector<>::enumerator en = vars->getStorage().first();
	bm::bvector<>::enumerator en_end = vars->getStorage().end();
	while (en < en_end){
		ss << "a" << *en << " v -a" << *en << "." << std::endl;
		en++;
	}

	DBGLOG(DBG, "SAT instance in ASP format:" << std::endl << ss.str());
#endif

	DBGLOG(DBG, "Prepare solver object");
	solve.reset(new Clasp::BasicSolve(*claspctx.master()));
	enumerationStarted = false;

	DBGLOG(DBG, "Adding post propagator");
	ep.reset(new ExternalPropagator(*this));
}

ClaspSolver::~ClaspSolver(){
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::~ClaspSolver");
	shutdownClasp();
}

void ClaspSolver::addProgram(const AnnotatedGroundProgram& p, InterpretationConstPtr frozen){

	assert(problemType == ASP && "programs can only be added in ASP mode");
	DBGLOG(DBG, "Adding program component incrementally");
	nextSolveStep = Restart;
	
	// remove post propagator to avoid that it tries the extract the assignment before the symbol table is updated
	if (!!ep.get()) ep.reset();
	//if (!!ep.get()) claspctx.master()->removePost(ep.get());

	// Update program
	//inconsistent |= asp.endProgram();
	asp.updateProgram();

	// transfer added edb
	DBGLOG(DBG, "Sending added EDB to clasp: " << *p.getGroundProgram().edb);
	bm::bvector<>::enumerator en = p.getGroundProgram().edb->getStorage().first();
	bm::bvector<>::enumerator en_end = p.getGroundProgram().edb->getStorage().end();
	while (en < en_end){
		// redundancy check
		if (convertHexToClaspSolverLit(*en).var() > 0){
			// add fact
			asp.startRule(Clasp::Asp::BASICRULE).addHead(convertHexToClaspSolverLit(*en).var()).endRule();
		}
		en++;
	}

	// transfer added idb
	DBGLOG(DBG, "Sending added IDB to clasp");
	BOOST_FOREACH (ID ruleId, p.getGroundProgram().idb){
		// redundancy check
		bool redundant = false;
		BOOST_FOREACH (ID h, reg->rules.getByID(ruleId).head){
			if (convertHexToClaspSolverLit(h.address).var() == 0){
				redundant = true;
				break;
			}
		}

		if (!redundant) sendRuleToClasp(asp, ruleId);
	}

	// finalize update
	freezeVariables(frozen, false /* do not freeze variables by default */);
	inconsistent = !asp.endProgram();
	DBGLOG(DBG, "ASP instance has " << claspctx.numVars() << " variables");
	DBGLOG(DBG, "Instance is now " << (inconsistent ? "" : "not ") << "inconsistent");

	// update projection
	if (!!p.getGroundProgram().mask){
		InterpretationPtr pm = InterpretationPtr(new Interpretation(reg));
		if (!!projectionMask) pm->add(*projectionMask);
		DBGLOG(DBG, "Adding to program mask:" << *p.getGroundProgram().mask);
		pm->add(*p.getGroundProgram().mask);
		projectionMask = pm;
	}

	if (inconsistent){
		DBGLOG(DBG, "Program is inconsistent, aborting initialization");
		inconsistent = true;
		return;
	}

	DBGLOG(DBG, "Prepare new model enumerator");
	modelEnumerator.reset(config.solve.createEnumerator(config.solve));
	modelEnumerator->init(claspctx, 0, config.solve.numModels);

	DBGLOG(DBG, "Finalizing reinitialization");
	if (!claspctx.endInit()){
		DBGLOG(DBG, "Program is inconsistent, aborting initialization");
		inconsistent = true;
		return;
	}

	updateSymbolTable();

	DBGLOG(DBG, "Resetting solver object");
	solve.reset(new Clasp::BasicSolve(*claspctx.master()));
	nextSolveStep = Restart;

	DBGLOG(DBG, "Resetting post propagator");
	ep.reset(new ExternalPropagator(*this));

#ifndef NDEBUG
	std::stringstream ss;
	asp.write(ss);
	DBGLOG(DBG, "Updated program in LParse format: " << ss.str());
#endif
}

void ClaspSolver::addNogoodSet(const NogoodSet& ns, InterpretationConstPtr frozen){

	assert(problemType == SAT && "programs can only be added in SAT mode");
	DBGLOG(DBG, "Adding set of nogoods incrementally");
	
	// remove post propagator to avoid that it tries the extract the assignment before the symbol table is updated
	if (!!ep.get()) ep.reset();

//	sat.updateProgram();
	claspctx.unfreeze();

	// add new variables
	claspctx.symbolTable().startInit(Clasp::SymbolTable::map_indirect);
	for (int i = 0; i < ns.getNogoodCount(); ++i){
		const Nogood ng = ns.getNogood(i);
		BOOST_FOREACH (ID id, ng){
			// this will register the variable if not already available
			convertHexToClaspProgramLit(id.address, true);
			assert(isMappedToClaspLiteral(id.address) && "new variable was not properly mapped to clasp");
		}
	}
	claspctx.symbolTable().endInit();

	// add the new constraints
	// (SATBuilder does not currently not support this, so we need to do it my hand!)
	claspctx.startAddConstraints();
	Clasp::ClauseCreator cc(claspctx.master());
	for (int i = 0; i < ns.getNogoodCount(); i++){
		const Nogood& ng = ns.getNogood(i);
		TransformNogoodToClaspResult ngClasp = nogoodToClaspClause(ng, true);
		assert (!ngClasp.outOfDomain && "literals were not properly mapped to clasp");
		if (!ngClasp.tautological){
			DBGLOG(DBG, "Adding nogood " << ng << " as clasp-clause");
			cc.start(Clasp::Constraint_t::learnt_other);
			BOOST_FOREACH (Clasp::Literal lit, ngClasp.clause){
				cc.add(lit);
			}

			Clasp::ClauseInfo ci(Clasp::Constraint_t::static_constraint);
			assert (!claspctx.master()->hasConflict() && (claspctx.master()->decisionLevel() == 0 || ci.learnt()));
			Clasp::ClauseCreator::Result res = cc.create(*claspctx.master(), cc.lits(), 0, ci);
			DBGLOG(DBG, "Assignment is " << (res.ok() ? "not " : "") << "conflicting, new clause is " << (res.unit() ? "" : "not ") << "unit");

			if (!res.ok()){
				inconsistent = true;
				break;
			}
		}else{
			DBGLOG(DBG, "Skipping tautological nogood");
		}
	}

	DBGLOG(DBG, "SAT instance has " << claspctx.numVars() << " variables");

	freezeVariables(frozen, true /* freeze variables by default */);

	DBGLOG(DBG, "Prepare new model enumerator");
	modelEnumerator.reset(config.solve.createEnumerator(config.solve));
	modelEnumerator->init(claspctx, 0, config.solve.numModels);

	DBGLOG(DBG, "Finalizing reinitialization");
	if (!claspctx.endInit()){
		DBGLOG(DBG, "Program is inconsistent, aborting initialization");
		inconsistent = true;
		return;
	}

	updateSymbolTable();

	DBGLOG(DBG, "Resetting solver object");
	solve.reset(new Clasp::BasicSolve(*claspctx.master()));
	nextSolveStep = Restart;

	DBGLOG(DBG, "Resetting post propagator");
	ep.reset(new ExternalPropagator(*this));
}

void ClaspSolver::restartWithAssumptions(const std::vector<ID>& assumptions){

	DBGLOG(DBG, "Restarting search");

	if (inconsistent){
		DBGLOG(DBG, "Program is unconditionally inconsistent, ignoring assumptions");
		return;
	}

	DBGLOG(DBG, "Setting new assumptions");
	this->assumptions.clear();
	BOOST_FOREACH (ID a, assumptions){
		if (isMappedToClaspLiteral(a.address)){
			DBGLOG(DBG, "Setting assumption H:" << RawPrinter::toString(reg, a) << " (clasp: C:" << convertHexToClaspSolverLit(a.address).index() << "/" << (convertHexToClaspSolverLit(a.address).sign() ^ a.isNaf() ? "!" : "") << convertHexToClaspSolverLit(a.address).var() << ")");
			Clasp::Literal al = Clasp::Literal(convertHexToClaspSolverLit(a.address).var(), convertHexToClaspSolverLit(a.address).sign() ^ a.isNaf());
			this->assumptions.push_back(al);
		}else{
			DBGLOG(DBG, "Ignoring assumption H:" << RawPrinter::toString(reg, a));
		}
	}
	nextSolveStep = Restart;
}

void ClaspSolver::addPropagator(PropagatorCallback* pb){
	propagators.insert(pb);
}

void ClaspSolver::removePropagator(PropagatorCallback* pb){
	propagators.erase(pb);
}

void ClaspSolver::addNogood(Nogood ng){
	nogoods.push_back(ng);
}

void ClaspSolver::setOptimum(std::vector<int>& optimum){
}

InterpretationPtr ClaspSolver::getNextModel(){

//	#define ENUMALGODBG(msg) { if (problemType == SAT) { std::cerr << "(" << msg << ")"; } }
	#define ENUMALGODBG(msg) { DBGLOG(DBG, "Model enumeration algorithm: (" << msg << ")"); }

	/*
		This method essentially implements the following algorithm:

											[Restart]
		while (solve.solve() == Clasp::value_true) {				[Solve]
			if (e->commitModel(solve.solver())) {				[CommitModel]
			   do {
				 onModel(e->lastModel());				[ExtractModel] & [ReturnModel]
			   } while (e->commitSymmetric(solve.solver()));		[CommitSymmetricModel]
			}
			e->update(solve.solver());					[Update]
		}

		However, the onModel-call is actually a return.
		The algorithm is then continued with the next call of the method.
		The enum type NextSolveStep is used to keep track of the current state of the algorithm.
	*/

	DBGLOG(DBG, "ClaspSolver::getNextModel");

	// ReturnModel is the only step which allows for interrupting the algorithm, i.e., leaving this loop
	while (nextSolveStep != ReturnModel) {
		switch (nextSolveStep){
			case Restart:
				ENUMALGODBG("ini");
				DBGLOG(DBG, "Starting new search");

				if (inconsistent){
					model = InterpretationPtr();
					nextSolveStep = ReturnModel;
				}else{
					DBGLOG(DBG, "Adding step literal to assumptions");
					assumptions.push_back(claspctx.stepLiteral());

					DBGLOG(DBG, "Starting enumerator with " << assumptions.size() << " assumptions (including step literal)");
					assert(!!modelEnumerator.get() && !!solve.get());
					if (enumerationStarted){
						modelEnumerator->end(solve->solver());
					}
					enumerationStarted = true;

					if (modelEnumerator->start(solve->solver(), assumptions)){
						ENUMALGODBG("sat");
						DBGLOG(DBG, "Instance is satisfiable wrt. assumptions");
						nextSolveStep = Solve;
					}else{
						ENUMALGODBG("ust");
						DBGLOG(DBG, "Instance is unsatisfiable wrt. assumptions");
						model = InterpretationPtr();
						nextSolveStep = ReturnModel;
					}
				}
				break;

			case Solve:
				ENUMALGODBG("sol");
				DBGLOG(DBG, "Solve for next model");
				if (solve->solve() == Clasp::value_true){
					nextSolveStep = CommitModel;
				}else{
					model = InterpretationPtr();
					nextSolveStep = ReturnModel;
				}
				break;

			case CommitModel:
				ENUMALGODBG("com");
				DBGLOG(DBG, "Committing model");

				if (modelEnumerator->commitModel(solve->solver())){
					nextSolveStep = ExtractModel;
				}else{
					nextSolveStep = Update;
				}
				break;

			case ExtractModel:
				ENUMALGODBG("ext");
				DBGLOG(DBG, "Extract model model");

				// Note: currentIntr does not necessarily coincide with the last model because clasp
				// possibly has already continued the search at this point
				model = InterpretationPtr(new Interpretation(reg));
				for (Clasp::SymbolTable::const_iterator it = claspctx.symbolTable().begin(); it != claspctx.symbolTable().end(); ++it) {
					if (modelEnumerator->lastModel().isTrue(it->second.lit) && !it->second.name.empty()) {
						BOOST_FOREACH (IDAddress adr, *convertClaspSolverLitToHex(it->second.lit.index())){
							model->setFact(adr);
						}
					}
				}
				outputProject(model);
				modelCount++;

#ifndef NDEBUG
				if (!!model){
					BOOST_FOREACH (Clasp::Literal lit, assumptions){
						if (lit != claspctx.stepLiteral() && !modelEnumerator->lastModel().isTrue(lit)){
							assert(false && "Model contradicts assumptions");
						}
					}
				}
#endif

				nextSolveStep = ReturnModel;
				break;

			case CommitSymmetricModel:
				ENUMALGODBG("cos");
				DBGLOG(DBG, "Committing symmetric model");
				if (modelEnumerator->commitSymmetric(solve->solver())){
					DBGLOG(DBG, "Found more symmetric models, going back to extraction");
					nextSolveStep = ExtractModel;
				}else{
					DBGLOG(DBG, "No more symmetric models, going to update");
					nextSolveStep = Update;
				}
				break;

			case Update:
				ENUMALGODBG("upd");

				bool optContinue;
				if (modelEnumerator->optimize()){
					DBGLOG(DBG, "Committing unsat (for optimization problems)");
					optContinue = modelEnumerator->commitUnsat(solve->solver());
				}

				DBGLOG(DBG, "Updating enumerator");
				modelEnumerator->update(solve->solver());

				if (modelEnumerator->optimize()){
					DBGLOG(DBG, "Committing complete (for optimization problems)");
					if (optContinue) optContinue = modelEnumerator->commitComplete();
				}

				nextSolveStep = Solve;
				break;
			default:
				assert (false && "invalid state");
		}
	}

	assert (nextSolveStep == ReturnModel);

	assert (!(inconsistent && !!model) && "algorithm produced a model although instance is inconsistent");
	// Note: !(!inconsistent && model) can *not* be asserted since an instance can be consistent but still
	//       produce an empty model because if
	//       (1) end of models; or
	//       (2) because the instance is inconsistent wrt. the *current* assumptions
	//           (variable "inconsistent" means that the instance is inconsistent wrt. *any* assumptions).

	ENUMALGODBG("ret");
	DBGLOG(DBG, "Returning " << (!model ? "empty " :"") << "model");
	if (!!model){
		// committing symmetric models is only necessary if some variables are frozen
		// but we still want to get all models
		nextSolveStep = CommitSymmetricModel;
//		nextSolveStep = Update;
	}else{
		nextSolveStep = ReturnModel;	// we stay in this state until restart
	}
	return model;
}

int ClaspSolver::getModelCount(){
	return modelCount;
}

std::string ClaspSolver::getStatistics(){
	std::stringstream ss;
	ss <<	"Guesses: " << claspctx.master()->stats.choices << std::endl <<
		"Conflicts: " << claspctx.master()->stats.conflicts << std::endl <<
		"Models: " << modelCount;
	return ss.str();
}

DLVHEX_NAMESPACE_END

#endif

// vim:noexpandtab:ts=8:
