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
 * @brief ASP solver based on conflict-driven nogood learning.
 */

#include "dlvhex2/InternalGroundASPSolver.h"

#include <iostream>
#include <sstream>
#include "dlvhex2/Logger.h"
#include "dlvhex2/GenuineSolver.h"
#include <boost/foreach.hpp>
#include <boost/graph/strong_components.hpp>

DLVHEX_NAMESPACE_BEGIN

//#define DBGLOGD(X,Y) DBGLOG(X,Y)
#define DBGLOGD(X,Y) do{}while(false);

// 1. body must not be false if all literals are true
// 2. body must not be true if a literal is false
// 3. head must not be false if body is true
void InternalGroundASPSolver::createNogoodsForRule(ID ruleBodyAtomID, ID ruleID){

	const Rule& r = reg->rules.getByID(ruleID);

	// 1 and 2
	createNogoodsForRuleBody(ruleBodyAtomID, r.body);

	// 3. head must not be false if body is true
	Nogood bodyImpliesHead;
	bodyImpliesHead.insert(createLiteral(ruleBodyAtomID));
	BOOST_FOREACH(ID headLit, r.head){
		bodyImpliesHead.insert(createLiteral(headLit.address, false));
	}
	nogoodset.addNogood(bodyImpliesHead);
}

// 1. body must not be false if all literals are true
// 2. body must not be true if a literal is false
void InternalGroundASPSolver::createNogoodsForRuleBody(ID ruleBodyAtomID, const Tuple& ruleBody){

	// 1. body must not be false if all literals are true
	Nogood bodySatIfLitSat;
	BOOST_FOREACH(ID bodyLit, ruleBody){
		bodySatIfLitSat.insert(createLiteral(bodyLit));
	}
	bodySatIfLitSat.insert(negation(ruleBodyAtomID));
	nogoodset.addNogood(bodySatIfLitSat);

	// 2. body must not be true if a literal is false
	BOOST_FOREACH(ID bodyLit, ruleBody){
		Nogood bodyFalseIfLitFalse;			
		bodyFalseIfLitFalse.insert(createLiteral(ruleBodyAtomID));
		bodyFalseIfLitFalse.insert(negation(createLiteral(bodyLit)));
		nogoodset.addNogood(bodyFalseIfLitFalse);
	}
}

Set<std::pair<ID, ID> > InternalGroundASPSolver::createShiftedProgram(){

	// create for each rule of kind
	//	a(1) v ... v a(m) :- B
	// all shifted rules of kind
	//	a(i) :- B, -a(1), ..., -a(i-1), -a(i+1), ..., a(m)
	// for all 1 <= i <= m

	DBGLOG(DBG, "Creating shifted program");

	Set<std::pair<ID, ID> > shiftedProg;
	BOOST_FOREACH (ID ruleID, program.idb){

		const Rule& r = reg->rules.getByID(ruleID);

		// real shifted rule?
		if (r.head.size() > 1){
			BOOST_FOREACH (ID headLit, r.head){
				// take body of r and current head literal
				Tuple singularHead;
				singularHead.push_back(createLiteral(headLit));
				Rule shiftedRule(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR, singularHead, r.body);

				// add negations of all other head literals to body
				BOOST_FOREACH (ID otherHeadLit, r.head){
					if (otherHeadLit != headLit){
						shiftedRule.body.push_back(negation(createLiteral(otherHeadLit)));
					}
				}

				// create a new body atom
				DBGLOG(DBG, "Creating real shifted rule: " << shiftedRule);
				ID shiftedRuleID = reg->rules.getIDByElement(shiftedRule);
				if (shiftedRuleID == ID_FAIL){
					shiftedRuleID = reg->rules.storeAndGetID(shiftedRule);
				}
				ID ruleBodyAtomID = createNewBodyAtom();

				shiftedProg.insert(std::pair<ID, ID>(shiftedRuleID, ruleBodyAtomID));
			}
		}else{
			// rule was already present in original program and has already a body literal
			DBGLOG(DBG, "Creating shifted rule which was already present in original program: " << r);

			shiftedProg.insert(std::pair<ID, ID>(ruleID, createLiteral(bodyAtomOfRule[ruleID.address])));
		}
	}

	DBGLOG(DBG, "Creating shifted program finished");

	return shiftedProg;
}

