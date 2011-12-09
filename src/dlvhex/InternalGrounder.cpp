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
 * @file InternalGrounder.cpp
 * @author Christoph Redl
 *
 * @brief Grounder for disjunctive logic programs.
 */

#define DLVHEX_BENCHMARK

#include "dlvhex/Logger.hpp"
#include "dlvhex/Registry.hpp"
#include "dlvhex/Printer.hpp"
#include "dlvhex/ASPSolver.h"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/PluginInterface.h"
#include "dlvhex/Benchmarking.h"
#include "dlvhex/InternalGrounder.hpp"

#include <boost/foreach.hpp>
#include <boost/graph/strong_components.hpp>
#include <boost/graph/topological_sort.hpp>

DLVHEX_NAMESPACE_BEGIN

void InternalGrounder::computeDepGraph(){

	// go through all rules
	BOOST_FOREACH (ID ruleID, inputprogram.idb){
		const Rule& rule = reg->rules.getByID(ruleID);

		DBGLOG(DBG, "Analyzing rule " << ruleToString(ruleID));

		// all predicates are nodes
		BOOST_FOREACH (ID headLiteralID, rule.head){
			ID pred = getPredicateOfAtom(headLiteralID);
			if (depNodes.find(pred) == depNodes.end()){
				depNodes[pred] = boost::add_vertex(pred, depGraph);
			}
		}
		BOOST_FOREACH (ID bodyLiteralID, rule.body){
			ID pred = getPredicateOfAtom(bodyLiteralID);
			if (depNodes.find(pred) == depNodes.end()){
				depNodes[pred] = boost::add_vertex(pred, depGraph);
			}
		}

		// add an arc from all predicates in head literals to all predicates in body literals
		BOOST_FOREACH (ID headLiteralID, rule.head){
			BOOST_FOREACH (ID bodyLiteralID, rule.body){
				boost::add_edge(depNodes[getPredicateOfAtom(headLiteralID)], depNodes[getPredicateOfAtom(bodyLiteralID)], depGraph);
				DBGLOG(DBG, "Predicate " << getPredicateOfAtom(headLiteralID) << " depends on " << getPredicateOfAtom(bodyLiteralID));
			}
		}

		// head predicates cyclically depend on each other
		BOOST_FOREACH (ID headLiteralID1, rule.head){
			BOOST_FOREACH (ID headLiteralID2, rule.head){
				if (headLiteralID1 != headLiteralID2){
					boost::add_edge(depNodes[getPredicateOfAtom(headLiteralID1)], depNodes[getPredicateOfAtom(headLiteralID2)], depGraph);
					DBGLOG(DBG, "Predicate " << getPredicateOfAtom(headLiteralID1) << " depends on " << getPredicateOfAtom(headLiteralID2));
				}
			}
		}
	}
}

