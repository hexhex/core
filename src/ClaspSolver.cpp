/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
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

#include <boost/foreach.hpp>
#include <boost/graph/strong_components.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include "clasp/program_rule.h"
#include "clasp/constraint.h"


DLVHEX_NAMESPACE_BEGIN

/*

// callback interface for printing answer sets
struct AspOutPrinter : public Clasp::Enumerator::Report {
	void reportModel(const Clasp::Solver& s, const Clasp::Enumerator&);
	void reportSolution(const Clasp::Solver& s, const Clasp::Enumerator&, bool);
};


// Compute the stable models of the program
//    a :- not b.
//    b :- not a.
void example1() {
	// The ProgramBuilder lets you define logic programs.
	// It preprocesses logic programs and converts them
	// to the internal solver format.
	// See program_builder.h for details
	Clasp::ProgramBuilder api;
	
	// Among other things, SharedContext maintains a Solver object 
	// which hosts the data and functions for CDNL answer set solving.
	// SharedContext also contains the symbol table which stores the 
	// mapping between atoms of the logic program and the 
	// propositional literals in the solver.
	// See solver.h and solver_types.h for details
	Clasp::SharedContext  ctx;
	
	// startProgram must be called once before we can add atoms/rules
	api.startProgram(ctx);
	
	// Populate symbol table. Each atoms must have a unique id, the name is optional.
	// The symbol table then maps the ids to the propositional 
	// literals in the solver.
	api.setAtomName(1, "a");
	api.setAtomName(2, "b");
	api.setAtomName(3, "c");
	api.setAtomName(4, "d");
	
	// Define the rules of the program
	api.startRule(Clasp::BASICRULE).addHead(1).addToBody(2, false).addToBody(3, false).addToBody(4, false).endRule();
	api.startRule(Clasp::BASICRULE).addHead(2).addToBody(1, false).addToBody(3, false).addToBody(4, false).endRule();
	api.startRule(Clasp::BASICRULE).addHead(3).addToBody(1, false).addToBody(2, false).addToBody(4, false).endRule();
	api.startRule(Clasp::BASICRULE).addHead(4).addToBody(1, false).addToBody(2, false).addToBody(3, false).endRule();
	api.startRule(Clasp::BASICRULE).addHead(1).addToBody(2, true).endRule();
	api.startRule(Clasp::BASICRULE).addHead(2).addToBody(1, true).endRule();
	api.startRule(Clasp::BASICRULE).addHead(1).addToBody(3, true).endRule();
	
	// Once all rules are defined, call endProgram() to load the (simplified)
	// program into the context object
	api.endProgram();
	if (api.dependencyGraph() && api.dependencyGraph()->nodes() > 0) {
		// program is non tight - add unfounded set checker
		Clasp::DefaultUnfoundedCheck* ufs = new Clasp::DefaultUnfoundedCheck();
		ufs->attachTo(*ctx.master(), api.dependencyGraph()); // register with solver and graph & transfer ownership
	}
	
	// For printing answer sets
	AspOutPrinter         printer;

	// Since we want to compute more than one
	// answer set, we need an enumerator.
	// See enumerator.h for details
	ctx.addEnumerator(new Clasp::BacktrackEnumerator(0,&printer));
	ctx.enumerator()->enumerate(0);

	// endInit() must be called once before the search starts
	ctx.endInit();

	// Aggregates some solving parameters, e.g.
	//  - restart-strategy
	//  - ...
	// See solve_algorithms.h for details
	Clasp::SolveParams    params;

	// solve() starts the search for answer sets. It uses the 
	// given parameters to control the search.
	Clasp::solve(ctx, params);
}

void AspOutPrinter::reportModel(const Clasp::Solver& s, const Clasp::Enumerator& e) {
	std::cout << "Model " << e.enumerated << ": \n";
	// get the symbol table from the solver
	const Clasp::SymbolTable& symTab = s.sharedContext()->symTab();
	for (Clasp::SymbolTable::const_iterator it = symTab.begin(); it != symTab.end(); ++it) {
		// print each named atom that is true w.r.t the current assignment
		if (s.isTrue(it->second.lit) && !it->second.name.empty()) {
			std::cout << it->second.name.c_str() << " ";
		}
	}
	std::cout << std::endl;
}
void AspOutPrinter::reportSolution(const Clasp::Solver&, const Clasp::Enumerator&, bool complete) {
	if (complete) std::cout << "No more models!" << std::endl;
	else          std::cout << "More models possible!" << std::endl;
}
*/

// ============================== ClaspSolver ==============================