void InternalGroundASPSolver::createSingularLoopNogoods(){

	DBGLOG(DBG, "Creating singular loop nogoods");

	typedef std::pair<ID, ID> RulePair;
	Set<RulePair> shiftedProgram = createShiftedProgram();

	// create for each real shifted rule the nogoods which associate the rule body with the body atom
	// (shifted rules which were already present in the original program were already handled before)
	BOOST_FOREACH (RulePair pair, shiftedProgram){
		if (!contains(program.idb, pair.first)){
			const Rule& r = reg->rules.getByID(pair.first);
			createNogoodsForRuleBody(pair.second, r.body);
		}
	}

	// an atom must not be true if the bodies of all supporting shifted rules are false
	BOOST_FOREACH (IDAddress litadr, ordinaryFacts){
		// only for atoms which are no facts
		if (!program.edb->getFact(litadr)){
			Nogood supNogood;
			supNogood.insert(createLiteral(litadr));

			// for all supporting rules
			BOOST_FOREACH(RulePair pair, shiftedProgram){
				const Rule& r = reg->rules.getByID(pair.first);
				if (r.head.size() > 0 && (r.head[0].address == litadr)){
					supNogood.insert(negation(pair.second));
				}
			}
			nogoodset.addNogood(supNogood);
		}
	}

	DBGLOG(DBG, "Nogoods with singular loop nogoods: " << nogoodset);
}

void InternalGroundASPSolver::resizeVectors(){

	CDNLSolver::resizeVectors();

	unsigned atomNamespaceSize = reg->ogatoms.getSize();

/*
	componentOfAtom.resize(atomNamespaceSize);
	bodyAtomOfRule.resize(atomNamespaceSize);
	rulesWithPosBodyLiteral.resize(atomNamespaceSize);
	rulesWithNegBodyLiteral.resize(atomNamespaceSize);
	rulesWithPosHeadLiteral.resize(atomNamespaceSize);
	foundedAtomsOfBodyAtom.resize(atomNamespaceSize);
*/
}

void InternalGroundASPSolver::computeClarkCompletion(){

	// compute completion
	BOOST_FOREACH (ID ruleID, program.idb){
		ID ruleBodyAtomID = createNewBodyAtom();
		bodyAtomOfRule[ruleID.address] = ruleBodyAtomID.address;
		createNogoodsForRule(ruleBodyAtomID, ruleID);
	}

	DBGLOG(DBG, "Created Clark's completion: " << nogoodset);
}

void InternalGroundASPSolver::setEDB(){

	DBGLOG(DBG, "Setting EDB");

	// set all facts at decision level 0 without cause
	bm::bvector<>::enumerator en = program.edb->getStorage().first();
	bm::bvector<>::enumerator en_end = program.edb->getStorage().end();

	while (en < en_end){
		if (ordinaryFacts.count(*en) > 0){
			setFact(createLiteral(*en), 0, -1);
		}
		++en;
	}
}

void InternalGroundASPSolver::computeDepGraph(){

	// all literals are nodes
	BOOST_FOREACH (IDAddress litadr, ordinaryFacts){
		depNodes[litadr] = boost::add_vertex(litadr, depGraph);
	}

	// go through all rules
	BOOST_FOREACH (ID ruleID, program.idb){
		const Rule& rule = reg->rules.getByID(ruleID);

		// add an arc from all head literals to all positive body literals
		BOOST_FOREACH (ID headLiteral, rule.head){
			BOOST_FOREACH (ID bodyLiteral, rule.body){
				if (!bodyLiteral.isNaf()){
					boost::add_edge(depNodes[headLiteral.address], depNodes[bodyLiteral.address], depGraph);
				}
			}
		}
	}
}

void InternalGroundASPSolver::computeStronglyConnectedComponents(){

	// find strongly connected components in the dependency graph using boost
	std::vector<int> componentMap(depNodes.size());
	int num = boost::strong_components(depGraph, &componentMap[0]);

	// translate into real map
	depSCC = std::vector<Set<IDAddress> >(num);
	Node nodeNr = 0;
	BOOST_FOREACH (int componentOfNode, componentMap){
		depSCC[componentOfNode].insert(depGraph[nodeNr]);
		componentOfAtom[depGraph[nodeNr]] = componentOfNode;
		nodeNr++;
	}

	// store which atoms occur in non-singular components
	BOOST_FOREACH (IDAddress litadr, ordinaryFacts){
		if (depSCC[componentOfAtom[litadr]].size() > 1){
			nonSingularFacts.insert(litadr);
		}
	}

#ifndef NDEBUG
	std::stringstream compStr;
	bool firstC = true;
	BOOST_FOREACH (Set<IDAddress> component, depSCC){
		if (!firstC) compStr << ", ";
		firstC = false;
		compStr << "{";
		bool firstL = true;
		BOOST_FOREACH (IDAddress litadr, component){
			if (!firstL) compStr << ", ";
			firstL = false;
			compStr << litadr;
		}
		compStr << "}";
	}
	DBGLOG(DBG, "Program components: " << compStr.str());
	DBGLOG(DBG, "All atoms: " << toString(allFacts));
	DBGLOG(DBG, "All ordinary atoms: " << toString(ordinaryFacts));
	DBGLOG(DBG, "Ordinary atoms in non-singular components: " << toString(nonSingularFacts));
#endif
}

