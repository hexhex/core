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
#include "dlvhex2/Benchmarking.h"

#include <boost/foreach.hpp>
#include <boost/graph/strong_components.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include "clasp/program_rule.h"
#include "clasp/constraint.h"


DLVHEX_NAMESPACE_BEGIN

// ============================== ClaspSolver ==============================

void ClaspSolver::ModelEnumerator::reportModel(const Clasp::Solver& s, const Clasp::Enumerator&){

	DLVHEX_BENCHMARK_REGISTER(sidsolvertime, "Solver time");
	DBGLOG(DBG, "ClaspThread: Start producing a model");

	// create a model
	// this line does not need exclusive access to dlvhex data structures as it sets only a reference to the registry, but does not access it
	InterpretationPtr model = InterpretationPtr(new Interpretation(cs.reg));

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
			while(cs.preparedModels.size() >= cs.modelqueueSize){
				DBGLOG(DBG, "Model queue is full; Waiting for models to be retrieved by MainThread");
				DLVHEX_BENCHMARK_STOP(sidsolvertime);
				cs.waitForQueueSpaceCondition.wait(lock);
				DLVHEX_BENCHMARK_START(sidsolvertime);
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
		DLVHEX_BENCHMARK_STOP(sidsolvertime);
		cs.sem_request.wait();
		DLVHEX_BENCHMARK_START(sidsolvertime);
	}

	static const bool quickTerminationMethod = true;
	if (quickTerminationMethod && cs.terminationRequest) throw ClaspSolver::ClaspTermination();
}

void ClaspSolver::ModelEnumerator::reportSolution(const Clasp::Solver& s, const Clasp::Enumerator&, bool complete){
}

ClaspSolver::ExternalPropagator::ExternalPropagator(ClaspSolver& cs) : cs(cs){
	interpretation = InterpretationPtr(new Interpretation(cs.reg));
	previousInterpretation = InterpretationPtr(new Interpretation(cs.reg));
	factWasSet = InterpretationPtr(new Interpretation(cs.reg));
	previousFactWasSet = InterpretationPtr(new Interpretation(cs.reg));
	changed = InterpretationPtr(new Interpretation(cs.reg));
}

