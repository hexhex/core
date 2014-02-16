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
}

bool ClaspSolver::ExternalPropagator::propToHEX(Clasp::Solver& s){

#ifndef NDEBUG
	// extract model and compare with the incrementally extracted one
	InterpretationPtr currentIntr = InterpretationPtr(new Interpretation(cs.reg));
	InterpretationPtr currentAssigned = InterpretationPtr(new Interpretation(cs.reg));
	cs.extractClaspInterpretation(currentIntr, currentAssigned, InterpretationPtr());
	std::stringstream ss;
	ss << "partial assignment " << *currentIntr << " (assigned: " << *currentAssigned << "); incrementally extracted one " << *cs.currentIntr << " (assigned: " << *cs.currentAssigned << ")";
	DBGLOG(DBG, ss.str());
	assert (currentIntr->getStorage() == cs.currentIntr->getStorage() && currentAssigned->getStorage() == cs.currentAssigned->getStorage());
#endif

	// call HEX propagators
	BOOST_FOREACH (PropagatorCallback* propagator, cs.propagators){
		DBGLOG(DBG, "ExternalPropagator: Calling HEX-Propagator");
		propagator->propagate(cs.currentIntr, cs.currentAssigned, cs.currentChanged);
	}
	cs.currentChanged->clear();

	// add new clauses to clasp
	DBGLOG(DBG, "ExternalPropagator: Adding new clauses to clasp (" << cs.nogoods.size() << " were prepared)");
	bool inconsistent = false;
	Clasp::ClauseCreator cc(&s);
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
			Clasp::ClauseCreator::Result res = cc.end(0);
			DBGLOG(DBG, "Assignment is " << (res.ok() ? "not " : "") << "conflicting");
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

	for (;;){
		if (propToHEX(s)) return false;
		if (s.queueSize() == 0) return true;
		if (!s.propagateUntil(this)) return false;
	}
}

bool ClaspSolver::ExternalPropagator::isModel(Clasp::Solver& s){
	DBGLOG(DBG, "ExternalPropagator::isModel");
	if (propToHEX(s)) return false;
	return s.numFreeVars() == 0 && !s.hasConflict();
}

uint32 ClaspSolver::ExternalPropagator::priority() const{
	return Clasp::PostPropagator::priority_class_general;
}

// ============================== ClaspSolver::AssignmentExtractor ==============================

ClaspSolver::AssignmentExtractor::AssignmentExtractor(ClaspSolver& cs) : cs(cs){
}

void ClaspSolver::AssignmentExtractor::setAssignment(InterpretationPtr intr, InterpretationPtr assigned, InterpretationPtr changed){
	this->intr = intr;
	this->assigned = assigned;
	this->changed = changed;

	// add watches for all literals
	for (Clasp::SymbolTable::const_iterator it = cs.libclasp.ctx.symbolTable().begin(); it != cs.libclasp.ctx.symbolTable().end(); ++it) {
		DBGLOG(DBG, "Adding watch for literal " << it->second.lit.index() << " and its negation");
		cs.libclasp.ctx.master()->addWatch(it->second.lit, this);
		cs.libclasp.ctx.master()->addWatch(Clasp::Literal(it->second.lit.var(), !it->second.lit.sign()), this);
	}

	// we need to extract the full assignment (only this time), to make sure that we did not miss any updates before initialization of this extractor
	this->intr->clear();
	this->assigned->clear();
	this->changed->clear();
	cs.extractClaspInterpretation(this->intr, this->assigned, this->changed);
}

Clasp::Constraint* ClaspSolver::AssignmentExtractor::cloneAttach(Clasp::Solver& other){
	assert(false && "AssignmentExtractor must not be copied");
}