void InternalGroundASPSolver::initSourcePointers(){

	DBGLOG(DBG, "Initialize source pointers");

	// initially, all atoms in non-singular components, except facts, are unfounded
	BOOST_FOREACH (IDAddress litadr, ordinaryFacts){
		if (program.edb->getFact(litadr)){
			// store pseudo source rule ID_FAIL to mark that this is a fact and founded by itself
			sourceRule[litadr] = ID_FAIL;
		}else{
			// all non-facts in non-singular components are unfounded
			if (nonSingularFacts.count(litadr) > 0){
				unfoundedAtoms.insert(litadr);
			}
		}
	}

	DBGLOG(DBG, "Initially unfounded atoms: " << toString(unfoundedAtoms));
}

void InternalGroundASPSolver::initializeLists(){

	// determine the set of all facts and a literal index
	BOOST_FOREACH (ID ruleID, program.idb){
		const Rule& r = reg->rules.getByID(ruleID);

		// remember this rule for each contained literal
		for (std::vector<ID>::const_iterator lIt = r.head.begin(); lIt != r.head.end(); ++lIt){
			rulesWithPosHeadLiteral[lIt->address].insert(ruleID);
			// collect all facts
			allFacts.insert(lIt->address);
			ordinaryFacts.insert(lIt->address);
		}
		for (std::vector<ID>::const_iterator lIt = r.body.begin(); lIt != r.body.end(); ++lIt){
			if (!lIt->isNaf()){
				rulesWithPosBodyLiteral[lIt->address].insert(ruleID);
			}else{
				rulesWithNegBodyLiteral[lIt->address].insert(ruleID);
			}
			// collect all facts
			allFacts.insert(lIt->address);
			ordinaryFacts.insert(lIt->address);
		}
	}

	// include facts in the list of all literals
	bm::bvector<>::enumerator en = program.edb->getStorage().first();
	bm::bvector<>::enumerator en_end = program.edb->getStorage().end();
	while (en < en_end){
		allFacts.insert(*en);
		ordinaryFacts.insert(*en);
		++en;
	}

	// built an interpretation of ordinary facts
	ordinaryFactsInt = InterpretationPtr(new Interpretation(reg));
	BOOST_FOREACH (IDAddress idadr, ordinaryFacts){
		ordinaryFactsInt->setFact(idadr);
	}
}

void InternalGroundASPSolver::setFact(ID fact, int dl, int cause = -1){
	CDNLSolver::setFact(fact, dl, cause);
	changed.set_bit(fact.address);
	updateUnfoundedSetStructuresAfterSetFact(fact);
}

void InternalGroundASPSolver::clearFact(IDAddress litadr){
	CDNLSolver::clearFact(litadr);
	changed.set_bit(litadr);
	updateUnfoundedSetStructuresAfterClearFact(litadr);
}

void InternalGroundASPSolver::removeSourceFromAtom(IDAddress litadr){

	// check if the literal has currently a source rule
	if (sourceRule.find(litadr) != sourceRule.end()){
		if (sourceRule[litadr] != ID_FAIL){
			ID sourceRuleID = sourceRule[litadr];
			DBGLOG(DBG, "Literal " << litadr << " canceled its source pointer to rule " << sourceRuleID.address);
			foundedAtomsOfBodyAtom[bodyAtomOfRule[sourceRuleID.address]].erase(litadr);
			sourceRule.erase(litadr);
		}
	}
}

void InternalGroundASPSolver::addSourceToAtom(IDAddress litadr, ID rule){
	DBGLOG(DBG, "Literal " << litadr << " sets a source pointer to " << rule.address);
	sourceRule[litadr] = rule;
	foundedAtomsOfBodyAtom[bodyAtomOfRule[rule.address]].insert(litadr);
}