void InternalGrounder::computeStrata(){

	// find strongly connected components in the dependency graph using boost
	std::vector<int> componentMap(depNodes.size());
	int num = boost::strong_components(depGraph, &componentMap[0]);

	// translate into real map
	depSCC = std::vector<std::set<ID> >(num);
	int nodeNr = 0;
	BOOST_FOREACH (int componentOfNode, componentMap){
		depSCC[componentOfNode].insert(depGraph[nodeNr]);
		nodeNr++;
	}

#ifndef NDEBUG
	std::stringstream compStr;
	bool firstC = true;
	BOOST_FOREACH (std::set<ID> component, depSCC){
		if (!firstC) compStr << ", ";
		firstC = false;
		compStr << "{";
		bool firstL = true;
		BOOST_FOREACH (ID predID, component){
			if (!firstL) compStr << ", ";
			firstL = false;
			compStr << predID;
		}
		compStr << "}";
	}
	DBGLOG(DBG, "Predicate components: " << compStr.str());
#endif

	// create a graph modeling the dependencies between the predicate components
	// one node for each component
	for (unsigned int compNr = 0; compNr < depSCC.size(); ++compNr){
		boost::add_vertex(compNr, compDependencies);
	}
	// go through all edges of the dep graph
	std::pair<DepGraph::edge_iterator, DepGraph::edge_iterator> edgeIteratorRange = boost::edges(depGraph);
	for(DepGraph::edge_iterator edgeIterator = edgeIteratorRange.first; edgeIterator != edgeIteratorRange.second; ++edgeIterator){
		// connect the according strongly connected components
		int sourceIdx = componentMap[boost::source(*edgeIterator, depGraph)];
		int targetIdx = componentMap[boost::target(*edgeIterator, depGraph)];
		if (sourceIdx != targetIdx){
			DBGLOG(DBG, "Component " << sourceIdx << " depends on " << targetIdx);
			boost::add_edge(sourceIdx, targetIdx, compDependencies);
		}
	}

	// compute topological ordering of components
	std::vector<int> compOrdering;
	topological_sort(compDependencies, std::back_inserter(compOrdering));
	DBGLOG(DBG, "Processing components in the following ordering:");
	BOOST_FOREACH (int comp, compOrdering){
		DBGLOG(DBG, "Component " << comp << ", predicates are: ");
		std::set<ID> stratum;
		BOOST_FOREACH (ID pred, depSCC[comp]){
			DBGLOG(DBG, "" << pred);
			stratum.insert(pred);
			stratumOfPredicate[pred] = predicatesOfStratum.size();
		}
		predicatesOfStratum.push_back(stratum);
	}

	// arrange the rules accordingly
	rulesOfStratum = std::vector<std::set<ID> >(compOrdering.size());
	BOOST_FOREACH (ID ruleID, inputprogram.idb){
		rulesOfStratum[getStratumOfRule(ruleID)].insert(ruleID);
	}
}

void InternalGrounder::buildPredicateIndex(){

	positionsOfPredicate.clear();

	int ruleNr = 0;
	BOOST_FOREACH (ID nonGroundRuleID, nonGroundRules){
		const Rule& rule = reg->rules.getByID(nonGroundRuleID);
		DBGLOG(DBG, "Processing rule " << rule);
		int bodyAtomNr = 0;
		BOOST_FOREACH (ID bodyAtomID, rule.body){
			DBGLOG(DBG, "Atom " << bodyAtomID << " has predicate " << getPredicateOfAtom(bodyAtomID));
			positionsOfPredicate[getPredicateOfAtom(bodyAtomID)].insert(std::pair<int, int>(ruleNr, bodyAtomNr));
			bodyAtomNr++;
		}
		ruleNr++;
	}
}

void InternalGrounder::loadStratum(int index){

	DBGLOG(DBG, "Loading stratum " << index);
	nonGroundRules.clear();
	nonGroundRules.insert(nonGroundRules.begin(), rulesOfStratum[index].begin(), rulesOfStratum[index].end());
	buildPredicateIndex();
}

void InternalGrounder::groundStratum(int stratumNr){

	loadStratum(stratumNr);

	DBGLOG(DBG, "Grounding stratum " << stratumNr);
	std::set<ID> newDerivableAtoms;

	// all facts are immediately derivable and true
	if (stratumNr == 0){
		DBGLOG(DBG, "Deriving all facts");

		bm::bvector<>::enumerator en = inputprogram.edb->getStorage().first();
		bm::bvector<>::enumerator en_end = inputprogram.edb->getStorage().end();

		std::set<ID> newGroundRules;
		while (en < en_end){
			ID atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en);
			setToTrue(atom);
			addDerivableAtom(atom, groundRules, newDerivableAtoms);
			en++;
		}
	}

	// ground all rules
	DBGLOG(DBG, "Processing rules");
	for (int ruleIndex = 0; ruleIndex < nonGroundRules.size(); ++ruleIndex){
		Substitution s;
		groundRule(nonGroundRules[ruleIndex], s, groundRules, newDerivableAtoms);
	}

	// as long as there were new rules generated, add their heads to the list of derivable atoms
	DBGLOG(DBG, "Processing cyclically depending rules");
	while (newDerivableAtoms.size() > 0){

		// generate further rules for the new derivable atoms
		std::set<ID> newDerivableAtoms2;
		BOOST_FOREACH (ID atom, newDerivableAtoms){
			addDerivableAtom(atom, groundRules, newDerivableAtoms2);
		}
		newDerivableAtoms = newDerivableAtoms2;
	}
	DBGLOG(DBG, "Produced " << groundRules.size() << " ground rules");

	resolvedPredicates.insert(predicatesOfStratum[stratumNr].begin(), predicatesOfStratum[stratumNr].end());
	DBGLOG(DBG, "Set of resolved predicates is now: ");
	BOOST_FOREACH (ID pred, resolvedPredicates){
		DBGLOG(DBG, pred);
	}
}

