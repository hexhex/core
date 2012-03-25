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

void ClaspSolver::ModelEnumerator::reportModel(const Clasp::Solver& s, const Clasp::Enumerator&){

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
//	if (cs.terminationRequest) throw std::runtime_error("ClaspThread was requested to terminate");
}

void ClaspSolver::ModelEnumerator::reportSolution(const Clasp::Solver& s, const Clasp::Enumerator&, bool complete){
}

bool ClaspSolver::ExternalPropagator::propagate(Clasp::Solver& s){

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

		DBGLOG(DBG, "Calling external learners with interpretation: " << *interpretation);
		bool learned = false;
		BOOST_FOREACH (LearningCallback* cb, cs.learner){
			// we are currently not able to check what changed inside clasp, so assume that all facts changed
			learned |= cb->learn(interpretation, factWasSet->getStorage(), factWasSet->getStorage());
		}

		// Now MainThread is allowed to access arbitrary code again, because we continue executing Clasp code,
		// which cannot interfere with dlvhex.
		if (!cs.strictSingleThreaded){
			DBGLOG(DBG, "ClaspThread: Leaving code which needs exclusive access to dlvhex data structures");
			cs.sem_dlvhexDataStructures.post();
		}
	}

	// add the produced nogoods to clasp
	bool inconsistent = false;
	{
	        boost::mutex::scoped_lock lock(cs.nogoodsMutex);

		DBGLOG(DBG, "External learners have produced " << (cs.nogoods.size() - cs.translatedNogoods) << " nogoods; transferring to clasp");
		for (int i = cs.translatedNogoods; i < cs.nogoods.size(); ++i){
			inconsistent |= cs.addNogoodToClasp(cs.nogoods[i]);
		}
//		inconsistent |= s.hasConflict();	// for some strange reason, this is necessary because there might be a conflict even if adding the nogood was successful
							// this is possibly due to conflicting derived nogoods!?
//if (inconsistent != s.hasConflict()) std::cout << "DIFFERENCE: inconsistent=" << inconsistent << ", s.hasConflict()=" << s.hasConflict() << std::endl;
		inconsistent = s.hasConflict();

		cs.translatedNogoods = cs.nogoods.size();
		DBGLOG(DBG, "Result: " << (inconsistent ? "" : "not ") << "inconsistent");
	}

	return !inconsistent;
}

bool ClaspSolver::addNogoodToClasp(Nogood& ng){

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
	clauseCreator->start(Clasp::Constraint_t::learnt_other);
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
#endif
	bool ret = false;
	if (clauseCreator->end().ok == false) ret = true;

	DBGLOG(DBG, "Adding nogood " << ng << " as clasp-clause " << ss.str());

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

void ClaspSolver::buildInitialSymbolTable(OrdinaryASPProgram& p, Clasp::ProgramBuilder& pb){

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

	Clasp::solve(claspInstance, params);
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

bool ClaspSolver::sendProgramToClasp(OrdinaryASPProgram& p){

	const int false_ = 1;	// 1 is our constant "false"

//	eqOptions.iters = 0;	// disable program optimization
	pb.startProgram(claspInstance, eqOptions);
	pb.setCompute(false_, false);

	buildInitialSymbolTable(p, pb);

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
		if (rule.head.size() > 1){
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

ClaspSolver::ClaspSolver(ProgramCtx& c, OrdinaryASPProgram& p) : ctx(c), program(p), sem_request(0), sem_answer(0), /*modelRequest(false),*/ terminationRequest(false), endOfModels(false), translatedNogoods(0), sem_dlvhexDataStructures(1), strictSingleThreaded(false)
//, NUM_PREPAREMODELS(5)
{
	DBGLOG(DBG, "Starting ClaspSolver in " << (strictSingleThreaded ? "single" : "multi") << "threaded mode");
	reg = ctx.registry();

	clauseCreator = new Clasp::ClauseCreator(claspInstance.master());
	bool initiallyInconsistent = sendProgramToClasp(p);
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

		DBGLOG(DBG, "Starting ClaspThread");

		claspThread = new boost::thread(boost::bind(&ClaspSolver::runClasp, this));
	}

	if (!strictSingleThreaded){
		// We now return to dlvhex code which is not in this class.
		// As we do not know what MainThread is doing there, ClaspThread must not access dlvhex data structures.
		DBGLOG(DBG, "MainThread: Entering code which needs exclusive access to dlvhex data structures");
		sem_dlvhexDataStructures.wait();
	}
}

ClaspSolver::~ClaspSolver(){

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
	// access learner vector
        boost::mutex::scoped_lock lock(learnerMutex);
	learner.insert(lb);
}

void ClaspSolver::removeExternalLearner(LearningCallback* lb){
	// access learner vector
        boost::mutex::scoped_lock lock(learnerMutex);
	learner.erase(lb);
}

int ClaspSolver::addNogood(Nogood ng){
	// access nogoods
        boost::mutex::scoped_lock lock(nogoodsMutex);
	nogoods.push_back(ng);
	return nogoods.size() - 1;
}

void ClaspSolver::removeNogood(int index){
	// access nogoods
        boost::mutex::scoped_lock lock(nogoodsMutex);
	throw std::runtime_error("ClaspSolver::removeNogood not implemented");
}

int ClaspSolver::getNogoodCount(){
	// access nogoods
        boost::mutex::scoped_lock lock(nogoodsMutex);
	return nogoods.size();
}

InterpretationConstPtr ClaspSolver::getNextModel(){

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

		if (program.mask != InterpretationConstPtr()){
			answer->getStorage() -= program.mask->getStorage();
		}

		DBGLOG(DBG, "Projected " << *intr << " to " << *answer);
		return answer;
	}
}

DLVHEX_NAMESPACE_END

#endif