void ClaspSolver::ModelEnumerator::reportModel(const Clasp::Solver& s, const Clasp::Enumerator&){

	DBGLOG(DBG, "ClaspThread: Start producing a model");

//	cs.sem_dlvhexDataStructures.wait();
//	DBGLOG(DBG, "ClaspThread: Entering code which needs exclusive access to dlvhex data structures");

	// create a model
	// this line does not need exclusive access to dlvhex data structures as it sets only a reference to the registry, but does not access it
	InterpretationPtr model = InterpretationPtr(new Interpretation(cs.reg));

//	DBGLOG(DBG, "ClaspThread: Leaving code which needs exclusive access to dlvhex data structures");
//	cs.sem_dlvhexDataStructures.post();

	// get the symbol table from the solver
	const Clasp::SymbolTable& symTab = s.sharedContext()->symTab();
	for (Clasp::SymbolTable::const_iterator it = symTab.begin(); it != symTab.end(); ++it) {
		// translate each named atom that is true w.r.t the current assignment into our dlvhex ID
		if (s.isTrue(it->second.lit) && !it->second.name.empty()) {
			IDAddress adr = ClaspSolver::stringToIDAddress(it->second.name.c_str());
			// set it in the model
			model->setFact(adr);
		}
	}

	// remember the model
	DBGLOG(DBG, "ClaspThread: Produced a model");

	// thread-safe queue access
	if (!cs.strictSingleThreaded){
		{
			// get lock and wait until there is free space in the model queue
			boost::mutex::scoped_lock lock(cs.modelsMutex);
			while(cs.preparedModels.size() >= ClaspSolver::MODELQUEUE_MAXSIZE){
				DBGLOG(DBG, "Model queue is full; Waiting for models to be retrieved by MainThread");
				cs.waitForQueueSpaceCondition.wait(lock);
			}

			DBGLOG(DBG, "Adding new model to model queue");
			cs.preparedModels.push(model);
		}

		DBGLOG(DBG, "Notifying MainThread about new model");
		cs.waitForModelCondition.notify_all();
	}else{
		cs.preparedModels.push(model);
		DBGLOG(DBG, "Notifying MainThread about new model");
		cs.sem_answer.post();
		DBGLOG(DBG, "ClaspThread: Waiting for further model requests");
		cs.sem_request.wait();
	}

	// @TODO: find a breakout possibility to terminate only the current thread!
	//        the following throw kills the whole application, which is not what we want
	static const bool quickTerminationMethod = true;
	if (quickTerminationMethod && cs.terminationRequest) throw ClaspSolver::ClaspTermination();
}

void ClaspSolver::ModelEnumerator::reportSolution(const Clasp::Solver& s, const Clasp::Enumerator&, bool complete){
}

bool ClaspSolver::ExternalPropagator::prop(Clasp::Solver& s, bool onlyOnCurrentDL){

	bool inconsistent = false;

	// thread-safe access of the learner vector
        boost::mutex::scoped_lock lock(cs.learnerMutex);
	if (cs.learner.size() != 0){
		// Wait until MainThread executes code of this class (in particular: getNextModel() ),
		// because only in this case we know what MainThread is doing and which dlvhex data structures it accesses.
		// Otherwise we could have a lot of unsynchronized data accesses.
		if (!cs.strictSingleThreaded){
			cs.sem_dlvhexDataStructures.wait();
			DBGLOG(DBG, "ClaspThread: Entering code which needs exclusive access to dlvhex data structures");
		}

		DBGLOG(DBG, "Translating clasp assignment to HEX-interpretation");
		// create an interpretation and a bitset of assigned values
		InterpretationPtr interpretation = InterpretationPtr(new Interpretation(cs.reg));
		InterpretationPtr factWasSet = InterpretationPtr(new Interpretation(cs.reg));
//		Interpretation::Storage interpretation;
//		Interpretation::Storage factWasSet;

		// translate clasp assignment to hex assignment
		// get the symbol table from the solver
		const Clasp::SymbolTable& symTab = s.sharedContext()->symTab();
		for (Clasp::SymbolTable::const_iterator it = symTab.begin(); it != symTab.end(); ++it) {
			// bitset of all assigned values
			if (s.isTrue(it->second.lit) || s.isFalse(it->second.lit)) {
				IDAddress adr = ClaspSolver::stringToIDAddress(it->second.name.c_str());
				factWasSet->setFact(adr);
			}
			// bitset of true values (partial interpretation)
			if (s.isTrue(it->second.lit)) {
				IDAddress adr = ClaspSolver::stringToIDAddress(it->second.name.c_str());
				interpretation->setFact(adr);
			}
		}
		// a fact changed iff
		// 1. (a) it was previously set but is not set now, or (b) it was previously not set but is set now; or
		// 2. it was set before and is still set but the truth value is different
		InterpretationPtr changed = InterpretationPtr(new Interpretation(cs.reg));
		if (!previousInterpretation || !previousFactWasSet){
			changed = factWasSet;	// everything changed
		}else{
			changed->getStorage() |= (factWasSet->getStorage() ^ previousFactWasSet->getStorage());
			changed->getStorage() |= (factWasSet->getStorage() & previousFactWasSet->getStorage() & (interpretation->getStorage() ^ previousInterpretation->getStorage()));
		}
		DBGLOG(DBG, "Changed truth values: " << *changed);
		previousInterpretation = interpretation;
		previousFactWasSet = factWasSet;

		DBGLOG(DBG, "Calling external learners");
		bool conflict = false;
		BOOST_FOREACH (LearningCallback* cb, cs.learner){
			conflict |= cb->learn(interpretation, factWasSet, changed);
		}
		// don't clear the changed literals if the external learner detected a conflict;
		// the learner will probably need to recheck the literals in the next call
		if (!conflict) changed->clear();

		interpretation.reset();
		factWasSet.reset();

		// Now MainThread is allowed to access arbitrary code again, because we continue executing Clasp code,
		// which cannot interfere with dlvhex.
		if (!cs.strictSingleThreaded){
			DBGLOG(DBG, "ClaspThread: Leaving code which needs exclusive access to dlvhex data structures");
			cs.sem_dlvhexDataStructures.post();
		}

//		inconsistent = conflict;
	}

	// add the produced nogoods to clasp
	{
	        boost::mutex::scoped_lock lock(cs.nogoodsMutex);

		DBGLOG(DBG, "External learners have produced " << (cs.nogoods.size() - cs.translatedNogoods) << " nogoods; transferring to clasp");
		for (int i = cs.translatedNogoods; i < cs.nogoods.size(); ++i){
			inconsistent |= cs.addNogoodToClasp(s, cs.nogoods[i], onlyOnCurrentDL);
		}

		if (!onlyOnCurrentDL){
			cs.translatedNogoods = cs.nogoods.size();
		}
	}
	DBGLOG(DBG, "Result: " << (inconsistent ? "" : "not ") << "inconsistent");

	return !inconsistent;
}