Set<IDAddress> InternalGroundASPSolver::getDependingAtoms(IDAddress litadr){

	// litadr became unfounded; now compute all atoms which depend on litadr and are
	// therefore become unfounded too
	Set<IDAddress> dependingAtoms;

	// go through all rules which contain litadr in their body
	BOOST_FOREACH (ID ruleID, rulesWithPosBodyLiteral[litadr]){

		// go through all atoms which use this rule as source
		BOOST_FOREACH (IDAddress dependingAtom, foundedAtomsOfBodyAtom[bodyAtomOfRule[ruleID.address]]){
			// this atom is depends on litadr
			dependingAtoms.insert(dependingAtom);
		}
	}

	return dependingAtoms;
}

void InternalGroundASPSolver::getInitialNewlyUnfoundedAtomsAfterSetFact(ID fact, Set<IDAddress>& newlyUnfoundedAtoms){

	// if the fact is a falsified body literal, all atoms which depend on it become unfounded
	if (fact.isNaf()){
		if (foundedAtomsOfBodyAtom.find(fact.address) != foundedAtomsOfBodyAtom.end()){
			BOOST_FOREACH (IDAddress dependingAtom, foundedAtomsOfBodyAtom[fact.address]){
				DBGLOGD(DBG, "" << dependingAtom << " is initially unfounded because the body of its source rule became false");
				newlyUnfoundedAtoms.insert(dependingAtom);
			}
		}
	}

	// if the fact is a satisfied head literal of a rule, all head literals which use it as source rule and
	// (i) which were set later; or
	// (ii) which are true in a different component
	// become unfounded
	else{
		// for all rules which contain the fact in their head
		if (rulesWithPosHeadLiteral.find(fact.address) != rulesWithPosHeadLiteral.end()){
			BOOST_FOREACH (ID ruleID, rulesWithPosHeadLiteral[fact.address]){
				const Rule& r = reg->rules.getByID(ruleID);

				// all other head literals cannot use this rule as source, if
				BOOST_FOREACH (ID otherHeadLit, r.head){
					if (otherHeadLit.address != fact.address && sourceRule[otherHeadLit.address] == ruleID){
						// (i) they were set to true later
						// TODO: maybe we have to compare the order of assignments instead of the decision levels
						//       or we can use the decision level (would be much more efficient)
						if (satisfied(createLiteral(otherHeadLit.address)) &&
						    getAssignmentOrderIndex(otherHeadLit.address) > getAssignmentOrderIndex(fact.address)
//						    decisionlevel[otherHeadLit.address] > decisionlevel[fact.address]
						){
							DBGLOGD(DBG, "" << otherHeadLit.address << " is initially unfounded because " << otherHeadLit.address <<
								" occurs in the head of its source rule and became true on a lower decision level");
							newlyUnfoundedAtoms.insert(otherHeadLit.address);
						}

						// (ii) they belong to a different component
						if (componentOfAtom[otherHeadLit.address] != componentOfAtom[fact.address]){
							DBGLOGD(DBG, "" << otherHeadLit.address << " is initially unfounded because " << fact.address <<
								" occurs in the head of its source rule and is true in a different component");
							newlyUnfoundedAtoms.insert(otherHeadLit.address);
						}
					}
				}
			}
		}
	}

	DBGLOGD(DBG, "Scope of unfounded set check is initially extended by " << toString(newlyUnfoundedAtoms));

}

