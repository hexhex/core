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
 * @file CDNLSolver.cpp
 * @author Christoph Redl
 *
 * @brief SAT solver based on conflict-driven nogood learning.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dlvhex2/CDNLSolver.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/GenuineSolver.h"
#include "dlvhex2/Benchmarking.h"

#include <iostream>
#include <sstream>
#include "dlvhex2/Logger.h"
#include <boost/functional/hash.hpp>

DLVHEX_NAMESPACE_BEGIN

#define DBGLOGD(X,Y) DBGLOG(X,Y)
//#define DBGLOGD(X,Y) do{}while(false);

// ---------- Class CDNLSolver ----------

bool CDNLSolver::unitPropagation(Nogood& violatedNogood){

	DBGLOG(DBG, "Unit propagation starts");
	int nogoodNr;
	while (unitNogoods.size() > 0){
		nogoodNr = *(unitNogoods.begin());
		const Nogood& nextUnitNogood = nogoodset.getNogood(nogoodNr);
		unitNogoods.erase(unitNogoods.begin());

		// find propagation DL
		int propDL = 0;
		BOOST_FOREACH (ID lit, nextUnitNogood){
			if (assigned(lit.address) && decisionlevel[lit.address] > propDL){
				propDL = decisionlevel[lit.address];
			}
		}

		// as the nogood is unit, it has a single watched literal
		// its negation is the propagated one
		ID propagatedLit = negation(*(watchedLiteralsOfNogood[nogoodNr].begin()));
		setFact(propagatedLit, propDL, nogoodNr);
	}

	if (contradictoryNogoods.size() > 0){
		violatedNogood = nogoodset.getNogood(*(contradictoryNogoods.begin()));
		DBGLOG(DBG, "Unit propagation finished with detected contradiction " << violatedNogood);
		return false;
	}

	DBGLOG(DBG, "Unit propagation finished successfully");
	return true;
}

void CDNLSolver::loadAddedNogoods(){
	for (int i = 0; i < nogoodsToAdd.getNogoodCount(); ++i){
		addNogoodAndUpdateWatchingStructures(nogoodsToAdd.getNogood(i));

	}
	nogoodsToAdd.clear();
}

void CDNLSolver::analysis(Nogood& violatedNogood, Nogood& learnedNogood, int& backtrackDL){

	DBGLOG(DBG,"Conflict detected, violated nogood: " << violatedNogood);

#ifndef NDEBUG
	++cntDetectedConflicts;
#endif

	// decision heuristic metric update
	touchVarsInNogood(violatedNogood);

	// check how many literals were assigned at top decision level
	// if there is more than one, resolve the nogood with the cause of one of the implied literals
	learnedNogood = violatedNogood;
	int count;
	int resSteps = 0;
	int latestDL;
	IDAddress impliedLit;
	long litAssignmentOrderIndex;
	ID latestLit;
	long latestLitAssignmentOrderIndex;
	int bt = 0;
	do{
		bool foundImpliedLit = false;
		count = 0;
		impliedLit = ID_FAIL;
		latestLit = ID_FAIL;
		latestLitAssignmentOrderIndex = -1;
		BOOST_FOREACH (ID lit, learnedNogood){
			litAssignmentOrderIndex = getAssignmentOrderIndex(lit.address);
			if (litAssignmentOrderIndex > latestLitAssignmentOrderIndex){
				latestLit = lit;
				latestLitAssignmentOrderIndex = getAssignmentOrderIndex(latestLit.address);
			}
		}
		latestDL = decisionlevel[latestLit.address];

		BOOST_FOREACH (ID lit, learnedNogood){
			// compute number of literals on latest dl
			if (decisionlevel[lit.address] == latestDL){
				count++;
				if (!isDecisionLiteral(lit.address)){
					impliedLit = lit.address;
					foundImpliedLit = true;
				}
			}

			// backtrack to the second-highest decision level
			if (decisionlevel[lit.address] > bt && lit.address != latestLit.address && decisionlevel[lit.address] < latestDL){
				bt = decisionlevel[lit.address];
			}
		}

		if (count > 1){
			// resolve the clause with multiple literals on top level
			// with the cause of one of the implied literals

			// at DL=0 we might have multiple literals without a cause (they are only spurious decision literals, actually they are facts)
			if (!foundImpliedLit && latestDL == 0){
				break;
			}else{
				assert(foundImpliedLit);
				Nogood& c = nogoodset.getNogood(cause[impliedLit]);
				touchVarsInNogood(c);
				learnedNogood = resolve(learnedNogood, c, impliedLit);
			}
#ifndef NDEBUG
	++cntResSteps;
#endif
			++resSteps;
		}
	}while(count > 1);

	if (resSteps > 0){
		// if resSteps == 0, then learnedNogood == violatedNogood, which was already touched
		touchVarsInNogood(learnedNogood);
	}

	DBGLOG(DBG, "Learned conflict nogood: " << learnedNogood << " (after " << resSteps << " resolution steps)");
	DBGLOG(DBG, "Backtrack-DL: " << bt);
	backtrackDL = bt;

	// decision heuristic metric update
	++conflicts;
	if (conflicts >= 255){
		DBGLOG(DBG, "Maximum conflicts count: dividing all counters by 2");
		BOOST_FOREACH (IDAddress litadr, allFacts){
			varCounterPos[litadr] /= 2;
			varCounterNeg[litadr] /= 2;
		}
		conflicts = 0;
	}
}