bool ClaspSolver::ExternalPropagator::propagate(Clasp::Solver& s){
	return prop(s);
}

bool ClaspSolver::ExternalPropagator::isModel(Clasp::Solver& s){
	// in this method we must not add nogoods which cause no conflict on the current decision level!
	// (see postcondition in clasp/constraint.h)
	return prop(s, true);
}

uint32 ClaspSolver::ExternalPropagator::priority() const{
	return Clasp::PostPropagator::priority_general;
}

bool ClaspSolver::addNogoodToClasp(Clasp::Solver& s, Nogood& ng, bool onlyOnCurrentDL){

#ifndef NDEBUG
	std::stringstream ss;
	ss << "{ ";
	bool first = true;
#endif
	// only nogoods are relevant where all variables occur in this clasp instance
	BOOST_FOREACH (ID lit, ng){
		if (hexToClasp.find(lit.address) == hexToClasp.end()){
			DBGLOG(DBG, "Skipping nogood because a literal is not in Clasp's literal list");
			return false;
		}
	}

	// translate dlvhex::Nogood to clasp clause
	bool onCurrentDL = false;
	clauseCreator->start(Clasp::Constraint_t::learnt_other);
	Set<uint32> pos;
	Set<uint32> neg;
	BOOST_FOREACH (ID lit, ng){
		// avoid duplicate literals
		// if the literal was already added with the same sign, skip it
		// if it was added with different sign, cancel adding the clause
		if (!(hexToClasp[lit.address].sign() ^ lit.isNaf())){
			if (pos.contains(hexToClasp[lit.address].var())) continue;
			else if (neg.contains(hexToClasp[lit.address].var())) return false;
			pos.insert(hexToClasp[lit.address].var());

			if (s.level(hexToClasp[lit.address].var()) == s.decisionLevel()) onCurrentDL = true;
		}else{
			if (neg.contains(hexToClasp[lit.address].var())) continue;
			else if (pos.contains(hexToClasp[lit.address].var())) return false;
			neg.insert(hexToClasp[lit.address].var());

			if (s.level(hexToClasp[lit.address].var()) == s.decisionLevel()) onCurrentDL = true;
		}

		// if this is requested, do not add conflict clauses which do not cause a conflict on the current decision level
		// (if this method is called by isModel() then is must not cause conflicts except on the top level)
		if (onlyOnCurrentDL && !onCurrentDL){
			DBGLOG(DBG, "Do not add " << ng << " because it is not conflicting on the current decision level");
			return false;
		}

		// 1. cs.hexToClasp maps hex-atoms to clasp-literals
		// 2. the sign must be changed if the hex-atom was default-negated (xor ^)
		// 3. the overall sign must be changed (negation !) because we work with nogoods and clasp works with clauses
		Clasp::Literal clit = Clasp::Literal(hexToClasp[lit.address].var(), !(hexToClasp[lit.address].sign() ^ lit.isNaf()));
		clauseCreator->add(clit);
#ifndef NDEBUG
		if (!first) ss << ", ";
		first = false;
		ss << (clit.sign() ? "" : "!") << clit.var();
#endif
	}

#ifndef NDEBUG
	ss << " }";
#endif

	DBGLOG(DBG, "Adding nogood " << ng << (onlyOnCurrentDL ? " at current DL " : "") << " as clasp-clause " << ss.str());

	return !Clasp::ClauseCreator::create(s, clauseCreator->lits(), Clasp::ClauseCreator::clause_known_order | Clasp::ClauseCreator::clause_not_sat, Clasp::Constraint_t::learnt_other).ok;
}

std::vector<std::vector<ID> > ClaspSolver::convertClaspNogood(Clasp::LearntConstraint& learnedConstraint){

	if (learnedConstraint.clause()){
		Clasp::LitVec lv;
		learnedConstraint.clause()->toLits(lv);
		return convertClaspNogood(lv);
	}
}

std::vector<std::vector<ID> > ClaspSolver::convertClaspNogood(const Clasp::LitVec& litvec){

	// A clasp literal possibly maps to multiple dlvhex literals
	// (due to optimization, equivalent and antivalent variables are represented by the same clasp literal).
	// Therefore a clasp clause can represent several dlvhex nogoods.
	// The result of this function is a vector of vectors of IDs.
	// The outer vector has one element for each clasp literal. The inner vector enumerates all possible back-translations of the clasp literal to dlvhex.

	std::vector<std::vector<ID> > ret;

	BOOST_FOREACH (Clasp::Literal l, litvec){
		// create for each clasp literal a vector of all possible back-translations to dlvhex
		std::vector<ID> translations;
		for (int i = 0; i < claspToHex[l].size(); ++i) translations.push_back(ID(ID::MAINKIND_LITERAL | ID::SUBKIND_ATOM_ORDINARYG | ID::NAF_MASK, claspToHex[l][i]));
		Clasp::Literal ln(l.var(), !l.sign());
		for (int i = 0; i < claspToHex[ln].size(); ++i) translations.push_back(ID(ID::MAINKIND_LITERAL | ID::SUBKIND_ATOM_ORDINARYG | ID::NAF_MASK, claspToHex[ln][i]));
		ret.push_back(translations);
	}
	return ret;
}

std::vector<Nogood> ClaspSolver::convertClaspNogood(std::vector<std::vector<ID> >& nogoods){

	// The method "unfolds" a set of nogoods, represented as
	// { l1[1], ..., l1[n1] } x { l2[1], ..., l2[n2] } x ... x { lk[1], ..., lk[nk] }
	// into a set of nogoods

	std::vector<Nogood> ret;
	if (nogoods.size() == 0) return ret;

	std::vector<int> ind(nogoods.size());
	for (int i = 0; i < nogoods.size(); ++i) ind[i] = 0;
	while (true){
		if (ind[0] >= nogoods[0].size()) break;

		// translate
		Nogood ng;
		for (int i = 0; i < nogoods.size(); ++i){
			ng.insert(nogoods[i][ind[i]]);
		}
		ret.push_back(ng);

		int k = nogoods.size() - 1;
		ind[k]++;
		while (ind[k] >= nogoods[k].size()){
			ind[k] = 0;
			k--;
			if (k < 0) break;
			ind[k]++;
			if (ind[0] >= nogoods[0].size()) break;
		}
	}

	return ret;
}

