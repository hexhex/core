/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schüller
 * Copyright (C) 2011-2015 Christoph Redl
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dlvhex2/Logger.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/ASPSolver.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/InternalGrounder.h"

#include <boost/foreach.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/graph/strong_components.hpp>
#include <boost/graph/topological_sort.hpp>

DLVHEX_NAMESPACE_BEGIN

void InternalGrounder::computeDepGraph()
{

    // add edb
    bm::bvector<>::enumerator en = inputprogram.edb->getStorage().first();
    bm::bvector<>::enumerator en_end = inputprogram.edb->getStorage().end();

    while (en < en_end) {
        ID pred = getPredicateOfAtom(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
        if (depNodes.find(pred) == depNodes.end()) {
            depNodes[pred] = boost::add_vertex(pred, depGraph);
        }
        en++;
    }

    // go through all rules
    BOOST_FOREACH (ID ruleID, inputprogram.idb) {
        const Rule& rule = reg->rules.getByID(ruleID);

        DBGLOG(DBG, "Analyzing rule " << ruleToString(ruleID));

        // all predicates are nodes
        BOOST_FOREACH (ID headLiteralID, rule.head) {
            ID pred = getPredicateOfAtom(headLiteralID);
            if (depNodes.find(pred) == depNodes.end()) {
                depNodes[pred] = boost::add_vertex(pred, depGraph);
            }
        }
        BOOST_FOREACH (ID bodyLiteralID, rule.body) {
            ID pred = getPredicateOfAtom(bodyLiteralID);
            if (depNodes.find(pred) == depNodes.end()) {
                depNodes[pred] = boost::add_vertex(pred, depGraph);
            }
        }

        // add an arc from all predicates in head literals to all predicates in body literals
        BOOST_FOREACH (ID headLiteralID, rule.head) {
            BOOST_FOREACH (ID bodyLiteralID, rule.body) {
                boost::add_edge(depNodes[getPredicateOfAtom(headLiteralID)], depNodes[getPredicateOfAtom(bodyLiteralID)], depGraph);
                DBGLOG(DBG, "Predicate " << getPredicateOfAtom(headLiteralID) << " depends on " << getPredicateOfAtom(bodyLiteralID));
            }
        }

        // head predicates cyclically depend on each other
        BOOST_FOREACH (ID headLiteralID1, rule.head) {
            BOOST_FOREACH (ID headLiteralID2, rule.head) {
                if (headLiteralID1 != headLiteralID2) {
                    boost::add_edge(depNodes[getPredicateOfAtom(headLiteralID1)], depNodes[getPredicateOfAtom(headLiteralID2)], depGraph);
                    DBGLOG(DBG, "Predicate " << getPredicateOfAtom(headLiteralID1) << " depends on " << getPredicateOfAtom(headLiteralID2));
                }
            }
        }
    }
}


ID InternalGrounder::preprocessRule(ID ruleID)
{

    DBGLOG(DBG, "Preprocessing rule " << printToString<RawPrinter>(ruleID, reg));
    const Rule& rule = reg->rules.getByID(ruleID);
    Rule newrule = rule;
    newrule.body.clear();

    if (ruleID.hasRuleHeadGuard()) {
        throw GeneralError("Internal grounder cannot handle guards in rule heads");
    }

    // find length of the longest variable name
    std::set<ID> vars;
    BOOST_FOREACH (ID a, rule.body) {
        reg->getVariablesInID(a, vars, true);
    }
    int vlen = 0;
    std::stringstream prefix;
    BOOST_FOREACH (ID v, vars) {
        int s = reg->terms.getByID(v).symbol.size();
        vlen = s > vlen ? s : vlen;
    }
    prefix << "Anonymous";
    for (int i = 0; i < vlen - 8; i++) prefix << "_";

    // check for nested terms in rule head
    BOOST_FOREACH (ID a, rule.head) {
        OrdinaryAtom oa = reg->lookupOrdinaryAtom(a);
        for (uint32_t i = 1; i < oa.tuple.size(); ++i) {
            if (oa.tuple[i].isNestedTerm()) {
                throw GeneralError("Internal grounder cannot handle function symbols");
            }
        }
    }
    BOOST_FOREACH (ID a, rule.body) {
        if (a.isOrdinaryAtom()) {
            OrdinaryAtom oa = reg->lookupOrdinaryAtom(a);
            for (uint32_t i = 1; i < oa.tuple.size(); ++i) {
                if (oa.tuple[i].isNestedTerm()) {
                    throw GeneralError("Internal grounder cannot handle function symbols");
                }
            }
        }
        else if (a.isExternalAtom()) {
            ExternalAtom ea = reg->eatoms.getByID(a);
            for (uint32_t i = 0; i < ea.inputs.size(); ++i) {
                if (ea.inputs[i].isNestedTerm()) {
                    throw GeneralError("Internal grounder cannot handle function symbols");
                }
            }
            for (uint32_t i = 0; i < ea.tuple.size(); ++i) {
                if (ea.tuple[i].isNestedTerm()) {
                    throw GeneralError("Internal grounder cannot handle function symbols");
                }
            }
        }
        else if (a.isNaf() && a.isBuiltinAtom()) {
            throw GeneralError("Internal grounder cannot default negated builtins");
        }
    }

    // replace anonymous variables by unique variable symbols and check for nested terms
    int varIndex = 1;
    BOOST_FOREACH (ID a, rule.body) {
        // check if the literals contains an anonamous variable
        vars.clear();
        reg->getVariablesInID(a, vars, true);
        bool av = false;
        BOOST_FOREACH (ID v, vars) {
            if (v.isAnonymousVariable()) {
                av = true;
                break;
            }
        }
        if (av) {
            // replace all anonamous variables by ordinary ones ("_N" with N>=0)
            if (a.isOrdinaryAtom()) {
                OrdinaryAtom oa = reg->lookupOrdinaryAtom(a);
                for (uint32_t i = 1; i < oa.tuple.size(); ++i) {
                    if (oa.tuple[i].isAnonymousVariable()) {
                        std::stringstream newVar;
                        newVar << prefix.str() << varIndex++;
                        oa.tuple[i] = reg->storeVariableTerm(newVar.str());
                    }
                }
                newrule.body.push_back(ID::literalFromAtom(reg->storeOrdinaryAtom(oa), a.isNaf()));
            }
            else if (a.isExternalAtom()) {
                ExternalAtom ea = reg->eatoms.getByID(a);
                for (uint32_t i = 0; i < ea.inputs.size(); ++i) {
                    if (ea.inputs[i].isAnonymousVariable()) {
                        std::stringstream newVar;
                        newVar << prefix.str() << varIndex++;
                        ea.inputs[i] = reg->storeVariableTerm(newVar.str());
                    }
                }
                for (uint32_t i = 0; i < ea.tuple.size(); ++i) {
                    if (ea.tuple[i].isAnonymousVariable()) {
                        std::stringstream newVar;
                        newVar << prefix.str() << varIndex++;
                        ea.tuple[i] = reg->storeVariableTerm(newVar.str());
                    }
                }
                newrule.body.push_back(ID::literalFromAtom(reg->eatoms.storeAndGetID(ea), a.isNaf()));
            }
            else {
                assert(false && "Unknown literal type");
            }
        }
        else {
            newrule.body.push_back(a);
        }
    }

    ID newRuleID = reg->storeRule(newrule);
    DBGLOG(DBG, "Preprocessed rule " << printToString<RawPrinter>(newRuleID, reg));
    return newRuleID;
}


void InternalGrounder::computeStrata()
{

    // find strongly connected components in the dependency graph using boost
    std::vector<int> componentMap(depNodes.size());
    int num = boost::strong_components(depGraph, boost::make_iterator_property_map(componentMap.begin(), get(boost::vertex_index, depGraph)));

    // translate into real map
    depSCC = std::vector<Set<ID> >(num);
    Node nodeNr = 0;
    BOOST_FOREACH (int componentOfNode, componentMap) {
        depSCC[componentOfNode].insert(depGraph[nodeNr]);
        nodeNr++;
    }

    #ifndef NDEBUG
    std::stringstream compStr;
    bool firstC = true;
    BOOST_FOREACH (Set<ID> component, depSCC) {
        if (!firstC) compStr << ", ";
        firstC = false;
        compStr << "{";
        bool firstL = true;
        BOOST_FOREACH (ID predID, component) {
            if (!firstL) compStr << ", ";
            firstL = false;
            compStr << predID;
        }
        compStr << "}";
    }
    DBGLOG(DBG, "Predicate components: " << compStr.str());
    #endif

    // create a graph modeling the dependencies between the strongly-connected predicate components
    // one node for each component
    std::map<int, SCCDepGraph::vertex_descriptor> compToVertex;
    std::map<SCCDepGraph::vertex_descriptor, int> vertexToComp;
    for (uint32_t compNr = 0; compNr < depSCC.size(); ++compNr) {
        compToVertex[compNr] = boost::add_vertex(compNr, compDependencies);
        vertexToComp[compToVertex[compNr]] = compNr;
    }
    // go through all edges of the dep graph
    std::pair<DepGraph::edge_iterator, DepGraph::edge_iterator> edgeIteratorRange = boost::edges(depGraph);
    for(DepGraph::edge_iterator edgeIterator = edgeIteratorRange.first; edgeIterator != edgeIteratorRange.second; ++edgeIterator) {
        // connect the according strongly connected components
        int sourceIdx = componentMap[boost::source(*edgeIterator, depGraph)];
        int targetIdx = componentMap[boost::target(*edgeIterator, depGraph)];
        if (sourceIdx != targetIdx) {
            DBGLOG(DBG, "Component " << sourceIdx << " depends on " << targetIdx);
            boost::add_edge(compToVertex[sourceIdx], compToVertex[targetIdx], compDependencies);
        }
    }

    // compute topological ordering of components
    std::vector<SCCDepGraph::vertex_descriptor> compOrdering;
    topological_sort(compDependencies, std::back_inserter(compOrdering));
    DBGLOG(DBG, "Processing components in the following ordering:");
    BOOST_FOREACH (SCCDepGraph::vertex_descriptor compVertex, compOrdering) {
        int comp = vertexToComp[compVertex];
        DBGLOG(DBG, "Component " << comp << ", predicates are: ");
        std::set<ID> stratum;
        BOOST_FOREACH (ID pred, depSCC[comp]) {
            DBGLOG(DBG, "" << pred);
            stratum.insert(pred);
            stratumOfPredicate[pred] = predicatesOfStratum.size();
        }
        predicatesOfStratum.push_back(stratum);
    }

    // arrange the rules accordingly
    rulesOfStratum = std::vector<std::set<ID> >(compOrdering.size());
    BOOST_FOREACH (ID ruleID, inputprogram.idb) {
        rulesOfStratum[getStratumOfRule(ruleID)].insert(preprocessRule(ruleID));
    }
}


void InternalGrounder::buildPredicateIndex()
{

    positionsOfPredicate.clear();

    int ruleNr = 0;
    BOOST_FOREACH (ID nonGroundRuleID, nonGroundRules) {
        const Rule& rule = reg->rules.getByID(nonGroundRuleID);
        DBGLOG(DBG, "Processing rule " << rule);
        int bodyAtomNr = 0;
        BOOST_FOREACH (ID bodyAtomID, rule.body) {
            DBGLOG(DBG, "Atom " << bodyAtomID << " has predicate " << getPredicateOfAtom(bodyAtomID));
            positionsOfPredicate[getPredicateOfAtom(bodyAtomID)].insert(std::pair<int, int>(ruleNr, bodyAtomNr));
            bodyAtomNr++;
        }
        ruleNr++;
    }
}


void InternalGrounder::loadStratum(int index)
{

    DBGLOG(DBG, "Loading stratum " << index);
    nonGroundRules.clear();
    nonGroundRules.insert(nonGroundRules.begin(), rulesOfStratum[index].begin(), rulesOfStratum[index].end());
    buildPredicateIndex();
}


void InternalGrounder::groundStratum(int stratumNr)
{

    loadStratum(stratumNr);

    DBGLOG(DBG, "Grounding stratum " << stratumNr);
    Set<ID> newDerivableAtoms;

    // all facts are immediately derivable and true
    if (stratumNr == 0) {
        DBGLOG(DBG, "Deriving all facts");

        bm::bvector<>::enumerator en = inputprogram.edb->getStorage().first();
        bm::bvector<>::enumerator en_end = inputprogram.edb->getStorage().end();

        Set<ID> newGroundRules;
        while (en < en_end) {
            ID atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en);
            setToTrue(atom);
            addDerivableAtom(atom, groundRules, newDerivableAtoms);
            en++;
        }
    }

    // ground all rules
    DBGLOG(DBG, "Processing rules");
    for (uint32_t ruleIndex = 0; ruleIndex < nonGroundRules.size(); ++ruleIndex) {
        Substitution s;
        groundRule(nonGroundRules[ruleIndex], s, groundRules, newDerivableAtoms);
    }

    // as long as there were new rules generated, add their heads to the list of derivable atoms
    DBGLOG(DBG, "Processing cyclically depending rules");
    while (newDerivableAtoms.size() > 0) {

        // generate further rules for the new derivable atoms
        Set<ID> newDerivableAtoms2;
        BOOST_FOREACH (ID atom, newDerivableAtoms) {
            addDerivableAtom(atom, groundRules, newDerivableAtoms2);
        }
        newDerivableAtoms = newDerivableAtoms2;
    }
    DBGLOG(DBG, "Produced " << groundRules.size() << " ground rules");

    groundedPredicates.insert(predicatesOfStratum[stratumNr].begin(), predicatesOfStratum[stratumNr].end());
    BOOST_FOREACH (ID pred, predicatesOfStratum[stratumNr]) {
        // check if the predicate is completely solved; this is, there does not exist a derivable atom which is not known to be true
        bool solved = true;
        BOOST_FOREACH (ID atom, derivableAtomsOfPredicate[pred]) {
            if (!trueAtoms->getFact(atom.address)) {
                solved = false;
                break;
            }
        }
        if (solved) solvedPredicates.insert(pred);
    }

    #ifndef NDEBUG
    DBGLOG(DBG, "Set of grounded predicates is now: ");
    BOOST_FOREACH (ID pred, groundedPredicates) {
        DBGLOG(DBG, pred);
    }

    DBGLOG(DBG, "Set of solved predicates is now: ");
    BOOST_FOREACH (ID pred, solvedPredicates) {
        DBGLOG(DBG, pred);
    }
    #endif
}


