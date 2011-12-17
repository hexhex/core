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
//#define OPTIMIZED
	Substitution currentSubstitution = s;
	const Rule& rule = reg->rules.getByID(ruleID);

	DBGLOG(DBG, "Grounding rule " << ruleToString(ruleID));

	// compute binders of the variables in the rule
	Binder binders = getBinderOfRule(ruleID);
	std::set<ID> outputVars = getOutputVariables(ruleID);
	std::vector<std::set<ID> > depVars;
	for (int i = 0; i < rule.body.size(); ++i){
		depVars.push_back(getDepVars(ruleID, i));
	}

	int csb = -1;
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
				int startSearchPos = searchPos[bodyLitIndex];
				searchPos[bodyLitIndex] = matchNextFromExtension(applySubstitutionToAtom(currentSubstitution, bodyAtomID), currentSubstitution, searchPos[bodyLitIndex]);
				DBGLOG(DBG, "Search result: " << searchPos[bodyLitIndex]);

				// match?
				if (searchPos[bodyLitIndex] == -1){

#ifdef OPTIMIZED
					int btIndex = -1;
					if (startSearchPos == 0){
						// failure on first match
						DBGLOG(DBG, "Failure on first match at position " << bodyLitIndex);
						std::set<ID> vars;
						reg->getVariablesInID(*it, vars);
						btIndex = getClosestBinder(ruleID, bodyLitIndex, vars);
						if (depends(ruleID, btIndex, csb)) csb = btIndex;
					}else{
						// failure on next match
						DBGLOG(DBG, "Failure on next match at position " << bodyLitIndex);

						if (bodyLitIndex == csb){
							csb = getClosestBinder(ruleID, bodyLitIndex, outputVars);
						}

						btIndex = getClosestBinder(ruleID, bodyLitIndex, depVars[bodyLitIndex]);
DBGLOG(DBG, "Closest binder wrt " << bodyLitIndex << ": " << btIndex << ", csb: " << csb);

						if (depends(ruleID, btIndex, csb)){
//						if (btIndex < csb){
							btIndex = csb;
						}

//						if (btIndex != -1 && csb > btIndex) btIndex = csb;
					}
					if (btIndex == -1){
						DBGLOG(DBG, "No more matches");
						return;
					}else{
						DBGLOG(DBG, "Backtracking to literal " << btIndex);
						it = rule.body.begin() + btIndex;
					}
#else



					// backtrack
					int btIndex = backtrack(ruleID, binders, bodyLitIndex);
					if (btIndex == -1){
						DBGLOG(DBG, "No more matches");
						return;
					}else{
						DBGLOG(DBG, "Backtracking to literal " << btIndex);
						it = rule.body.begin() + btIndex;
					}
#endif

					continue;
				}
			}

			// match
			// if we are at the end of the body list we have found a valid substitution
			if (it == rule.body.end() - 1){
				DBGLOG(DBG, "Substitution complete");
				buildGroundInstance(ruleID, currentSubstitution, groundedRules, newDerivableAtoms);
#ifdef OPTIMIZED
				int btIndex = getClosestBinder(ruleID, bodyLitIndex + 1, outputVars);
				if (btIndex == -1){
					DBGLOG(DBG, "No more matches after solution found");
					return;
				}else{
					DBGLOG(DBG, "Backtracking to literal " << btIndex << " after solution found");
					csb = btIndex;
					it = rule.body.begin() + btIndex;
				}
#else

				// go back to last non-naf body literal
				while(bodyAtomID.isNaf()){
					if (it == rule.body.begin()) return;
					--it;
					bodyAtomID = *it;
				}
#endif
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

			if (groundBodyLiteralID.isBuiltinAtom() && optimization){
				// at this point, built-in atoms are always true, otherwise the grounding terminates even earlier
				continue;
			}

			if (groundBodyLiteralID.isOrdinaryAtom() && optimization){
				const OrdinaryAtom& groundBodyLiteral = reg->ogatoms.getByID(groundBodyLiteralID);

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

	if (atomID.isOrdinaryAtom()){
		return matchOrdinary(atomID, patternAtomID, s);
	}else if(atomID.isBuiltinAtom()){
		return matchBuiltin(atomID, patternAtomID, s);
	}

	assert(false);
}

bool InternalGrounder::matchOrdinary(ID atomID, ID patternAtomID, Substitution& s){

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

bool InternalGrounder::matchBuiltin(ID atomID, ID patternAtomID, Substitution& s){

	const BuiltinAtom& atom = reg->batoms.getByID(atomID);
	const BuiltinAtom& patternAtom = reg->batoms.getByID(patternAtomID);

	// compute the unifying substitution
	Substitution s2 = s;
	for (unsigned int termIndex = 0; termIndex < atom.tuple.size(); ++termIndex){
		if (atom.tuple[termIndex].isVariableTerm()){
			if (s.find(atom.tuple[termIndex]) != s.end() && s[atom.tuple[termIndex]] != patternAtom.tuple[termIndex]) return false;
			s[atom.tuple[termIndex]] = patternAtom.tuple[termIndex];
		}else{
			if (atom.tuple[termIndex] != patternAtom.tuple[termIndex]) return false;
		}
	}
	s = s2;
	return true;
}

int InternalGrounder::matchNextFromExtension(ID atomID, Substitution& s, int startSearchIndex){

	if (atomID.isOrdinaryAtom()){
		return matchNextFromExtensionOrdinary(atomID, s, startSearchIndex);
	}else if (atomID.isBuiltinAtom()){
		return matchNextFromExtensionBuiltin(atomID, s, startSearchIndex);
	}

	assert(false);
}

int InternalGrounder::matchNextFromExtensionOrdinary(ID atomID, Substitution& s, int startSearchIndex){

	DBGLOG(DBG, "Macthing ordinary atom");
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

int InternalGrounder::matchNextFromExtensionBuiltin(ID atomID, Substitution& s, int startSearchIndex){

	DBGLOG(DBG, "Matching builtin atom");
	const BuiltinAtom& atom = reg->batoms.getByID(atomID);

	switch (atom.tuple[0].address){
		case ID::TERM_BUILTIN_INT:
			return matchNextFromExtensionBuiltinUnary(atomID, s, startSearchIndex);

		case ID::TERM_BUILTIN_EQ:
		case ID::TERM_BUILTIN_NE:
		case ID::TERM_BUILTIN_LT:
		case ID::TERM_BUILTIN_LE:
		case ID::TERM_BUILTIN_GT:
		case ID::TERM_BUILTIN_GE:
			return matchNextFromExtensionBuiltinBinary(atomID, s, startSearchIndex);

		case ID::TERM_BUILTIN_ADD:
		case ID::TERM_BUILTIN_MUL:
		case ID::TERM_BUILTIN_SUB:
		case ID::TERM_BUILTIN_DIV:
		case ID::TERM_BUILTIN_MOD:
			return matchNextFromExtensionBuiltinTrinary(atomID, s, startSearchIndex);
	}
	assert(false);
}

int InternalGrounder::matchNextFromExtensionBuiltinUnary(ID atomID, Substitution& s, int startSearchIndex){

	const BuiltinAtom& atom = reg->batoms.getByID(atomID);
	switch (atom.tuple[0].address){
		case ID::TERM_BUILTIN_INT:
			if (startSearchIndex > ctx.maxint /* max int */){
				return -1;
			}else{
				if (atom.tuple[1].isVariableTerm()){
					s[atom.tuple[1]] = ID::termFromInteger(startSearchIndex);
					return startSearchIndex + 1;
				}else if (atom.tuple[1].isConstantTerm()){
					return -1;
				}else{
					assert(atom.tuple[1].isIntegerTerm());

					if (startSearchIndex <= atom.tuple[1].address){
						return atom.tuple[1].address + 1;
					}
				}
			}
	}
	assert(false);
}

int InternalGrounder::matchNextFromExtensionBuiltinBinary(ID atomID, Substitution& s, int startSearchIndex){

	const BuiltinAtom& atom = reg->batoms.getByID(atomID);

	if (startSearchIndex > 0) return -1;

	bool cmp;
	switch (atom.tuple[0].address){
		case ID::TERM_BUILTIN_EQ: cmp = (atom.tuple[1].address == atom.tuple[2].address); break;
		case ID::TERM_BUILTIN_NE: cmp = (atom.tuple[1].address != atom.tuple[2].address); break;
		case ID::TERM_BUILTIN_LT: cmp = (atom.tuple[1].address < atom.tuple[2].address); break;
		case ID::TERM_BUILTIN_LE: cmp = (atom.tuple[1].address <= atom.tuple[2].address); break;
		case ID::TERM_BUILTIN_GT: cmp = (atom.tuple[1].address > atom.tuple[2].address); break;
		case ID::TERM_BUILTIN_GE: cmp = (atom.tuple[1].address >= atom.tuple[2].address); break;
	}
	if (cmp) return 1;
	else return -1;
}

int InternalGrounder::matchNextFromExtensionBuiltinTrinary(ID atomID, Substitution& s, int startSearchIndex){

	if (startSearchIndex > (ctx.maxint + 1) * (ctx.maxint + 1)){
		return -1;
	}else{
		const BuiltinAtom& atom = reg->batoms.getByID(atomID);

		int x = startSearchIndex / (ctx.maxint + 1);
		int y = startSearchIndex % (ctx.maxint + 1);

		if (atom.tuple[1].isConstantTerm() || atom.tuple[2].isConstantTerm() || atom.tuple[3].isConstantTerm()) return -1;

/*
		if (atom.tuple[1].isVariableTerm() && atom.tuple[2].isVariableTerm()){
			if (atom.tuple[3].isVariableTerm()){
				s[atom.tuple[1]] = ID::termFromInteger(x);
				s[atom.tuple[2]] = ID::termFromInteger(y);
				s[atom.tuple[3]] = ID::termFromInteger(x + y);
				y++;
				if (y > 100){
					y = 0;
					x++;
				}
				return x * 101 + y + 1;
			}else{
				while ((x + y != atom.tuple[3].address) && (x <= 100 && y <= 100)){
					y++;
					if (y > 100){
						y = 0;
						x++;
					}
				}
				if (x <= 100 && y <= 100){
					s[atom.tuple[1]] = ID::termFromInteger(x);
					s[atom.tuple[2]] = ID::termFromInteger(y);
					s[atom.tuple[3]] = ID::termFromInteger(x + y);
					return x * 101 + y + 1;
				}else{
					return -1;
				}
			}
		}
*/

		if (atom.tuple[1].isIntegerTerm() && atom.tuple[2].isIntegerTerm()){
			if (x <= atom.tuple[1].address && y < atom.tuple[2].address){
				x = atom.tuple[1].address;
				y = atom.tuple[2].address;
				int z = applyIntFunction(x_op_y_eq_ret, atom.tuple[0], x, y);
				if (atom.tuple[3].isIntegerTerm()){
					if (atom.tuple[3].address != z) return -1;
					else return x * (ctx.maxint + 1) + y + 1;
				}else{
					s[atom.tuple[3]] = ID::termFromInteger(z);
					return x * (ctx.maxint + 1) + y + 1;
				}
			}else{
				return -1;
			}
		}
	}
}

int InternalGrounder::backtrack(ID ruleID, Binder& binders, int currentIndex){

	// TODO: Jumping to the maximum binder of variables in the current literal does not work!
	// counter example:

/*
kw(subm6,"#WSMO").
pc("#mkif").

dloverlapsWith("#WSMO","#OWL-S").
dloverlapsWith("#WSMO","#SWSF").

dlisAuthorOf("#mkif","#pa14").
dlkeyword("#pa14","#SWSF").

cand(X,P) :- kw(P,K), pc(X), dloverlapsWith(K,K1), dlisAuthorOf(X,P1), dlkeyword(P1,K1).
*/

	// for K1="#OWL-S", X="#mkif", P1="#pa14", the final match of dlkeyword(P1,K1) fails
	// the algorithm backtracks to dlisAuthorOf(X,P1) and checks for another match, which fails as well
	// it then jumps back to the maximum binder of X and P1, which is pc(X)
	// note that dloverlapsWith(K,K1) is skipped!!
	// however, another match (dloverlapsWith("#WSMO","#SWSF")) would lead to a complete match!

	// output:
/*
 1 Undoing variable assignments before position 4
 1 Finding next match at position 4 in extension after index 0
 1 stored oatom OrdinaryAtom(0,'dlkeyword("#pa14","#OWL-S")',[ID(0x10000000,  10 term constant),ID(0x10000000,   9 term constant),ID(0x10000000,   6 term constant)]) which got ID(0x00000000,   6 atom ordinary_ground)
 1 Macthing ordinary atom
 1 unifiesWith ENTRY
 1 unifiesWith starting with result1 tuple [ID(0x10000000,  10 term constant),ID(0x10000000,   9 term constant),ID(0x10000000,   6 term constant)]
 1 unifiesWith starting with result2 tuple [ID(0x10000000,  10 term constant),ID(0x10000000,   9 term constant),ID(0x10000000,   7 term constant)]
 1 unifiesWith at position 0: checking ID(0x10000000,  10 term constant) vs ID(0x10000000,  10 term constant)
 1 unifiesWith at position 1: checking ID(0x10000000,   9 term constant) vs ID(0x10000000,   9 term constant)
 1 unifiesWith at position 2: checking ID(0x10000000,   6 term constant) vs ID(0x10000000,   7 term constant)
 1 unifiesWith EXIT
 1 Search result: -1
 1 Backtracking to literal 3
 1 Undoing variable assignments before position 3
 1 Finding next match at position 3 in extension after index 1
 1 Macthing ordinary atom
 1 Search result: -1
 1 Backtracking to literal 1
 1 Undoing variable assignments before position 1
 1 Finding next match at position 1 in extension after index 1
 1 Macthing ordinary atom
 1 Search result: -1
 1 Backtracking to literal 0
 1 Undoing variable assignments before position 0
 1 Finding next match at position 0 in extension after index 1
 1 Macthing ordinary atom
 1 Search result: -1
 1 No more matches
 1 Processing cyclically depending rules
 1 Produced 0 ground rules
*/



	// backtrack to the maximum binder of variables in the current literal, which is not the current literal itself
	const Rule& rule = reg->rules.getByID(ruleID);
//	std::set<ID> currentVars;
//	reg->getVariablesInID(rule.body[currentIndex], currentVars);

	int bt = -1;
//	BOOST_FOREACH (ID var, currentVars){
//		if (binders[var] > bt && binders[var] < currentIndex){
//			bt = binders[var];
//		}
//	}

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

	if (atomID.isOrdinaryAtom()){
		return applySubstitutionToOrdinaryAtom(s, atomID);
	}

	if (atomID.isBuiltinAtom()){
		return applySubstitutionToBuiltinAtom(s, atomID);
	}
}

ID InternalGrounder::applySubstitutionToOrdinaryAtom(Substitution s, ID atomID){

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

ID InternalGrounder::applySubstitutionToBuiltinAtom(Substitution s, ID atomID){

	const BuiltinAtom& batom = reg->batoms.getByID(atomID);
	Tuple t = batom.tuple;
	bool isGround = true;
	for (unsigned int termIndex = 0; termIndex < t.size(); ++termIndex){
		if (s.find(t[termIndex]) != s.end()){
			t[termIndex] = s[t[termIndex]];
		}
		if (t[termIndex].isVariableTerm()) isGround = false;
	}

	BuiltinAtom sbatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_BUILTIN, t);
	// TODO: We have to check if sbatom is already present, otherwise the registry crashes!
	return reg->batoms.storeAndGetID(sbatom);
}

std::string InternalGrounder::ruleToString(ID ruleID){
	std::stringstream ss;
	RawPrinter p(ss, reg);
	p.print(ruleID);
	return ss.str();
}

ID InternalGrounder::getPredicateOfAtom(ID atomID){

	if (atomID.isOrdinaryAtom()){
		const OrdinaryAtom& atom = (atomID.isOrdinaryGroundAtom() ? reg->ogatoms.getByID(atomID) : reg->onatoms.getByID(atomID));
		return atom.front();
	}
	if (atomID.isBuiltinAtom()){
		const BuiltinAtom& atom = reg->batoms.getByID(atomID);
		return atom.front();
	}

	return ID_FAIL;
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
	int stratum = 0;

	// constraints are grounded at highest level
//	if (rule.head.size() == 0){

//		stratum = predicatesOfStratum.size() - 1;

		BOOST_FOREACH (ID lit, rule.body){
			int s;
			if (lit.isOrdinaryAtom()){
				const OrdinaryAtom& atom = (lit.isOrdinaryGroundAtom() ? reg->ogatoms.getByID(lit) : reg->onatoms.getByID(lit));
				s = stratumOfPredicate[atom.front()];
			}else{
				s = 0;
			}
			stratum = (s > stratum ? s : stratum);
		}

//		DBGLOG(DBG, "Stratum of constraint " << ruleToString(ruleID) << " is " << stratum);//(predicatesOfStratum.size() - 1));
//		return stratum;





//		return predicatesOfStratum.size() - 1;
//	}

	// ordinary rules: compute the highest stratum of all head atoms

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

int InternalGrounder::getClosestBinder(ID ruleID, int litIndex, std::set<ID> variables){

	const Rule& rule = reg->rules.getByID(ruleID);
	int cb = -1;
	for (int i = 0; i < litIndex; ++i){
		// is this literal a binder of some variable?
		if (!rule.body[i].isNaf()){
			std::set<ID> varsInLit;
			reg->getVariablesInID(rule.body[i], varsInLit);
			BOOST_FOREACH(ID v, varsInLit){
				if (variables.count(v) > 0){
					cb = i;
					break;
				}
			}
		}
	}
	return cb;
}

std::set<ID> InternalGrounder::getDepVars(ID ruleID, int litIndex){

	const Rule& rule = reg->rules.getByID(ruleID);
	std::set<ID> vars;
	reg->getVariablesInID(rule.body[litIndex], vars);
	for (int i = litIndex + 1; i < rule.body.size(); ++i){
		std::set<ID> v2;
		reg->getVariablesInID(rule.body[i], v2);
		// check if v2 contains some variable in vars
		bool depending = false;
		BOOST_FOREACH (ID v, v2){
			if (vars.count(v) > 0){
				depending = true;
				break;
			}
		}
		if (depending){
			reg->getVariablesInID(rule.body[i], vars);
		}
	}
	return vars;
}

std::set<ID> InternalGrounder::getOutputVariables(ID ruleID){

	// compute output variables of this rule
	// this is the set of all variables which occur in literals over non-resolved predicates
	const Rule& rule = reg->rules.getByID(ruleID);
	std::set<ID> outputVars;
	BOOST_FOREACH (ID headLitIndex, rule.head){
		if (!isPredicateResolved(getPredicateOfAtom(headLitIndex))){
			reg->getVariablesInID(headLitIndex, outputVars);	
		}
	}
	BOOST_FOREACH (ID bodyLitIndex, rule.body){
		if (!isPredicateResolved(getPredicateOfAtom(bodyLitIndex))){
			reg->getVariablesInID(bodyLitIndex, outputVars);	
		}
	}
	return outputVars;
}

bool InternalGrounder::depends(ID ruleID, int lit1, int lit2){

	if (lit1 == -1 || lit2 == -1) return false;
	if (lit1 > lit2) return false;

	const Rule& rule = reg->rules.getByID(ruleID);
	std::set<ID> depVars = getDepVars(ruleID, lit1);
	std::set<ID> vars2;
	reg->getVariablesInID(rule.body[lit2], vars2);
	BOOST_FOREACH (ID v2, vars2){
		if (depVars.count(v2) > 0) return true;
	}
	return false;
}

int InternalGrounder::applyIntFunction(AppDir ad, ID op, int x, int y){

	switch(ad){
		case x_op_y_eq_ret:
			switch (op.address){
				case ID::TERM_BUILTIN_ADD: return x + y;
				case ID::TERM_BUILTIN_MUL: DBGLOG(DBG, "!!!"); return x * y;
				case ID::TERM_BUILTIN_SUB: return x - y;
				case ID::TERM_BUILTIN_DIV: return x / y;
				case ID::TERM_BUILTIN_MOD: return x % y;
			}
			break;
		case x_op_ret_eq_y:
			switch (op.address){
				case ID::TERM_BUILTIN_ADD: return y - x;
				case ID::TERM_BUILTIN_MUL: if (y % x == 0) return y / x;
				case ID::TERM_BUILTIN_SUB: return x - y;
				case ID::TERM_BUILTIN_DIV: if (x % y == 0) return x / y;
			}
			break;
		case ret_op_y_eq_x:
			switch (op.address){
				case ID::TERM_BUILTIN_ADD: return x - y;
				case ID::TERM_BUILTIN_MUL: if (x % y == 0) return x / y;
				case ID::TERM_BUILTIN_SUB: return x + y;
				case ID::TERM_BUILTIN_DIV: return x * y;
			}
			break;
	}
	return -1;
}

InternalGrounder::InternalGrounder(ProgramCtx& c, ASPProgram& p) : inputprogram(p), ctx(c){

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