Nogood CDNLSolver::resolve(Nogood& ng1, Nogood& ng2, IDAddress litadr){
	// resolvent = union of ng1 and ng2 minus both polarities of the resolved literal
	Nogood resolvent = ng1;
	resolvent.insert(ng2.begin(), ng2.end());
	resolvent.erase(createLiteral(litadr));
	resolvent.erase(negation(createLiteral(litadr)));
	DBGLOG(DBG, "Resolution " << ng1 << " with " << ng2 << ": " << resolvent);

#ifndef NDEBUG
	++cntResSteps;
#endif
	return resolvent;
}


void CDNLSolver::setFact(ID fact, int dl, int c = -1){
	if (c > -1){
		DBGLOG(DBG, "Assigning " << litToString(fact) << "@" << dl << " with cause " << nogoodset.getNogood(c));
	}else{
		DBGLOG(DBG, "Assigning " << litToString(fact) << "@" << dl);
	}
	factWasSet->setFact(fact.address);			// fact was set
	changed->setFact(fact.address);
	decisionlevel[fact.address] = dl;			// store decision level
	//if (c > -1)
	cause[fact.address] = c;				// store cause
	if (fact.isNaf()){					// store truth value
		interpretation->clearFact(fact.address);
	}else{
		interpretation->setFact(fact.address);
	}
	assignmentOrder.insert(fact.address);
	factsOnDecisionLevel[dl].push_back(fact.address);

	updateWatchingStructuresAfterSetFact(fact);

#ifndef NDEBUG
	++cntAssignments;
#endif
}

void CDNLSolver::clearFact(IDAddress litadr){
	DBGLOG(DBG, "Unassigning " << litadr << "@" << decisionlevel[litadr]);
	factWasSet->clearFact(litadr);
	changed->setFact(litadr);
	cause[litadr] = -1;
	assignmentOrder.erase(litadr);

	// getFact will return the truth value which was just cleared
	// (truth value remains until it is overridden by a new one)
	updateWatchingStructuresAfterClearFact(createLiteral(litadr, interpretation->getFact(litadr)));
}

void CDNLSolver::backtrack(int dl){

	for (int i = dl + 1; i < factsOnDecisionLevel.size(); ++i){
		BOOST_FOREACH (IDAddress f, factsOnDecisionLevel[i]){
			clearFact(f);
		}
		factsOnDecisionLevel[i].clear();
	}

#ifndef NDEBUG
	++cntBacktracks;
#endif
}

ID CDNLSolver::getGuess(){

#ifndef NDEBUG
	++cntGuesses;
#endif

	// simple heuristic: guess the next unassigned literal
	/*
	for (std::set<IDAddress>::reverse_iterator rit = allFacts.rbegin(); rit != allFacts.rend(); ++rit){
		if (!assigned(*rit)){
			return createLiteral(*rit);
		}
	}
	return ID_FAIL;
	*/

	/*
	BOOST_FOREACH (IDAddress litadr, allFacts){
		if (!assigned(litadr)){
			return createLiteral(litadr);
		}
	}
	return ID_FAIL;
	*/


	// iterate over recent conflicts, beginning at the most recent conflict
	for (std::vector<int>::reverse_iterator rit = recentConflicts.rbegin(); rit != recentConflicts.rend(); ++rit){
		Nogood& ng = nogoodset.getNogood(*rit);

		// skip satisfied and contraditory nogoods
		if (watchedLiteralsOfNogood[*rit].size() == 0){
			continue;
		}

		// find most active unassigned variable in this nogood
		ID mostActive = ID_FAIL;
		BOOST_FOREACH (ID lit, ng){
			if (!assigned(lit.address)){
				if (mostActive == ID_FAIL ||
				   (varCounterPos[lit.address] + varCounterNeg[lit.address]) > (varCounterPos[mostActive.address] + varCounterNeg[mostActive.address])){
					mostActive = varCounterPos[lit.address] > varCounterNeg[lit.address] ? negation(createLiteral(lit.address)) : createLiteral(lit.address);
				}
			}
		}

		// if the nogood has no unassigned variable, it must be either satisfied or contradictory and the if above applies
		assert(mostActive != ID_FAIL);

		DBGLOG(DBG, "Guessing " << litToString(mostActive) << " because it occurs in recent conflicts");
		return mostActive;
	}

	// no recent conflicts;
	// use alternative heuristic: choose globally most active literal
	ID mostActive = ID_FAIL;
	BOOST_FOREACH (IDAddress litadr, allFacts){
		if (!assigned(litadr)){
			if (mostActive == ID_FAIL ||
			   (varCounterPos[litadr] + varCounterNeg[litadr]) > (varCounterPos[mostActive.address] + varCounterNeg[mostActive.address])){
				mostActive = varCounterPos[litadr] > varCounterNeg[litadr] ? negation(createLiteral(litadr)) : createLiteral(litadr);
			}
		}
	}

	DBGLOG(DBG, "Guessing " << litToString(mostActive) << " because it is globally active");
	return mostActive;
}

void CDNLSolver::initWatchingStructures(){

	// reset lazy data structures
	watchedLiteralsOfNogood = std::vector<Set<ID> >(nogoodset.getNogoodCount());
	watchingNogoodsOfPosLiteral.clear();
	watchingNogoodsOfNegLiteral.clear();
	nogoodsOfPosLiteral.clear();
	nogoodsOfNegLiteral.clear();

	// reset unit and contradictory nogoods
	unitNogoods.clear();
	contradictoryNogoods.clear();

	// each nogood watches (at most) two of its literals
	for (unsigned int nogoodNr = 0; nogoodNr < nogoodset.getNogoodCount(); ++nogoodNr){
		updateWatchingStructuresAfterAddNogood(nogoodNr);
	}
}

void CDNLSolver::updateWatchingStructuresAfterAddNogood(int index){

	DBGLOGD(DBG, "updateWatchingStructuresAfterAddNogood after adding nogood " << index);
	const Nogood& ng = nogoodset.getNogood(index);

	// remember for all literals in the nogood that they are contained in this nogood
	BOOST_FOREACH (ID lit, ng){
		if (!lit.isNaf()){
			nogoodsOfPosLiteral[lit.address].insert(index);
		}else{
			nogoodsOfNegLiteral[lit.address].insert(index);
		}
	}

	// search for up to two unassigned literals to watch
	bool inactive = false;
	tmpWatched.clear();
	BOOST_FOREACH (ID lit, ng){
		if (!assigned(lit.address) && tmpWatched.size() < 2){
			tmpWatched.insert(lit);
		}else if(falsified(lit)){
			inactive = true;
		}
	}

	// remember watches
	if (!inactive){
		BOOST_FOREACH (ID lit, tmpWatched){
			startWatching(index, lit);
		}
	}

	if (inactive){
		DBGLOGD(DBG, "Nogood " << index << " is inactive");
	}else if (tmpWatched.size() == 1){
		DBGLOGD(DBG, "Nogood " << index << " is unit");
		unitNogoods.insert(index);
	}else if (tmpWatched.size() == 0){
		DBGLOGD(DBG, "Nogood " << index << " is contradictory");
		contradictoryNogoods.insert(index);
	}
}

void CDNLSolver::updateWatchingStructuresAfterRemoveNogood(int index){
	const Nogood& ng = nogoodset.getNogood(index);

	// remove the nogood from all literal lists
	BOOST_FOREACH (ID lit, ng){
		nogoodsOfPosLiteral[lit.address].erase(index);
		nogoodsOfNegLiteral[lit.address].erase(index);
	}

	// remove all watched literals
	Set<ID>& watched = watchedLiteralsOfNogood[index];
	BOOST_FOREACH (ID lit, watched){
		stopWatching(index, lit);
	}
}

void CDNLSolver::updateWatchingStructuresAfterSetFact(ID lit){

	DBGLOGD(DBG, "updateWatchingStructuresAfterSetFact after " << litToString(lit) << " was set");
	bool changed;

	// go through all nogoods which watch this literal negatively and inactivate them
	if ((lit.isNaf() && watchingNogoodsOfPosLiteral.find(lit.address) != watchingNogoodsOfPosLiteral.end()) ||
	    (!lit.isNaf() && watchingNogoodsOfNegLiteral.find(lit.address) != watchingNogoodsOfNegLiteral.end())){
		do{
			changed = false;

			BOOST_FOREACH (int nogoodNr, lit.isNaf() ? watchingNogoodsOfPosLiteral[lit.address] : watchingNogoodsOfNegLiteral[lit.address]){
				inactivateNogood(nogoodNr);
				changed = true;
				break;
			}
		}while(changed);
	}

	// go through all nogoods which watch this literal positively and find a new watched literal
	if (watchingNogoodsOfPosLiteral.find(lit.address) != watchingNogoodsOfPosLiteral.end()){
		do{
			changed = false;

			BOOST_FOREACH (int nogoodNr, lit.isNaf() ? watchingNogoodsOfNegLiteral[lit.address] : watchingNogoodsOfPosLiteral[lit.address]){
				const Nogood& ng = nogoodset.getNogood(nogoodNr);

				// stop watching lit
				stopWatching(nogoodNr, lit);

				// search for a new literal which is
				// 1. not assigned yet
				// 2. currently not watched
				bool inactive = false;
				BOOST_FOREACH (ID nglit, ng){
					if ((watchedLiteralsOfNogood[nogoodNr].size() < 2) && !assigned(nglit.address) && (watchedLiteralsOfNogood[nogoodNr].count(nglit) == 0)){
						// watch it
						startWatching(nogoodNr, nglit);
					}else if (falsified(nglit)){
						DBGLOGD(DBG, "Nogood " << nogoodNr << " is now inactive");
						inactivateNogood(nogoodNr);
						inactive = true;
						break;
					}
				}
				if (!inactive){
					// nogood might have become unit or contradictory
					if (watchedLiteralsOfNogood[nogoodNr].size() == 1){
						DBGLOGD(DBG, "Nogood " << nogoodNr << " is now unit");
						unitNogoods.insert(nogoodNr);
					}else if (watchedLiteralsOfNogood[nogoodNr].size() == 0){
						DBGLOGD(DBG, "Nogood " << nogoodNr << " is now contradictory");
						contradictoryNogoods.insert(nogoodNr);
						unitNogoods.erase(nogoodNr);
					}
				}

				changed = true;
				break;
			}
		}while(changed);
	}
}

void CDNLSolver::updateWatchingStructuresAfterClearFact(ID literal){

	// nogoods which watch the literal negatively were inactive before
	// 	they can now either (i) still be inactive, or (ii) have at least one unassigned literal

	// nogoods which watch the literal positively were either active or inconsistent before
	//	if they were active, they can now (i) have more watched literals (if they had only one before); or
	//					 (ii) have as many watched literals as before (if they had already two)
	//	if they were inconsistent before, they have at least one watched literal

	DBGLOGD(DBG, "updateWatchingStructuresAfterClearFact after " << litToString(literal) << " was cleared");

	// go through all nogoods which contain this literal either positively or negatively
	for (int positiveAndNegativeLiteral = 1; positiveAndNegativeLiteral <= 2; ++positiveAndNegativeLiteral){

		if ((positiveAndNegativeLiteral == 1 && nogoodsOfPosLiteral.find(literal.address) != nogoodsOfPosLiteral.end()) ||
		    (positiveAndNegativeLiteral == 2 && nogoodsOfNegLiteral.find(literal.address) != nogoodsOfNegLiteral.end())){
			BOOST_FOREACH (int nogoodNr, positiveAndNegativeLiteral == 1 ? nogoodsOfPosLiteral[literal.address] : nogoodsOfNegLiteral[literal.address]){
				DBGLOG(DBG, "Updating nogood " << nogoodNr);

				const Nogood& ng = nogoodset.getNogood(nogoodNr);

				bool stillInactive = false;

				// check the number of currently watched literals
				int watchedNum = watchedLiteralsOfNogood[nogoodNr].size();
				switch(watchedNum){
					case 0:		// nogood was inactive or contradictory before
						// nogood can:
						// 1. still be inactive
						// 2. have one unassigned literal
						// 3. have multiple unassigned literals
						// it cannot be contraditory anymore because at least one literal is unassigned!
						tmpWatched.clear();
						BOOST_FOREACH (ID lit, ng){
							if (falsified(lit)){
								stillInactive = true;
								break;
							}
							// collect up to 2 watched literals
							if (!assigned(lit.address) && tmpWatched.size() < 2){
								tmpWatched.insert(lit);
							}
						}
						if (!stillInactive){
							DBGLOG(DBG, "Nogood " << nogoodNr << " is reactivated");
							BOOST_FOREACH (ID lit, tmpWatched){
								startWatching(nogoodNr, lit);
							}

							if (tmpWatched.size() == 1){
								DBGLOGD(DBG, "Nogood " << nogoodNr << " becomes unit");
								unitNogoods.insert(nogoodNr);
							}

							// nogood is (for sure) not contradictory anymore
							contradictoryNogoods.erase(nogoodNr);
						}
						break;
					case 1:		// nogood was unit before
						// watch litadr
						startWatching(nogoodNr, literal);

						// nogood is not unit anymore
						DBGLOGD(DBG, "Nogood " << nogoodNr << " is not unit anymore");
						unitNogoods.erase(nogoodNr);
						break;
					default:	// nogood has more than one watched literal
						// number of unassigned literals has possibly even increased
						// nothing to do
						break;
				}
			}
		}
	}
}

void CDNLSolver::inactivateNogood(int nogoodNr){
	DBGLOGD(DBG, "Nogood " << nogoodNr << " gets inactive");

	BOOST_FOREACH (ID lit, watchedLiteralsOfNogood[nogoodNr]){
		watchingNogoodsOfPosLiteral[lit.address].erase(nogoodNr);
		watchingNogoodsOfNegLiteral[lit.address].erase(nogoodNr);
	}
	watchedLiteralsOfNogood[nogoodNr].clear();

	unitNogoods.erase(nogoodNr);
	contradictoryNogoods.erase(nogoodNr);
}

void CDNLSolver::stopWatching(int nogoodNr, ID lit){
	DBGLOGD(DBG, "Nogood " << nogoodNr << " stops watching " << litToString(lit));
	if (!lit.isNaf()){
		watchingNogoodsOfPosLiteral[lit.address].erase(nogoodNr);
	}else{
		watchingNogoodsOfNegLiteral[lit.address].erase(nogoodNr);
	}
	watchedLiteralsOfNogood[nogoodNr].erase(lit);
}

void CDNLSolver::startWatching(int nogoodNr, ID lit){
	DBGLOGD(DBG, "Nogood " << nogoodNr << " starts watching " << litToString(lit));
	watchedLiteralsOfNogood[nogoodNr].insert(lit);
	if (!lit.isNaf()){
		watchingNogoodsOfPosLiteral[lit.address].insert(nogoodNr);
	}else{
		watchingNogoodsOfNegLiteral[lit.address].insert(nogoodNr);
	}
}

void CDNLSolver::touchVarsInNogood(Nogood& ng){
	BOOST_FOREACH (ID lit, ng){
		if (lit.isNaf()){
			varCounterNeg[lit.address]++;
		}else{
			varCounterPos[lit.address]++;
		}
	}
}

void CDNLSolver::initListOfAllFacts(){

	// build a list of all literals which need to be assigned
	// go through all nogoods
	for (int i = 0; i < nogoodset.getNogoodCount(); ++i){
		const Nogood& ng = nogoodset.getNogood(i);
		// go through all literals of the nogood
		for (Nogood::const_iterator lIt = ng.begin(); lIt != ng.end(); ++lIt){
			allFacts.insert(lIt->address);
		}
	}
}

void CDNLSolver::resizeVectors(){

	unsigned atomNamespaceSize = ctx.registry()->ogatoms.getSize();
	DBGLOG(DBG, "Resizing vectors to ground-atom namespace of size: " << atomNamespaceSize);
	assignmentOrder.resize(atomNamespaceSize);
}

std::string CDNLSolver::litToString(ID lit){
	std::stringstream ss;
	ss << (lit.isNaf() ? std::string("-") : std::string("")) << lit.address;
	return ss.str();
}

int CDNLSolver::addNogoodAndUpdateWatchingStructures(Nogood ng){

	assert(ng.isGround());

	// do not add nogoods which expand the domain
	BOOST_FOREACH (ID lit, ng){
		if (!allFacts.contains(lit.address)) return 0;
	}

	int index = nogoodset.addNogood(ng);
	DBGLOG(DBG, "Adding nogood " << ng << " with index " << index);
	if ((int)watchedLiteralsOfNogood.size() <= index){
		watchedLiteralsOfNogood.push_back(Set<ID>(2, 1));
	}
	updateWatchingStructuresAfterAddNogood(index);

	return index;
}

std::string CDNLSolver::getStatistics(){

#ifndef NDEBUG
	std::stringstream ss;
	ss	<< "Assignments: " << cntAssignments << std::endl
		<< "Guesses: " << cntGuesses << std::endl
		<< "Backtracks: " << cntBacktracks << std::endl
		<< "Resolution steps: " << cntResSteps << std::endl
		<< "Conflicts: " << cntDetectedConflicts;
	return ss.str();
#else
	std::stringstream ss;
	ss << "Only available in debug mode";
	return ss.str();
#endif
}

CDNLSolver::CDNLSolver(ProgramCtx& c, NogoodSet ns) : ctx(c), nogoodset(ns), conflicts(0), cntAssignments(0), cntGuesses(0), cntBacktracks(0), cntResSteps(0), cntDetectedConflicts(0), tmpWatched(2, 1){

	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidsolvertime, "Solver time");
	resizeVectors();
	initListOfAllFacts();

	// create an interpretation and a storage for assigned facts (we need 3 values)
	interpretation.reset(new Interpretation(ctx.registry()));
	factWasSet.reset(new Interpretation(ctx.registry()));
	changed.reset(new Interpretation(ctx.registry()));
	currentDL = 0;
	exhaustedDL = 0;

	initWatchingStructures();
};

void CDNLSolver::restartWithAssumptions(const std::vector<ID>& assumptions){

	// reset
	DBGLOG(DBG, "Resetting solver");
	std::vector<IDAddress> toClear;
	bm::bvector<>::enumerator en = factWasSet->getStorage().first();
	bm::bvector<>::enumerator en_end = factWasSet->getStorage().end();
	while (en < en_end){
		toClear.push_back(*en);
		en++;
	}
	BOOST_FOREACH (IDAddress adr, toClear) clearFact(adr);
/*
	interpretation.reset(new Interpretation(ctx.registry()));
	factWasSet.reset(new Interpretation(ctx.registry()));
	changed.reset(new Interpretation(ctx.registry()));
	currentDL = 0;
	exhaustedDL = 0;

	initWatchingStructures();
*/
	// set assumptions at DL=0
	DBGLOG(DBG, "Setting assumptions");
	BOOST_FOREACH (ID a, assumptions){
		setFact(createLiteral(a.address, !a.isNaf()), 0);
	}
}

void CDNLSolver::addPropagator(PropagatorCallback* pb){
	propagator.insert(pb);
}

void CDNLSolver::removePropagator(PropagatorCallback* pb){
	propagator.erase(pb);
}

bool CDNLSolver::handlePreviousModel(){

	// is there a previous model?
	if (complete()){
		if (currentDL == 0){
			return false;
		}else{
			// add model as nogood to get another one
			// a restriction to the decision literals suffices
			Nogood modelNogood;
			BOOST_FOREACH (IDAddress fact, allFacts){
				if (isDecisionLiteral(fact)){
					modelNogood.insert(createLiteral(fact, interpretation->getFact(fact)));
				}
			}
			addNogoodAndUpdateWatchingStructures(modelNogood);
			DBGLOG(DBG, "Found previous model. Adding model as nogood " << (nogoodset.getNogoodCount() - 1) << ": " << modelNogood);

			// the new nogood is for sure contraditory
			Nogood learnedNogood;
			analysis(modelNogood, learnedNogood, currentDL);
			recentConflicts.push_back(addNogoodAndUpdateWatchingStructures(learnedNogood));
			DBGLOG(DBG, "Backtrack");
			backtrack(currentDL);
			return true;
		}
	}
	return true;
}

void CDNLSolver::flipDecisionLiteral(){

	// find decision literal dLit of current decision level
	ID dLit = decisionLiteralOfDecisionLevel[currentDL];
	currentDL--;
	exhaustedDL = currentDL;

	// goto previous decision level
	DBGLOG(DBG, "Backtrack to DL " << currentDL);
	backtrack(currentDL);

	// flip dLit, but now on the previous decision level!
	DBGLOG(DBG, "Flipping decision literal: " << litToString(negation(dLit)));
	setFact(negation(dLit), currentDL);
}

InterpretationPtr CDNLSolver::getNextModel(){
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidsolvertime, "Solver time");

	Nogood violatedNogood;

	// handle previous model
	if (complete()){
		if (currentDL == 0){
			DBGLOG(DBG, "No more models");
			return InterpretationPtr();
		}else{
			flipDecisionLiteral();
		}
	}

	bool anotherIterationEvenIfComplete = false;	// if set to true, the loop will run even if the interpretation is already complete
							// (needed to check if newly added nogood (e.g. by external learners) are satisfied)
	while (!complete()){
		anotherIterationEvenIfComplete = false;
		DBGLOG(DBG, "Unit propagation");
		if (!unitPropagation(violatedNogood)){
			if (currentDL == 0){
				// no answer set
				return InterpretationPtr();
			}else{
				if (currentDL > exhaustedDL){
					// backtrack
					Nogood learnedNogood;
					int k = currentDL;
					analysis(violatedNogood, learnedNogood, k);
					recentConflicts.push_back(addNogoodAndUpdateWatchingStructures(learnedNogood));
					currentDL = k > exhaustedDL ? k : exhaustedDL;	// do not jump below exhausted level, this could lead to regeneration of models
					backtrack(currentDL);
				}else{
					flipDecisionLiteral();
				}
			}
		}else{
			DBGLOG(DBG, "Calling external learner");
			int nogoodCount = nogoodset.getNogoodCount();
			BOOST_FOREACH (PropagatorCallback* cb, propagator){
				DBGLOG(DBG, "Calling external learners with interpretation: " << *interpretation);
				cb->propagate(interpretation, factWasSet, changed);
			}
			// add new nogoods
			int ngc = nogoodset.getNogoodCount();
			loadAddedNogoods();
			if (ngc != nogoodset.getNogoodCount()) anotherIterationEvenIfComplete = true;
			changed->clear();

			if (nogoodset.getNogoodCount() != nogoodCount){
				DBGLOG(DBG, "Learned something");
			}else{
				DBGLOG(DBG, "Did not learn anything");

				if (!complete()){
					// guess
					currentDL++;
					ID guess = getGuess();
					DBGLOG(DBG, "Guess: " << litToString(guess));
					decisionLiteralOfDecisionLevel[currentDL] = guess;
					setFact(guess, currentDL);
				}
			}
		}
		// add new nogoods
		loadAddedNogoods();
	}
	DBGLOG(DBG, "Got model");

	InterpretationPtr icp(new Interpretation(*interpretation));
	return icp;
}

void CDNLSolver::addNogood(Nogood ng){
	nogoodsToAdd.addNogood(ng);
}

std::vector<Nogood> CDNLSolver::getContradictoryNogoods(){

	std::vector<Nogood> ngg;
	BOOST_FOREACH (int idx, contradictoryNogoods){
		ngg.push_back(nogoodset.getNogood(idx));
	}
	return ngg;
}

Nogood CDNLSolver::getCause(IDAddress adr){
	return nogoodset.getNogood(cause[adr]);
}

DLVHEX_NAMESPACE_END
