/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * Copyright (C) 2011, 2012, 2013 Christoph Redl
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
 * @file ClaspSolver.cpp
 * @author Christoph Redl
 * @author Peter Schueller <peterschueller@sabanciuniv.edu> (performance improvements, incremental model update)
 *
 * @brief Interface to genuine clasp 2.0.5-based Solver.
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

DLVHEX_NAMESPACE_BEGIN

// ============================== ExternalPropagator ==============================

ClaspSolver::ExternalPropagator::ExternalPropagator(ClaspSolver& cs) : cs(cs){

	startAssignmentExtraction();

	// initialize propagation deferring
	lastPropagation = boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time());
	double deferMS = cs.ctx.config.getOption("ClaspDeferMaxTMilliseconds");
	skipMaxDuration = boost::posix_time::microseconds(deferMS * 1000.0);

	skipAmount = cs.ctx.config.getOption("ClaspDeferNPropagations");
	skipCounter = 0;
}

ClaspSolver::ExternalPropagator::~ExternalPropagator(){
	stopAssignmentExtraction();
}

void ClaspSolver::ExternalPropagator::startAssignmentExtraction(){

	DBGLOG(DBG, "Initializing assignment extraction");
	currentIntr = InterpretationPtr(new Interpretation(cs.reg));
	currentAssigned = InterpretationPtr(new Interpretation(cs.reg));
	currentChanged = InterpretationPtr(new Interpretation(cs.reg));

	// add watches for all literals
	for (Clasp::SymbolTable::const_iterator it = cs.claspctx.master()->symbolTable().begin(); it != cs.claspctx.master()->symbolTable().end(); ++it) {
		// skip eliminated variables
		if (cs.claspctx.eliminated(it->second.lit.var())) continue;

		DBGLOG(DBG, "Adding watch for literal C:" << it->second.lit.index() << "/" << (it->second.lit.sign() ? "!" : "") << it->second.lit.var() << " and its negation");
		cs.claspctx.master()->addWatch(it->second.lit, this);
		cs.claspctx.master()->addWatch(Clasp::Literal(it->second.lit.var(), !it->second.lit.sign()), this);
	}

	// we need to extract the full assignment (only this time), to make sure that we did not miss any updates before initialization of this extractor
	DBGLOG(DBG, "Extracting full interpretation from clasp");
	cs.extractClaspInterpretation(*cs.claspctx.master(), currentIntr, currentAssigned, currentChanged);
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
	for (int i = 0; i <= assignmentsOnDecisionLevel.size(); i++){
		if (cs.claspctx.master()->validLevel(i)){
			DBGLOG(DBG, "Removing watch for decision level " << i);
			cs.claspctx.master()->removeUndoWatch(i, this);
		}
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

uint32 ClaspSolver::ExternalPropagator::priority() const{
	return Clasp::PostPropagator::priority_class_general;
}

Clasp::Constraint::PropResult ClaspSolver::ExternalPropagator::propagate(Clasp::Solver& s, Clasp::Literal p, uint32& data){

	DBGLOG(DBG, "ExternalPropagator::propagate");
	Clasp::Literal pneg(p.var(), !p.sign());

	assert(s.isTrue(p) && s.isFalse(pneg));

	int level = s.level(p.var());

	DBGLOG(DBG, "Clasp notified about literal C:" << p.index() << "/" << (p.sign() ? "!" : "") << p.var() << " becoming true on dl " << level);
	if (cs.claspToHex.size() > p.index()){
		BOOST_FOREACH (IDAddress adr, *cs.claspToHex[p.index()]){
			DBGLOG(DBG, "Assigning H:" << adr << " to true");
			currentIntr->setFact(adr);
			currentAssigned->setFact(adr);
			currentChanged->setFact(adr);

			// add the variable to the undo watch for the decision level on which it was assigned
			while (assignmentsOnDecisionLevel.size() < level + 1){
				assignmentsOnDecisionLevel.push_back(std::vector<IDAddress>());
			}
			assignmentsOnDecisionLevel[level].push_back(adr);
			if(level > 0 && assignmentsOnDecisionLevel[level].size() == 1){
				DBGLOG(DBG, "Adding undo watch to level " << level);
				cs.claspctx.master()->addUndoWatch(level, this);
			}else{
				DBGLOG(DBG, "Do not add undo watch to level " << level);
			}
		}
	}

	DBGLOG(DBG, "This implies that literal C:" << pneg.index() << "/" << (pneg.sign() ? "!" : "") << p.var() << " becomes false on dl " << level);
	if (cs.claspToHex.size() > pneg.index()){
		BOOST_FOREACH (IDAddress adr, *cs.claspToHex[pneg.index()]){
			DBGLOG(DBG, "Assigning H:" << adr << " to false");
			currentIntr->clearFact(adr);
			currentAssigned->setFact(adr);
			currentChanged->setFact(adr);

			// add the variable to the undo watch for the decision level on which it was assigned
			while (assignmentsOnDecisionLevel.size() < level + 1){
				assignmentsOnDecisionLevel.push_back(std::vector<IDAddress>());
			}
			assignmentsOnDecisionLevel[level].push_back(adr);
			if(level > 0) cs.claspctx.master()->addUndoWatch(level, this);
		}
	}
	DBGLOG(DBG, "ExternalPropagator::propagate finished");
	return PropResult();
}

void ClaspSolver::ExternalPropagator::undoLevel(Clasp::Solver& s){

	DBGLOG(DBG, "Backtracking to decision level " << s.decisionLevel());
	for (int i = s.decisionLevel(); i < assignmentsOnDecisionLevel.size(); i++){
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
			DBGLOG(DBG, "Literal C:" << it->second.lit.index() << (it->second.lit.sign() ? "!" : "") << "/" << it->second.lit.var() << "@" <<
			             solver.level(it->second.lit.var()) << (claspctx.eliminated(it->second.lit.var()) ? "(elim)" : "") << ",H:" << it->second.name.c_str() << " is true");
			BOOST_FOREACH (IDAddress adr, *claspToHex[it->second.lit.index()]){
				if (!!extractCurrentIntr) extractCurrentIntr->setFact(adr);
				if (!!extractCurrentAssigned) extractCurrentAssigned->setFact(adr);
				if (!!extractCurrentChanged) extractCurrentChanged->setFact(adr);
			}
		}
		if (solver.isFalse(it->second.lit) && !it->second.name.empty()) {
			DBGLOG(DBG, "Literal C:" << it->second.lit.index() << (it->second.lit.sign() ? "!" : "") << "/" << it->second.lit.var() << "@" <<
			             solver.level(it->second.lit.var()) << (claspctx.eliminated(it->second.lit.var()) ? "(elim)" : "") << ",H:" << it->second.name.c_str() << " is true");
			BOOST_FOREACH (IDAddress adr, *claspToHex[it->second.lit.index()]){
				if (!!extractCurrentAssigned) extractCurrentAssigned->setFact(adr);
				if (!!extractCurrentChanged) extractCurrentChanged->setFact(adr);
			}
		}
	}
}

// freezes all variables in "frozen"; if the pointer is 0, then all variables are frozen
void ClaspSolver::freezeVariables(InterpretationConstPtr frozen){

	if (!!frozen){
		DBGLOG(DBG, "Setting selected variables to frozen");

#ifndef NDEBUG
		int cntFrozen = 0;
		std::set<int> alreadyFrozen;
#endif
		bm::bvector<>::enumerator en = frozen->getStorage().first();
		bm::bvector<>::enumerator en_end = frozen->getStorage().end();
		while (en < en_end){
#ifndef NDEBUG
			if (alreadyFrozen.count(hexToClasp[*en].var()) == 0){
				cntFrozen++;
				alreadyFrozen.insert(hexToClasp[*en].var());
			}
#endif
			claspctx.setFrozen(hexToClasp[*en].var(), true);
			en++;
		}
#ifndef NDEBUG
		DBGLOG(DBG, "Setting " << cntFrozen << " out of " << claspctx.numVars() << " variables to frozen");
#endif
	}else{
		DBGLOG(DBG, "Setting all " << claspctx.numVars() << " variables to frozen");
		for (int i = 1; i <= claspctx.numVars(); i++){
			claspctx.setFrozen(i, true);
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
		asp.addHead(mapHexToClasp(h.address).var());
	}
	for (int i = 0; i < rule.body.size(); ++i){
		// add literal to body
		asp.addToBody(mapHexToClasp(rule.body[i].address).var(), !rule.body[i].isNaf(), rule.bodyWeightVector[i].address);
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
		asp.addHead(mapHexToClasp(h.address).var());
	}
	BOOST_FOREACH (ID b, rule.body){
		// add literal to body
		asp.addToBody(mapHexToClasp(b.address).var(), !b.isNaf());
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

void ClaspSolver::sendProgramToClasp(const AnnotatedGroundProgram& p){
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::sendProgramTC");

	Clasp::Asp::LogicProgram asp;
	asp.start(claspctx, config.asp);
	asp.setNonHcfConfiguration(config.testerConfig());

	false_ = asp.newAtom();
	asp.setCompute(false_, false);

	buildInitialSymbolTable(asp, p.getGroundProgram());

	// transfer edb
	DBGLOG(DBG, "Sending EDB to clasp: " << *p.getGroundProgram().edb);
	bm::bvector<>::enumerator en = p.getGroundProgram().edb->getStorage().first();
	bm::bvector<>::enumerator en_end = p.getGroundProgram().edb->getStorage().end();
	while (en < en_end){
		// add fact
		asp.startRule(Clasp::Asp::BASICRULE).addHead(mapHexToClasp(*en).var()).endRule();
		en++;
	}

	// transfer idb
	DBGLOG(DBG, "Sending IDB to clasp");
	BOOST_FOREACH (ID ruleId, p.getGroundProgram().idb){
		sendRuleToClasp(asp, ruleId);
	}

	inconsistent = !asp.endProgram();
	DBGLOG(DBG, "SAT instance has " << claspctx.numVars() << " variables");

	DBGLOG(DBG, "Instance is " << (inconsistent ? "" : "not ") << "inconsistent");
}

void ClaspSolver::sendNogoodSetToClasp(const NogoodSet& ns){

	#ifdef DLVHEX_CLASPSOLVER_PROGRAMINIT_BENCHMARKING
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::sendNogoodSetTC");
	#endif

	DBGLOG(DBG, "Sending NogoodSet to clasp: " << ns);

	Clasp::SatBuilder sat;
	sat.startProgram(claspctx);

	buildInitialSymbolTable(sat, ns);

	for (int i = 0; i < ns.getNogoodCount(); i++){
		const Nogood& ng = ns.getNogood(i);
		TransformNogoodToClaspResult ngClasp = nogoodToClaspClause(ng);
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
}

void ClaspSolver::interpretClaspCommandline(Clasp::Problem_t::Type type){

	DBGLOG(DBG, "Interpreting clasp command-line");

	std::string claspconfigstr = ctx.config.getStringOption("ClaspConfiguration");
	if( claspconfigstr == "none" ) return; // do not do anything
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
		config.enumerate.numModels = 0;
		claspctx.setConfiguration(&config, false);

		DBGLOG(DBG, "Finished option parsing");
	}catch(...){
		DBGLOG(DBG, "Could not parse clasp options: " + claspconfigstr);
		throw;
	}
}

void ClaspSolver::shutdownClasp(){
	if (!!ep.get()) claspctx.master()->removePost(ep.get());
	resetAndResizeClaspToHex(0);
}

ClaspSolver::TransformNogoodToClaspResult ClaspSolver::nogoodToClaspClause(const Nogood& ng) const{

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
		if (!isMappedToClaspLiteral(lit.address)){
			DBGLOG(DBG, "some literal was not properly mapped to clasp");
			return TransformNogoodToClaspResult(Clasp::LitVec(), false, true);
		}

		// mclit = mapped clasp literal
		const Clasp::Literal mclit = mapHexToClasp(lit.address);
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

		// 1. cs.hexToClasp maps hex-atoms to clasp-literals
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

void ClaspSolver::buildInitialSymbolTable(Clasp::Asp::LogicProgram& asp, const OrdinaryASPProgram& p){

	#ifdef DLVHEX_CLASPSOLVER_PROGRAMINIT_BENCHMARKING
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::buildInitSymTab P pb");
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

void ClaspSolver::buildInitialSymbolTable(Clasp::SatBuilder& sat, const NogoodSet& ns){
	#ifdef DLVHEX_CLASPSOLVER_PROGRAMINIT_BENCHMARKING
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::buildInitSymTab ns");
	#endif

	DBGLOG(DBG, "Building atom index");
	DBGLOG(DBG, "symbol table has " << claspctx.symbolTable().size() << " entries");

	bool inverselits = ctx.config.getOption("ClaspInverseLiterals");

	assert(hexToClasp.empty());
	hexToClasp.reserve(reg->ogatoms.getSize());

	claspctx.symbolTable().startInit();

	// build symbol table and hexToClasp
	unsigned largestIdx = 0;
	unsigned varCnt = 0;
	for (int i = 0; i < ns.getNogoodCount(); i++){
		const Nogood& ng = ns.getNogood(i);
		BOOST_FOREACH (ID lit, ng){
			if (!isMappedToClaspLiteral(lit.address)) {
				uint32_t c = claspctx.addVar(Clasp::Var_t::atom_var); //lit.address + 2;
				varCnt++;
				std::string str = idAddressToString(lit.address);
				DBGLOG(DBG, "Clasp index of atom H:" << lit.address << " is " << c);
				Clasp::Literal clasplit(c, inverselits); // create positive literal -> false
				storeHexToClasp(lit.address, clasplit);
				if (clasplit.index() > largestIdx) largestIdx = clasplit.index();
				claspctx.symbolTable().addUnique(c, str.c_str()).lit = clasplit;
			}
		}
	}

	// resize
       	// (+1 because largest index must also be covered +1 because negative literal may also be there)
	resetAndResizeClaspToHex(largestIdx + 1 + 1);

	// build back mapping
	for(std::vector<Clasp::Literal>::const_iterator it = hexToClasp.begin();
	    it != hexToClasp.end(); ++it)
	{
		const Clasp::Literal clit = *it;
		if( clit == noLiteral )
			continue;
		const IDAddress hexlitaddress = it - hexToClasp.begin();
		AddressVector* &c2h = claspToHex[clit.index()];
		c2h->push_back(hexlitaddress);
	}

	claspctx.symbolTable().endInit();

	DBGLOG(DBG, "SAT instance has " << varCnt << " variables, symbol table has " << claspctx.symbolTable().size() << " entries");
	sat.prepareProblem(varCnt);
}

void ClaspSolver::resetAndResizeClaspToHex(unsigned size)
{
	for(unsigned u = 0; u < claspToHex.size(); ++u)
	{
		if( claspToHex[u] )
			delete claspToHex[u];
	}
	claspToHex.resize(size, NULL);
	for(unsigned u = 0; u < claspToHex.size(); ++u)
		claspToHex[u] = new AddressVector;
}

void ClaspSolver::buildOptimizedSymbolTable(){

	#ifdef DLVHEX_CLASPSOLVER_PROGRAMINIT_BENCHMARKING
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::backmapClaspLiterals");
	#endif

	hexToClasp.clear();
	hexToClasp.reserve(reg->ogatoms.getSize());

	// go through clasp symbol table
	const Clasp::SymbolTable& symTab = claspctx.symbolTable();

       	// each variable can be a positive or negative literal, literals are (var << 1 | sign)
	// build empty set of NULL-pointers to vectors
       	// (literals which are internal variables and have no HEX equivalent do not show up in symbol table)
	DBGLOG(DBG, "Problem has " << claspctx.numVars() << " variables");
	resetAndResizeClaspToHex(claspctx.numVars() * 2 + 1);

	LOG(DBG, "Symbol table of optimized program:");
	for (Clasp::SymbolTable::const_iterator it = symTab.begin(); it != symTab.end(); ++it) {
		IDAddress hexAdr = stringToIDAddress(it->second.name.c_str());
		storeHexToClasp(hexAdr, it->second.lit);
		DBGLOG(DBG, "H:" << hexAdr << " (" << reg->ogatoms.getByAddress(hexAdr).text <<  ") <--> "
		            "C:" << it->second.lit.index() << "/" << (it->second.lit.sign() ? "!" : "") << it->second.lit.var());
		assert(it->second.lit.index() < claspToHex.size());
		AddressVector* &c2h = claspToHex[it->second.lit.index()];
		c2h->push_back(hexAdr);
	}

	DBGLOG(DBG, "hexToClasp.size()=" << hexToClasp.size() << ", symTab.size()=" << symTab.size());
}

void ClaspSolver::storeHexToClasp(IDAddress addr, Clasp::Literal lit){

	if( addr >= hexToClasp.size() )
		hexToClasp.resize(addr + 1, noLiteral);
       	hexToClasp[addr] = lit;
}

void ClaspSolver::outputProject(InterpretationPtr intr){
	if( !!intr && !!projectionMask )
	{
		intr->getStorage() -= projectionMask->getStorage();
		DBGLOG(DBG, "Projected to " << *intr);
	}
}

ClaspSolver::ClaspSolver(ProgramCtx& ctx, const AnnotatedGroundProgram& p, InterpretationConstPtr frozen)
 : ctx(ctx), solve(0), ep(0), modelCount(0), projectionMask(p.getGroundProgram().mask), noLiteral(Clasp::Literal::fromRep(~0x0)){
	reg = ctx.registry();

	DBGLOG(DBG, "Configure clasp in ASP mode");
	problemType = ASP;
	config.reset();
	interpretClaspCommandline(Clasp::Problem_t::ASP);
	nextSolveStep = Restart;

	claspctx.requestStepVar();
	sendProgramToClasp(p);
	freezeVariables(frozen);

	if (inconsistent){
		DBGLOG(DBG, "Program is inconsistent, aborting initialization");
		inconsistent = true;
		return;
	}

	DBGLOG(DBG, "Prepare model enumerator");
	modelEnumerator.reset(config.enumerate.createEnumerator());
	modelEnumerator->init(claspctx, 0, config.enumerate.numModels);

	DBGLOG(DBG, "Finalizing initialization");
	if (!claspctx.endInit()){
		DBGLOG(DBG, "Program is inconsistent, aborting initialization");
		inconsistent = true;
		return;
	}

	buildOptimizedSymbolTable();

	DBGLOG(DBG, "Prepare solver object");
	solve.reset(new Clasp::BasicSolve(*claspctx.master()));
	enumerationStarted = false;

	DBGLOG(DBG, "Adding post propagator");
	ep.reset(new ExternalPropagator(*this));
	claspctx.master()->addPost(ep.get());
}

ClaspSolver::ClaspSolver(ProgramCtx& ctx, const NogoodSet& ns, InterpretationConstPtr frozen)
 : ctx(ctx), solve(0), ep(0), modelCount(0), noLiteral(Clasp::Literal::fromRep(~0x0)){
	reg = ctx.registry();

	DBGLOG(DBG, "Configure clasp in SAT mode");
	problemType = SAT;
	config.reset();
	interpretClaspCommandline(Clasp::Problem_t::SAT);
	nextSolveStep = Restart;

	claspctx.requestStepVar();
	sendNogoodSetToClasp(ns);
	freezeVariables(frozen);

	if (inconsistent){
		DBGLOG(DBG, "Program is inconsistent, aborting initialization");
		inconsistent = true;
		return;
	}

	DBGLOG(DBG, "Prepare model enumerator");
	modelEnumerator.reset(config.enumerate.createEnumerator());
	modelEnumerator->init(claspctx, 0, config.enumerate.numModels);

	DBGLOG(DBG, "Finalizing initialization");
	if (!claspctx.endInit()){
		DBGLOG(DBG, "SAT instance is unsatisfiable");
		inconsistent = true;
		return;
	}

	buildOptimizedSymbolTable();

#ifndef NDEBUG
	std::stringstream ss;
	InterpretationPtr vars = InterpretationPtr(new Interpretation(reg));
	for (int ngi = 0; ngi < ns.getNogoodCount(); ++ngi){
		const Nogood& ng = ns.getNogood(ngi);
		TransformNogoodToClaspResult ngClasp = nogoodToClaspClause(ng);		
		bool first = true;
		ss << ":- ";
		for (int i = 0; i < ngClasp.clause.size(); ++i){
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
	claspctx.master()->addPost(ep.get());
}

ClaspSolver::~ClaspSolver(){
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::~ClaspSolver");
	shutdownClasp();
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
			DBGLOG(DBG, "Setting assumption H:" << RawPrinter::toString(reg, a) << " (clasp: C:" << mapHexToClasp(a.address).index() << "/" << (mapHexToClasp(a.address).sign() ^ a.isNaf() ? "!" : "") << mapHexToClasp(a.address).var() << ")");
			Clasp::Literal al = Clasp::Literal(mapHexToClasp(a.address).var(), mapHexToClasp(a.address).sign() ^ a.isNaf());
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
						BOOST_FOREACH (IDAddress adr, *claspToHex[it->second.lit.index()]){
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