void InternalGroundASPSolver::updateUnfoundedSetStructuresAfterSetFact(ID fact){

	DBGLOGD(DBG, "Updating set of atoms without source pointers, currently: " << toString(unfoundedAtoms));

#ifndef NDEBUG
	{
		std::stringstream ss;
		typedef std::pair<IDAddress, Set<IDAddress> > SourcePair;
		BOOST_FOREACH (SourcePair pair, foundedAtomsOfBodyAtom){
			ss << "Body atom " << pair.first << " is source for " << toString(pair.second) << "; ";
		}
		DBGLOGD(DBG, "Source pointer: " << ss.str());
	}
#endif

	// atom does not need a source pointer if it is assigned to false
	if (fact.isNaf()){
		DBGLOGD(DBG, "Literal " << fact.address << " was assigned to false and is not unfounded anymore");
		removeSourceFromAtom(fact.address);
		unfoundedAtoms.erase(fact.address);
	}

	// update the unfounded data structures
	DBGLOGD(DBG, "Computing initially newly unfounded atoms");
	Set<IDAddress> newlyUnfoundedAtoms;
	getInitialNewlyUnfoundedAtomsAfterSetFact(fact, newlyUnfoundedAtoms);

	while (newlyUnfoundedAtoms.size() > 0){
		Set<IDAddress> nextNewlyUnfoundedAtoms;

		DBGLOGD(DBG, "Collecting depending atoms of " << toString(newlyUnfoundedAtoms));

		BOOST_FOREACH (IDAddress newlyUnfoundedAtom, newlyUnfoundedAtoms){
			// only atoms which occur in non-singular components
			// (singular atoms are already handled by static loop nogoods)
			if (nonSingularFacts.count(newlyUnfoundedAtom) > 0){
				// only atoms which are not already unfounded or false
				if (!falsified(createLiteral(newlyUnfoundedAtom)) && (unfoundedAtoms.count(newlyUnfoundedAtom) == 0)){
					// only atoms which occur in a component that depends on unfounded atoms
					if (intersect(depSCC[componentOfAtom[newlyUnfoundedAtom]], unfoundedAtoms).size() > 0 ||
					    intersect(depSCC[componentOfAtom[newlyUnfoundedAtom]], newlyUnfoundedAtoms).size() > 0){
						DBGLOGD(DBG, "Atom " << newlyUnfoundedAtom << " becomes unfounded");
						removeSourceFromAtom(newlyUnfoundedAtom);
						unfoundedAtoms.insert(newlyUnfoundedAtom);

						// collect depending atoms
						Set<IDAddress> dependingAtoms = getDependingAtoms(newlyUnfoundedAtom);
						DBGLOGD(DBG, "Depending on " << newlyUnfoundedAtom << ": " << toString(dependingAtoms));
						nextNewlyUnfoundedAtoms.insert(dependingAtoms.begin(), dependingAtoms.end());
					}
				}
			}
		}
		newlyUnfoundedAtoms = nextNewlyUnfoundedAtoms;
	}

	DBGLOG(DBG, "Updated set of unfounded atoms: " << toString(unfoundedAtoms));
}

void InternalGroundASPSolver::updateUnfoundedSetStructuresAfterClearFact(IDAddress litadr){

	DBGLOGD(DBG, "Updating set of atoms without source pointers, currently: " << toString(unfoundedAtoms));

	// fact becomes unfounded if it has no source pointer
	// and if it is non-singular
	if (nonSingularFacts.count(litadr) > 0){
		if (sourceRule.find(litadr) == sourceRule.end()){
			unfoundedAtoms.insert(litadr);
		}
	}

	DBGLOGD(DBG, "Updated set of unfounded atoms: " << toString(unfoundedAtoms));
}

ID InternalGroundASPSolver::getPossibleSourceRule(const Set<ID>& ufs){

	DBGLOG(DBG, "Computing externally supporting rules for " << toString(ufs));

	Set<ID> extSup = getExternalSupport(ufs);

#ifndef NDEBUG
	{
		std::stringstream ss;
		ss << "Externally supporting rules of potential ufs: {";
		bool first = true;
		BOOST_FOREACH (ID ruleID, extSup){
			if (!first) ss << ", ";
			first = false;
			ss << ruleID.address;
		}
		ss << "}";
		DBGLOG(DBG, ss.str());
	}
#endif

	// from this set, remove all rules which are satisfied independently from ufs
	// and can therefore not be used as source rules
	BOOST_FOREACH (ID extRuleID, extSup){
		Set<ID> satInd = satisfiesIndependently(extRuleID, ufs);
		bool skipRule = false;
		BOOST_FOREACH (ID indSatLit, satInd){
			if (satisfied(indSatLit)){
				skipRule = true;
				break;
			}
		}
		if (!skipRule){
			DBGLOG(DBG, "Found possible source rule: " << extRuleID.address);
			return extRuleID;
		}else{
			DBGLOG(DBG, "Rule " << extRuleID.address << " is removed (independently satisfied)");
		}
	}

	return ID_FAIL;
}

