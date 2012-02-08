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
 * @file InternalGroundASPSolver.cpp
 * @author Christoph Redl
 *
 * @brief Interface to genuine clasp-based Solver.
 */

#include "dlvhex/ClaspSolver.hpp"

#include <iostream>
#include <sstream>
#include "dlvhex/Logger.hpp"
#include "dlvhex/GenuineSolver.hpp"
#include "dlvhex/Printer.hpp"
#include <boost/foreach.hpp>
#include <boost/graph/strong_components.hpp>

#include "clasp/program_rule.h"


DLVHEX_NAMESPACE_BEGIN

std::string ClaspSolver::idAddressToString(IDAddress adr){
	std::stringstream ss;
	ss << adr;
	return ss.str();
}

IDAddress ClaspSolver::stringToIDAddress(std::string str){
	return atoi(str.c_str());
}

void ClaspSolver::ModelEnumerator::reportModel(const Clasp::Solver& s, const Clasp::Enumerator&){

	// block if buffer is full
	if (cs.bufferSize != 0){
		int undelivered = cs.models.size() - cs.nextModel;
		if (undelivered >= cs.bufferSize){
			DBGLOG(DBG, "Blocking computation of further models: buffer is full");
			sleep(10);
		}
	}
	DBGLOG(DBG, "Computing model number " << cs.models.size());

	// create a model
	InterpretationPtr model = InterpretationPtr(new Interpretation(cs.reg));

	// get the symbol table from the solver
	const Clasp::AtomIndex& symTab = *s.strategies().symTab;
	for (Clasp::AtomIndex::const_iterator it = symTab.begin(); it != symTab.end(); ++it) {
		// translate each named atom that is true w.r.t the current assignment into our dlvhex ID
		if (s.isTrue(it->second.lit) && !it->second.name.empty()) {
			IDAddress adr = cs.claspToHex[it->second.lit]; // ClaspSolver::stringToIDAddress(it->second.name);
			// set it in the model
			model->setFact(adr);
		}
	}

	// remember the model
	cs.models.push_back(model);

//std::cout << "Model: " << *model << std::endl;
}

void ClaspSolver::ModelEnumerator::reportSolution(const Clasp::Solver& s, const Clasp::Enumerator&, bool complete){
}

bool ClaspSolver::ExternalPropagator::propagate(Clasp::Solver& s){

	if (cs.learner.size() == 0) return true;

	DBGLOG(DBG, "Translating clasp assignment to HEX-interpretation");
	// create an interpretation and a bitset of assigned values
	InterpretationPtr interpretation = InterpretationPtr(new Interpretation(cs.reg));
	InterpretationPtr factWasSet = InterpretationPtr(new Interpretation(cs.reg));

	// translate clasp assignment to hex assignment
	// get the symbol table from the solver
	const Clasp::AtomIndex& symTab = *s.strategies().symTab;
	for (Clasp::AtomIndex::const_iterator it = symTab.begin(); it != symTab.end(); ++it) {
		// bitset of all assigned values
		if (s.isTrue(it->second.lit) || s.isFalse(it->second.lit)) {
			IDAddress adr = cs.claspToHex[it->second.lit];
			factWasSet->setFact(adr);
		}
		// bitset of true values (partial interpretation)
		if (s.isTrue(it->second.lit)) {
			IDAddress adr = cs.claspToHex[it->second.lit];
			interpretation->setFact(adr);
		}
	}

	DBGLOG(DBG, "Calling external learners with interpretation: " << *interpretation);
	bool learned = false;
	BOOST_FOREACH (LearningCallback* cb, cs.learner){
		// we are currently not able to check what changed inside clasp, so assume that all facts changed
		learned |= cb->learn(interpretation, factWasSet->getStorage(), factWasSet->getStorage());
	}

	// add the produced nogoods to clasp
	bool inconsistent = cs.recentlyBecameInconsistentByAddingNogoods;
	cs.recentlyBecameInconsistentByAddingNogoods = false;
//	DBGLOG(DBG, "External learners have produced " << cs.nogoods.size() << " nogoods; transferring to clasp");
//	BOOST_FOREACH (Nogood ng, cs.nogoods){
//		inconsistent |= cs.addNogoodToClasp(ng);
//	}
//	cs.nogoods.clear();
	DBGLOG(DBG, "Result: " << (inconsistent ? "" : "not ") << "inconsistent");
	return !inconsistent;

/*
	Nogood ng;
	ng.insert(ID(ID::MAINKIND_LITERAL | ID::SUBKIND_ATOM_ORDINARYG, 0));
	ng.insert(ID(ID::MAINKIND_LITERAL | ID::SUBKIND_ATOM_ORDINARYG, 2));
	return addNogoodToSolver(cg, ng);
*/
}