void InternalGrounder::groundRule(ID ruleID, Substitution& s, std::vector<ID>& groundedRules, std::set<ID>& newDerivableAtoms){

	Substitution currentSubstitution = s;
	const Rule& rule = reg->rules.getByID(ruleID);

	DBGLOG(DBG, "Grounding rule " << ruleToString(ruleID));

	// compute binders of the variables in the rule
	Binder binders = getBinderOfRule(ruleID);

	if (rule.body.size() == 0){
		// grounding of choice rules
		buildGroundInstance(ruleID, currentSubstitution, groundedRules, newDerivableAtoms);
	}else{
		// start search at position 0 in the extension of all predicates
		std::vector<int> searchPos;
		for (int i = 0; i < rule.body.size(); ++i) searchPos.push_back(0);

		// go through all (positive) body atoms
		for (std::vector<ID>::const_iterator it = rule.body.begin(); it != rule.body.end(); ){

			int bodyLitIndex = it - rule.body.begin();
			ID bodyAtomID = *it;

			if (!bodyAtomID.isNaf()){

				// remove assignments to all variables which do not occur between rule.body.begin() and it - 1
				DBGLOG(DBG, "Undoing variable assignments before position " << bodyLitIndex);
				std::set<ID> keepVars;
				for (std::vector<ID>::const_iterator itVarCheck = rule.body.begin(); itVarCheck != it; ++itVarCheck){
					reg->getVariablesInID(*itVarCheck, keepVars);
				}
				Substitution newSubst = s;
				BOOST_FOREACH (ID var, keepVars){
					newSubst[var] = currentSubstitution[var];
				}
				currentSubstitution = newSubst;

				DBGLOG(DBG, "Finding next match at position " << bodyLitIndex << " in extension after index " << searchPos[bodyLitIndex]);
				searchPos[bodyLitIndex] = matchNextFromExtension(applySubstitutionToAtom(currentSubstitution, bodyAtomID), currentSubstitution, searchPos[bodyLitIndex]);
				DBGLOG(DBG, "Search result: " << searchPos[bodyLitIndex]);

				// match?
				if (searchPos[bodyLitIndex] == -1){

					// backtrack
					int btIndex = backtrack(ruleID, binders, bodyLitIndex);
					if (btIndex == -1){
						DBGLOG(DBG, "No more matches");
						return;
					}else{
						DBGLOG(DBG, "Backtracking to literal " << btIndex);
						it = rule.body.begin() + btIndex;
					}
					continue;
				}
			}

			// match
			// if we are at the end of the body list we have found a valid substitution
			if (it == rule.body.end() - 1){
				DBGLOG(DBG, "Substitution complete");
				buildGroundInstance(ruleID, currentSubstitution, groundedRules, newDerivableAtoms);

				// go back to last non-naf body literal
				while(bodyAtomID.isNaf()){
					if (it == rule.body.begin()) return;
					--it;
					bodyAtomID = *it;
				}
			}else{
				// go to next atom in rule body
				++it;
				searchPos[bodyLitIndex + 1] = 0;	// start from scratch
			}
		}
	}
}