Clasp::Constraint::PropResult ClaspSolver::AssignmentExtractor::propagate(Clasp::Solver& s, Clasp::Literal p, uint32& data){

	Clasp::Literal pneg(p.var(), !p.sign());

	assert(s.isTrue(p) && s.isFalse(pneg));

	int level = s.level(p.var());

	DBGLOG(DBG, "Clasp notified about literal (idx " << p.index() << ")" << (p.sign() ? "" : "!") << p.var() << " becoming true on dl " << level);
	if (cs.claspToHex.size() > p.index()){
		BOOST_FOREACH (IDAddress adr, *cs.claspToHex[p.index()]){
			DBGLOG(DBG, "Assigning " << adr << " to true");
			intr->setFact(adr);
			assigned->setFact(adr);
			changed->setFact(adr);

			// add the variable to the undo watch for the decision level on which it was assigned
			while (assignmentsOnDecisionLevel.size() < level + 1){
				assignmentsOnDecisionLevel.push_back(std::vector<IDAddress>());
			}
			assignmentsOnDecisionLevel[level].push_back(adr);
			if(level > 0) cs.libclasp.ctx.master()->addUndoWatch(level, this);
		}
	}

	DBGLOG(DBG, "This implies that literal (idx " << pneg.index() << ")" << (pneg.sign() ? "" : "!") << p.var() << " becomes false on dl " << level);
	if (cs.claspToHex.size() > pneg.index()){
		BOOST_FOREACH (IDAddress adr, *cs.claspToHex[pneg.index()]){
			DBGLOG(DBG, "Assigning " << adr << " to false");
			intr->clearFact(adr);
			assigned->setFact(adr);
			changed->setFact(adr);

			// add the variable to the undo watch for the decision level on which it was assigned
			while (assignmentsOnDecisionLevel.size() < level + 1){
				assignmentsOnDecisionLevel.push_back(std::vector<IDAddress>());
			}
			assignmentsOnDecisionLevel[level].push_back(adr);
			if(level > 0) cs.libclasp.ctx.master()->addUndoWatch(level, this);
		}
	}
	return PropResult();
}

void ClaspSolver::AssignmentExtractor::undoLevel(Clasp::Solver& s){

	DBGLOG(DBG, "Backtracking to decision level " << s.decisionLevel());
	for (int i = s.decisionLevel(); i < assignmentsOnDecisionLevel.size(); i++){
		DBGLOG(DBG, "Undoing decision level " << i);
		BOOST_FOREACH (IDAddress adr, assignmentsOnDecisionLevel[i]){
			DBGLOG(DBG, "Unassigning " << adr);
			intr->clearFact(adr);
			assigned->clearFact(adr);
			changed->setFact(adr);
		}
	}
	assignmentsOnDecisionLevel.erase(assignmentsOnDecisionLevel.begin() + s.decisionLevel(), assignmentsOnDecisionLevel.end());
}