bool ClaspSolver::addNogoodToClasp(Nogood& ng){

#ifndef NDEBUG
	std::stringstream ss;
	ss << "{";
	bool first = true;
#endif

	clauseCreator->start();
	BOOST_FOREACH (ID lit, ng){
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
	ss << "}";
	DBGLOG(DBG, "Adding nogood " << ng << " as clasp-clause " << ss.str());
#endif
	return !clauseCreator->end();
}

void ClaspSolver::buildAtomIndex(OrdinaryASPProgram& p, Clasp::ProgramBuilder& pb){
	pb.newAtom();

	DBGLOG(DBG, "Building atom index");
	// edb
	bm::bvector<>::enumerator en = p.edb->getStorage().first();
	bm::bvector<>::enumerator en_end = p.edb->getStorage().end();
	while (en < en_end){
		if (hexToClasp.find(*en) == hexToClasp.end()){
			uint32_t c = pb.newAtom();
			DBGLOG(DBG, "Clasp index of atom " << *en << " is " << c);
			hexToClasp[*en] = Clasp::Literal(c, true);

			std::string str = idAddressToString(*en);
			claspInstance.strategies().symTab->addUnique(c, Clasp::Atom(str.c_str()));
		}
		en++;
	}

	// idb
	BOOST_FOREACH (ID ruleId, p.idb){
		const Rule& rule = reg->rules.getByID(ruleId);
		BOOST_FOREACH (ID h, rule.head){
			if (hexToClasp.find(h.address) == hexToClasp.end()){
				uint32_t c = pb.newAtom();
				DBGLOG(DBG, "Clasp index of atom " << h.address << " is " << c);
				hexToClasp[h.address] = Clasp::Literal(c, true);

				std::string str = idAddressToString(h.address);
				claspInstance.strategies().symTab->addUnique(c, Clasp::Atom(str.c_str()));
			}
		}
		BOOST_FOREACH (ID b, rule.body){
			if (hexToClasp.find(b.address) == hexToClasp.end()){
				uint32_t c = pb.newAtom();
				DBGLOG(DBG, "Clasp index of atom " << b.address << " is " << c);
				hexToClasp[b.address] = Clasp::Literal(c, true);

				std::string str = idAddressToString(b.address);
				claspInstance.strategies().symTab->addUnique(c, Clasp::Atom(str.c_str()));
			}
		}
	}
}

void ClaspSolver::buildOptimizedAtomIndex(Clasp::Solver& solver){

	hexToClasp.clear();
	claspToHex.clear();

#ifndef NDEBUG
	std::stringstream ss;
#endif

	// go through symbol table
	const Clasp::AtomIndex& symTab = *solver.strategies().symTab;
	for (Clasp::AtomIndex::const_iterator it = symTab.begin(); it != symTab.end(); ++it) {
		IDAddress hexAdr = stringToIDAddress(it->second.name);
		hexToClasp[hexAdr] = it->second.lit;
		claspToHex[it->second.lit] = hexAdr;
#ifndef NDEBUG
		ss << "Hex " << hexAdr << " <--> " << (it->second.lit.sign() ? "" : "!") << it->second.lit.var() << std::endl;
#endif
	}
	DBGLOG(DBG, "AtomIndex of optimized program: " << std::endl << ss.str());
}

void* ClaspSolver::runClasp(void *cs){
	DBGLOG(DBG, "Running clasp");
	ClaspSolver* claspSolver = (ClaspSolver*)cs;

	DBGLOG(DBG, "Integrity check of ClaspSolver: " << claspSolver->claspThread);

	Clasp::solve(claspSolver->claspInstance, claspSolver->params);
	claspSolver->claspFinished = true;
	DBGLOG(DBG, "Clasp terminated -> Exit thread");
	return 0;
}

ClaspSolver::ClaspSolver(ProgramCtx& c, OrdinaryASPProgram& p) : ctx(c), program(p){

	reg = ctx.registry();

	clauseCreator = new Clasp::ClauseCreator(&claspInstance);
	claspInstance.strategies().symTab.reset(new Clasp::AtomIndex());
	
	Clasp::ProgramBuilder pb;
//	pb.setEqOptions(0);
	pb.startProgram(*claspInstance.strategies().symTab, new Clasp::DefaultUnfoundedCheck());
	pb.setCompute(1, false);	// 1 is our constant "false"
	recentlyBecameInconsistentByAddingNogoods = false;

	buildAtomIndex(p, pb);

#ifndef NDEBUG
	std::stringstream programstring;
	RawPrinter printer(programstring, reg);
#endif

	// transfer edb
	DBGLOG(DBG, "Sending EDB to clasp");
	bm::bvector<>::enumerator en = p.edb->getStorage().first();
	bm::bvector<>::enumerator en_end = p.edb->getStorage().end();
	while (en < en_end){
		// add fact
		pb.startRule();
		pb.addHead(hexToClasp[*en].var());
		pb.endRule();
		en++;
	}
#ifndef NDEBUG
	programstring << *p.edb << std::endl;
#endif

	// transfer idb
	DBGLOG(DBG, "Sending IDB to clasp");
	BOOST_FOREACH (ID ruleId, p.idb){
		const Rule& rule = reg->rules.getByID(ruleId);
#ifndef NDEBUG
		programstring << (rule.head.size() == 0 ? "(constraint)" : "(rule)") << " ";
		printer.print(ruleId);
		programstring << std::endl;
#endif
		if (rule.head.size() == 0){
			pb.startRule(Clasp::BASICRULE);
			pb.addHead(1);	// constant "false"
		}else{
			pb.startRule(Clasp::BASICRULE);
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

#ifndef NDEBUG
	DBGLOG(DBG, "Program is: " << std::endl << programstring.str());
//std::cout << std::endl << std::endl;
//std::cout << "Program: " << programstring.str() << std::endl;
#endif

	bool ic = pb.endProgram(claspInstance, true);
	DBGLOG(DBG, "Initially inconsistent: " << ic);
	buildOptimizedAtomIndex(claspInstance);

	std::stringstream prog;
	pb.writeProgram(prog);
//	std::cout << prog.str() << std::endl;

	// add enumerator
	ModelEnumerator enumerator(*this);
	params.setEnumerator(new Clasp::BacktrackEnumerator(0, &enumerator));

	// add propagator
	ExternalPropagator* ep = new ExternalPropagator(*this);
	claspInstance.addPost(ep);

	// solve
	params.enumerator()->init(claspInstance, 0);

	// start clasp in a new thread
	claspFinished = false;
	if (false){	// threading or not
		bufferSize = 5;
		DBGLOG(DBG, "Starting clasp thread");
		int rc = pthread_create(&claspThread, NULL, runClasp, this);
		DBGLOG(DBG, "Continuing in main thread (clasp thread has identifier " << claspThread << ")");
		assert(0 == rc);
	}else{
		bufferSize = 0;
		runClasp(this);
	}
	nextModel = 0;
}

ClaspSolver::~ClaspSolver(){
	delete clauseCreator;
}

std::string ClaspSolver::getStatistics(){
	return "";
}

void ClaspSolver::addExternalLearner(LearningCallback* lb){
	learner.insert(lb);
}

void ClaspSolver::removeExternalLearner(LearningCallback* lb){
	learner.erase(lb);
}

int ClaspSolver::addNogood(Nogood ng){
	recentlyBecameInconsistentByAddingNogoods |= addNogoodToClasp(ng);
	nogoods.push_back(ng);
	return nogoods.size() - 1;
}

void ClaspSolver::removeNogood(int index){
	nogoods.erase(nogoods.begin() + index);
}

int ClaspSolver::getNogoodCount(){
	return nogoods.size();
}

InterpretationConstPtr ClaspSolver::getNextModel(){
	// if clasp has not finished yet, wait until new models arrive or clasp terminates
	while (!claspFinished && nextModel >= models.size()){
		DBGLOG(DBG, "Waiting for new models from clasp");
		sleep(10);
	}

	if (nextModel >= models.size()){
		DBGLOG(DBG, "No more models");
		return InterpretationConstPtr();
	}else{
		DBGLOG(DBG, "Retrieve model");
		return models[nextModel++];
	}
}

InterpretationPtr ClaspSolver::projectToOrdinaryAtoms(InterpretationConstPtr intr){
	if (intr == InterpretationConstPtr()){
		return InterpretationPtr();
	}else{
		InterpretationPtr answer = InterpretationPtr(new Interpretation(reg));
		answer->add(*intr);

		if (program.mask != InterpretationConstPtr()){
			answer->getStorage() -= program.mask->getStorage();
		}

		return answer;
	}
}

DLVHEX_NAMESPACE_END