void ClaspSolver::buildInitialSymbolTable(const OrdinaryASPProgram& p, Clasp::ProgramBuilder& pb){

	DBGLOG(DBG, "Building atom index");

	// edb
	bm::bvector<>::enumerator en = p.edb->getStorage().first();
	bm::bvector<>::enumerator en_end = p.edb->getStorage().end();
	while (en < en_end){
		if (hexToClasp.find(*en) == hexToClasp.end()){
			uint32_t c = *en + p.idb.size() + 2;
			DBGLOG(DBG, "Clasp index of atom " << *en << " is " << c);
			hexToClasp[*en] = Clasp::Literal(c, true);

			std::string str = idAddressToString(*en);
			claspInstance.symTab().addUnique(c, str.c_str());
		}
		en++;
	}

	// idb
	BOOST_FOREACH (ID ruleId, p.idb){
		const Rule& rule = reg->rules.getByID(ruleId);
		BOOST_FOREACH (ID h, rule.head){
			if (hexToClasp.find(h.address) == hexToClasp.end()){
				uint32_t c = h.address + p.idb.size() + 2;
				DBGLOG(DBG, "Clasp index of atom " << h.address << " is " << c);
				hexToClasp[h.address] = Clasp::Literal(c, true);

				std::string str = idAddressToString(h.address);
				claspInstance.symTab().addUnique(c, str.c_str());
			}
		}
		BOOST_FOREACH (ID b, rule.body){
			if (hexToClasp.find(b.address) == hexToClasp.end()){
				uint32_t c = b.address + p.idb.size() + 2;
				DBGLOG(DBG, "Clasp index of atom " << b.address << " is " << c);
				hexToClasp[b.address] = Clasp::Literal(c, true);

				std::string str = idAddressToString(b.address);
				claspInstance.symTab().addUnique(c, str.c_str());
			}
		}
	}
}

void ClaspSolver::buildInitialSymbolTable(const NogoodSet& ns){

	DBGLOG(DBG, "Building atom index");

	claspInstance.symTab().startInit();
	BOOST_FOREACH (Nogood ng, ns.nogoods){
		BOOST_FOREACH (ID lit, ng){
			if (hexToClasp.find(lit.address) == hexToClasp.end()){
				uint32_t c = claspInstance.addVar(Clasp::Var_t::atom_var); //lit.address + 2;
				std::string str = idAddressToString(lit.address);
				DBGLOG(DBG, "Clasp index of atom " << lit.address << " is " << c);
				hexToClasp[lit.address] = Clasp::Literal(c, true);
				claspToHex[Clasp::Literal(c, true)].push_back(lit.address);
				claspInstance.symTab().addUnique(c, str.c_str()).lit = Clasp::Literal(c, true);
			}
		}
	}
	claspInstance.symTab().endInit();
}

void ClaspSolver::buildOptimizedSymbolTable(){

	hexToClasp.clear();

#ifndef NDEBUG
	std::stringstream ss;
#endif

	// go through symbol table
	const Clasp::SymbolTable& symTab = claspInstance.symTab();
	for (Clasp::SymbolTable::const_iterator it = symTab.begin(); it != symTab.end(); ++it) {
		IDAddress hexAdr = stringToIDAddress(it->second.name.c_str());
		hexToClasp[hexAdr] = it->second.lit;
		claspToHex[it->second.lit].push_back(hexAdr);
#ifndef NDEBUG
		ss << "Hex " << hexAdr << " <--> " << (it->second.lit.sign() ? "" : "!") << it->second.lit.var() << std::endl;
#endif
	}
	DBGLOG(DBG, "Symbol table of optimized program: " << std::endl << ss.str());
}

std::string ClaspSolver::idAddressToString(IDAddress adr){
	std::stringstream ss;
	ss << adr;
	return ss.str();
}

IDAddress ClaspSolver::stringToIDAddress(std::string str){
	return atoi(str.c_str());
}

void ClaspSolver::runClasp(){

	DBGLOG(DBG, "ClaspThread: Initialization");
	if (strictSingleThreaded){
		DBGLOG(DBG, "ClaspThread: Waiting for requests");
		sem_request.wait();	// continue with execution of MainThread
	}

	try{
		Clasp::solve(claspInstance, params);
	}catch(ClaspSolver::ClaspTermination){
		DBGLOG(DBG, "Clasp was requested to terminate before all models were enumerated");
	}catch(...){
		throw;
	}

	DBGLOG(DBG, "Clasp terminated");

	{
		DBGLOG(DBG, "Notifying MainThread about end of models");
		boost::mutex::scoped_lock lock(modelsMutex);
		endOfModels = true;
	}
	if (!strictSingleThreaded){
		waitForModelCondition.notify_all();
	}else{
		sem_answer.post();
	}
}