bool ClaspSolver::ExternalPropagator::prop(Clasp::Solver& s, bool onlyOnCurrentDL){

	// thread-safe access to the propagator vector
        boost::mutex::scoped_lock lock(cs.propagatorMutex);
	if (cs.propagator.size() != 0){
		// Wait until MainThread executes code of this class (in particular: getNextModel() ),
		// because only in this case we know what MainThread is doing and which dlvhex data structures it accesses.
		// Otherwise we could have a lot of unsynchronized data accesses.
		if (!cs.strictSingleThreaded){
			cs.sem_dlvhexDataStructures.wait();
			DBGLOG(DBG, "ClaspThread: Entering code which needs exclusive access to dlvhex data structures");
		}

		DBGLOG(DBG, "Translating clasp assignment to HEX-interpretation");

		// translate clasp assignment to hex assignment
		// get the symbol table from the solver
		interpretation->clear();
		factWasSet->clear();
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
		changed->clear();
		changed->getStorage() |= (factWasSet->getStorage() ^ previousFactWasSet->getStorage());
		changed->getStorage() |= (factWasSet->getStorage() & previousFactWasSet->getStorage() & (interpretation->getStorage() ^ previousInterpretation->getStorage()));
		DBGLOG(DBG, "Changed truth values: " << *changed);

		DBGLOG(DBG, "Calling external propagators");
		bool conflict = false;
		BOOST_FOREACH (PropagatorCallback* cb, cs.propagator){
			cb->propagate(interpretation, factWasSet, changed);
		}

		previousInterpretation->getStorage() = interpretation->getStorage();
		previousFactWasSet->getStorage() = factWasSet->getStorage();

		// Now MainThread is allowed to access arbitrary code again, because we continue executing Clasp code,
		// which cannot interfere with dlvhex.
		if (!cs.strictSingleThreaded){
			DBGLOG(DBG, "ClaspThread: Leaving code which needs exclusive access to dlvhex data structures");
			cs.sem_dlvhexDataStructures.post();
		}
	}

	// add the new nogoods to clasp
	bool inconsistent = false;
	{
	        boost::mutex::scoped_lock lock(cs.nogoodsMutex);

		DBGLOG(DBG, "External learners have produced " << cs.nogoods.size() << " nogoods; transferring to clasp");

		bool processed = true;
		while (cs.nogoods.size() > 0 && processed && !inconsistent){
			Nogood& ng = cs.nogoods.front();
			std::pair<bool, bool> ret = cs.addNogoodToClasp(s, ng, onlyOnCurrentDL);
			processed = ret.first;
			inconsistent = ret.second; // we must not add more clauses if we have already a conflict
			if (processed) cs.nogoods.pop();
		}
	}
	DBGLOG(DBG, "Result: " << (inconsistent ? "" : "not ") << "inconsistent");
	assert(!inconsistent || s.hasConflict());

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

/**
 * Adds a nogood to the running clasp instance.
 * @return std::pair<bool, bool>
 *             The first return value indicates if the nogood was successfully processed.
 *               That is, it is true iff it was either added or excluded from being added, and false if it might be added at some future point
               The second is true iff adding has produced a conflict
 */
std::pair<bool, bool> ClaspSolver::addNogoodToClasp(Clasp::Solver& s, Nogood& ng, bool onlyOnCurrentDL){

#ifndef NDEBUG
	std::stringstream ss;
	ss << "{ ";
	bool first = true;
#endif
	// only nogoods are relevant where all variables occur in this clasp instance
	BOOST_FOREACH (ID lit, ng){
		if (hexToClasp.find(lit.address) == hexToClasp.end()){
			DBGLOG(DBG, "Skipping nogood because a literal is not in Clasp's literal list");
			return std::pair<bool, bool>(true, false);
		}
	}

	// translate dlvhex::Nogood to clasp clause
	bool conflictOnLowerDL = true;
	clauseCreator->start(Clasp::Constraint_t::learnt_other);
	Set<uint32> pos;
	Set<uint32> neg;
	BOOST_FOREACH (ID lit, ng){
		// avoid duplicate literals
		// if the literal was already added with the same sign, skip it
		// if it was added with different sign, cancel adding the clause
		if (!(hexToClasp[lit.address].sign() ^ lit.isNaf())){
			if (pos.contains(hexToClasp[lit.address].var())) continue;
			else if (neg.contains(hexToClasp[lit.address].var())){
				DBGLOG(DBG, "Dropping tautological nogood");
				return std::pair<bool, bool>(true, false);
			}
			pos.insert(hexToClasp[lit.address].var());

			if (s.level(hexToClasp[lit.address].var()) == s.decisionLevel()) conflictOnLowerDL = false;
		}else{
			if (neg.contains(hexToClasp[lit.address].var())) continue;
			else if (pos.contains(hexToClasp[lit.address].var())){
				DBGLOG(DBG, "Dropping tautological nogood");
				return std::pair<bool, bool>(true, false);
			}
			neg.insert(hexToClasp[lit.address].var());

			if (s.level(hexToClasp[lit.address].var()) == s.decisionLevel()) conflictOnLowerDL = false;
		}

		// 1. cs.hexToClasp maps hex-atoms to clasp-literals
		// 2. the sign must be changed if the hex-atom was default-negated (xor ^)
		// 3. the overall sign must be changed (negation !) because we work with nogoods and clasp works with clauses
		Clasp::Literal clit = Clasp::Literal(hexToClasp[lit.address].var(), !(hexToClasp[lit.address].sign() ^ lit.isNaf()));
		clauseCreator->add(clit);

//		// non-conflicting clauses can always be added
//		if (!s.isFalse(clit)) conflictOnLowerDL = false;

		// if requested, do not add clauses which do not cause a conflict on the current decision level
		// (if this method is called by isModel() then we must not cause conflicts except on the top level)
		if (onlyOnCurrentDL && !s.isFalse(clit)){
			DBGLOG(DBG, "Do not add " << ng.getStringRepresentation(reg) << " because it is not conflicting on the current decision level (it is not conflicting at all)");
			return std::pair<bool, bool>(false, false);
		}
#ifndef NDEBUG
		if (!first) ss << ", ";
		first = false;
		ss << (clit.sign() ? "" : "!") << clit.var();
#endif
	}

	// if requested, do not add conflict clauses which cause a conflict on a decision level lower than the current one
	// (if this method is called by isModel() then we must not cause conflicts except on the top level)
	if (onlyOnCurrentDL && conflictOnLowerDL){
		DBGLOG(DBG, "Do not add " << ng.getStringRepresentation(reg) << " because it is conflicting on a lower decision level");
		return std::pair<bool, bool>(false, false);
	}

#ifndef NDEBUG
	ss << " }";
#endif

	DBGLOG(DBG, "Adding nogood " << ng.getStringRepresentation(reg) << (onlyOnCurrentDL ? " at current DL " : "") << " as clasp-clause " << ss.str());
	std::pair<bool, bool> ret(true, !Clasp::ClauseCreator::create(s, clauseCreator->lits(), Clasp::ClauseCreator::clause_known_order, Clasp::Constraint_t::learnt_other).ok);

	return ret;
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
			uint32_t c = *en + 2;
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
				uint32_t c = h.address + 2;
				DBGLOG(DBG, "Clasp index of atom " << h.address << " is " << c);
				hexToClasp[h.address] = Clasp::Literal(c, true);

				std::string str = idAddressToString(h.address);
				claspInstance.symTab().addUnique(c, str.c_str());
			}
		}
		BOOST_FOREACH (ID b, rule.body){
			if (hexToClasp.find(b.address) == hexToClasp.end()){
				uint32_t c = b.address + 2;
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
	for (int i = 0; i < ns.getNogoodCount(); i++){
		const Nogood& ng = ns.getNogood(i);
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
	DLVHEX_BENCHMARK_REGISTER(sidsolvertime, "Solver time");

	DBGLOG(DBG, "ClaspThread: Initialization");
	if (strictSingleThreaded){
		DBGLOG(DBG, "ClaspThread: Waiting for requests");
		sem_request.wait();	// continue with execution of MainThread
	}

	try{
		DLVHEX_BENCHMARK_START(sidsolvertime);
		Clasp::solve(claspInstance, params, assumptions);
		DLVHEX_BENCHMARK_STOP(sidsolvertime);
	}catch(ClaspSolver::ClaspTermination){
		DLVHEX_BENCHMARK_STOP(sidsolvertime);
		DBGLOG(DBG, "Clasp was requested to terminate before all models were enumerated");
	}catch(...){
		DLVHEX_BENCHMARK_STOP(sidsolvertime);
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

	// new clasp variables are located after all atom variables
	OrdinaryAtomTable::AddressIterator it_begin;
	OrdinaryAtomTable::AddressIterator it_end;
	boost::tie(it_begin, it_end) = reg->ogatoms.getAllByAddress();
	int nextVarIndex = 2 + (it_end - it_begin);

	std::map<IDAddress, std::vector<int> > singletonNogoods; // check support of singletons using shifted rules
	BOOST_FOREACH (ID ruleId, p.getGroundProgram().idb){
		int atLeastOneAtom = nextVarIndex++;

		if (ruleId.isWeakConstraint()) throw GeneralError("clasp-based solver does not support weak constraints");
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
						// add literal to body	BOOST_FOREACH(ID bodyLit, ruleBody){
						if (b.isAggregateAtom()) throw GeneralError("clasp-based solver does not support aggregate atoms");
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
				// ============================== Choice rule ==============================
				// derive head atoms
				pb.startRule(Clasp::CHOICERULE);
				BOOST_FOREACH (ID h, rule.head){
					pb.addHead(hexToClasp[h.address].var());
				}
				BOOST_FOREACH (ID b, rule.body){
					if (b.isAggregateAtom()) throw GeneralError("clasp-based solver does not support aggregate atoms");
					pb.addToBody(hexToClasp[b.address].var(), !b.isNaf());
				}
				pb.endRule();

				// derive special atom if at least one head atom is true
				pb.startRule(Clasp::CONSTRAINTRULE, 1);
				pb.addHead(atLeastOneAtom);
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
				pb.addToBody(atLeastOneAtom, false);
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
				if (b.isAggregateAtom()) throw GeneralError("clasp-based solver does not support aggregate atoms");
				pb.addToBody(hexToClasp[b.address].var(), !b.isNaf());
			}
			pb.endRule();
		}


		// ============================== Singleton loop nogoods ==============================
		// for non-shifted disjunctive rules, check support of singleton atoms
		DBGLOG(DBG, "Generating singleton loop nogoods");
		BOOST_FOREACH (ID h, rule.head){
#if 0	// these constraints model nogoods which are necessary in theory; however, since we work with rules instead of nogoods they are implicitly fulfilled by foundedness
			// derive shiftedBody iff all atoms in the shifted body are true
			BOOST_FOREACH (ID b, rule.body){	// shiftedBody is false if a body literal is false
				pb.startRule(Clasp::BASICRULE);
				pb.addHead(false_);
				pb.addToBody(nextVarIndex, true);
				pb.addToBody(hexToClasp[b.address].var(), b.isNaf());
				pb.endRule();
			}
			BOOST_FOREACH (ID hshifted, rule.head){
				if (h != hshifted){		// shiftedBody is false if a head atom is true, i.e. a body literal in the shifted rule is false
					pb.startRule(Clasp::BASICRULE);
					pb.addHead(false_);
					pb.addToBody(nextVarIndex, true);
					pb.addToBody(hexToClasp[hshifted.address].var(), true);
					pb.endRule();
				pb.addToBody(nextVarIndex, true);
				}
			}
#endif

			// otherwise shiftedBody is true
			pb.startRule(Clasp::BASICRULE);
			pb.addHead(nextVarIndex);
			BOOST_FOREACH (ID b, rule.body){
				pb.addToBody(hexToClasp[b.address].var(), !b.isNaf());
			}
			BOOST_FOREACH (ID hshifted, rule.head){
				if (h != hshifted){
					pb.addToBody(hexToClasp[hshifted.address].var(), false);
				}
			}
			pb.endRule();

			// remember supporting shifted rule
			singletonNogoods[h.address].push_back(nextVarIndex++);
		}
	}

	// ============================== Singleton loop nogoods ==============================

	// an atom is not true if no supporting shifted rule fires
	typedef std::pair<IDAddress, std::vector<int> > Pair;
	BOOST_FOREACH (Pair pair, singletonNogoods){
		// exception: facts are always true
		if (p.getGroundProgram().edb->getFact(pair.first)) continue;

		pb.startRule(Clasp::BASICRULE);
		pb.addHead(false_);
		pb.addToBody(hexToClasp[pair.first].var(), true);
		BOOST_FOREACH (int b, pair.second){
			pb.addToBody(b, false);
		}
		pb.endRule();
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

void ClaspSolver::addMinimizeConstraints(const AnnotatedGroundProgram& p){

	// one minimize statement for each level
	std::vector<Clasp::WeightLitVec> minimizeStatements;
#ifndef NDEBUG
	std::vector<std::vector<IDAddress> > minimizeStatementsHex;
#endif

	// construct the minimize statements for each level
	bm::bvector<>::enumerator en = p.getGroundProgram().edb->getStorage().first();
	bm::bvector<>::enumerator en_end = p.getGroundProgram().edb->getStorage().end();
	while (en < en_end){
		const OrdinaryAtom& weightAtom = reg->ogatoms.getByAddress(*en);
		if (weightAtom.tuple[0].isAuxiliary() && reg->getTypeByAuxiliaryConstantSymbol(weightAtom.tuple[0]) == 'w'){
			int level = weightAtom.tuple[2].address;
			while (minimizeStatements.size() <= level) minimizeStatements.push_back(Clasp::WeightLitVec());
			minimizeStatements[level].push_back(Clasp::WeightLiteral(Clasp::Literal(hexToClasp[*en].var(), hexToClasp[*en].sign()), weightAtom.tuple[1].address));
#ifndef NDEBUG
			while (minimizeStatementsHex.size() <= level) minimizeStatementsHex.push_back(std::vector<IDAddress>());
			minimizeStatementsHex[level].push_back(*en);
#endif
		}
		en++;
	}
	BOOST_FOREACH (ID ruleID, p.getGroundProgram().idb){
		const Rule& rule = reg->rules.getByID(ruleID);

		// check if this is a weight rule
		if (rule.head.size() == 1){
			const OrdinaryAtom& weightAtom = reg->ogatoms.getByID(rule.head[0]);
			if (weightAtom.tuple[0].isAuxiliary() && reg->getTypeByAuxiliaryConstantSymbol(weightAtom.tuple[0]) == 'w'){

				int level = weightAtom.tuple[2].address;
				while (minimizeStatements.size() <= level) minimizeStatements.push_back(Clasp::WeightLitVec());
				minimizeStatements[level].push_back(Clasp::WeightLiteral(Clasp::Literal(hexToClasp[rule.head[0].address].var(), hexToClasp[rule.head[0].address].sign()), weightAtom.tuple[1].address));

#ifndef NDEBUG
				while (minimizeStatementsHex.size() <= level) minimizeStatementsHex.push_back(std::vector<IDAddress>());
				minimizeStatementsHex[level].push_back(rule.head[0].address);
#endif
			}
		}
	}
//	if (minimizeStatements.size() == 0) return;

	// add the minimize statements to clasp
	for (int level = minimizeStatements.size() - 1; level >= 0; --level){
#ifndef NDEBUG
		std::stringstream ss;
		ss << "Minimize statement at level " << level << ": ";
		for (int l = 0; l < minimizeStatementsHex[level].size(); ++l){
			ss << (l > 0 ? ", " : "") << minimizeStatementsHex[level][l];
		}
		DBGLOG(DBG, ss.str());
#endif
		minb.addRule(minimizeStatements[level]);
	}

	DBGLOG(DBG, "Constructing minimize constraint");
	sharedMinimizeData = minb.build(claspInstance);
	minc = 0;
	if (!!sharedMinimizeData){
		sharedMinimizeData->setMode(Clasp::MinimizeMode_t::enumerate, true);
		minc = sharedMinimizeData->attach(*claspInstance.master(), true);

		// use the current optimum as upper bound for this unit
		setOptimum(ctx.currentOptimum);
	}
}

bool ClaspSolver::sendNogoodSetToClasp(const NogoodSet& ns){

	const int false_ = 1;	// 1 is our constant "false"

	buildInitialSymbolTable(ns);

	DBGLOG(DBG, "Sending NogoodSet to clasp: " << ns);
	bool initiallyInconsistent = false;

	claspInstance.startAddConstraints();

	for (int i = 0; i < ns.getNogoodCount(); i++){
		const Nogood& ng = ns.getNogood(i);

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

InterpretationPtr ClaspSolver::outputProjection(InterpretationConstPtr intr){
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

ClaspSolver::ClaspSolver(ProgramCtx& c, const AnnotatedGroundProgram& p, bool interleavedThreading, DisjunctionMode dm) : ctx(c), projectionMask(p.getGroundProgram().mask), sem_request(0), sem_answer(0), terminationRequest(false), endOfModels(false), sem_dlvhexDataStructures(1), strictSingleThreaded(!interleavedThreading), claspStarted(false), modelqueueSize(c.config.getOption("ModelQueueSize"))
{
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidsolvertime, "Solver time");
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
		claspInstance.addEnumerator(new Clasp::RecordEnumerator(new ModelEnumerator(*this)));
		claspInstance.enumerator()->enumerate(0);
/*
claspInstance.enumerator()->setMinimize(sharedMinimizeData);
std::cerr << "Optimize: " << claspInstance.enumerator()->optimize();
*/

		// respect weak constraints
		addMinimizeConstraints(p);

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


ClaspSolver::ClaspSolver(ProgramCtx& c, const NogoodSet& ns, bool interleavedThreading) : ctx(c), sem_request(0), sem_answer(0), terminationRequest(false), endOfModels(false), sem_dlvhexDataStructures(1), strictSingleThreaded(!interleavedThreading), claspStarted(false), modelqueueSize(c.config.getOption("ModelQueueSize"))
{
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidsolvertime, "Solver time");
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
	while (getNextModel() != InterpretationPtr());
	DBGLOG(DBG, "Joining ClaspThread");
	if (claspThread) claspThread->join();

	DBGLOG(DBG, "Deleting ClauseCreator");
	delete clauseCreator;

	DBGLOG(DBG, "Deleting ClaspThread");
	if (claspThread) delete claspThread;
}

void ClaspSolver::restartWithAssumptions(const std::vector<ID>& assumptions){

	// shutdown
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
	while (getNextModel() != InterpretationPtr());
	DBGLOG(DBG, "Joining ClaspThread");
	if (claspThread) claspThread->join();

	DBGLOG(DBG, "Deleting ClaspThread");
	if (claspThread) delete claspThread;

	// restart
	claspStarted = false;
	endOfModels = false;
	terminationRequest = false;

	this->assumptions.clear();
	BOOST_FOREACH (ID a, assumptions){
		Clasp::Literal al = Clasp::Literal(hexToClasp[a.address].var(), hexToClasp[a.address].sign() ^ a.isNaf());
		this->assumptions.push_back(al);
	}
}

void ClaspSolver::addPropagator(PropagatorCallback* pb){
	if (!strictSingleThreaded){
		sem_dlvhexDataStructures.post();
		DBGLOG(DBG, "MainThread: Leaving code which needs exclusive access to dlvhex data structures");
	}

	// access learner vector
        {
		boost::mutex::scoped_lock lock(propagatorMutex);
		propagator.insert(pb);
	}

	if (!strictSingleThreaded){
		sem_dlvhexDataStructures.wait();
		DBGLOG(DBG, "MainThread: Entering code which needs exclusive access to dlvhex data structures");
	}
}

void ClaspSolver::removePropagator(PropagatorCallback* pb){
	if (!strictSingleThreaded){
		sem_dlvhexDataStructures.post();
		DBGLOG(DBG, "MainThread: Leaving code which needs exclusive access to dlvhex data structures");
	}

	// access propagator vector
	{
	        boost::mutex::scoped_lock lock(propagatorMutex);
		propagator.erase(pb);
	}

	if (!strictSingleThreaded){
		sem_dlvhexDataStructures.wait();
		DBGLOG(DBG, "MainThread: Entering code which needs exclusive access to dlvhex data structures");
	}
}

void ClaspSolver::addNogood(Nogood ng){
	assert(ng.isGround());

	if (!strictSingleThreaded){
		sem_dlvhexDataStructures.post();
		DBGLOG(DBG, "MainThread: Leaving code which needs exclusive access to dlvhex data structures");
	}

	// access nogoods
	{
	        boost::mutex::scoped_lock lock(nogoodsMutex);
		nogoods.push(ng);
	}

	if (!strictSingleThreaded){
		sem_dlvhexDataStructures.wait();
		DBGLOG(DBG, "MainThread: Entering code which needs exclusive access to dlvhex data structures");
	}
}

void ClaspSolver::setOptimum(std::vector<int>& optimum){

	if (!minc) return;

	// This method helps the reasoner to eliminate non-optimal partial models in advance
	// by setting the internal upper bound to a given value.
	//
	// Warning: A call of this method is just a hint for the reasoner, i.e.,
	//          it is not guaranteed that the solver will no longer create models with higher cost.
	//
	// This is because clasp does not allow to decrease the upper bound if the new bound is violated
	// by the current assignment. Therefore, the new optimum is only integrated into the clasp instance
	// if it is compatible with the assignment.

	// transform optimum vector to clasp-internal representation
	int optlen = minb.numRules() < optimum.size() ? minb.numRules() : optimum.size();
	DBGLOG(DBG, "Transforming optimum (length: " << optlen << ") to clasp-internal representation");
	Clasp::wsum_t* newopt = new Clasp::wsum_t[optlen];
	for (int l = optlen - 1; l >= 0; --l){
		newopt[l] = optimum[optlen - 1 - l];
	}

	// check if the new upper bound is compatible with the current assignment
	DBGLOG(DBG, "Ensure that current assignment is compatible with the new optimum");
	bool violated = true;
	while (violated){
		violated = false;
		for (int i = 0; i < optlen; ++i){
			if (newopt[i] > minc->sum(i)) break;
			if (newopt[i] < minc->sum(i)){
				violated = true;
				break;
			}
		}
		if (!violated) break;

		delete []newopt;
		return;

/*
		DBGLOG(DBG, "Backtrack because current assignment violates optimum");
		uint32 dl = claspInstance.master()->decisionLevel();
		if (dl == claspInstance.master()->rootLevel()){
			DBGLOG(DBG, "Stop backtracking because root level reached");
			claspInstance.master()->setStopConflict();
			delete []newopt;
			return;
		}
		claspInstance.master()->undoUntil(dl - 1, true);
*/
	}

	// send the new upper bound to clasp
	DBGLOG(DBG, "Current assignment is compatible with the new optimum");
#ifndef NDEBUG
	std::stringstream ss;
	ss << "Setting optimum upper bound: ";
#endif
	for (int l = 0; l < optlen; ++l){
#ifndef NDEBUG
		ss << l << ":" << newopt[l] << " ";
#endif
	}
	if (optlen > 0) sharedMinimizeData->setOptimum(newopt);	
#ifndef NDEBUG
	DBGLOG(DBG, ss.str());
#endif
	minc->restoreOptimum();
	minc->integrateNext(*claspInstance.master());
	delete []newopt;
}

InterpretationPtr ClaspSolver::getNextModel(){

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
				nextModel = InterpretationPtr();
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

	return outputProjection(nextModel);
}

int ClaspSolver::getModelCount(){
	return modelCount;
}

std::string ClaspSolver::getStatistics(){
	std::stringstream ss;
	ss <<	"Guesses: " << claspInstance.master()->stats.choices << std::endl <<
		"Conflicts: " << claspInstance.master()->stats.conflicts << std::endl <<
		"Models: " << claspInstance.master()->stats.models;
	return ss.str();
}



// ============================== DisjunctiveClaspSolver ==============================

DisjunctiveClaspSolver::DisjunctiveClaspSolver(ProgramCtx& ctx, const AnnotatedGroundProgram& p, bool interleavedThreading) :
	ClaspSolver(ctx, p, interleavedThreading, ClaspSolver::ChoiceRules),
	program(p), ufscm(ctx, p, true){
}

DisjunctiveClaspSolver::~DisjunctiveClaspSolver(){
}

InterpretationPtr DisjunctiveClaspSolver::getNextModel(){

	InterpretationPtr model = ClaspSolver::getNextModel();

	bool ufsFound = true;
	while (model && ufsFound){
		ufsFound = false;

		std::vector<IDAddress> ufs = ufscm.getUnfoundedSet(model);

		if (ufs.size() > 0){
			Nogood ng = ufscm.getLastUFSNogood();
			addNogood(ng);
			ufsFound = true;
			model = ClaspSolver::getNextModel();
		}
	}
	return model;
}


DLVHEX_NAMESPACE_END

#endif