void InternalGrounder::groundRule(ID ruleID, Substitution& s, std::vector<ID>& groundedRules, Set<ID>& newDerivableAtoms)
{
    #define OPTIMIZED
    Substitution currentSubstitution = s;
    const Rule& rule = reg->rules.getByID(ruleID);

    DBGLOG(DBG, "Grounding rule " << ruleToString(ruleID));

    std::vector<ID> body = reorderRuleBody(ruleID);

    // compute binders of the variables in the rule
    Binder binders = getBinderOfRule(body);
    std::set<ID> outputVars = getOutputVariables(ruleID);
    std::vector<std::set<ID> > freeVars;
    for (uint32_t i = 0; i < body.size(); ++i) {
        freeVars.push_back(getFreeVars(body, i));
    }
    std::set<ID> failureVars;

    int csb = -1;                // barrier for backjumping
    if (body.size() == 0) {
        // grounding of choice rules
        buildGroundInstance(ruleID, currentSubstitution, groundedRules, newDerivableAtoms);
    }
    else {
        // start search at position 0 in the extension of all predicates
        std::vector<int> searchPos;
        for (uint32_t i = 0; i < body.size(); ++i) searchPos.push_back(0);

        // go through all (positive) body atoms
        for (std::vector<ID>::const_iterator it = body.begin(); it != body.end(); ) {
            if (it->isAggregateAtom()) throw GeneralError("Error: Internal grounder does not support aggregate atoms");

            int bodyLitIndex = it - body.begin();
            ID bodyLiteralID = *it;

            // remove assignments to all variables which do not occur between body.begin() and it - 1
            DBGLOG(DBG, "Undoing variable assignments before position " << bodyLitIndex);
            std::set<ID> keepVars;
            for (std::vector<ID>::const_iterator itVarCheck = body.begin(); itVarCheck != it; ++itVarCheck) {
                reg->getVariablesInID(*itVarCheck, keepVars);
            }
            Substitution newSubst = s;
            BOOST_FOREACH (ID var, keepVars) {
                newSubst[var] = currentSubstitution[var];
            }
            currentSubstitution = newSubst;

            DBGLOG(DBG, "Finding next match at position " << bodyLitIndex << " in extension after index " << searchPos[bodyLitIndex]);
            int startSearchPos = searchPos[bodyLitIndex];
            searchPos[bodyLitIndex] = matchNextFromExtension(applySubstitutionToAtom(currentSubstitution, bodyLiteralID), currentSubstitution, searchPos[bodyLitIndex]);
            DBGLOG(DBG, "Search result: " << searchPos[bodyLitIndex]);

            // match?
            if (searchPos[bodyLitIndex] == -1) {

                #ifdef OPTIMIZED
                // the conflict in this literal is due to any of the variables occurring in it
                reg->getVariablesInID(*it, failureVars);

                int btIndex = -1;
                if (startSearchPos == 0) {
                    // failure on first match
                    DBGLOG(DBG, "Failure on first match at position " << bodyLitIndex);
                    std::set<ID> vars;
                    reg->getVariablesInID(*it, vars);
                    btIndex = getClosestBinder(body, bodyLitIndex, vars);
                }
                else {
                    // failure on next match
                    DBGLOG(DBG, "Failure on next match at position " << bodyLitIndex);

                    btIndex = getClosestBinder(body, bodyLitIndex, failureVars);
                    btIndex = csb > btIndex ? csb : btIndex;

                    if (btIndex == csb) {
                        csb = getClosestBinder(body, btIndex, outputVars);
                    }
                }
                if (btIndex == -1) {
                    DBGLOG(DBG, "No more matches");
                    return;
                }
                else {
                    DBGLOG(DBG, "Backtracking to literal " << btIndex);
                    it = body.begin() + btIndex;
                }
                #else
                // backtrack
                int btIndex = backtrack(ruleID, binders, bodyLitIndex);
                if (btIndex == -1) {
                    DBGLOG(DBG, "No more matches");
                    return;
                }
                else {
                    DBGLOG(DBG, "Backtracking to literal " << btIndex);
                    it = body.begin() + btIndex;
                }
                #endif

                continue;
            }
            else {
                // variables which occur in the current literals are now no failure variables anymore
                BOOST_FOREACH (ID v, freeVars[bodyLitIndex]) {
                    failureVars.erase(v);
                }
            }

            // match
            // if we are at the end of the body list we have found a valid substitution
            if (it == body.end() - 1) {
                DBGLOG(DBG, "Substitution complete");
                buildGroundInstance(ruleID, currentSubstitution, groundedRules, newDerivableAtoms);
                #ifdef OPTIMIZED
                int btIndex = getClosestBinder(body, bodyLitIndex + 1, outputVars);
                if (btIndex == -1) {
                    DBGLOG(DBG, "No more matches after solution found");
                    return;
                }
                else {
                    DBGLOG(DBG, "Backtracking to literal " << btIndex << " after solution found");
                    csb = getClosestBinder(body, btIndex, outputVars);
                    it = body.begin() + btIndex;
                }
                #else

                // go back to last non-naf body literal
                while(bodyLiteralID.isNaf()) {
                    if (it == body.begin()) return;
                    --it;
                    bodyLiteralID = *it;
                }
                #endif
            }
            else {
                // go to next atom in rule body
                ++it;
                                 // start from scratch
                searchPos[bodyLitIndex + 1] = 0;
            }
        }
    }
}