bool ClaspSolver::sendProgramToClasp(const AnnotatedGroundProgram& p, DisjunctionMode dm){

	const int false_ = 1;	// 1 is our constant "false"

//	eqOptions.iters = 0;	// disable program optimization
	pb.startProgram(claspInstance, eqOptions);
	pb.setCompute(false_, false);

	buildInitialSymbolTable(p.getGroundProgram(), pb);

#ifndef NDEBUG
	std::stringstream programstring;
	RawPrinter printer(programstring, reg);
#endif

	// transfer edb
	DBGLOG(DBG, "Sending EDB to clasp");
	bm::bvector<>::enumerator en = p.getGroundProgram().edb->getStorage().first();
	bm::bvector<>::enumerator en_end = p.getGroundProgram().edb->getStorage().end();
	while (en < en_end){
		// add fact
		pb.startRule();
		pb.addHead(hexToClasp[*en].var());
		pb.endRule();

		en++;
	}
#ifndef NDEBUG
	programstring << *p.getGroundProgram().edb << std::endl;
#endif

	// transfer idb
	DBGLOG(DBG, "Sending IDB to clasp");
	int ruleIndex = -1;
	BOOST_FOREACH (ID ruleId, p.getGroundProgram().idb){
		ruleIndex++;

		const Rule& rule = reg->rules.getByID(ruleId);
#ifndef NDEBUG
		programstring << (rule.head.size() == 0 ? "(constraint)" : "(rule)") << " ";
		printer.print(ruleId);
		programstring << std::endl;
#endif
		if (rule.head.size() > 1){
			if (dm == Shifting || !p.containsHeadCycles(ruleId) || rule.isEAGuessingRule()){	// EA-guessing rules cannot be involved in head cycles, therefore we can shift it
				// shifting
				DBGLOG(DBG, "Shifting disjunctive rule " << ruleId);
				for (int keep = 0; keep < rule.head.size(); ++keep){
					pb.startRule(Clasp::BASICRULE);
					int hi = 0;
					BOOST_FOREACH (ID h, rule.head){
						if (hi == keep){
							// add literal to head
							pb.addHead(hexToClasp[h.address].var());
						}
						hi++;
					}
					BOOST_FOREACH (ID b, rule.body){
						// add literal to body
						pb.addToBody(hexToClasp[b.address].var(), !b.isNaf());
					}
					// shifted head atoms
					hi = 0;
					BOOST_FOREACH (ID h, rule.head){
						if (hi != keep){
							// add literal to head
							pb.addToBody(hexToClasp[h.address].var(), false);
						}
						hi++;
					}
					pb.endRule();
				}
			}else if (dm == ChoiceRules){
				DBGLOG(DBG, "Generating choice for disjunctive rule " << ruleId);
				// derive head atoms
				pb.startRule(Clasp::CHOICERULE);
				BOOST_FOREACH (ID h, rule.head){
					pb.addHead(hexToClasp[h.address].var());
				}
				BOOST_FOREACH (ID b, rule.body){
					pb.addToBody(hexToClasp[b.address].var(), !b.isNaf());
				}
				pb.endRule();

				// derive special atom if at least one head atom is true
				pb.startRule(Clasp::CONSTRAINTRULE, 1);
				pb.addHead(2 + ruleIndex);
				BOOST_FOREACH (ID h, rule.head){
					pb.addToBody(hexToClasp[h.address].var(), true);
				}
				pb.endRule();

				// forbid that the body is true if the special atom is false (i.e. no head atom is true)
				pb.startRule(Clasp::BASICRULE);
				pb.addHead(false_);
				BOOST_FOREACH (ID b, rule.body){
					pb.addToBody(hexToClasp[b.address].var(), !b.isNaf());
				}
				pb.addToBody(2 + ruleIndex, false);
				pb.endRule();
			}
		}else{
			pb.startRule(Clasp::BASICRULE);
			if (rule.head.size() == 0){
				pb.addHead(false_);
			}
			BOOST_FOREACH (ID h, rule.head){
				// add literal to head
				pb.addHead(hexToClasp[h.address].var());
			}
			BOOST_FOREACH (ID b, rule.body){
				// add literal to body
				pb.addToBody(hexToClasp[b.address].var(), !b.isNaf());
			}
			pb.endRule();
		}
	}

#ifndef NDEBUG
	DBGLOG(DBG, "Program is: " << std::endl << programstring.str());
#endif
	// Once all rules are defined, call endProgram() to load the (simplified)
	bool initiallyInconsistent = !pb.endProgram();

	// rebuild the symbol table as it might have changed due to optimization
	buildOptimizedSymbolTable();

	return initiallyInconsistent;
}

bool ClaspSolver::sendNogoodSetToClasp(const NogoodSet& ns){

	const int false_ = 1;	// 1 is our constant "false"

	buildInitialSymbolTable(ns);

	DBGLOG(DBG, "Sending NogoodSet to clasp: " << ns);
	bool initiallyInconsistent = false;

	claspInstance.startAddConstraints();

	BOOST_FOREACH (Nogood ng, ns.nogoods){

#ifndef NDEBUG
		std::stringstream ss;
		ss << "{ ";
		bool first = true;
#endif

		// only nogoods are relevant where all variables occur in this clasp instance
		BOOST_FOREACH (ID lit, ng){
			if (hexToClasp.find(lit.address) == hexToClasp.end()){
				DBGLOG(DBG, "Skipping nogood because a literal is not in Clasp's literal list");
				return false;
			}
		}

		// translate dlvhex::Nogood to clasp clause
		clauseCreator->start();
		Set<uint32> pos;
		Set<uint32> neg;
		BOOST_FOREACH (ID lit, ng){
			// avoid duplicate literals
			// if the literal was already added with the same sign, skip it
			// if it was added with different sign, cancel adding the clause
			if (!(hexToClasp[lit.address].sign() ^ lit.isNaf())){
				if (pos.contains(hexToClasp[lit.address].var())) continue;
				else if (neg.contains(hexToClasp[lit.address].var())) return false;
				pos.insert(hexToClasp[lit.address].var());
			}else{
				if (neg.contains(hexToClasp[lit.address].var())) continue;
				else if (pos.contains(hexToClasp[lit.address].var())) return false;
				neg.insert(hexToClasp[lit.address].var());
			}

			// 1. cs.hexToClasp maps hex-atoms to clasp-literals
			// 2. the sign must be changed if the hex-atom was default-negated (xor ^)
			// 3. the overall sign must be changed (negation !) because we work with nogoods and clasp works with clauses
			Clasp::Literal clit = Clasp::Literal(hexToClasp[lit.address].var(), !(hexToClasp[lit.address].sign() ^ lit.isNaf()));
			clauseCreator->add(clit);
#ifndef NDEBUG
			if (!first) ss << ", ";
			first = false;
			ss << (clit.sign() ? "" : "!") << clit.var();
#endif
		}

#ifndef NDEBUG
		ss << " }";
#endif

		DBGLOG(DBG, "Adding nogood " << ng << " as clasp-clause " << ss.str());
		initiallyInconsistent |= !Clasp::ClauseCreator::create(*claspInstance.master(), clauseCreator->lits(), Clasp::ClauseCreator::clause_known_order).ok;
	}

	return initiallyInconsistent;
}