void InternalGrounder::buildGroundInstance(ID ruleID, Substitution s, std::vector<ID>& groundedRules, std::set<ID>& newDerivableAtoms){

	const Rule& rule = reg->rules.getByID(ruleID);

	Tuple groundedHead, groundedBody;

	// ground head
	BOOST_FOREACH (ID headAtom, rule.head){
		ID groundHeadAtom = applySubstitutionToAtom(s, headAtom);
		groundedHead.push_back(groundHeadAtom);
		newDerivableAtoms.insert(groundHeadAtom);
	}

	// ground body
	bool optimization = false;
	do{	// if optimization eliminates ALL body literals and also the head is empty, we need another run without optimization as empty rules are illegal
		optimization = !optimization;	// first we try it with optimization, then without

		BOOST_FOREACH (ID bodyLitID, rule.body){
			ID groundBodyLiteralID = applySubstitutionToAtom(s, bodyLitID);
			const OrdinaryAtom& groundBodyLiteral = reg->ogatoms.getByID(groundBodyLiteralID);

			if (optimization){
				// h :- a, not b         where a is known to be true
				// optimization: skip satisfied literals
				if (!groundBodyLiteralID.isNaf() && trueAtoms->getFact(groundBodyLiteralID.address)){
					DBGLOG(DBG, "Skipping true " << groundBodyLiteralID);
					continue;
				}

				// h :- a, not b         where b is known to be not derivable
				// optimization for stratified negation: skip naf-body literals over known predicates which are not derivable
				if (groundBodyLiteralID.isNaf() && isPredicateResolved(groundBodyLiteral.front()) && !isAtomDerivable(groundBodyLiteralID)){
					DBGLOG(DBG, "Skipping underivable " << groundBodyLiteralID);
					continue;
				}

				// h :- a, not b         where b is known to be true
				// optimization: skip rules which contain a naf-literal which in known to be true
				if (groundBodyLiteralID.isNaf() && trueAtoms->getFact(groundBodyLiteralID.address)){
					DBGLOG(DBG, "Skipping rule " << ruleToString(ruleID) << " due to true " << groundBodyLiteralID);
					return;
				}

				// h :- a, not b         where a is known to be not derivable
				// optimization for stratified negation: skip naf-body literals over known predicates which are not derivable
				if (!groundBodyLiteralID.isNaf() && isPredicateResolved(groundBodyLiteral.front()) && !isAtomDerivable(groundBodyLiteralID)){
					DBGLOG(DBG, "Skipping rule " << ruleToString(ruleID) << " due to " << groundBodyLiteralID);
					return;
				}
			}

			groundedBody.push_back(groundBodyLiteralID);
		}

	}while(groundedHead.size() == 0 && groundedBody.size() == 0 && optimization == true);

	// determine type of rule
	IDKind kind = ID::MAINKIND_RULE;
	if (groundedHead.size() == 0) kind |= ID::SUBKIND_RULE_CONSTRAINT;
	if (groundedHead.size() > 0) kind |= ID::SUBKIND_RULE_REGULAR;
	if (groundedHead.size() > 1) kind |= ID::PROPERTY_RULE_DISJ;

	// build rule
	Rule groundedRule(kind, groundedHead, groundedBody);

	// new facts are set immediately
	if (groundedHead.size() == 1 && groundedBody.size() == 0){
		// derive new fact
		DBGLOG(DBG, "Adding fact " << groundedHead[0]);
		setToTrue(groundedHead[0]);
	}else{
		// avoid duplicate entries (they cause the registry to crash)
		ID id = reg->rules.getIDByElement(groundedRule);
		if (id == ID_FAIL){
				DBGLOG(DBG, "Adding ground rule");
				id = reg->rules.storeAndGetID(groundedRule);
				DBGLOG(DBG, "Added ground rule " << ruleToString(id));
		}else{
				DBGLOG(DBG, "Ground rule " << ruleToString(id) << " was already present");
		}
		groundedRules.push_back(id);
	}
}

bool InternalGrounder::match(ID atomID, ID patternAtomID, Substitution& s){

	const OrdinaryAtom& atom = atomID.isOrdinaryGroundAtom() ? reg->ogatoms.getByID(atomID) : reg->onatoms.getByID(atomID);
	const OrdinaryAtom& patternAtom = reg->ogatoms.getByID(patternAtomID);

	if (!atom.unifiesWith(patternAtom)) return false;

	// compute the unifying substitution
	for (unsigned int termIndex = 0; termIndex < atom.tuple.size(); ++termIndex){
		if (atom.tuple[termIndex].isVariableTerm()){
			s[atom.tuple[termIndex]] = patternAtom.tuple[termIndex];
		}
	}
	return true;
}