// a head atom uses the rule as source, if
// 1. the atom is currently unfounded
// 2. no other head literal was set to true earlier
bool InternalGroundASPSolver::useAsNewSourceForHeadAtom(IDAddress headAtom, ID sourceRuleID){

	DBGLOG(DBG, "Checking if " << headAtom << " uses rule " << sourceRuleID.address << " as source");
	if (unfoundedAtoms.count(headAtom) == 0){
		DBGLOG(DBG, "No: " << headAtom << " is currently not unfounded");
		return false;
	}

	const Rule& sourceRule = reg->rules.getByID(sourceRuleID);

	// only the literal which was set first can use a rule as source:
	// 
	// if headLit is currently assigned, other head literals must not be set to true earlier
	// if headLit is currently unassigned, other head literals must not be true at all
	if (assigned(headAtom)){
		int headLitDecisionLevel = decisionlevel[headAtom];
		BOOST_FOREACH (ID otherHeadLit, sourceRule.head){
			if (otherHeadLit.address != headAtom){
				if (satisfied(otherHeadLit)){
					// TODO: maybe we have to compare the order of assignments instead of the decision levels
					//       or we can use the decision level (would be much more efficient)
					if (
					    getAssignmentOrderIndex(otherHeadLit.address) < getAssignmentOrderIndex(headAtom)
//					    decisionlevel[otherHeadLit.address] < headLitDecisionLevel
					){
						DBGLOG(DBG, "No: Head literal " << otherHeadLit.address << " was set to true on a lower decision level");
						return false;
					}
				}
			}
		}
	}else{
		BOOST_FOREACH (ID otherHeadLit, sourceRule.head){
			if (otherHeadLit.address != headAtom){
				if (satisfied(otherHeadLit)){
					DBGLOG(DBG, "No: Head literal " << otherHeadLit.address << " was already set to true, whereas " << headAtom << " is unassigned");
					return false;
				}
			}
		}
	}
	return true;
}

Set<ID> InternalGroundASPSolver::getUnfoundedSet(){

	DBGLOG(DBG, "Currently unfounded atoms: " << toString(unfoundedAtoms));

	Set<ID> ufs(5, 10);
	while (unfoundedAtoms.size() > 0){
		IDAddress atom = *(unfoundedAtoms.begin());
		ufs.clear();
		ufs.insert(createLiteral(atom));
		do{
			DBGLOG(DBG, "Trying to build an unfounded set over " << toString(ufs));

			// find a rule which externally supports ufs and
			// which is not satisfied independently of ufs
			ID supportingRuleID = getPossibleSourceRule(ufs);

			// if no rule survives, ufs is indeed unfounded
			if (supportingRuleID == ID_FAIL) return ufs;

			// check if this rule depends on unfounded atoms from atom's component
			const Rule& supportingRule = reg->rules.getByID(supportingRuleID);
			bool dependsOnUnfoundedAtoms = false;
			BOOST_FOREACH (ID bodyLit, supportingRule.body){
				if (!bodyLit.isNaf() && (unfoundedAtoms.count(bodyLit.address) > 0) && (depSCC[componentOfAtom[atom]].count(bodyLit.address) > 0)){
					// extend the unfounded set by this atom
					DBGLOG(DBG, "Rule depends on unfounded " << litToString(bodyLit) << " --> adding to ufs");
					ufs.insert(bodyLit);
					dependsOnUnfoundedAtoms = true;
				}
			}

			// if the rule does not depend on unfounded atoms, it can be used as the new source for its head atom(s)
			if (!dependsOnUnfoundedAtoms){
				BOOST_FOREACH (ID headLit, supportingRule.head){

					if (useAsNewSourceForHeadAtom(headLit.address, supportingRuleID)){
						// use the rule as new source
						DBGLOG(DBG, "Using rule " << supportingRuleID.address << " as new source for " << headLit.address);
						addSourceToAtom(headLit.address, supportingRuleID);

						// atom hIt->address is no longer unfounded
						unfoundedAtoms.erase(headLit.address);
						ufs.erase(createLiteral(headLit.address));
					}
				}
			}
		}while(ufs.size() > 0);
	}

	return Set<ID>();
}

bool InternalGroundASPSolver::doesRuleExternallySupportLiteral(ID ruleID, ID lit, const Set<ID>& s){

	const Rule& rule = reg->rules.getByID(ruleID);

	// check if the rule supports the literal
	bool supportsLit = false;
	BOOST_FOREACH (ID headLit, rule.head){
		if (headLit.address == lit.address){
			supportsLit = true;
			break;
		}
	}
	if (!supportsLit) return false;

	// check if the support is external wrt s
	BOOST_FOREACH (ID sLit, s){
		if (contains(rule.body, sLit)){
			return false;
		}
	}

	return true;
}