ClaspSolver::ClaspSolver(ProgramCtx& c, const OrdinaryASPProgram& p, bool interleavedThreading, DisjunctionMode dm) : ctx(c), projectionMask(p.mask), sem_request(0), sem_answer(0), terminationRequest(false), endOfModels(false), translatedNogoods(0), sem_dlvhexDataStructures(1), strictSingleThreaded(!interleavedThreading), claspStarted(false)
{
	assert(false);
}

ClaspSolver::ClaspSolver(ProgramCtx& c, const AnnotatedGroundProgram& p, bool interleavedThreading, DisjunctionMode dm) : ctx(c), projectionMask(p.getGroundProgram().mask), sem_request(0), sem_answer(0), terminationRequest(false), endOfModels(false), translatedNogoods(0), sem_dlvhexDataStructures(1), strictSingleThreaded(!interleavedThreading), claspStarted(false)
{
	DBGLOG(DBG, "Starting ClaspSolver (ASP) in " << (strictSingleThreaded ? "single" : "multi") << "threaded mode");
	reg = ctx.registry();

	clauseCreator = new Clasp::ClauseCreator(claspInstance.master());
	bool initiallyInconsistent = sendProgramToClasp(p, dm);
	DBGLOG(DBG, "Initially inconsistent: " << initiallyInconsistent);

	// if the program is initially inconsistent we do not need to do a search at all
	modelCount = 0;
	if (initiallyInconsistent){
		endOfModels = true;
		claspThread = NULL;
	}else{
		if (pb.dependencyGraph() && pb.dependencyGraph()->nodes() > 0) {
			DBGLOG(DBG, "Adding unfounded set checker");
			Clasp::DefaultUnfoundedCheck* ufs = new Clasp::DefaultUnfoundedCheck();
			ufs->attachTo(*claspInstance.master(), pb.dependencyGraph()); // register with solver and graph & transfer ownership
		}

		std::stringstream prog;
		pb.writeProgram(prog);
		DBGLOG(DBG, "Program in LParse format: " << prog.str());

		// add enumerator
		DBGLOG(DBG, "Adding enumerator");
		claspInstance.addEnumerator(new Clasp::BacktrackEnumerator(0, new ModelEnumerator(*this)));
		claspInstance.enumerator()->enumerate(0);

		// add propagator
		DBGLOG(DBG, "Adding external propagator");
		ExternalPropagator* ep = new ExternalPropagator(*this);
		claspInstance.addPost(ep);

		// endInit() must be called once before the search starts
		DBGLOG(DBG, "Finalizing clasp initialization");
		claspInstance.endInit();
	}

	if (!strictSingleThreaded){
		// We now return to dlvhex code which is not in this class.
		// As we do not know what MainThread is doing there, ClaspThread must not access dlvhex data structures.
		DBGLOG(DBG, "MainThread: Entering code which needs exclusive access to dlvhex data structures");
		sem_dlvhexDataStructures.wait();
	}
}


ClaspSolver::ClaspSolver(ProgramCtx& c, const NogoodSet& ns, bool interleavedThreading) : ctx(c), sem_request(0), sem_answer(0), terminationRequest(false), endOfModels(false), translatedNogoods(0), sem_dlvhexDataStructures(1), strictSingleThreaded(!interleavedThreading), claspStarted(false)
{
	DBGLOG(DBG, "Starting ClaspSolver (SAT) in " << (strictSingleThreaded ? "single" : "multi") << "threaded mode");
	reg = ctx.registry();

	clauseCreator = new Clasp::ClauseCreator(claspInstance.master());

	bool initiallyInconsistent = sendNogoodSetToClasp(ns);
	DBGLOG(DBG, "Initially inconsistent: " << initiallyInconsistent);

	// if the program is initially inconsistent we do not need to do a search at all
	modelCount = 0;
	if (initiallyInconsistent){
		endOfModels = true;
		claspThread = NULL;
	}else{
		// add enumerator
		DBGLOG(DBG, "Adding enumerator");
		claspInstance.addEnumerator(new Clasp::BacktrackEnumerator(0, new ModelEnumerator(*this)));
		claspInstance.enumerator()->enumerate(0);

		// add propagator
		DBGLOG(DBG, "Adding external propagator");
		ExternalPropagator* ep = new ExternalPropagator(*this);
		claspInstance.addPost(ep);

		// endInit() must be called once before the search starts
		DBGLOG(DBG, "Finalizing clasp initialization");
		claspInstance.endInit();
	}

	if (!strictSingleThreaded){
		// We now return to dlvhex code which is not in this class.
		// As we do not know what MainThread is doing there, ClaspThread must not access dlvhex data structures.
		DBGLOG(DBG, "MainThread: Entering code which needs exclusive access to dlvhex data structures");
		sem_dlvhexDataStructures.wait();
	}
}

