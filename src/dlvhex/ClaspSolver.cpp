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
 * @brief Interface to genuine clasp-based Solver.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_LIBCLASP

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

std::string ClaspSolver::idAddressToString(IDAddress adr){
	std::stringstream ss;
	ss << adr;
	return ss.str();
}

IDAddress ClaspSolver::stringToIDAddress(std::string str){
	return atoi(str.c_str());
}

void ClaspSolver::ModelEnumerator::reportModel(const Clasp::Solver& s, const Clasp::Enumerator&){

/*
	// block if buffer is full
	while (true){
		{
			boost::mutex::scoped_lock lock(cs.models_mutex);
			if (cs.bufferSize != 0){
				int undelivered = cs.models.size() - cs.nextModel;
				if (undelivered < cs.bufferSize){
					break;
				}
			}else{
				break;
			}
		}
		DBGLOG(DBG, "Blocking computation of further models: buffer is full");
		sleep(1);
	}
	DBGLOG(DBG, "Computing model number " << cs.models.size());
*/
	{
		boost::mutex::scoped_lock lock(cs.models_mutex);

		// compute number of free slots in the models buffer
		int undelivered = cs.models.size() - cs.nextModel;
		while (cs.bufferSize != 0 && undelivered >= cs.bufferSize){
			// buffer is full: wait for free slots
			DBGLOG(DBG, "Waiting for model slots to become available");
			cs.models_condition.wait(cs.models_mutex);
			undelivered = cs.models.size() - cs.nextModel;
		}

		// create a model
		InterpretationPtr model = InterpretationPtr(new Interpretation(cs.reg));

		// get the symbol table from the solver
		const Clasp::SymbolTable& symTab = s.sharedContext()->symTab();
		for (Clasp::SymbolTable::const_iterator it = symTab.begin(); it != symTab.end(); ++it) {
			// translate each named atom that is true w.r.t the current assignment into our dlvhex ID
			if (s.isTrue(it->second.lit) && !it->second.name.empty()) {
				IDAddress adr = ClaspSolver::stringToIDAddress(it->second.name.c_str());
	//			IDAddress adr = cs.claspToHex[it->second.lit];
				// set it in the model
				model->setFact(adr);
			}
		}

		// remember the model
		DBGLOG(DBG, "Model is: " << *model);
//	{
//		boost::mutex::scoped_lock lock(cs.models_mutex);
		cs.models.push_back(model);

		// at least one model has arrived: notify the consumer thread
		DBGLOG(DBG, "Notifying consumer about arrived model");
	}
	cs.models_condition.notify_all();

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
	const Clasp::SymbolTable& symTab = s.sharedContext()->symTab();
	for (Clasp::SymbolTable::const_iterator it = symTab.begin(); it != symTab.end(); ++it) {
		// bitset of all assigned values
		if (s.isTrue(it->second.lit) || s.isFalse(it->second.lit)) {
			IDAddress adr = ClaspSolver::stringToIDAddress(it->second.name.c_str());
//			IDAddress adr = cs.claspToHex[it->second.lit];
			factWasSet->setFact(adr);
		}
		// bitset of true values (partial interpretation)
		if (s.isTrue(it->second.lit)) {
			IDAddress adr = ClaspSolver::stringToIDAddress(it->second.name.c_str());
//			IDAddress adr = cs.claspToHex[it->second.lit];
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
	ss << "{ ";
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
	ss << " }";
	clauseCreator->end();
	DBGLOG(DBG, "Adding nogood " << ng << " as clasp-clause " << ss.str());
#endif
	return false;
}

void ClaspSolver::buildAtomIndex(OrdinaryASPProgram& p, Clasp::ProgramBuilder& pb){
//	pb.newAtom();

	DBGLOG(DBG, "Building atom index");
	// edb
	bm::bvector<>::enumerator en = p.edb->getStorage().first();
	bm::bvector<>::enumerator en_end = p.edb->getStorage().end();
	while (en < en_end){
		if (hexToClasp.find(*en) == hexToClasp.end()){
			uint32_t c = *en + 2; // pb.newAtom();
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
				uint32_t c = h.address + 2; // pb.newAtom();
				DBGLOG(DBG, "Clasp index of atom " << h.address << " is " << c);
				hexToClasp[h.address] = Clasp::Literal(c, true);

				std::string str = idAddressToString(h.address);
				claspInstance.symTab().addUnique(c, str.c_str());
			}
		}
		BOOST_FOREACH (ID b, rule.body){
			if (hexToClasp.find(b.address) == hexToClasp.end()){
				uint32_t c = b.address + 2; // pb.newAtom();
				DBGLOG(DBG, "Clasp index of atom " << b.address << " is " << c);
				hexToClasp[b.address] = Clasp::Literal(c, true);

				std::string str = idAddressToString(b.address);
				claspInstance.symTab().addUnique(c, str.c_str());
			}
		}
	}
}

void ClaspSolver::buildOptimizedAtomIndex(){

	hexToClasp.clear();

#ifndef NDEBUG
	std::stringstream ss;
#endif

	// go through symbol table
	const Clasp::SymbolTable& symTab = claspInstance.symTab();
	for (Clasp::SymbolTable::const_iterator it = symTab.begin(); it != symTab.end(); ++it) {
		IDAddress hexAdr = stringToIDAddress(it->second.name.c_str());
		hexToClasp[hexAdr] = it->second.lit;
//		claspToHex[it->second.lit] = hexAdr;
#ifndef NDEBUG
		ss << "Hex " << hexAdr << " <--> " << (it->second.lit.sign() ? "" : "!") << it->second.lit.var() << std::endl;
#endif
	}
	DBGLOG(DBG, "AtomIndex of optimized program: " << std::endl << ss.str());
}

void ClaspSolver::runClasp(){
	DBGLOG(DBG, "Running clasp");

	DBGLOG(DBG, "Integrity check of ClaspSolver: " << this);

	Clasp::solve(this->claspInstance, this->params);

	{
		boost::mutex::scoped_lock lock(this->models_mutex);
		this->claspFinished = true;
		this->models_condition.notify_all();
	}

	DBGLOG(DBG, "Clasp terminated -> Exit thread");
}

ClaspSolver::ClaspSolver(ProgramCtx& c, OrdinaryASPProgram& p) : ctx(c), program(p){

	const int false_ = 1;	// 1 is our constant "false"

	reg = ctx.registry();

	clauseCreator = new Clasp::ClauseCreator(claspInstance.master());
	
//	eqOptions.iters = 0;
	pb.startProgram(claspInstance, eqOptions);
	pb.setCompute(false_, false);
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
DBGLOG(DBG, "Fact " << hexToClasp[*en].var());
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
		if (rule.head.size() > 1){
			// shifting
			DBGLOG(DBG, "Shifting");
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
//std::cout << std::endl << std::endl;
//std::cout << "Program: " << programstring.str() << std::endl;
#endif
	bool ic = pb.endProgram();
	// rebuild the atom index as it might have changed due to optimization
	buildOptimizedAtomIndex();

	// Once all rules are defined, call endProgram() to load the (simplified)
	// program into the context object
//	if (pb.dependencyGraph() && pb.dependencyGraph()->nodes() > 0) {
//		DBGLOG(DBG, "Program is not tight, adding unfounded set checker");
		// program is non tight - add unfounded set checker
		DBGLOG(DBG, "Adding unfounded set checker");
		Clasp::DefaultUnfoundedCheck* ufs = new Clasp::DefaultUnfoundedCheck();
		ufs->attachTo(*claspInstance.master(), pb.dependencyGraph()); // register with solver and graph & transfer ownership
//	}

	DBGLOG(DBG, "Initially inconsistent: " << !ic);

	std::stringstream prog;
	pb.writeProgram(prog);
	DBGLOG(DBG, "Program in LParse format: " << prog.str());
//	std::cout << prog.str() << std::endl;

	// add enumerator
	claspInstance.addEnumerator(new Clasp::BacktrackEnumerator(0, new ModelEnumerator(*this)));
	claspInstance.enumerator()->enumerate(0);

	// add propagator
	ExternalPropagator* ep = new ExternalPropagator(*this);
	claspInstance.addPost(ep);

	// endInit() must be called once before the search starts
	claspInstance.endInit();

	DBGLOG(DBG, "Starting clasp thread, this-pointer is " << this);
	claspFinished = false;
	bufferSize = 1;
	claspThread = new boost::thread(boost::bind(&ClaspSolver::runClasp, this));

//	DBGLOG(DBG, "Waiting for clasp to terminate");
//	claspThread->join();
//	DBGLOG(DBG, "Clasp terminated");
/*
	// start clasp in a new thread
	if (false){	// threading or not
		bufferSize = 5;
		DBGLOG(DBG, "Starting clasp thread");
		int rc = pthread_create(&claspThread, NULL, runClasp, this);
		DBGLOG(DBG, "Continue in main thread (clasp thread has identifier " << claspThread << ")");
		assert(0 == rc);
	}else{
		bufferSize = 0;
		runClasp(this);
	}
*/
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
/*	while (true){
		{
			boost::mutex::scoped_lock lock(models_mutex);
			if (claspFinished || nextModel < models.size()){
				break;
			}
		}

		DBGLOG(DBG, "Waiting for new models from clasp");
		sleep(1);
	}
*/
	InterpretationConstPtr ip;
	{
		boost::mutex::scoped_lock lock(models_mutex);

		// wait for arrived models (if necessary)
		while (!claspFinished && nextModel >= models.size()){
			DBGLOG(DBG, "Waiting for models to arrive");
			models_condition.wait(models_mutex);
		}

//		DBGLOG(DBG, "Clasp finished: " << claspFinished << "; number of available models : " << models.size() << "; next model index: " << nextModel);
		if (claspFinished && nextModel >= models.size()){
			DBGLOG(DBG, "No more models");
	/*
	std::cout << "EXAMPLE 1" << std::endl;
	example1();
	std::cout << "END EXAMPLE 1" << std::endl;
	*/
			ip = InterpretationConstPtr();
		}else{
			DBGLOG(DBG, "Retrieve model");
			ip = models[nextModel++];
		}
	}
	// at least one slot is free: notify the clasp thread
	DBGLOG(DBG, "Notifying clasp about free slot");
	models_condition.notify_all();
	return ip;
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

		DBGLOG(DBG, "Projected " << *intr << " to " << *answer);
		return answer;
	}
}

DLVHEX_NAMESPACE_END

#endif