Set<ID> InternalGroundASPSolver::getExternalSupport(const Set<ID>& s){

	Set<ID> extRules;
	DBGLOG(DBG, "Computing externally supporting rules for set " << toString(s));

	// go through all rules which contain one of s in their head
	BOOST_FOREACH (ID lit, s){

		const Set<ID>& containingRules = rulesWithPosHeadLiteral[lit.address];

		BOOST_FOREACH (ID ruleID, containingRules){

			// check if none of the elements of s occurs in the body of r
			if (doesRuleExternallySupportLiteral(ruleID, lit, s)){
				DBGLOG(DBG, "Found external rule " << ruleID.address << " for set " << toString(s));
				extRules.insert(ruleID);
			}else{
				DBGLOGD(DBG, "Rule " << ruleID.address << " contains " << lit.address << " but does not externally support it wrt " << toString(s));
			}
		}
	}
	return extRules;
}

Set<ID> InternalGroundASPSolver::satisfiesIndependently(ID ruleID, const Set<ID>& y){

	const Rule& rule = reg->rules.getByID(ruleID);

	// compute all literals which satisfy the rule independently of set y:
	// either (i) the body of rule is false; or
	//        (ii) some head literal, which is not in y, is true
	Set<ID> indSat;
	indSat.insert(createLiteral(bodyAtomOfRule[ruleID.address], false));	// (i)
	BOOST_FOREACH (ID headLiteral, rule.head){				// (ii)
		if (y.count(createLiteral(headLiteral.address)) == 0){
			indSat.insert(createLiteral(headLiteral.address));
		}
	}
	DBGLOG(DBG, "Rule " << ruleID.address << " is satisfied independently from " << toString(y) << " by " << toString(indSat));
	return indSat;
}

Nogood InternalGroundASPSolver::getLoopNogood(const Set<ID>& ufs){

	Nogood loopNogood;

	// there are exponentially many loop nogoods for ufs;
	// choose one l from
	// lamba(ufs) = { Ta | a in ufs} x Prod_{r in extsup(ufs)} indsat(r, ufs)
	// such that l \ { Ta | a in ufs} is currently satisfied
	loopNogood.insert(createLiteral(*(ufs.begin())));

	// choose for each external rule one literal which
	// (i) satisfies it independently from ufs; and
	// (ii) is currently true
	Set<ID> extSup = getExternalSupport(ufs);
	BOOST_FOREACH (ID ruleID, extSup){
		Set<ID> satInd = satisfiesIndependently(ruleID, ufs);	// (i)
		BOOST_FOREACH (ID indLit, satInd){
			if (satisfied(indLit)){				// (ii)
				loopNogood.insert(createLiteral(indLit));
				break;
			}
		}
	}
	DBGLOG(DBG, "Loop nogood for " << toString(ufs) << " is " << loopNogood);

	return loopNogood;
}

/*
ID InternalGroundASPSolver::createNewAtom(std::string predName){
	OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
	Term predTerm(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, predName);
	ID predID = reg->storeTerm(predTerm);
	atom.tuple.push_back(predID);
	return createLiteral(reg->storeOrdinaryGAtom(atom).address);
}
*/

ID InternalGroundASPSolver::createNewAtom(ID predID){
	OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
	atom.tuple.push_back(predID);
	return createLiteral(reg->storeOrdinaryGAtom(atom).address);
}

ID InternalGroundASPSolver::createNewBodyAtom(){
	std::stringstream bodyPred;
	bodyPred << bodyAtomPrefix << bodyAtomNumber;
	DBGLOG(DBG, "Creating body atom " << bodyPred);
	bodyAtomNumber++;
//	ID bodyAtom = createNewAtom(bodyPred.str());
	ID bodyAtom = createNewAtom(reg->getNewConstantTerm("body"));
	allFacts.insert(bodyAtom.address);
	return bodyAtom;
}

std::string InternalGroundASPSolver::toString(const Set<ID>& lits){
	std::stringstream ss;
	ss << "{";
	bool first = true;
	BOOST_FOREACH (ID lit, lits){
		if (!first) ss << ", ";
		if ((lit.kind & ID::NAF_MASK) != 0) ss << "-";
		ss << lit.address;
		first = false;
	}
	ss << "}";
	return ss.str();
}