ClaspSolver::~ClaspSolver(){

	if (!strictSingleThreaded){
		sem_dlvhexDataStructures.post();
		DBGLOG(DBG, "MainThread: Leaving code which needs exclusive access to dlvhex data structures");
	}

	DBGLOG(DBG, "ClaspSolver Destructor");
	{
		// send termination request
		boost::mutex::scoped_lock lock(modelsMutex);
		terminationRequest = true;
	}

	// is clasp still active?
	while (getNextModel() != InterpretationConstPtr());
	DBGLOG(DBG, "Joining ClaspThread");
	if (claspThread) claspThread->join();

	DBGLOG(DBG, "Deleting ClauseCreator");
	delete clauseCreator;

	DBGLOG(DBG, "Deleting ClaspThread");
	if (claspThread) delete claspThread;
}

std::string ClaspSolver::getStatistics(){
	std::stringstream ss;
	ss <<	"Guesses: " << claspInstance.master()->stats.choices << std::endl <<
		"Conflicts: " << claspInstance.master()->stats.conflicts << std::endl <<
		"Models: " << claspInstance.master()->stats.models;
	return ss.str();
}

void ClaspSolver::addExternalLearner(LearningCallback* lb){
	if (!strictSingleThreaded){
		sem_dlvhexDataStructures.post();
		DBGLOG(DBG, "MainThread: Leaving code which needs exclusive access to dlvhex data structures");
	}

	// access learner vector
        {
		boost::mutex::scoped_lock lock(learnerMutex);
		learner.insert(lb);
	}

	if (!strictSingleThreaded){
		sem_dlvhexDataStructures.wait();
		DBGLOG(DBG, "MainThread: Entering code which needs exclusive access to dlvhex data structures");
	}
}

void ClaspSolver::removeExternalLearner(LearningCallback* lb){
	if (!strictSingleThreaded){
		sem_dlvhexDataStructures.post();
		DBGLOG(DBG, "MainThread: Leaving code which needs exclusive access to dlvhex data structures");
	}

	// access learner vector
	{
	        boost::mutex::scoped_lock lock(learnerMutex);
		learner.erase(lb);
	}

	if (!strictSingleThreaded){
		sem_dlvhexDataStructures.wait();
		DBGLOG(DBG, "MainThread: Entering code which needs exclusive access to dlvhex data structures");
	}
}

int ClaspSolver::addNogood(Nogood ng){
	if (!strictSingleThreaded){
		sem_dlvhexDataStructures.post();
		DBGLOG(DBG, "MainThread: Leaving code which needs exclusive access to dlvhex data structures");
	}

	// access nogoods
	int s;
	{
	        boost::mutex::scoped_lock lock(nogoodsMutex);
		nogoods.push_back(ng);
		s = nogoods.size() - 1;
	}

	if (!strictSingleThreaded){
		sem_dlvhexDataStructures.wait();
		DBGLOG(DBG, "MainThread: Entering code which needs exclusive access to dlvhex data structures");
	}
	return s;
}

Nogood ClaspSolver::getNogood(int index){
	Nogood ng;

	if (!strictSingleThreaded){
		sem_dlvhexDataStructures.post();
		DBGLOG(DBG, "MainThread: Leaving code which needs exclusive access to dlvhex data structures");
	}

	{
		// access nogoods
        	boost::mutex::scoped_lock lock(nogoodsMutex);
		ng = nogoods[index];
	}

	if (!strictSingleThreaded){
		sem_dlvhexDataStructures.wait();
		DBGLOG(DBG, "MainThread: Entering code which needs exclusive access to dlvhex data structures");
	}

	return ng;
}

void ClaspSolver::removeNogood(int index){
        boost::mutex::scoped_lock lock(nogoodsMutex);
	throw std::runtime_error("ClaspSolver::removeNogood not implemented");
}

int ClaspSolver::getNogoodCount(){

	int s;

	if (!strictSingleThreaded){
		sem_dlvhexDataStructures.post();
		DBGLOG(DBG, "MainThread: Leaving code which needs exclusive access to dlvhex data structures");
	}

	{
		// access nogoods
	        boost::mutex::scoped_lock lock(nogoodsMutex);
		s = nogoods.size();
	}


	if (!strictSingleThreaded){
		sem_dlvhexDataStructures.wait();
		DBGLOG(DBG, "MainThread: Entering code which needs exclusive access to dlvhex data structures");
	}

	return s;
}