void InternalGrounder::buildGroundInstance(ID ruleID, Substitution s, std::vector<ID>& groundedRules, Set<ID>& newDerivableAtoms)
{

    const Rule& rule = reg->rules.getByID(ruleID);

    Tuple groundedHead, groundedBody;

    // ground head
    BOOST_FOREACH (ID headAtom, rule.head) {
        ID groundHeadAtom = applySubstitutionToAtom(s, headAtom);
        groundedHead.push_back(groundHeadAtom);
        newDerivableAtoms.insert(groundHeadAtom);
    }

    // ground body
    BOOST_FOREACH (ID bodyLitID, rule.body) {
        ID groundBodyLiteralID = applySubstitutionToAtom(s, bodyLitID);

        if (groundBodyLiteralID.isBuiltinAtom() && optlevel != none) {
            // at this point, built-in atoms are always true, otherwise the grounding terminates even earlier
            continue;
        }

        if (groundBodyLiteralID.isOrdinaryAtom() && optlevel == full) {
            const OrdinaryAtom& groundBodyLiteral = reg->ogatoms.getByID(groundBodyLiteralID);

            // h :- a, not b         where a is known to be true
            // optimization: skip satisfied literals
            if (!groundBodyLiteralID.isNaf() && trueAtoms->getFact(groundBodyLiteralID.address)) {
                DBGLOG(DBG, "Skipping true " << groundBodyLiteralID);
                continue;
            }

            // h :- a, not b         where b is known to be not derivable
            // optimization for stratified negation: skip naf-body literals over known predicates which are not derivable
            if (groundBodyLiteralID.isNaf() && isPredicateGrounded(groundBodyLiteral.front()) && !isAtomDerivable(groundBodyLiteralID)) {
                DBGLOG(DBG, "Skipping underivable " << groundBodyLiteralID);
                continue;
            }

            // h :- a, not b         where b is known to be true
            // optimization: skip rules which contain a naf-literal which in known to be true
            if (groundBodyLiteralID.isNaf() && trueAtoms->getFact(groundBodyLiteralID.address)) {
                DBGLOG(DBG, "Skipping rule " << ruleToString(ruleID) << " due to true " << groundBodyLiteralID);
                return;
            }

            // h :- a, not b         where a is known to be not derivable
            // optimization for stratified negation: skip naf-body literals over known predicates which are not derivable
            if (!groundBodyLiteralID.isNaf() && isPredicateGrounded(groundBodyLiteral.front()) && !isAtomDerivable(groundBodyLiteralID)) {
                DBGLOG(DBG, "Skipping rule " << ruleToString(ruleID) << " due to " << groundBodyLiteralID);
                return;
            }
        }

        groundedBody.push_back(groundBodyLiteralID);
    }

    // determine type of rule
    IDKind kind = ID::MAINKIND_RULE;
    if (groundedHead.size() == 0) kind |= ID::SUBKIND_RULE_CONSTRAINT;
    if (groundedHead.size() > 0) kind |= ID::SUBKIND_RULE_REGULAR;
    if (groundedHead.size() > 1) kind |= ID::PROPERTY_RULE_DISJ;

    // avoid empty rules:
    // in case that both the head and the body are empty, add default-negated atom which does not occur elsewhere
    if (groundedHead.size() == 0 && groundedBody.size() == 0) {
        DBGLOG(DBG, "Adding globally new naf-literal " << globallyNewAtom.address << " to body");
        groundedBody.push_back(ID(ID::MAINKIND_LITERAL | ID::SUBKIND_ATOM_ORDINARYG | ID::NAF_MASK, globallyNewAtom.address));
    }

    // new facts are set immediately
    if (groundedHead.size() == 1 && groundedBody.size() == 0) {
        // derive new fact
        DBGLOG(DBG, "Adding fact " << groundedHead[0]);
        setToTrue(groundedHead[0]);
    }
    else {
        // build rule
        Rule groundedRule(kind, groundedHead, groundedBody, rule.weight, rule.level);

        // avoid duplicate entries (they cause the registry to crash)
        ID id = reg->rules.getIDByElement(groundedRule);
        if (id == ID_FAIL) {
            DBGLOG(DBG, "Adding ground rule");
            id = reg->rules.storeAndGetID(groundedRule);
            DBGLOG(DBG, "Added ground rule " << ruleToString(id));
        }
        else {
            DBGLOG(DBG, "Ground rule " << ruleToString(id) << " was already present");
        }
        groundedRules.push_back(id);
    }
}


bool InternalGrounder::match(ID literalID, ID patternLiteralID, Substitution& s)
{

    // sign must be equal
    if (literalID.isNaf() != patternLiteralID.isNaf()) return false;

    if (literalID.isOrdinaryAtom()) {
        return matchOrdinary(literalID, patternLiteralID, s);
    }
    else if(literalID.isBuiltinAtom()) {
        return matchBuiltin(literalID, patternLiteralID, s);
    }
    else {
        // other types of atoms are currently not implemented (e.g. aggregate atoms)
        assert(false);
        return false;
    }
}


bool InternalGrounder::matchOrdinary(ID literalID, ID patternLiteralID, Substitution& s)
{

    const OrdinaryAtom& atom = literalID.isOrdinaryGroundAtom() ? reg->ogatoms.getByID(literalID) : reg->onatoms.getByID(literalID);
    const OrdinaryAtom& patternAtom = reg->ogatoms.getByID(patternLiteralID);

    if (!atom.unifiesWith(patternAtom)) return false;

    // compute the unifying substitution
    for (uint32_t termIndex = 0; termIndex < atom.tuple.size(); ++termIndex) {
        if (atom.tuple[termIndex].isVariableTerm()) {
            s[atom.tuple[termIndex]] = patternAtom.tuple[termIndex];
        }
    }
    return true;
}


bool InternalGrounder::matchBuiltin(ID literalID, ID patternLiteralID, Substitution& s)
{

    // builtin-atoms must not be default-negated
    assert(!literalID.isNaf());
    assert(!patternLiteralID.isNaf());

    const BuiltinAtom& atom = reg->batoms.getByID(literalID);
    const BuiltinAtom& patternAtom = reg->batoms.getByID(patternLiteralID);

    // compute the unifying substitution
    Substitution s2 = s;
    for (uint32_t termIndex = 0; termIndex < atom.tuple.size(); ++termIndex) {
        if (atom.tuple[termIndex].isVariableTerm()) {
            if (s.find(atom.tuple[termIndex]) != s.end() && s[atom.tuple[termIndex]] != patternAtom.tuple[termIndex]) return false;
            s[atom.tuple[termIndex]] = patternAtom.tuple[termIndex];
        }
        else {
            if (atom.tuple[termIndex] != patternAtom.tuple[termIndex]) return false;
        }
    }
    s = s2;
    return true;
}


int InternalGrounder::matchNextFromExtension(ID literalID, Substitution& s, int startSearchIndex)
{

    if (literalID.isOrdinaryAtom()) {
        return matchNextFromExtensionOrdinary(literalID, s, startSearchIndex);
    }
    else if (literalID.isBuiltinAtom()) {
        return matchNextFromExtensionBuiltin(literalID, s, startSearchIndex);
    }
    else {
        // other types of atoms are currently not implemented (e.g. aggregate atoms)
        assert(false);
        return false;
    }
}


int InternalGrounder::matchNextFromExtensionOrdinary(ID literalID, Substitution& s, int startSearchIndex)
{

    DBGLOG(DBG, "Matching ordinary atom");
    if (!literalID.isNaf()) {
        const OrdinaryAtom& atom = literalID.isOrdinaryGroundAtom() ? reg->ogatoms.getByID(literalID) : reg->onatoms.getByID(literalID);
        std::vector<ID>& extension = derivableAtomsOfPredicate[atom.front()];

        for (std::vector<ID>::const_iterator it = extension.begin() + startSearchIndex; it != extension.end(); ++it) {

            if (match(literalID, *it, s)) {
                // yes
                // return next start search index
                return it - extension.begin() + 1;
            }
        }
        // no match
        return -1;
    }
    else {
                                 // matching of non-ground naf-literals is illegal
        assert(!literalID.isOrdinaryNongroundAtom());

                                 // only one match
        if (startSearchIndex > 0) return -1;

        // naf-literals will always match if the predicates is unsolved
        if (!isPredicateSolved(getPredicateOfAtom(literalID))) {
            return 1;
        }
        else {
            // check if the ground literal is NOT in the (complete) extension
            ID posID = ID(((literalID.kind & (ID::ALL_ONES ^ ID::MAINKIND_MASK)) | ID::MAINKIND_ATOM) ^ ID::NAF_MASK, literalID.address);
            const OrdinaryAtom& atom = reg->ogatoms.getByID(posID);
            std::vector<ID>& extension = derivableAtomsOfPredicate[atom.front()];
            if (isAtomDerivable(posID)) {
                return -1;       // no match of naf-literal
            }
            else {
                return 1;        // match of naf-literal
            }
        }
    }
}


int InternalGrounder::matchNextFromExtensionBuiltin(ID literalID, Substitution& s, int startSearchIndex)
{

    // builtin-atoms must not be default-negated
    assert (!literalID.isNaf());

    DBGLOG(DBG, "Matching builtin atom");
    const BuiltinAtom& atom = reg->batoms.getByID(literalID);

    switch (atom.tuple[0].address) {
        case ID::TERM_BUILTIN_INT:
            return matchNextFromExtensionBuiltinUnary(literalID, s, startSearchIndex);

        case ID::TERM_BUILTIN_EQ:
        case ID::TERM_BUILTIN_NE:
        case ID::TERM_BUILTIN_LT:
        case ID::TERM_BUILTIN_LE:
        case ID::TERM_BUILTIN_GT:
        case ID::TERM_BUILTIN_GE:
        case ID::TERM_BUILTIN_SUCC:
            return matchNextFromExtensionBuiltinBinary(literalID, s, startSearchIndex);

        case ID::TERM_BUILTIN_ADD:
        case ID::TERM_BUILTIN_MUL:
        case ID::TERM_BUILTIN_SUB:
        case ID::TERM_BUILTIN_DIV:
        case ID::TERM_BUILTIN_MOD:
            return matchNextFromExtensionBuiltinTernary(literalID, s, startSearchIndex);
    }
    assert(false);
    return -1;
}


int InternalGrounder::matchNextFromExtensionBuiltinUnary(ID literalID, Substitution& s, int startSearchIndex)
{

    const BuiltinAtom& atom = reg->batoms.getByID(literalID);
    switch (atom.tuple[0].address) {
        case ID::TERM_BUILTIN_INT:
            if (startSearchIndex > (int)ctx.maxint) {
                return -1;
            }
            else {
                if (atom.tuple[1].isVariableTerm()) {
                    s[atom.tuple[1]] = ID::termFromInteger(startSearchIndex);
                    return startSearchIndex + 1;
                }
                else if (atom.tuple[1].isConstantTerm()) {
                    return -1;
                }
                else {
                    assert(atom.tuple[1].isIntegerTerm());

                    if (startSearchIndex <= (int)atom.tuple[1].address) {
                        return atom.tuple[1].address + 1;
                    }
                }
            }
    }
    assert(false);
    return 0;
}


int InternalGrounder::matchNextFromExtensionBuiltinBinary(ID literalID, Substitution& s, int startSearchIndex)
{

    const BuiltinAtom& atom = reg->batoms.getByID(literalID);

    if (startSearchIndex > 0) return -1;

    if (atom.tuple[1].isVariableTerm() && atom.tuple[2].isVariableTerm()) {
        return -1;
    }
    else if ((atom.tuple[1].isConstantTerm() || atom.tuple[1].isIntegerTerm()) && atom.tuple[2].isVariableTerm() && atom.tuple[0].address == ID::TERM_BUILTIN_EQ) {
        s[atom.tuple[2]] = atom.tuple[1];
        return 1;
    }
    else if ((atom.tuple[2].isConstantTerm() || atom.tuple[2].isIntegerTerm()) && atom.tuple[1].isVariableTerm() && atom.tuple[0].address == ID::TERM_BUILTIN_EQ) {
        s[atom.tuple[1]] = atom.tuple[2];
        return 1;
    }
    else if (atom.tuple[1].isIntegerTerm() && atom.tuple[2].isVariableTerm() && atom.tuple[0].address == ID::TERM_BUILTIN_SUCC) {
        s[atom.tuple[2]] = ID::termFromInteger(atom.tuple[1].address + 1);
        return 1;
    }
    else if (atom.tuple[1].isVariableTerm() && atom.tuple[2].isIntegerTerm() && atom.tuple[0].address == ID::TERM_BUILTIN_SUCC) {
        if (atom.tuple[2].address == 0) return -1;
        s[atom.tuple[1]] = ID::termFromInteger(atom.tuple[2].address - 1);
        return 1;
    }
    else {
        // all values are fixed
        bool cmp;
        switch (atom.tuple[0].address) {
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
}


int InternalGrounder::matchNextFromExtensionBuiltinTernary(ID literalID, Substitution& s, int startSearchIndex)
{

    if (startSearchIndex > (int)((ctx.maxint + 1) * (ctx.maxint + 1))) {
        return -1;
    }
    else {
        const BuiltinAtom& atom = reg->batoms.getByID(literalID);

        uint32_t x = startSearchIndex / (ctx.maxint + 1);
        uint32_t y = startSearchIndex % (ctx.maxint + 1);

        if (atom.tuple[1].isConstantTerm() || atom.tuple[2].isConstantTerm() || atom.tuple[3].isConstantTerm()) return -1;

        if (atom.tuple[1].isIntegerTerm() && atom.tuple[2].isIntegerTerm()) {
            if (x <= atom.tuple[1].address && y <= atom.tuple[2].address) {
                x = atom.tuple[1].address;
                y = atom.tuple[2].address;
                int z = applyIntFunction(x_op_y_eq_ret, atom.tuple[0], x, y);
                if (atom.tuple[3].isIntegerTerm()) {
                    if (atom.tuple[3].address != z) return -1;
                    else return x * (ctx.maxint + 1) + y + 1;
                }
                else {
                    s[atom.tuple[3]] = ID::termFromInteger(z);
                    return x * (ctx.maxint + 1) + y + 1;
                }
            }
            else {
                return -1;
            }
        }

        return -1;
    }
}


int InternalGrounder::backtrack(ID ruleID, Binder& binders, int currentIndex)
{

    // backtrack to the last positive literal
    const Rule& rule = reg->rules.getByID(ruleID);

    int bt = currentIndex - 1;
    while (bt > -1 && rule.body[bt].isNaf()) bt--;
    return bt;
}


void InternalGrounder::setToTrue(ID atom)
{

    DBGLOG(DBG, "Setting " << atom << " to true");
    trueAtoms->setFact(atom.address);
}


void InternalGrounder::addDerivableAtom(ID atomID, std::vector<ID>& groundRules, Set<ID>& newDerivableAtoms)
{

    DBGLOG(DBG, "" << atomID << " becomes derivable");

    const OrdinaryAtom& ogatom = reg->ogatoms.getByID(atomID);
    if (isAtomDerivable(atomID)) {
        // is already marked as derivable: nothing to do
        return;
    }
    else {
        derivableAtomsOfPredicate[ogatom.front()].push_back(atomID);
    }

    // go through all rules which contain this predicate positively in their body
    typedef std::pair<int, int> Pair;
    BOOST_FOREACH (Pair location, positionsOfPredicate[ogatom.front()]) {
        // extract the atom from the body
        // if it unifies with atomID, ground the rule with this substitution being predetermined
        const Rule& rule = reg->rules.getByID(nonGroundRules[location.first]);
        if (!rule.body[location.second].isNaf()) {
            DBGLOG(DBG, "Atom occurs in rule " << location.first << " at position " << location.second);
            Substitution s;
            match(rule.body[location.second], atomID, s);

            groundRule(nonGroundRules[location.first], s, groundRules, newDerivableAtoms);
        }
    }
}


ID InternalGrounder::applySubstitutionToAtom(Substitution s, ID atomID)
{

    if (atomID.isOrdinaryAtom()) {
        return applySubstitutionToOrdinaryAtom(s, atomID);
    }

    if (atomID.isBuiltinAtom()) {
        return applySubstitutionToBuiltinAtom(s, atomID);
    }

    assert(false && "unsupported atom type");
    return ID_FAIL;
}


ID InternalGrounder::applySubstitutionToOrdinaryAtom(Substitution s, ID atomID)
{

    if (atomID.isOrdinaryGroundAtom()) return atomID;

    // apply substitution to tuple of atom
    const OrdinaryAtom& onatom = reg->onatoms.getByID(atomID);
    Tuple t = onatom.tuple;
    bool isGround = true;
    for (uint32_t termIndex = 0; termIndex < t.size(); ++termIndex) {
        if (s.find(t[termIndex]) != s.end()) {
            t[termIndex] = s[t[termIndex]];
        }
        if (t[termIndex].isVariableTerm()) isGround = false;
    }

    // replace mainkind by ATOM and subkind according to groundness
    IDKind kind = atomID.kind;
    kind &= (ID::ALL_ONES ^ ID::MAINKIND_MASK);
    kind &= (ID::ALL_ONES ^ ID::SUBKIND_MASK);
    kind |= ID::MAINKIND_ATOM;
    if (isGround) {
        kind |= ID::SUBKIND_ATOM_ORDINARYG;
    }
    else {
        kind |= ID::SUBKIND_ATOM_ORDINARYN;
    }
    OrdinaryAtom atom(kind);
    atom.tuple = t;
    ID id;
    if (isGround) {
        id = reg->storeOrdinaryGAtom(atom);
    }
    else {
        id = reg->storeOrdinaryNAtom(atom);
    }

    // output: use kind of input, except for the subtype, which is according to groundness
    kind &= (ID::ALL_ONES ^ ID::MAINKIND_MASK);
    kind |= (atomID.kind & ID::MAINKIND_MASK);
    return ID(kind, id.address);
}


ID InternalGrounder::applySubstitutionToBuiltinAtom(Substitution s, ID atomID)
{

    const BuiltinAtom& batom = reg->batoms.getByID(atomID);
    Tuple t = batom.tuple;
    bool isGround = true;
    for (uint32_t termIndex = 1; termIndex < t.size(); ++termIndex) {
        if (s.find(t[termIndex]) != s.end()) {
            t[termIndex] = s[t[termIndex]];
        }
        if (t[termIndex].isVariableTerm()) isGround = false;
    }

    BuiltinAtom sbatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_BUILTIN, t);
    // TODO: We have to check if sbatom is already present, otherwise the registry crashes!
    return reg->batoms.storeAndGetID(sbatom);
}


std::string InternalGrounder::atomToString(ID atomID)
{
    std::stringstream ss;
    RawPrinter p(ss, reg);
    p.print(atomID);
    return ss.str();
}


std::string InternalGrounder::ruleToString(ID ruleID)
{
    std::stringstream ss;
    RawPrinter p(ss, reg);
    p.print(ruleID);
    return ss.str();
}


ID InternalGrounder::getPredicateOfAtom(ID atomID)
{

    if (atomID.isOrdinaryAtom()) {
        const OrdinaryAtom& atom = (atomID.isOrdinaryGroundAtom() ? reg->ogatoms.getByID(atomID) : reg->onatoms.getByID(atomID));
        return atom.front();
    }
    if (atomID.isBuiltinAtom()) {
        const BuiltinAtom& atom = reg->batoms.getByID(atomID);
        return atom.front();
    }
    if (atomID.isAggregateAtom()) throw GeneralError("Error: Internal grounder does not support aggregate atoms");

    return ID_FAIL;
}


bool InternalGrounder::isGroundRule(ID ruleID)
{

    const Rule& rule = reg->rules.getByID(ruleID);
    BOOST_FOREACH (ID bodyAtom, rule.body) {
        if (!bodyAtom.isOrdinaryGroundAtom()) return false;
    }
    return true;
}


bool InternalGrounder::isPredicateGrounded(ID pred)
{

    return (std::find(groundedPredicates.begin(), groundedPredicates.end(), pred) != groundedPredicates.end());
}


bool InternalGrounder::isPredicateSolved(ID pred)
{

    return (std::find(solvedPredicates.begin(), solvedPredicates.end(), pred) != solvedPredicates.end());
}


bool InternalGrounder::isAtomDerivable(ID atom)
{

    ID pred = getPredicateOfAtom(atom);
    BOOST_FOREACH (ID derv, derivableAtomsOfPredicate[pred]) {
        if (atom.address == derv.address) return true;
    }
    return false;
}


int InternalGrounder::getStratumOfRule(ID ruleID)
{

    const Rule& rule = reg->rules.getByID(ruleID);
    int stratum = 0;

    // compute the highest stratum of all head atoms
    BOOST_FOREACH (ID lit, rule.body) {
        int s;
        if (lit.isOrdinaryAtom()) {
            const OrdinaryAtom& atom = (lit.isOrdinaryGroundAtom() ? reg->ogatoms.getByID(lit) : reg->onatoms.getByID(lit));
            s = stratumOfPredicate[atom.front()];
        }
        else {
            s = 0;
        }
        stratum = (s > stratum ? s : stratum);
    }

    BOOST_FOREACH (ID headLit, rule.head) {
        const OrdinaryAtom& atom = (headLit.isOrdinaryGroundAtom() ? reg->ogatoms.getByID(headLit) : reg->onatoms.getByID(headLit));
        int s = stratumOfPredicate[atom.front()];
        stratum = (s > stratum ? s : stratum);
    }
    DBGLOG(DBG, "Stratum of rule " << ruleToString(ruleID) << " is " << stratum);
    return stratum;
}


void InternalGrounder::computeGloballyNewAtom()
{

    // get new predicate name
    std::string newPredicateName("newPredName");

    // idb
    BOOST_FOREACH (ID ruleID, inputprogram.idb) {
        const Rule& rule = reg->rules.getByID(ruleID);

        BOOST_FOREACH (ID headLiteralID, rule.head) {
            ID pred = getPredicateOfAtom(headLiteralID);
            if (pred.isConstantTerm() || pred.isPredicateTerm()) {
                while (boost::starts_with(reg->getTermStringByID(pred), newPredicateName)) {
                    newPredicateName += std::string("0");
                }
            }
        }
        BOOST_FOREACH (ID bodyLiteralID, rule.body) {
            ID pred = getPredicateOfAtom(bodyLiteralID);
            if (pred.isConstantTerm() || pred.isPredicateTerm()) {
                while (boost::starts_with(reg->getTermStringByID(pred), newPredicateName)) {
                    newPredicateName += std::string("0");
                }
            }
        }
    }

    // edb
    bm::bvector<>::enumerator en = inputprogram.edb->getStorage().first();
    bm::bvector<>::enumerator en_end = inputprogram.edb->getStorage().end();
    while (en < en_end) {
        ID pred = getPredicateOfAtom(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
        while (boost::starts_with(reg->getTermStringByID(pred), newPredicateName)) {
            newPredicateName += std::string("0");
        }
        en++;
    }

    // create atom
    OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
    Term predTerm(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, newPredicateName);
    ID predID = reg->storeTerm(predTerm);
    atom.tuple.push_back(predID);
    globallyNewAtom = reg->storeOrdinaryGAtom(atom);
}


InternalGrounder::Binder InternalGrounder::getBinderOfRule(std::vector<ID>& body)
{

    Binder binders;
    int litIdx = 0;
    BOOST_FOREACH (ID lit, body) {
        std::set<ID> varsInLit;
        reg->getVariablesInID(lit, varsInLit);
        BOOST_FOREACH (ID var, varsInLit) {
            if (binders.find(var) == binders.end()) {
                DBGLOG(DBG, "Binder of variable " << var << " is literal " << litIdx);
                binders[var] = litIdx;
            }
        }
        ++litIdx;
    }
    return binders;
}


int InternalGrounder::getClosestBinder(std::vector<ID>& body, int litIndex, std::set<ID> variables /*do not make this a reference because it is used as working copy!*/ ){

int cb = -1;
for (int i = 0; i < litIndex; ++i) {
    // is this literal a binder of some variable?
    if (!body[i].isNaf()) {
        std::set<ID> varsInLit;
        reg->getVariablesInID(body[i], varsInLit);
        BOOST_FOREACH(ID v, varsInLit) {
            if (variables.count(v) > 0) {
                cb = i;
                                 // make sure that only the first literal with v is recognized as binder of v
                variables.erase(v);
            }
        }
    }
}


return cb;
}


std::set<ID> InternalGrounder::getFreeVars(std::vector<ID>& body, int litIndex)
{

    std::set<ID> vars;
    reg->getVariablesInID(body[litIndex], vars);
    for (int i = 0; i < litIndex; ++i) {
        std::set<ID> v2;
        reg->getVariablesInID(body[i], v2);
        // check if v2 contains some variable in vars
        BOOST_FOREACH (ID v, v2) {
            vars.erase(v);
        }
    }
    return vars;
}


std::set<ID> InternalGrounder::getOutputVariables(ID ruleID)
{

    // compute output variables of this rule
    // this is the set of all variables which occur in literals over unsolved predicates
    const Rule& rule = reg->rules.getByID(ruleID);
    std::set<ID> outputVars;
    BOOST_FOREACH (ID headLitIndex, rule.head) {
        if (!isPredicateSolved(getPredicateOfAtom(headLitIndex))) {
            reg->getVariablesInID(headLitIndex, outputVars);
        }
    }
    BOOST_FOREACH (ID bodyLitIndex, rule.body) {
        // if the head does not contain variables, all body variables are output variables
        if (!isPredicateSolved(getPredicateOfAtom(bodyLitIndex))) {
            reg->getVariablesInID(bodyLitIndex, outputVars);
        }
    }

    #ifndef NDEBUG
    DBGLOG(DBG, "Output variables:");
    BOOST_FOREACH (ID var, outputVars) {
        DBGLOG(DBG, "" << var.address);
    }
    #endif

    return outputVars;
}


std::vector<ID> InternalGrounder::reorderRuleBody(ID ruleID)
{

    const Rule& rule = reg->rules.getByID(ruleID);

    // reorder literals in rule body
    std::vector<ID> body;

    // 1. positive
    BOOST_FOREACH (ID lit, rule.body) {
        if (!(lit.isNaf() || lit.isBuiltinAtom())) body.push_back(lit);
    }

    // 2. builtin

    // dependency graph
    typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, ID> BIDepGraph;
    typedef BIDepGraph::vertex_descriptor BINode;
    std::map<ID, BINode> biDepNodes;
    std::map<BINode, ID> biDepNodesIndex;
    BIDepGraph biDepGraph;

    BOOST_FOREACH (ID lit, rule.body) {
        // all builtin-atoms are nodes in the dep graph
        if (lit.isBuiltinAtom()) {

            biDepNodes[lit] = boost::add_vertex(lit, biDepGraph);
            biDepNodesIndex[biDepNodes[lit]] = lit;

            // add edges between depending nodes
            BOOST_FOREACH (ID lit2, rule.body) {
                if (lit2.isBuiltinAtom() && lit != lit2) {
                    if (biDependency(lit, lit2)) {
                        boost::add_edge(biDepNodes[lit], biDepNodes[lit2], biDepGraph);
                    }
                }
            }
        }
    }
    // get topological ordering

    // compute topological ordering of components
    std::vector<BINode> biOrdering;
    topological_sort(biDepGraph, std::back_inserter(biOrdering));
    DBGLOG(DBG, "Processing BI-atoms in the following ordering:");
    BOOST_FOREACH (BINode biAtomNode, biOrdering) {
        ID biAtom = biDepNodesIndex[biAtomNode];
        DBGLOG(DBG, "" << biAtom.address);
        body.push_back(biAtom);
    }

    // 3. naf
    BOOST_FOREACH (ID lit, rule.body) {
        if (lit.isNaf()) body.push_back(lit);
    }

    return body;
}


bool InternalGrounder::biDependency(ID bi1, ID bi2)
{

    const BuiltinAtom& biAtom1 = reg->batoms.getByID(bi1);
    const BuiltinAtom& biAtom2 = reg->batoms.getByID(bi2);

    std::set<ID> output1;
    std::set<ID> input2;

    switch(biAtom1.tuple.size()) {
        case 2:
            output1.insert(biAtom1.tuple[1]);
            break;
        case 3:
            output1.insert(biAtom1.tuple[2]);
            break;
        case 4:
            output1.insert(biAtom1.tuple[3]);
            break;
        default:
            assert(false);
    }
    switch(biAtom2.tuple.size()) {
        case 2:
            break;
        case 3:
            output1.insert(biAtom1.tuple[1]);
            break;
        case 4:
            output1.insert(biAtom1.tuple[1]);
            output1.insert(biAtom1.tuple[2]);
            break;
        default:
            assert(false);
    }
    BOOST_FOREACH (ID i2, input2) {
        if (output1.count(i2) > 0) return true;
    }
    return false;
}


int InternalGrounder::applyIntFunction(AppDir ad, ID op, int x, int y)
{

    switch(ad) {
        case x_op_y_eq_ret:
            switch (op.address) {
                case ID::TERM_BUILTIN_ADD: return x + y;
                case ID::TERM_BUILTIN_MUL: return x * y;
                case ID::TERM_BUILTIN_SUB: return x - y;
                case ID::TERM_BUILTIN_DIV:
                    if (y == 0) return -1;
                    return x / y;
                case ID::TERM_BUILTIN_MOD:
                    if (y == 0) return -1;
                    return x % y;
            }
            break;
        case x_op_ret_eq_y:
            switch (op.address) {
                case ID::TERM_BUILTIN_ADD: return y - x;
                case ID::TERM_BUILTIN_MUL:
                    if (x == 0) return -1;
                    if (y % x == 0) return y / x;
                case ID::TERM_BUILTIN_SUB: return x - y;
                case ID::TERM_BUILTIN_DIV:
                    if (y == 0) return -1;
                    if (x % y == 0) return x / y;
            }
            break;
        case ret_op_y_eq_x:
            switch (op.address) {
                case ID::TERM_BUILTIN_ADD: return x - y;
                case ID::TERM_BUILTIN_MUL:
                    if (y == 0) return -1;
                    if (x % y == 0) return x / y;
                case ID::TERM_BUILTIN_SUB: return x + y;
                case ID::TERM_BUILTIN_DIV: return x * y;
            }
            break;
    }
    return -1;
}


InternalGrounder::InternalGrounder(ProgramCtx& c, const OrdinaryASPProgram& p, OptLevel ol) : inputprogram(p), groundProgram(p), ctx(c), optlevel(ol)
{

    DBGLOG(DBG, "Starting grounding");
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidgroundertime, "Grounder time");

    reg = ctx.registry();

    trueAtoms = InterpretationPtr(new Interpretation(reg));

    computeDepGraph();
    computeStrata();
    computeGloballyNewAtom();

    // now ground stratum by stratum
    for (uint32_t stratumNr = 0; stratumNr < predicatesOfStratum.size(); ++stratumNr) {
        groundStratum(stratumNr);
    }

    groundProgram = OrdinaryASPProgram(reg, groundRules, trueAtoms, inputprogram.maxint, inputprogram.mask);

    #ifndef NDEBUG
    const OrdinaryASPProgram& gp = getGroundProgram();

    std::stringstream ss;
    ss << *trueAtoms << " (" << trueAtoms->getStorage().count() << ")";
    ss << std::endl;
    BOOST_FOREACH (ID ruleID, groundRules) {
        ss << ruleToString(ruleID) << std::endl;
    }
    DBGLOG(DBG, "Grounded program: " << std::endl << ss.str());
    #endif
}


std::string InternalGrounder::getGroundProgramString()
{

    std::stringstream ss;

    // add edb
    bm::bvector<>::enumerator en = trueAtoms->getStorage().first();
    bm::bvector<>::enumerator en_end = trueAtoms->getStorage().end();

    std::set<ID> newGroundRules;
    while (en < en_end) {
        ss << ruleToString(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en)) << "." << std::endl;
        en++;
    }

    // add idb
    ss << std::endl;
    BOOST_FOREACH (ID ruleID, groundRules) {
        ss << ruleToString(ruleID) << std::endl;
    }
    return ss.str();
}


std::string InternalGrounder::getNongroundProgramString()
{

    std::stringstream ss;

    // add edb
    bm::bvector<>::enumerator en = inputprogram.edb->getStorage().first();
    bm::bvector<>::enumerator en_end = inputprogram.edb->getStorage().end();

    while (en < en_end) {
        ss << ruleToString(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en)) << "." << std::endl;
        en++;
    }

    // add idb
    ss << std::endl;
    BOOST_FOREACH (ID ruleID, inputprogram.idb) {
        ss << ruleToString(ruleID) << std::endl;
    }
    return ss.str();
}


const OrdinaryASPProgram& InternalGrounder::getGroundProgram()
{

    return groundProgram;
}


const OrdinaryASPProgram& InternalGrounder::getNongroundProgram()
{

    return inputprogram;
}


DLVHEX_NAMESPACE_END