std::string InternalGroundASPSolver::toString(const Set<IDAddress>& lits){
	std::stringstream ss;
	ss << "{";
	bool first = true;
	BOOST_FOREACH (IDAddress lit, lits){
		if (!first) ss << ", ";
		ss << lit;
		first = false;
	}
	ss << "}";
	return ss.str();
}

std::string InternalGroundASPSolver::toString(const std::vector<IDAddress>& lits){
	std::stringstream ss;
	ss << "{";
	for (std::vector<IDAddress>::const_iterator it = lits.begin(); it != lits.end(); ++it){
		if (it != lits.begin()) ss << ", ";
		ss << *it;
	}
	ss << "}";
	return ss.str();
}

std::string InternalGroundASPSolver::getStatistics(){

#ifndef NDEBUG
	std::stringstream ss;
	ss	<< CDNLSolver::getStatistics() << std::endl
		<< "Detected unfounded sets: " << cntDetectedUnfoundedSets;
	return ss.str();
#else
	std::stringstream ss;
	ss << "Only available in debug mode";
	return ss.str();
#endif
}

InternalGroundASPSolver::InternalGroundASPSolver(ProgramCtx& c, OrdinaryASPProgram& p) : CDNLSolver(c, NogoodSet()), program(p), bodyAtomPrefix(std::string("body_")), bodyAtomNumber(0), firstmodel(true), cntDetectedUnfoundedSets(0){
	DBGLOG(DBG, "Internal Ground ASP Solver Init");

	reg = ctx.registry();

	resizeVectors();
	initializeLists();
	computeClarkCompletion();
	createSingularLoopNogoods();
	resizeVectors();
	initWatchingStructures();
	computeDepGraph();
	computeStronglyConnectedComponents();
	initSourcePointers();
	setEDB();
}

void InternalGroundASPSolver::addExternalLearner(LearningCallback* lb){
	learner.insert(lb);
}

void InternalGroundASPSolver::removeExternalLearner(LearningCallback* lb){
	learner.erase(lb);
}

InterpretationConstPtr InternalGroundASPSolver::getNextModel(){

	Nogood violatedNogood;

	if (!firstmodel && complete()){
/*
		if (!handlePreviousModel()){
			return InterpretationConstPtr();
		}
*/
		if (currentDL == 0){
			return InterpretationConstPtr();
		}else{
			flipDecisionLiteral();
		}
	}
	firstmodel = false;

	bool newUFSFound = false;
	while (!complete() || newUFSFound){
		newUFSFound = false;
		if (!unitPropagation(violatedNogood)){
			if (currentDL == 0){
				// no answer set
				return InterpretationConstPtr();
			}else{
				if (currentDL > exhaustedDL){
					DBGLOG(DBG, "Conflict analysis");
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
			Set<ID> ufs = getUnfoundedSet();

			if (ufs.size() > 0){
				DBGLOG(DBG, "Found UFS: " << toString(ufs));
#ifndef NDEBUG
				++cntDetectedUnfoundedSets;
#endif

				Nogood loopNogood = getLoopNogood(ufs);
				addNogood(loopNogood);
				newUFSFound = true;
			}else{
				// no ufs
				DBGLOG(DBG, "No unfounded set exists");

				DBGLOG(DBG, "Calling external learner");
				bool learned = false;
				BOOST_FOREACH (LearningCallback* cb, learner){
					DBGLOG(DBG, "Calling external learners with interpretation: " << *interpretation);
					learned |= cb->learn(interpretation, factWasSet, changed);
				}
				changed.clear();

				if (learned){
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
		}
	}


	InterpretationConstPtr icp(interpretation);
	return icp;
}

InterpretationPtr InternalGroundASPSolver::projectToOrdinaryAtoms(InterpretationConstPtr intr){

	if (intr == InterpretationConstPtr()){
		return InterpretationPtr();
	}else{
		InterpretationPtr answer = InterpretationPtr(new Interpretation(reg));
		answer->add(*intr);
		answer->bit_and(*ordinaryFactsInt);
		if (program.mask != InterpretationConstPtr()){
			answer->getStorage() -= program.mask->getStorage();
		}
//		InterpretationPtr answer = InterpretationPtr(new Interpretation(reg));
//		BOOST_FOREACH (IDAddress ordAt, ordinaryFacts){
//			if (intr->getFact(ordAt)) answer->setFact(ordAt);
//		}
		return answer;
	}
}

DLVHEX_NAMESPACE_END