void ClaspSolver::AssignmentExtractor::reason(Clasp::Solver& s, Clasp::Literal p, Clasp::LitVec& lits){
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

void ClaspSolver::extractClaspInterpretation(InterpretationPtr currentIntr, InterpretationPtr currentAssigned, InterpretationPtr currentChanged){
	InterpretationPtr partialInterpretation = InterpretationPtr(new Interpretation(reg));
	InterpretationPtr assigned = InterpretationPtr(new Interpretation(reg));
	for (Clasp::SymbolTable::const_iterator it = libclasp.ctx.symbolTable().begin(); it != libclasp.ctx.symbolTable().end(); ++it) {
		if (libclasp.ctx.master()->isTrue(it->second.lit) && !it->second.name.empty()) {
			BOOST_FOREACH (IDAddress adr, *claspToHex[it->second.lit.index()]){
				if (!!currentIntr) currentIntr->setFact(adr);
				if (!!currentAssigned) currentAssigned->setFact(adr);
				if (!!currentChanged) currentChanged->setFact(adr);
			}
		}
		if (libclasp.ctx.master()->isFalse(it->second.lit) && !it->second.name.empty()) {
			BOOST_FOREACH (IDAddress adr, *claspToHex[it->second.lit.index()]){
				if (!!currentAssigned) currentAssigned->setFact(adr);
				if (!!currentChanged) currentChanged->setFact(adr);
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

	Clasp::Asp::LogicProgram& asp = libclasp.startAsp(config);

	false_ = asp.newAtom();
	assert(false_ == 1);
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
	libclasp.prepare();

	buildOptimizedSymbolTable();
}

void ClaspSolver::sendNogoodSetToClasp(const NogoodSet& ns){
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::sendNogoodSetTC");

	DBGLOG(DBG, "Sending NogoodSet to clasp: " << ns);

	Clasp::SatBuilder& sat = libclasp.startSat(config);

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

	libclasp.prepare();

	buildOptimizedSymbolTable();
}

void ClaspSolver::interpretClaspCommandline(Clasp::Problem_t::Type type){

	// from clasp source code
	#define DEF_SOLVE    "--heuristic=Berkmin --restarts=x,100,1.5 --deletion=basic,75 --del-init=200,40000 --del-max=400000 --contraction=250 --loops=common --save-p=180"
	#define FRUMPY_SOLVE DEF_SOLVE " --del-grow=1.1 --strengthen=local"
	#define JUMPY_SOLVE  "--heuristic=Vsids --restarts=L,100 --del-init=1000,20000 --deletion=basic,75,2 --del-grow=1.1,25,x,100,1.5 --del-cfl=x,10000,1.1 --del-glue=2 --update-lbd=3 --strengthen=recursive --otfs=2 --save-p=70"
	#define HANDY_SOLVE  "--heuristic=Vsids --restarts=D,100,0.7 --del-max=200000 --deletion=sort,50,2 --del-init=1000,14000 --del-cfl=+,4000,600 --del-glue=2 --update-lbd --strengthen=recursive --otfs=2 --save-p=20 --contraction=600 --loops=distinct --counter-restarts=7 --counter-bump=1023 --reverse-arcs=2"
	#define CRAFTY_SOLVE "--heuristic=Vsids --restarts=x,128,1.5 --deletion=basic,75,0 --del-init=1000,9000 --del-grow=1.1,20.0 --del-cfl=+,10000,1000 --del-glue=2 --otfs=2 --reverse-arcs=1 --counter-restarts=3 --contraction=250"
	#define TRENDY_SOLVE "--heuristic=Vsids --restarts=D,100,0.7 --deletion=basic,50 --del-init=500,19500 --del-grow=1.1,20.0,x,100,1.5 --del-cfl=+,10000,2000 --del-glue=2 --strengthen=recursive --update-lbd --otfs=2 --save-p=75 --counter-restarts=3 --counter-bump=1023 --reverse-arcs=2  --contraction=250 --loops=common"

	DBGLOG(DBG, "Interpreting clasp command-line");

	std::string claspconfigstr = ctx.config.getStringOption("ClaspConfiguration");
	if( claspconfigstr == "none" ) return; // do not do anything
	else if( claspconfigstr == "default" ) claspconfigstr = DEF_SOLVE;
	else if( claspconfigstr == "frumpy" ) claspconfigstr = FRUMPY_SOLVE;
	else if( claspconfigstr == "jumpy" ) claspconfigstr = JUMPY_SOLVE;
	else if( claspconfigstr == "handy" ) claspconfigstr = HANDY_SOLVE;
	else if( claspconfigstr == "crafty" ) claspconfigstr = CRAFTY_SOLVE;
	else if( claspconfigstr == "trendy" ) claspconfigstr = TRENDY_SOLVE;
	// otherwise let the config string itself be parsed by clasp

	DBGLOG(DBG, "Found configuration: " << claspconfigstr);
	try{
		// options found in command-line
		ProgramOptions::OptionContext allOpts("<clasp_dlvhex>");
		DBGLOG(DBG, "Configuring options");
		config.reset();
		config.addOptions(allOpts);
		DBGLOG(DBG, "Parsing command-line");
		ProgramOptions::ParsedValues values(ProgramOptions::parseCommandString(claspconfigstr, allOpts));
		DBGLOG(DBG, "Assigning specified values");
		parsedOptions.assign(values);
		DBGLOG(DBG, "Assigning defaults");
		allOpts.assignDefaults(parsedOptions);

		config.finalize(parsedOptions, type, true);
		DBGLOG(DBG, "Finished option parsing");
	}catch(...){
		DBGLOG(DBG, "Could not parse clasp options: " + claspconfigstr);
		throw;
	}
}

void ClaspSolver::shutdownClasp(){
	if(!!solve) delete solve;
	if (!!ep){
		libclasp.ctx.master()->removePost(ep);
		delete ep;
	}
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
		ss << (clit.sign() ? "" : "!") << clit.var();
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
			DBGLOG(DBG, "Clasp index of atom " << *en << " is " << c);

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
				DBGLOG(DBG, "Clasp index of atom " << h.address << " is " << c);

			       	// create positive literal -> false
				storeHexToClasp(h.address, Clasp::Literal(c, false));

				std::string str = idAddressToString(h.address);
				asp.setAtomName(c, str.c_str());
			}
		}
		BOOST_FOREACH (ID b, rule.body){
			if (!isMappedToClaspLiteral(b.address)){
				uint32_t c = b.address + 2;
				DBGLOG(DBG, "Clasp index of atom " << b.address << " is " << c);

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
	DBGLOG(DBG, "symbol table has " << libclasp.ctx.symbolTable().size() << " entries");

	bool inverselits = ctx.config.getOption("ClaspInverseLiterals");

	assert(hexToClasp.empty());
	hexToClasp.reserve(reg->ogatoms.getSize());

	libclasp.ctx.symbolTable().startInit();

	// build symbol table and hexToClasp
	unsigned largestIdx = 0;
	unsigned varCnt = 0;
	for (int i = 0; i < ns.getNogoodCount(); i++){
		const Nogood& ng = ns.getNogood(i);
		BOOST_FOREACH (ID lit, ng){
			if (!isMappedToClaspLiteral(lit.address)) {
				uint32_t c = libclasp.ctx.addVar(Clasp::Var_t::atom_var); //lit.address + 2;
				varCnt++;
				std::string str = idAddressToString(lit.address);
				DBGLOG(DBG, "Clasp index of atom " << lit.address << " is " << c);
				Clasp::Literal clasplit(c, inverselits); // create positive literal -> false
				storeHexToClasp(lit.address, clasplit);
				if (clasplit.index() > largestIdx) largestIdx = clasplit.index();
				libclasp.ctx.symbolTable().addUnique(c, str.c_str()).lit = clasplit;
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

	libclasp.ctx.symbolTable().endInit();

	DBGLOG(DBG, "SAT instance has " << varCnt << " variables, symbol table has " << libclasp.ctx.symbolTable().size() << " entries");
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
	const Clasp::SymbolTable& symTab = libclasp.ctx.symbolTable();

       	// each variable can be a positive or negative literal, literals are (var << 1 | sign)
	// build empty set of NULL-pointers to vectors
       	// (literals which are internal variables and have no HEX equivalent do not show up in symbol table)
	resetAndResizeClaspToHex((libclasp.ctx.numVars() + 1) * 2);

	LOG(DBG, "Symbol table of optimized program:");
	for (Clasp::SymbolTable::const_iterator it = symTab.begin(); it != symTab.end(); ++it) {
		IDAddress hexAdr = stringToIDAddress(it->second.name.c_str());
		storeHexToClasp(hexAdr, it->second.lit);
		assert(it->second.lit.index() < claspToHex.size());
		AddressVector* &c2h = claspToHex[it->second.lit.index()];
		if( !c2h ) c2h = new AddressVector;
		c2h->push_back(hexAdr);
		DBGLOG(DBG, "Hex " << hexAdr << " (" << reg->ogatoms.getByAddress(hexAdr).text <<  ") <--> "
		            "(idx " << it->second.lit.index() << ")" << (it->second.lit.sign() ? "" : "!") << it->second.lit.var());
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

ClaspSolver::ClaspSolver(ProgramCtx& ctx, const AnnotatedGroundProgram& p)
 : ctx(ctx), assignmentExtractor(*this), solve(0), ep(0), modelCount(0), projectionMask(p.getGroundProgram().mask), noLiteral(Clasp::Literal::fromRep(~0x0)){
	reg = ctx.registry();

	DBGLOG(DBG, "Configure clasp");
	config.reset();
//	interpretClaspCommandline(Clasp::Problem_t::SAT);
	config.enumerate.numModels = 0;

	sendProgramToClasp(p);

	DBGLOG(DBG, "Adding assignment extractor");
	currentIntr = InterpretationPtr(new Interpretation(reg));
	currentAssigned = InterpretationPtr(new Interpretation(reg));
	currentChanged = InterpretationPtr(new Interpretation(reg));
	assignmentExtractor.setAssignment(currentIntr, currentAssigned, currentChanged);

	DBGLOG(DBG, "Adding post propagator");
	ep = new ExternalPropagator(*this);
	libclasp.ctx.master()->addPost(ep);

	DBGLOG(DBG, "Prepare model enumerator");
	modelEnumerator.setStrategy(Clasp::ModelEnumerator::strategy_backtrack);
}

ClaspSolver::ClaspSolver(ProgramCtx& ctx, const NogoodSet& ns)
 : ctx(ctx), assignmentExtractor(*this), solve(0), ep(0), modelCount(0), noLiteral(Clasp::Literal::fromRep(~0x0)){
	reg = ctx.registry();

	DBGLOG(DBG, "Configure clasp");
	config.reset();
	config.enumerate.numModels = 0;
//	interpretClaspCommandline(Clasp::Problem_t::SAT);

	sendNogoodSetToClasp(ns);

	DBGLOG(DBG, "Adding assignment extractor");
	currentIntr = InterpretationPtr(new Interpretation(reg));
	currentAssigned = InterpretationPtr(new Interpretation(reg));
	currentChanged = InterpretationPtr(new Interpretation(reg));
	assignmentExtractor.setAssignment(currentIntr, currentAssigned, currentChanged);

	DBGLOG(DBG, "Adding post propagator");
	ep = new ExternalPropagator(*this);
	libclasp.ctx.master()->addPost(ep);

	DBGLOG(DBG, "Prepare model enumerator");
	modelEnumerator.setStrategy(Clasp::ModelEnumerator::strategy_backtrack);
}

ClaspSolver::~ClaspSolver(){
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "ClaspSlv::~ClaspSolver");
	shutdownClasp();
}

void ClaspSolver::restartWithAssumptions(const std::vector<ID>& assumptions){

	DBGLOG(DBG, "Restarting search");
	if(!!solve){
		delete solve;
		modelCount = 0;
		solve = 0;
	}

	DBGLOG(DBG, "Setting assumptions");
	libclasp.ctx.master()->clearAssumptions();
	this->assumptions.clear();
	BOOST_FOREACH (ID a, assumptions){
		if (isMappedToClaspLiteral(a.address)){
			DBGLOG(DBG, "Setting assumption " << RawPrinter::toString(reg, a) << " (clasp: " << (mapHexToClasp(a.address).sign() ^ a.isNaf() ? "" : "!") << mapHexToClasp(a.address).var() << ")");
			Clasp::Literal al = Clasp::Literal(mapHexToClasp(a.address).var(), mapHexToClasp(a.address).sign() ^ a.isNaf());
			this->assumptions.push_back(al);
		}else{
			DBGLOG(DBG, "Ignoring assumption " << RawPrinter::toString(reg, a));
		}
	}
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

	if (!solve){
		DBGLOG(DBG, "Starting search");
		solve = new Clasp::BasicSolve(*libclasp.ctx.master());
		modelEnumerator.init(libclasp.ctx, 0);
		modelEnumerator.start(solve->solver());
		bool conflicting = !solve->assume(assumptions);
		DBGLOG(DBG, "Assumptions are " << (!conflicting ? "not " : "") << "conflicting");
		if (conflicting){
#ifndef NDEBUG
			std::stringstream ss;
			for (Clasp::SymbolTable::const_iterator it = libclasp.ctx.symbolTable().begin(); it != libclasp.ctx.symbolTable().end(); ++it) {
				if (libclasp.ctx.master()->isTrue(it->second.lit) && !it->second.name.empty()) {
					ss << (it->second.lit.sign() ? "" : "!") << it->second.lit.var() << "@" << libclasp.ctx.master()->level(it->second.lit.var()) << " ";
				}
				if (libclasp.ctx.master()->isFalse(it->second.lit) && !it->second.name.empty()) {
					ss << (!it->second.lit.sign() ? "" : "!") << it->second.lit.var() << "@" << libclasp.ctx.master()->level(it->second.lit.var()) << " ";
				}
			}
			DBGLOG(DBG, "Conflicting assignment: " << ss.str());
#endif
			return InterpretationPtr();
		}
	}

	DBGLOG(DBG, "ClaspSolver::getNextModel");
	if (solve->solve() == Clasp::value_true) {
		DBGLOG(DBG, "Found model");

#ifndef NDEBUG
		// extract model and compare with the incrementally extracted one
		InterpretationPtr nextModel = InterpretationPtr(new Interpretation(reg));
		extractClaspInterpretation(nextModel);
		std::stringstream ss;
		ss << "model " << *nextModel << "; incrementally extracted one " << *currentIntr;
		DBGLOG(DBG, ss.str());
		assert (currentIntr->getStorage() == nextModel->getStorage());

		// check if the model respects the assumptions
		BOOST_FOREACH (Clasp::Literal lit, assumptions){
			assert (solve->solver().isTrue(lit) && "assumption violated");
		}
#endif
		InterpretationPtr model(new Interpretation(reg));
		model->add(*currentIntr);
		outputProject(model);
		modelCount++;

		// go to next model
		modelEnumerator.commitModel(solve->solver());
		modelEnumerator.update(solve->solver());

		return model;
	}else{
		DBGLOG(DBG, "End of models");
		return InterpretationPtr();
	}
}

int ClaspSolver::getModelCount(){
	return modelCount;
}

std::string ClaspSolver::getStatistics(){
	std::stringstream ss;
	ss <<	"Guesses: " << libclasp.ctx.master()->stats.choices << std::endl <<
		"Conflicts: " << libclasp.ctx.master()->stats.conflicts << std::endl <<
		"Models: " << modelCount;
	return ss.str();
}

DLVHEX_NAMESPACE_END

#endif

// vim:noexpandtab:ts=8:
