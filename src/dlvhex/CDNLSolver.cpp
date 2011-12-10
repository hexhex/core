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

#include "dlvhex/CDNLSolver.hpp"

#include "dlvhex/ProgramCtx.h"

#include <iostream>
#include <sstream>
#include "dlvhex/Logger.hpp"
#include <boost/functional/hash.hpp>

DLVHEX_NAMESPACE_BEGIN

//#define DBGLOGD(X,Y) DBGLOG(X,Y)
#define DBGLOGD(X,Y) do{}while(false);

// ---------- Class CDNLSolver ----------

bool CDNLSolver::unitPropagation(Nogood& violatedNogood){

	DBGLOG(DBG, "Unit propagation starts");
	while (unitNogoods.size() > 0){
		int nogoodNr = *(unitNogoods.begin());
		const Nogood& nextUnitNogood = nogoodset.nogoods[nogoodNr];
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
		violatedNogood = nogoodset.nogoods[*(contradictoryNogoods.begin())];
		DBGLOG(DBG, "Unit propagation finished with detected contradiction " << violatedNogood);
		return false;
	}

	DBGLOG(DBG, "Unit propagation finished successfully");
	return true;
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
	do{
		count = 0;
		IDAddress impliedLit = ID_FAIL;

		ID latestLit = ID_FAIL;
		BOOST_FOREACH (ID lit, learnedNogood){
//DBGLOG(DBG, "I(" << lit.address << ") = " << getAssignmentOrderIndex(lit.address) << ", dl(" << lit.address << ") = " << decisionlevel[lit.address]);
			if (latestLit == ID_FAIL || getAssignmentOrderIndex(lit.address) > getAssignmentOrderIndex(latestLit.address)){
				latestLit = lit;
			}
		}
		latestDL = decisionlevel[latestLit.address];
//		DBGLOG(DBG, "Latest literal in nogood is " << latestLit.address);

//		int maxDL = 0;
		BOOST_FOREACH (ID lit, learnedNogood){
//			if (decisionlevel[lit.address] > maxDL){
//				maxDL = decisionlevel[lit.address];
//				count = 0;
//			}
			if (decisionlevel[lit.address] == latestDL){
				count++;
				if (!isDecisionLiteral(lit.address)){
					impliedLit = lit.address;
				}
			}
		}

		if (count > 1){
			// resolve the clause with multiple literals on top level
			// with the cause of one of the implied literals
			assert(impliedLit != ID_FAIL);

			Nogood& c = nogoodset.nogoods[cause[impliedLit]];
			touchVarsInNogood(c);
			learnedNogood = resolve(learnedNogood, c, impliedLit);
			++resSteps;
		}
	}while(count > 1);

	if (resSteps > 0){
		// if resSteps == 0, then learnedNogood == violatedNogood, which was already touched
		touchVarsInNogood(learnedNogood);
	}

	// backtrack to the second-highest decision level
	int bt = 0;
	BOOST_FOREACH (ID lit, learnedNogood){
		if (decisionlevel[lit.address] > bt && decisionlevel[lit.address] < latestDL){
			bt = decisionlevel[lit.address];
		}
	}	

	DBGLOG(DBG, "Learned conflict nogood: " << learnedNogood << " (after " << resSteps << " resolution steps)");
	DBGLOG(DBG, "Backtrack-DL: " << bt);
	backtrackDL = bt;

	// decision heuristic metric update
	++conflicts;
	if (conflicts >= 255){
		DBGLOG(DBG, "Maximum conflicts count: dividing all counters by 2");
		typedef std::pair<int, int> Pair;
		BOOST_FOREACH (Pair p, varCounterPos) p.second /= 2;
		BOOST_FOREACH (Pair p, varCounterNeg) p.second /= 2;
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
		DBGLOG(DBG, "Assigning " << litToString(fact) << "@" << dl << " with cause " << nogoodset.nogoods[c]);
	}else{
		DBGLOG(DBG, "Assigning " << litToString(fact) << "@" << dl);
	}
	factWasSet.set_bit(fact.address);			// fact was set
	decisionlevel[fact.address] = dl;			// store decision level
	if (c > -1) cause[fact.address] = c;			// store cause
	if (fact.isNaf()){					// store truth value
		interpretation->clearFact(fact.address);
	}else{
		interpretation->setFact(fact.address);
	}
	assignmentOrder.insert(fact.address);

	updateWatchingStructuresAfterSetFact(fact);

#ifndef NDEBUG
	++cntAssignments;
#endif
}