InterpretationConstPtr ClaspSolver::getNextModel(){

	// make sure that clasp runs
	if (!claspStarted && !endOfModels){
		DBGLOG(DBG, "Starting ClaspThread");
		claspThread = new boost::thread(boost::bind(&ClaspSolver::runClasp, this));
		claspStarted = true;
	}

	InterpretationConstPtr nextModel;

	if (!strictSingleThreaded){
		// MainThread now exectures code of this class, hence we know what it is doing.
		// As the code below does not interfere with simultanous accessed of dlvhex data structures,
		// ClaspThread is now allows to enter critical sections.
		DBGLOG(DBG, "MainThread: Leaving code which needs exclusive access to dlvhex data structures");
		sem_dlvhexDataStructures.post();

		{
			// get lock and wait until there is at least one model in the queue
			boost::mutex::scoped_lock lock(modelsMutex);
			while(!endOfModels && preparedModels.empty()){
				DBGLOG(DBG, "Model queue is empty (end endOfModels was not set yet); Waiting for ClaspThread to add models (or set endOfModels)");
				waitForModelCondition.wait(lock);
			}

			// now we have either a model or endOfModels is set
			// Note: both conditions may apply simultanously (the queue is not empty, but no more models will come, i.e. all remaining ones have arrived)
			if (preparedModels.size() == 0){
				// all prepared models are exhausted and also clasp has no more models
				DBGLOG(DBG, "End of models");
				nextModel = InterpretationConstPtr();
			}else{
				// return next prepared model
				nextModel = preparedModels.front();
				preparedModels.pop();
				DBGLOG(DBG, "MainThread: Got a model");
				modelCount++;
			}
		}
		DBGLOG(DBG, "Notifying ClaspThread about empty space in model queue");
		waitForQueueSpaceCondition.notify_all();

		// MainThread is now leaving this class. As we do not know what it is doing outside,
		// ClaspThread is now not allowed to access dlvhex data structures simultanously.
		sem_dlvhexDataStructures.wait();
		DBGLOG(DBG, "MainThread: Entering code which needs exclusive access to dlvhex data structures");
	}else{
		nextModel = InterpretationConstPtr();
		if (!endOfModels){
			DBGLOG(DBG, "MainThread: Sending NextModelRequest");
			sem_request.post();

			DBGLOG(DBG, "MainThread: Waiting for an answer");
			sem_answer.wait();

			if (endOfModels){
				DBGLOG(DBG, "End of models");
			}else{
				assert(preparedModels.size() > 0);
				DBGLOG(DBG, "MainThread: Got a model");
				nextModel = preparedModels.front();
				preparedModels.pop();
			}
		}
	}

	return nextModel;
}

int ClaspSolver::getModelCount(){
	return modelCount;
}

InterpretationPtr ClaspSolver::projectToOrdinaryAtoms(InterpretationConstPtr intr){
	if (intr == InterpretationConstPtr()){
		return InterpretationPtr();
	}else{
		InterpretationPtr answer = InterpretationPtr(new Interpretation(reg));
		answer->add(*intr);

		if (projectionMask != InterpretationConstPtr()){
			answer->getStorage() -= projectionMask->getStorage();
		}
		DBGLOG(DBG, "Projected " << *intr << " to " << *answer);
		return answer;
	}
}


// ============================== DisjunctiveClaspSolver ==============================

bool DisjunctiveClaspSolver::initHeadCycles(RegistryPtr reg, const OrdinaryASPProgram& p){

	// construct a simple atom level dependency graph
	typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, IDAddress> Graph;
	typedef Graph::vertex_descriptor Node;
	std::map<IDAddress, Node> depNodes;
	Graph depGraph;

	BOOST_FOREACH (ID ruleID, p.idb){
		const Rule& rule = reg->rules.getByID(ruleID);

		// add an arc from all head literals to all positive body literals
		BOOST_FOREACH (ID headLiteral, rule.head){
			// all atoms are nodes
			if (depNodes.find(headLiteral.address) == depNodes.end()){
				depNodes[headLiteral.address] = boost::add_vertex(headLiteral.address, depGraph);
			}

			BOOST_FOREACH (ID bodyLiteral, rule.body){
				if (!bodyLiteral.isNaf()){
					if (depNodes.find(bodyLiteral.address) == depNodes.end()){
						depNodes[bodyLiteral.address] = boost::add_vertex(bodyLiteral.address, depGraph);
					}
					boost::add_edge(depNodes[headLiteral.address], depNodes[bodyLiteral.address], depGraph);
				}
			}
		}
	}

	// find strongly connected components in the dependency graph using boost
	std::vector<int> componentMap(depNodes.size());
	int num = boost::strong_components(depGraph, &componentMap[0]);

	// translate into real map
	std::vector<Set<IDAddress> > depSCC = std::vector<Set<IDAddress> >(num);
	Node nodeNr = 0;
	BOOST_FOREACH (int componentOfNode, componentMap){
		depSCC[componentOfNode].insert(depGraph[nodeNr]);
		nodeNr++;
	}

	// check for head-cycles
	for (int compNr = 0; compNr < depSCC.size(); ++compNr){
		BOOST_FOREACH (ID ruleID, p.idb){
			int numberOfHeadLits = 0;
			const Rule& r = reg->rules.getByID(ruleID);
			BOOST_FOREACH (ID headLit, r.head){
				if (depSCC[compNr].count(headLit.address) > 0) numberOfHeadLits++;
			}
			if (numberOfHeadLits > 1){
				headCycles = true;
				return true;
			}
		}
	}
	headCycles = false;
	return false;
}

DisjunctiveClaspSolver::DisjunctiveClaspSolver(ProgramCtx& ctx, const OrdinaryASPProgram& p, bool interleavedThreading) :
	ClaspSolver(ctx, AnnotatedGroundProgram(ctx.registry(), p), interleavedThreading, /*initHeadCycles(ctx.registry(), p) ? ClaspSolver::ChoiceRules : ClaspSolver::Shifting*/ ClaspSolver::ChoiceRules),
	program(p), ufscm(ctx, AnnotatedGroundProgram(ctx.registry(), p)){
}

DisjunctiveClaspSolver::~DisjunctiveClaspSolver(){
}

InterpretationConstPtr DisjunctiveClaspSolver::getNextModel(){

	InterpretationConstPtr model = ClaspSolver::getNextModel();

	bool ufsFound = true;
	while (model && ufsFound){
		ufsFound = false;

		std::vector<IDAddress> ufs = ufscm.getUnfoundedSet(model);

//		UnfoundedSetChecker ufsc(ctx, program, model);
//		std::vector<IDAddress> ufs = ufsc.getUnfoundedSet();

		if (ufs.size() > 0){
			Nogood ng; // = ufscm.getUFSNogood(ufs, model);
//			Nogood ng = ufsc.getUFSNogood(ufs, model);
			addNogood(ng);
			ufsFound = true;
			model = ClaspSolver::getNextModel();
		}
	}
	return model;
}


DLVHEX_NAMESPACE_END

#endif