int InternalGrounder::matchNextFromExtension(ID atomID, Substitution& s, int startSearchIndex){

	const OrdinaryAtom& atom = atomID.isOrdinaryGroundAtom() ? reg->ogatoms.getByID(atomID) : reg->onatoms.getByID(atomID);
	std::vector<ID>& extension = derivableAtomsOfPredicate[atom.front()];

	for (std::vector<ID>::const_iterator it = extension.begin() + startSearchIndex; it != extension.end(); ++it){

		if (match(atomID, *it, s)){
			// yes
			// return next start search index
			return it - extension.begin() + 1;
		}
	}
	// no match
	return -1;
}

int InternalGrounder::backtrack(ID ruleID, Binder& binders, int currentIndex){

	// backtrack to the maximum binder of variables in the current literal, which is not the current literal itself
	const Rule& rule = reg->rules.getByID(ruleID);
	std::set<ID> currentVars;
	reg->getVariablesInID(rule.body[currentIndex], currentVars);

	int bt = -1;
	BOOST_FOREACH (ID var, currentVars){
		if (binders[var] > bt && binders[var] < currentIndex){
			bt = binders[var];
		}
	}

	// no binder: backtrack to last positive literal
	if (bt == -1){
		bt = currentIndex - 1;
		while (bt > -1 && rule.body[bt].isNaf()) bt--;
	}
	return bt;
}

void InternalGrounder::setToTrue(ID atom){

	DBGLOG(DBG, "Setting " << atom << " to true");
	trueAtoms->setFact(atom.address);
}

void InternalGrounder::addDerivableAtom(ID atomID, std::vector<ID>& groundRules, std::set<ID>& newDerivableAtoms){

	DBGLOG(DBG, "" << atomID << " becomes derivable");

	const OrdinaryAtom& ogatom = reg->ogatoms.getByID(atomID);
	if (isAtomDerivable(atomID)){
		// is already marked as derivable: nothing to do
		return;
	}else{
		derivableAtomsOfPredicate[ogatom.front()].push_back(atomID);
	}

	// go through all rules which contain this predicate positively in their body
	typedef std::pair<int, int> Pair;
	BOOST_FOREACH (Pair location, positionsOfPredicate[ogatom.front()]){
		// extract the atom from the body
		// if it unifies with atomID, ground the rule with this substitution being predetermined
		const Rule& rule = reg->rules.getByID(nonGroundRules[location.first]);
		if (!rule.body[location.second].isNaf()){
			DBGLOG(DBG, "Atom occurs in rule " << location.first << " at position " << location.second);
			Substitution s;
			match(rule.body[location.second], atomID, s);

			groundRule(nonGroundRules[location.first], s, groundRules, newDerivableAtoms);
		}
	}
}

ID InternalGrounder::applySubstitutionToAtom(Substitution s, ID atomID){

	if (atomID.isOrdinaryGroundAtom()) return atomID;

	// apply substitution to tuple of atom
	const OrdinaryAtom& onatom = reg->onatoms.getByID(atomID);
	Tuple t = onatom.tuple;
	bool isGround = true;
	for (unsigned int termIndex = 0; termIndex < t.size(); ++termIndex){
		if (s.find(t[termIndex]) != s.end()){
			t[termIndex] = s[t[termIndex]];
		}
		if (t[termIndex].isVariableTerm()) isGround = false;
	}

	// replace mainkind by ATOM and subkind according to groundness
	IDKind kind = atomID.kind;
	kind &= (ID::ALL_ONES ^ ID::MAINKIND_MASK);
	kind &= (ID::ALL_ONES ^ ID::SUBKIND_MASK);
	kind |= ID::MAINKIND_ATOM;
	if (isGround){
		kind |= ID::SUBKIND_ATOM_ORDINARYG;
	}else{
		kind |= ID::SUBKIND_ATOM_ORDINARYN;
	}
	OrdinaryAtom atom(kind);
	atom.tuple = t;
	ID id;
	if (isGround){
		id = reg->storeOrdinaryGAtom(atom);
	}else{
		id = reg->storeOrdinaryNAtom(atom);
	}

	// output: use kind of input, except for the subtype, which is according to groundness
	kind &= (ID::ALL_ONES ^ ID::MAINKIND_MASK);
	kind |= (atomID.kind & ID::MAINKIND_MASK);
	return ID(kind, id.address);
}