void CDNLSolver::clearFact(IDAddress litadr){
	DBGLOG(DBG, "Unassigning " << litadr << "@" << decisionlevel[litadr]);
	factWasSet.clear_bit(litadr);
	decisionlevel.erase(litadr);
	cause.erase(litadr);
	assignmentOrder.erase(litadr);

	// getFact will return the truth value which was just unset
	// (truth value remains until it is overridden by a new one)
	updateWatchingStructuresAfterClearFact(createLiteral(litadr, interpretation->getFact(litadr)));
}

void CDNLSolver::backtrack(int dl){
	// collect assignments to undo
	std::vector<IDAddress> undo;
	typedef std::pair<IDAddress, int> DecisionPair;
	BOOST_FOREACH (DecisionPair decision, decisionlevel){
		if (decision.second > dl){
			undo.push_back(decision.first);
		}
	}
	// actually undo them
	BOOST_FOREACH (IDAddress litadr, undo){
		clearFact(litadr);
	}

#ifndef NDEBUG
	++cntBacktracks;
#endif
}

ID CDNLSolver::getGuess(){


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
		Nogood& ng = nogoodset.nogoods[*rit];

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

		// if the nogood has no unassigned variable, it must be either satisfied or contradictory
		assert(mostActive != ID_FAIL);

		DBGLOG(DBG, "Guessing " << litToString(mostActive) << " because it occurs in recent conflicts");
		return mostActive;
	}

	// no recent conflicts
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
	watchedLiteralsOfNogood = std::vector<Set<ID> >(nogoodset.nogoods.size());
	watchingNogoodsOfLiteral.clear();
	nogoodsOfLiteral.clear();

	// each nogood watches (at most) two of its literals
	for (unsigned int nogoodNr = 0; nogoodNr < nogoodset.nogoods.size(); ++nogoodNr){
		updateWatchingStructuresAfterAddNogood(nogoodNr);
	}
}

void CDNLSolver::updateWatchingStructuresAfterAddNogood(int index){
	const Nogood& ng = nogoodset.nogoods[index];

	// remember for all literals in the nogood that they are contained in this nogood
	BOOST_FOREACH (ID lit, ng){
		nogoodsOfLiteral[lit].insert(index);
	}

	// search for up to two unassigned literals to watch
	bool inactive = false;
	Set<ID> watched;
	BOOST_FOREACH (ID lit, ng){
		if (!assigned(lit.address) && watched.size() < 2){
			watched.insert(lit);
		}else if(falsified(lit)){
			inactive = true;
		}
	}

	// remember watches
	if (!inactive){
		BOOST_FOREACH (ID lit, watched){
			startWatching(index, lit);
		}
	}

	if (inactive){
		DBGLOGD(DBG, "Nogood " << index << " is inactive");
	}else if (watched.size() == 1){
		DBGLOGD(DBG, "Nogood " << index << " is unit");
		unitNogoods.insert(index);
	}else if (watched.size() == 0){
		DBGLOGD(DBG, "Nogood " << index << " is contradictory");
		contradictoryNogoods.insert(index);
	}
}

void CDNLSolver::updateWatchingStructuresAfterRemoveNogood(int index){
	const Nogood& ng = nogoodset.nogoods[index];

	// remove the nogood from all literal lists
	BOOST_FOREACH (ID lit, ng){
		nogoodsOfLiteral[lit].erase(index);
	}

	// remove all watched literals
	Set<ID> watched = watchedLiteralsOfNogood[index];
	BOOST_FOREACH (ID lit, watched){
		stopWatching(index, lit);
	}
}

void CDNLSolver::updateWatchingStructuresAfterSetFact(ID lit){

	DBGLOGD(DBG, "updateWatchingStructuresAfterSetFact after " << litToString(lit) << " was set");
	bool changed;
	do{
		changed = false;

		// go through all nogoods which watch this literal negatively and inactivate them
		BOOST_FOREACH (int nogoodNr, watchingNogoodsOfLiteral[negation(lit)]){
			inactivateNogood(nogoodNr);
		}

		// go through all nogoods which watch this literal positively and find a new watched literal
		BOOST_FOREACH (int nogoodNr, watchingNogoodsOfLiteral[lit]){
			const Nogood& ng = nogoodset.nogoods[nogoodNr];

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
			// nogood might have become unit or contradictory
			if (watchedLiteralsOfNogood[nogoodNr].size() == 1){
				DBGLOGD(DBG, "Nogood " << nogoodNr << " is now unit");
				unitNogoods.insert(nogoodNr);
			}else if (!inactive && watchedLiteralsOfNogood[nogoodNr].size() == 0){
				DBGLOGD(DBG, "Nogood " << nogoodNr << " is now contradictory");
				contradictoryNogoods.insert(nogoodNr);
				unitNogoods.erase(nogoodNr);
			}

			changed = true;
			break;
		}
	}while(changed);
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
	Set<ID> positiveAndNegativeLiteral;
	positiveAndNegativeLiteral.insert(literal);
	positiveAndNegativeLiteral.insert(negation(literal));
	BOOST_FOREACH (ID lit, positiveAndNegativeLiteral){
		BOOST_FOREACH (int nogoodNr, nogoodsOfLiteral[lit]){

			const Nogood& ng = nogoodset.nogoods[nogoodNr];

			bool stillInactive = false;
			Set<ID> watched;

			// check the number of currently watched literals
			int watchedNum = watchedLiteralsOfNogood[nogoodNr].size();
			switch(watchedNum){
				case 0:		// nogood was inactive or contradictory before
					// nogood can:
					// 1. still be inactive
					// 2. have one unassigned literal
					// 3. have multiple unassigned literals
					// it cannot be contraditory anymore because at least one literal is unassigned!
					BOOST_FOREACH (ID lit, ng){
						if (falsified(lit)){
							stillInactive = true;
							break;
						}
						// collect up to 2 watched literals
						if (!assigned(lit.address) && watched.size() < 2){
							watched.insert(lit);
						}
					}
					if (!stillInactive){
						DBGLOGD(DBG, "Nogood " << nogoodNr << " is reactivated");
						BOOST_FOREACH (ID lit, watched){
							startWatching(nogoodNr, lit);
						}

						if (watched.size() == 1){
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

void CDNLSolver::inactivateNogood(int nogoodNr){
	DBGLOGD(DBG, "Nogood " << nogoodNr << " gets inactive");

	BOOST_FOREACH (ID lit, watchedLiteralsOfNogood[nogoodNr]){
		watchingNogoodsOfLiteral[lit].erase(nogoodNr);
	}
	watchedLiteralsOfNogood[nogoodNr].clear();

	unitNogoods.erase(nogoodNr);
	contradictoryNogoods.erase(nogoodNr);
}

void CDNLSolver::stopWatching(int nogoodNr, ID lit){
	DBGLOGD(DBG, "Nogood " << nogoodNr << " stops watching " << litToString(lit));
	watchingNogoodsOfLiteral[lit].erase(nogoodNr);
	watchedLiteralsOfNogood[nogoodNr].erase(lit);
}

void CDNLSolver::startWatching(int nogoodNr, ID lit){
	DBGLOGD(DBG, "Nogood " << nogoodNr << " starts watching " << litToString(lit));
	watchedLiteralsOfNogood[nogoodNr].insert(lit);
	watchingNogoodsOfLiteral[lit].insert(nogoodNr);
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
	for (std::vector<Nogood>::const_iterator nIt = nogoodset.nogoods.begin(); nIt != nogoodset.nogoods.end(); ++nIt){
		// go through all literals of the nogood
		for (Nogood::const_iterator lIt = nIt->begin(); lIt != nIt->end(); ++lIt){
			allFacts.insert(lIt->address);
		}
	}
}

std::string CDNLSolver::litToString(ID lit){
	std::stringstream ss;
	ss << (lit.isNaf() ? std::string("-") : std::string("")) << lit.address;
	return ss.str();
}

long CDNLSolver::getAssignmentOrderIndex(IDAddress adr){
	if (!assigned(adr)) return -1;
	return assignmentOrder.getInsertionIndex(adr); //std::find(assignmentOrder.begin(), assignmentOrder.end(), adr) - assignmentOrder.begin();
}

std::string CDNLSolver::getStatistics(){

#ifndef NDEBUG
	std::stringstream ss;
	ss	<< "Assignments: " << cntAssignments << std::endl
		<< "Backtracks: " << cntBacktracks << std::endl
		<< "Resolving steps: " << cntResSteps << std::endl
		<< "Conflicts: " << cntDetectedConflicts;
	return ss.str();
#else
	return str::str("Only available in debug mode");
#endif
}

CDNLSolver::CDNLSolver(ProgramCtx& c, NogoodSet ns) : ctx(c), nogoodset(ns), conflicts(0), cntAssignments(0), cntBacktracks(0), cntResSteps(0), cntDetectedConflicts(0){

	initListOfAllFacts();

	// create an interpretation
	interpretation.reset(new Interpretation(ctx.registry()));
	currentDL = 0;
	exhaustedDL = 0;

	initWatchingStructures();
};

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
			addNogood(modelNogood);
			DBGLOG(DBG, "Found previous model. Adding model as nogood " << (nogoodset.nogoods.size() - 1) << ": " << modelNogood);

			// the new nogood is for sure contraditory
			Nogood learnedNogood;
			analysis(modelNogood, learnedNogood, currentDL);
			recentConflicts.push_back(addNogood(learnedNogood));
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
	backtrack(currentDL);

	// flip dLit, but now on the previous decision level!
	DBGLOG(DBG, "Flipping decision literal: " << litToString(negation(dLit)));
	setFact(negation(dLit), currentDL);
}

InterpretationConstPtr CDNLSolver::getNextModel(){

	Nogood violatedNogood;

	// handle previous model
	if (complete()){
		if (currentDL == 0){
			DBGLOG(DBG, "No more models");
			return InterpretationConstPtr();
		}else{
			flipDecisionLiteral();
		}
	}

/*
	if (!handlePreviousModel()){
		return InterpretationConstPtr();
	}
*/

	while (!complete()){
		DBGLOG(DBG, "Unit propagation");
		if (!unitPropagation(violatedNogood)){
			if (currentDL == 0){
				// no answer set
				return InterpretationConstPtr();
			}else{
				if (currentDL > exhaustedDL){
					// backtrack
					Nogood learnedNogood;
					int k = currentDL;
					analysis(violatedNogood, learnedNogood, k);
					recentConflicts.push_back(addNogood(learnedNogood));
					currentDL = k > exhaustedDL ? k : exhaustedDL;	// do not jump below exhausted level, this could lead to regeneration of models
					backtrack(currentDL);
				}else{
					flipDecisionLiteral();
				}
			}
		}else{
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
	DBGLOG(DBG, "Got model");

	InterpretationConstPtr icp(interpretation);
	return icp;
}

int CDNLSolver::addNogood(Nogood ng){

	// TODO: How to handle new facts?
	BOOST_FOREACH (ID lit, ng){
	//	allFacts.insert(lit.address);
		if (!allFacts.contains(lit.address)) return 0;
	}

	int index = nogoodset.addNogood(ng);
	DBGLOG(DBG, "Adding nogood " << ng << " with index " << index);
	if ((int)watchedLiteralsOfNogood.size() <= index){
		watchedLiteralsOfNogood.push_back(Set<ID>());
	}
	updateWatchingStructuresAfterAddNogood(index);

	return index;
}

void CDNLSolver::removeNogood(int nogoodIndex){
	nogoodset.removeNogood(nogoodIndex);
	updateWatchingStructuresAfterRemoveNogood(nogoodIndex);
}

int CDNLSolver::getNogoodCount(){
	return nogoodset.nogoods.size();
}

DLVHEX_NAMESPACE_END