std::string InternalGrounder::ruleToString(ID ruleID){
	std::stringstream ss;
	RawPrinter p(ss, reg);
	p.print(ruleID);
	return ss.str();
}

ID InternalGrounder::getPredicateOfAtom(ID atomID){

	const OrdinaryAtom& atom = (atomID.isOrdinaryGroundAtom() ? reg->ogatoms.getByID(atomID) : reg->onatoms.getByID(atomID));
	return atom.front();
}

bool InternalGrounder::isGroundRule(ID ruleID){

	const Rule& rule = reg->rules.getByID(ruleID);
	BOOST_FOREACH (ID bodyAtom, rule.body){
		if (!bodyAtom.isOrdinaryGroundAtom()) return false;
	}
	return true;
}

bool InternalGrounder::isPredicateResolved(ID pred){

	return (std::find(resolvedPredicates.begin(), resolvedPredicates.end(), pred) != resolvedPredicates.end());
}

bool InternalGrounder::isAtomDerivable(ID atom){

	ID pred = getPredicateOfAtom(atom);
	BOOST_FOREACH (ID derv, derivableAtomsOfPredicate[pred]){
		if (atom.address == derv.address) return true;
	}
	return false;
}

int InternalGrounder::getStratumOfRule(ID ruleID){

	const Rule& rule = reg->rules.getByID(ruleID);

	// constraints are grounded at highest level
	if (rule.head.size() == 0){
		DBGLOG(DBG, "Stratum of constraint " << ruleToString(ruleID) << " is " << (predicatesOfStratum.size() - 1));
		return predicatesOfStratum.size() - 1;
	}

	// ordinary rules: compute the highest stratum of all head atoms
	int stratum = 0;
	BOOST_FOREACH (ID headLit, rule.head){
		const OrdinaryAtom& atom = (headLit.isOrdinaryGroundAtom() ? reg->ogatoms.getByID(headLit) : reg->onatoms.getByID(headLit));
		int s = stratumOfPredicate[atom.front()];
		stratum = (s > stratum ? s : stratum);
	}
	DBGLOG(DBG, "Stratum of rule " << ruleToString(ruleID) << " is " << stratum);
	return stratum;
}

InternalGrounder::Binder InternalGrounder::getBinderOfRule(ID ruleID){

	Binder binders;
	const Rule& rule = reg->rules.getByID(ruleID);
	int litIdx = 0;
	BOOST_FOREACH (ID lit, rule.body){
		std::set<ID> varsInLit;
		reg->getVariablesInID(lit, varsInLit);
		BOOST_FOREACH (ID var, varsInLit){
			if (binders.find(var) == binders.end()){
				DBGLOG(DBG, "Binder of variable " << var << " is literal " << litIdx);
				binders[var] = litIdx;
			}
		}
		++litIdx;
	}
	return binders;
}

InternalGrounder::InternalGrounder(ProgramCtx& ctx, ASPProgram& p) : inputprogram(p){

	DBGLOG(DBG, "Starting grounding");

	reg = ctx.registry();

	trueAtoms = InterpretationPtr(new Interpretation(reg));
	falseAtoms = InterpretationPtr(new Interpretation(reg));

	computeDepGraph();
	computeStrata();

	// now ground stratum by stratum
	for (unsigned int stratumNr = 0; stratumNr < predicatesOfStratum.size(); ++stratumNr){
		groundStratum(stratumNr);
	}

#ifndef NDEBUG
	ASPProgram gp = getGroundProgram();

	std::stringstream ss;
	ss << *trueAtoms << " (" << trueAtoms->getStorage().count() << ")";
	ss << std::endl;
	BOOST_FOREACH (ID ruleID, groundRules){
		ss << ruleToString(ruleID) << std::endl;
	}
	DBGLOG(DBG, "Grounded program: " << std::endl << ss.str());
#endif
}

ASPProgram InternalGrounder::getGroundProgram(){

	ASPProgram gp(reg, groundRules, trueAtoms, inputprogram.maxint, inputprogram.mask);
	return gp;
}

DLVHEX_NAMESPACE_END

