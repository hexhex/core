/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2016 Peter Schüller
 * Copyright (C) 2011-2016 Christoph Redl
 * Copyright (C) 2015-2016 Tobias Kaminski
 * Copyright (C) 2015-2016 Antonius Weinzierl
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dlvhex2/InternalGroundASPSolver.h"

#include <iostream>
#include <sstream>

#include "dlvhex2/Logger.h"
#include "dlvhex2/GenuineSolver.h"
#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/Printer.h"

#include <boost/foreach.hpp>
#include <boost/graph/strong_components.hpp>

DLVHEX_NAMESPACE_BEGIN

//#define DBGLOGD(X,Y) DBGLOG(X,Y)
#define DBGLOGD(X,Y) do{}while(false);

// 1. body must not be false if all literals are true
// 2. body must not be true if a literal is false
// 3. head must not be false if body is true
void InternalGroundASPSolver::createNogoodsForRule(ID ruleBodyAtomID, ID ruleID)
{

    if (ruleID.isWeightRule()) throw GeneralError("Internal solver does not support weight rules");
    const Rule& r = reg->rules.getByID(ruleID);

    // 1 and 2
    createNogoodsForRuleBody(ruleBodyAtomID, r.body);

    // 3. head must not be false if body is true
    Nogood bodyImpliesHead;
    bodyImpliesHead.insert(createLiteral(ruleBodyAtomID));
    BOOST_FOREACH(ID headLit, r.head) {
        bodyImpliesHead.insert(createLiteral(headLit.address, false));
    }
    nogoodset.addNogood(bodyImpliesHead);
}


// 1. body must not be false if all literals are true
// 2. body must not be true if a literal is false
void InternalGroundASPSolver::createNogoodsForRuleBody(ID ruleBodyAtomID, const Tuple& ruleBody)
{
    BOOST_FOREACH(ID bodyLit, ruleBody) {
        if (bodyLit.isAggregateAtom()) throw GeneralError("Internal solver does not support aggregate atoms");
    }

    // 1. body must not be false if all literals are true
    Nogood bodySatIfLitSat;
    BOOST_FOREACH(ID bodyLit, ruleBody) {
        bodySatIfLitSat.insert(createLiteral(bodyLit));
    }
    bodySatIfLitSat.insert(negation(createLiteral(ruleBodyAtomID)));
    nogoodset.addNogood(bodySatIfLitSat);

    // 2. body must not be true if a literal is false
    BOOST_FOREACH(ID bodyLit, ruleBody) {
        Nogood bodyFalseIfLitFalse;
        bodyFalseIfLitFalse.insert(createLiteral(ruleBodyAtomID));
        bodyFalseIfLitFalse.insert(negation(createLiteral(bodyLit)));
        nogoodset.addNogood(bodyFalseIfLitFalse);
    }
}


Set<std::pair<ID, ID> > InternalGroundASPSolver::createShiftedProgram()
{

    // create for each rule of kind
    //	a(1) v ... v a(m) :- B
    // all shifted rules of kind
    //	a(i) :- B, -a(1), ..., -a(i-1), -a(i+1), ..., a(m)
    // for all 1 <= i <= m

    DBGLOG(DBG, "Creating shifted program");

    Set<std::pair<ID, ID> > shiftedProg;
    BOOST_FOREACH (ID ruleID, program.getGroundProgram().idb) {

        const Rule& r = reg->rules.getByID(ruleID);

        // real shifted rule?
        if (r.head.size() > 1) {
            BOOST_FOREACH (ID headLit, r.head) {
                // take body of r and current head literal
                Tuple singularHead;
                singularHead.push_back(createLiteral(headLit));
                Rule shiftedRule(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR, singularHead, r.body);

                // add negations of all other head literals to body
                BOOST_FOREACH (ID otherHeadLit, r.head) {
                    if (otherHeadLit != headLit) {
                        shiftedRule.body.push_back(negation(createLiteral(otherHeadLit)));
                    }
                }

                // create a new body atom
                DBGLOG(DBG, "Creating real shifted rule: " << shiftedRule);
                ID shiftedRuleID = reg->rules.getIDByElement(shiftedRule);
                if (shiftedRuleID == ID_FAIL) {
                    shiftedRuleID = reg->rules.storeAndGetID(shiftedRule);
                }
                ID ruleBodyAtomID = createNewBodyAtom();

                shiftedProg.insert(std::pair<ID, ID>(shiftedRuleID, ruleBodyAtomID));
            }
        }
        else {
            // rule was already present in original program and has already a body literal
            DBGLOG(DBG, "Creating shifted rule which was already present in original program: " << r);

            shiftedProg.insert(std::pair<ID, ID>(ruleID, createLiteral(bodyAtomOfRule[ruleID.address])));
        }
    }

    DBGLOG(DBG, "Creating shifted program finished");

    return shiftedProg;
}


void InternalGroundASPSolver::createSingularLoopNogoods(InterpretationConstPtr frozen)
{

    DBGLOG(DBG, "Creating singular loop nogoods");

    typedef std::pair<ID, ID> RulePair;
    Set<RulePair> shiftedProgram = createShiftedProgram();

    // create for each real shifted rule the nogoods which associate the rule body with the body atom
    // (shifted rules which were already present in the original program were already handled before)
    BOOST_FOREACH (RulePair pair, shiftedProgram) {
        if (!contains(program.getGroundProgram().idb, pair.first)) {
            const Rule& r = reg->rules.getByID(pair.first);
            createNogoodsForRuleBody(pair.second, r.body);
        }
    }

    // an atom must not be true if the bodies of all supporting shifted rules are false
    BOOST_FOREACH (IDAddress litadr, ordinaryFacts) {
        // only for atoms which are no facts
        if (!program.getGroundProgram().edb->getFact(litadr) && (!frozen || !frozen->getFact(litadr))) {
            Nogood supNogood;
            supNogood.insert(createLiteral(litadr));

            // for all supporting rules
            BOOST_FOREACH(RulePair pair, shiftedProgram) {
                const Rule& r = reg->rules.getByID(pair.first);
                if (r.head.size() > 0 && (r.head[0].address == litadr)) {
                    supNogood.insert(negation(createLiteral(pair.second)));
                }
            }
            nogoodset.addNogood(supNogood);
        }
    }

    DBGLOG(DBG, "Nogoods with singular loop nogoods: " << nogoodset);
}


void InternalGroundASPSolver::resizeVectors()
{

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


void InternalGroundASPSolver::computeClarkCompletion()
{

    // compute completion
    BOOST_FOREACH (ID ruleID, program.getGroundProgram().idb) {
        if (ruleID.isWeakConstraint()) throw GeneralError("Internal solver does not support weak constraints");

        ID ruleBodyAtomID = createNewBodyAtom();
        bodyAtomOfRule[ruleID.address] = ruleBodyAtomID.address;
        createNogoodsForRule(ruleBodyAtomID, ruleID);
    }

    DBGLOG(DBG, "Created Clark's completion: " << nogoodset);
}


void InternalGroundASPSolver::setEDB()
{

    DBGLOG(DBG, "Setting EDB");

    // set all facts at decision level 0 without cause
    bm::bvector<>::enumerator en = program.getGroundProgram().edb->getStorage().first();
    bm::bvector<>::enumerator en_end = program.getGroundProgram().edb->getStorage().end();

    while (en < en_end) {
        if (ordinaryFacts.count(*en) > 0) {
            setFact(createLiteral(*en), 0, -1);
        }
        ++en;
    }
}


void InternalGroundASPSolver::computeDepGraph()
{

    // all literals are nodes
    BOOST_FOREACH (IDAddress litadr, ordinaryFacts) {
        depNodes[litadr] = boost::add_vertex(litadr, depGraph);
    }

    // go through all rules
    BOOST_FOREACH (ID ruleID, program.getGroundProgram().idb) {
        const Rule& rule = reg->rules.getByID(ruleID);

        // add an arc from all head literals to all positive body literals
        BOOST_FOREACH (ID headLiteral, rule.head) {
            BOOST_FOREACH (ID bodyLiteral, rule.body) {
                if (!bodyLiteral.isNaf()) {
                    boost::add_edge(depNodes[headLiteral.address], depNodes[bodyLiteral.address], depGraph);
                }
            }
        }
    }
}


void InternalGroundASPSolver::computeStronglyConnectedComponents()
{

    // find strongly connected components in the dependency graph using boost
    std::vector<int> componentMap(depNodes.size());
    int num = boost::strong_components(depGraph, boost::make_iterator_property_map(componentMap.begin(), get(boost::vertex_index, depGraph)));

    // translate into real map
    depSCC = std::vector<Set<IDAddress> >(num);
    Node nodeNr = 0;
    BOOST_FOREACH (int componentOfNode, componentMap) {
        depSCC[componentOfNode].insert(depGraph[nodeNr]);
        componentOfAtom[depGraph[nodeNr]] = componentOfNode;
        nodeNr++;
    }

    // store which atoms occur in non-singular components
    BOOST_FOREACH (IDAddress litadr, ordinaryFacts) {
        if (depSCC[componentOfAtom[litadr]].size() > 1) {
            nonSingularFacts.insert(litadr);
        }
    }

    #ifndef NDEBUG
    std::stringstream compStr;
    bool firstC = true;
    BOOST_FOREACH (Set<IDAddress> component, depSCC) {
        if (!firstC) compStr << ", ";
        firstC = false;
        compStr << "{";
        bool firstL = true;
        BOOST_FOREACH (IDAddress litadr, component) {
            if (!firstL) compStr << ", ";
            firstL = false;
            compStr << litadr;
        }
        compStr << "}";
    }
    DBGLOG(DBG, "Program components: " << compStr.str());
    DBGLOG(DBG, "All atoms: " << toString(allAtoms));
    DBGLOG(DBG, "All ordinary atoms: " << toString(ordinaryFacts));
    DBGLOG(DBG, "Ordinary atoms in non-singular components: " << toString(nonSingularFacts));
    #endif
}


void InternalGroundASPSolver::initSourcePointers()
{

    DBGLOG(DBG, "Initialize source pointers");

    // initially, all atoms in non-singular components, except facts, are unfounded
    BOOST_FOREACH (IDAddress litadr, ordinaryFacts) {
        if (program.getGroundProgram().edb->getFact(litadr)) {
            // store pseudo source rule ID_FAIL to mark that this is a fact and founded by itself
            sourceRule[litadr] = ID_FAIL;
        }
        else {
            // all non-facts in non-singular components are unfounded
            if (nonSingularFacts.count(litadr) > 0) {
                unfoundedAtoms.insert(litadr);
            }
        }
    }

    DBGLOG(DBG, "Initially unfounded atoms: " << toString(unfoundedAtoms));
}


void InternalGroundASPSolver::initializeLists(InterpretationConstPtr frozen)
{

    // determine the set of all facts and a literal index
    BOOST_FOREACH (ID ruleID, program.getGroundProgram().idb) {
        const Rule& r = reg->rules.getByID(ruleID);

        // remember this rule for each contained literal
        for (std::vector<ID>::const_iterator lIt = r.head.begin(); lIt != r.head.end(); ++lIt) {
            if (lIt->isOrdinaryNongroundAtom()) throw GeneralError("Got nonground program");

            rulesWithPosHeadLiteral[lIt->address].insert(ruleID);
            // collect all facts
            allAtoms.insert(lIt->address);
            ordinaryFacts.insert(lIt->address);
        }
        for (std::vector<ID>::const_iterator lIt = r.body.begin(); lIt != r.body.end(); ++lIt) {
            if (lIt->isOrdinaryNongroundAtom()) throw GeneralError("Got nonground program");

            if (!lIt->isNaf()) {
                rulesWithPosBodyLiteral[lIt->address].insert(ruleID);
            }
            else {
                rulesWithNegBodyLiteral[lIt->address].insert(ruleID);
            }
            // collect all facts
            allAtoms.insert(lIt->address);
            ordinaryFacts.insert(lIt->address);
        }
    }

    // include facts in the list of all atoms
    bm::bvector<>::enumerator en = program.getGroundProgram().edb->getStorage().first();
    bm::bvector<>::enumerator en_end = program.getGroundProgram().edb->getStorage().end();
    while (en < en_end) {
        allAtoms.insert(*en);
        ordinaryFacts.insert(*en);
        ++en;
    }

    // frozen atoms in the list of all atoms
    if (!!frozen) {
        en = frozen->getStorage().first();
        en_end = frozen->getStorage().end();
        while (en < en_end) {
            allAtoms.insert(*en);
            ordinaryFacts.insert(*en);
            en++;
        }
    }

    // built an interpretation of ordinary facts
    ordinaryFactsInt = InterpretationPtr(new Interpretation(reg));
    BOOST_FOREACH (IDAddress idadr, ordinaryFacts) {
        ordinaryFactsInt->setFact(idadr);
    }
}


void InternalGroundASPSolver::setFact(ID fact, int dl, int cause = -1)
{
    CDNLSolver::setFact(fact, dl, cause);
    updateUnfoundedSetStructuresAfterSetFact(fact);
}


void InternalGroundASPSolver::clearFact(IDAddress litadr)
{
    CDNLSolver::clearFact(litadr);
    updateUnfoundedSetStructuresAfterClearFact(litadr);
}


void InternalGroundASPSolver::removeSourceFromAtom(IDAddress litadr)
{

    // check if the literal has currently a source rule
    if (sourceRule.find(litadr) != sourceRule.end()) {
        if (sourceRule[litadr] != ID_FAIL) {
            ID sourceRuleID = sourceRule[litadr];
            DBGLOG(DBG, "Literal " << litadr << " canceled its source pointer to rule " << sourceRuleID.address);
            foundedAtomsOfBodyAtom[bodyAtomOfRule[sourceRuleID.address]].erase(litadr);
            sourceRule.erase(litadr);
        }
    }
}


void InternalGroundASPSolver::addSourceToAtom(IDAddress litadr, ID rule)
{
    DBGLOG(DBG, "Literal " << litadr << " sets a source pointer to " << rule.address);
    sourceRule[litadr] = rule;
    foundedAtomsOfBodyAtom[bodyAtomOfRule[rule.address]].insert(litadr);
}


Set<IDAddress> InternalGroundASPSolver::getDependingAtoms(IDAddress litadr)
{

    // litadr became unfounded; now compute all atoms which depend on litadr and are
    // therefore become unfounded too
    Set<IDAddress> dependingAtoms;

    // go through all rules which contain litadr in their body
    BOOST_FOREACH (ID ruleID, rulesWithPosBodyLiteral[litadr]) {

        // go through all atoms which use this rule as source
        BOOST_FOREACH (IDAddress dependingAtom, foundedAtomsOfBodyAtom[bodyAtomOfRule[ruleID.address]]) {
            // this atom is depends on litadr
            dependingAtoms.insert(dependingAtom);
        }
    }

    return dependingAtoms;
}


void InternalGroundASPSolver::getInitialNewlyUnfoundedAtomsAfterSetFact(ID fact, Set<IDAddress>& newlyUnfoundedAtoms)
{

    // if the fact is a falsified body literal, all atoms which depend on it become unfounded
    if (fact.isNaf()) {
        if (foundedAtomsOfBodyAtom.find(fact.address) != foundedAtomsOfBodyAtom.end()) {
            BOOST_FOREACH (IDAddress dependingAtom, foundedAtomsOfBodyAtom[fact.address]) {
                DBGLOGD(DBG, "" << dependingAtom << " is initially unfounded because the body of its source rule became false");
                newlyUnfoundedAtoms.insert(dependingAtom);
            }
        }
    }

    // if the fact is a satisfied head literal of a rule, all head literals which use it as source rule and
    // (i) which were set later; or
    // (ii) which are true in a different component
    // become unfounded
    else {
        // for all rules which contain the fact in their head
        if (rulesWithPosHeadLiteral.find(fact.address) != rulesWithPosHeadLiteral.end()) {
            BOOST_FOREACH (ID ruleID, rulesWithPosHeadLiteral[fact.address]) {
                const Rule& r = reg->rules.getByID(ruleID);

                // all other head literals cannot use this rule as source, if
                BOOST_FOREACH (ID otherHeadLit, r.head) {
                    if (otherHeadLit.address != fact.address && sourceRule[otherHeadLit.address] == ruleID) {
                        // (i) they were set to true later
                        // TODO: maybe we have to compare the order of assignments instead of the decision levels
                        //       or we can use the decision level (would be much more efficient)
                        if (satisfied(createLiteral(otherHeadLit.address)) &&
                            getAssignmentOrderIndex(otherHeadLit.address) > getAssignmentOrderIndex(fact.address)
                        ) {
                            DBGLOGD(DBG, "" << otherHeadLit.address << " is initially unfounded because " << otherHeadLit.address <<
                                " occurs in the head of its source rule and became true on a lower decision level");
                            newlyUnfoundedAtoms.insert(otherHeadLit.address);
                        }

                        // (ii) they belong to a different component
                        if (componentOfAtom[otherHeadLit.address] != componentOfAtom[fact.address]) {
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


void InternalGroundASPSolver::updateUnfoundedSetStructuresAfterSetFact(ID fact)
{

    DBGLOGD(DBG, "Updating set of atoms without source pointers, currently: " << toString(unfoundedAtoms));

    #ifndef NDEBUG
    {
        std::stringstream ss;
        typedef std::pair<IDAddress, Set<IDAddress> > SourcePair;
        BOOST_FOREACH (SourcePair pair, foundedAtomsOfBodyAtom) {
            ss << "Body atom " << pair.first << " is source for " << toString(pair.second) << "; ";
        }
        DBGLOGD(DBG, "Source pointer: " << ss.str());
    }
    #endif

    // atom does not need a source pointer if it is assigned to false
    if (fact.isNaf()) {
        DBGLOGD(DBG, "Literal " << fact.address << " was assigned to false and is not unfounded anymore");
        removeSourceFromAtom(fact.address);
        unfoundedAtoms.erase(fact.address);
    }

    // update the unfounded data structures
    DBGLOGD(DBG, "Computing initially newly unfounded atoms");
    Set<IDAddress> newlyUnfoundedAtoms;
    getInitialNewlyUnfoundedAtomsAfterSetFact(fact, newlyUnfoundedAtoms);

    while (newlyUnfoundedAtoms.size() > 0) {
        Set<IDAddress> nextNewlyUnfoundedAtoms;

        DBGLOGD(DBG, "Collecting depending atoms of " << toString(newlyUnfoundedAtoms));

        BOOST_FOREACH (IDAddress newlyUnfoundedAtom, newlyUnfoundedAtoms) {
            // only atoms which occur in non-singular components
            // (singular atoms are already handled by static loop nogoods)
            if (nonSingularFacts.count(newlyUnfoundedAtom) > 0) {
                // only atoms which are not already unfounded or false
                if (!falsified(createLiteral(newlyUnfoundedAtom)) && (unfoundedAtoms.count(newlyUnfoundedAtom) == 0)) {
                    // only atoms which occur in a component that depends on unfounded atoms
                    if (intersect(depSCC[componentOfAtom[newlyUnfoundedAtom]], unfoundedAtoms).size() > 0 ||
                    intersect(depSCC[componentOfAtom[newlyUnfoundedAtom]], newlyUnfoundedAtoms).size() > 0) {
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


void InternalGroundASPSolver::updateUnfoundedSetStructuresAfterClearFact(IDAddress litadr)
{

    DBGLOGD(DBG, "Updating set of atoms without source pointers, currently: " << toString(unfoundedAtoms));

    // fact becomes unfounded if it has no source pointer
    // and if it is non-singular
    if (nonSingularFacts.count(litadr) > 0) {
        if (sourceRule.find(litadr) == sourceRule.end()) {
            unfoundedAtoms.insert(litadr);
        }
    }

    DBGLOGD(DBG, "Updated set of unfounded atoms: " << toString(unfoundedAtoms));
}


ID InternalGroundASPSolver::getPossibleSourceRule(const Set<ID>& ufs)
{

    DBGLOG(DBG, "Computing externally supporting rules for " << toString(ufs));

    Set<ID> extSup = getExternalSupport(ufs);

    #ifndef NDEBUG
    {
        std::stringstream ss;
        ss << "Externally supporting rules of potential ufs: {";
        bool first = true;
        BOOST_FOREACH (ID ruleID, extSup) {
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
    BOOST_FOREACH (ID extRuleID, extSup) {
        Set<ID> satInd = satisfiesIndependently(extRuleID, ufs);
        bool skipRule = false;
        BOOST_FOREACH (ID indSatLit, satInd) {
            if (satisfied(indSatLit)) {
                skipRule = true;
                break;
            }
        }
        if (!skipRule) {
            DBGLOG(DBG, "Found possible source rule: " << extRuleID.address);
            return extRuleID;
        }
        else {
            DBGLOG(DBG, "Rule " << extRuleID.address << " is removed (independently satisfied)");
        }
    }

    return ID_FAIL;
}


// a head atom uses the rule as source, if
// 1. the atom is currently unfounded
// 2. no other head literal was set to true earlier
bool InternalGroundASPSolver::useAsNewSourceForHeadAtom(IDAddress headAtom, ID sourceRuleID)
{

    DBGLOG(DBG, "Checking if " << headAtom << " uses rule " << sourceRuleID.address << " as source");
    if (unfoundedAtoms.count(headAtom) == 0) {
        DBGLOG(DBG, "No: " << headAtom << " is currently not unfounded");
        return false;
    }

    const Rule& sourceRule = reg->rules.getByID(sourceRuleID);

    // only the literal which was set first can use a rule as source:
    //
    // if headLit is currently assigned, other head literals must not be set to true earlier
    // if headLit is currently unassigned, other head literals must not be true at all
    if (assigned(headAtom)) {
        int headLitDecisionLevel = decisionlevel[headAtom];
        BOOST_FOREACH (ID otherHeadLit, sourceRule.head) {
            if (otherHeadLit.address != headAtom) {
                if (satisfied(otherHeadLit)) {
                    // TODO: maybe we have to compare the order of assignments instead of the decision levels
                    //       or we can use the decision level (would be much more efficient)
                    if (
                        getAssignmentOrderIndex(otherHeadLit.address) < getAssignmentOrderIndex(headAtom)
                    ) {
                        DBGLOG(DBG, "No: Head literal " << otherHeadLit.address << " was set to true on a lower decision level");
                        return false;
                    }
                }
            }
        }
    }
    else {
        BOOST_FOREACH (ID otherHeadLit, sourceRule.head) {
            if (otherHeadLit.address != headAtom) {
                if (satisfied(otherHeadLit)) {
                    DBGLOG(DBG, "No: Head literal " << otherHeadLit.address << " was already set to true, whereas " << headAtom << " is unassigned");
                    return false;
                }
            }
        }
    }
    return true;
}


Set<ID> InternalGroundASPSolver::getUnfoundedSet()
{

    DBGLOG(DBG, "Currently unfounded atoms: " << toString(unfoundedAtoms));

    Set<ID> ufs(5, 10);
    while (unfoundedAtoms.size() > 0) {
        IDAddress atom = *(unfoundedAtoms.begin());
        ufs.clear();
        ufs.insert(createLiteral(atom));
        do {
            DBGLOG(DBG, "Trying to build an unfounded set over " << toString(ufs));

            // find a rule which externally supports ufs and
            // which is not satisfied independently of ufs
            ID supportingRuleID = getPossibleSourceRule(ufs);

            // if no rule survives, ufs is indeed unfounded
            if (supportingRuleID == ID_FAIL) return ufs;

            // check if this rule depends on unfounded atoms from atom's component
            const Rule& supportingRule = reg->rules.getByID(supportingRuleID);
            bool dependsOnUnfoundedAtoms = false;
            BOOST_FOREACH (ID bodyLit, supportingRule.body) {
                if (!bodyLit.isNaf() && (unfoundedAtoms.count(bodyLit.address) > 0) && (depSCC[componentOfAtom[atom]].count(bodyLit.address) > 0)) {
                    // extend the unfounded set by this atom
                    DBGLOG(DBG, "Rule depends on unfounded " << litToString(bodyLit) << " --> adding to ufs");
                    ufs.insert(createLiteral(bodyLit));
                    dependsOnUnfoundedAtoms = true;
                }
            }

            // if the rule does not depend on unfounded atoms, it can be used as the new source for its head atom(s)
            if (!dependsOnUnfoundedAtoms) {
                BOOST_FOREACH (ID headLit, supportingRule.head) {

                    if (useAsNewSourceForHeadAtom(headLit.address, supportingRuleID)) {
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


bool InternalGroundASPSolver::doesRuleExternallySupportLiteral(ID ruleID, ID lit, const Set<ID>& s)
{

    const Rule& rule = reg->rules.getByID(ruleID);

    // check if the rule supports the literal
    bool supportsLit = false;
    BOOST_FOREACH (ID headLit, rule.head) {
        if (headLit.address == lit.address) {
            supportsLit = true;
            break;
        }
    }
    if (!supportsLit) return false;

    // check if the support is external wrt s
    BOOST_FOREACH (ID sLit, s) {
        if (contains(rule.body, sLit)) {
            return false;
        }
    }

    return true;
}


Set<ID> InternalGroundASPSolver::getExternalSupport(const Set<ID>& s)
{

    Set<ID> extRules;
    DBGLOG(DBG, "Computing externally supporting rules for set " << toString(s));

    // go through all rules which contain one of s in their head
    BOOST_FOREACH (ID lit, s) {

        const Set<ID>& containingRules = rulesWithPosHeadLiteral[lit.address];

        BOOST_FOREACH (ID ruleID, containingRules) {

            // check if none of the elements of s occurs in the body of r
            if (doesRuleExternallySupportLiteral(ruleID, lit, s)) {
                DBGLOG(DBG, "Found external rule " << ruleID.address << " for set " << toString(s));
                extRules.insert(ruleID);
            }
            else {
                DBGLOGD(DBG, "Rule " << ruleID.address << " contains " << lit.address << " but does not externally support it wrt " << toString(s));
            }
        }
    }
    return extRules;
}


Set<ID> InternalGroundASPSolver::satisfiesIndependently(ID ruleID, const Set<ID>& y)
{

    const Rule& rule = reg->rules.getByID(ruleID);

    // compute all literals which satisfy the rule independently of set y:
    // either (i) the body of rule is false; or
    //        (ii) some head literal, which is not in y, is true
    Set<ID> indSat;
                                 // (i)
    indSat.insert(createLiteral(bodyAtomOfRule[ruleID.address], false));
                                 // (ii)
    BOOST_FOREACH (ID headLiteral, rule.head) {
        if (y.count(createLiteral(headLiteral.address)) == 0) {
            indSat.insert(createLiteral(headLiteral.address));
        }
    }
    DBGLOG(DBG, "Rule " << ruleID.address << " is satisfied independently from " << toString(y) << " by " << toString(indSat));
    return indSat;
}


Nogood InternalGroundASPSolver::getLoopNogood(const Set<ID>& ufs)
{

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
    BOOST_FOREACH (ID ruleID, extSup) {
                                 // (i)
        Set<ID> satInd = satisfiesIndependently(ruleID, ufs);
        BOOST_FOREACH (ID indLit, satInd) {
                                 // (ii)
            if (satisfied(indLit)) {
                loopNogood.insert(createLiteral(indLit));
                break;
            }
        }
    }
    DBGLOG(DBG, "Loop nogood for " << toString(ufs) << " is " << loopNogood);

    return loopNogood;
}


ID InternalGroundASPSolver::createNewAtom(ID predID)
{
    OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
    atom.tuple.push_back(predID);
    return createLiteral(reg->storeOrdinaryGAtom(atom).address);
}


ID InternalGroundASPSolver::createNewBodyAtom()
{
    std::stringstream bodyPred;
    bodyPred << bodyAtomPrefix << bodyAtomNumber;
    DBGLOG(DBG, "Creating body atom " << bodyPred.str());
    bodyAtomNumber++;
    ID bodyAtom = createNewAtom(reg->getNewConstantTerm("body"));
    allAtoms.insert(bodyAtom.address);
    return bodyAtom;
}


std::string InternalGroundASPSolver::toString(const Set<ID>& lits)
{
    std::stringstream ss;
    ss << "{";
    bool first = true;
    BOOST_FOREACH (ID lit, lits) {
        if (!first) ss << ", ";
        if ((lit.kind & ID::NAF_MASK) != 0) ss << "-";
        ss << lit.address;
        first = false;
    }
    ss << "}";
    return ss.str();
}


std::string InternalGroundASPSolver::toString(const Set<IDAddress>& lits)
{
    std::stringstream ss;
    ss << "{";
    bool first = true;
    BOOST_FOREACH (IDAddress lit, lits) {
        if (!first) ss << ", ";
        ss << lit;
        first = false;
    }
    ss << "}";
    return ss.str();
}


std::string InternalGroundASPSolver::toString(const std::vector<IDAddress>& lits)
{
    std::stringstream ss;
    ss << "{";
    for (std::vector<IDAddress>::const_iterator it = lits.begin(); it != lits.end(); ++it) {
        if (it != lits.begin()) ss << ", ";
        ss << *it;
    }
    ss << "}";
    return ss.str();
}


InterpretationPtr InternalGroundASPSolver::outputProjection(InterpretationConstPtr intr)
{

    if (intr == InterpretationPtr()) {
        return InterpretationPtr();
    }
    else {
        InterpretationPtr answer = InterpretationPtr(new Interpretation(reg));
        answer->add(*intr);
        answer->bit_and(*ordinaryFactsInt);
        if (program.getGroundProgram().mask != InterpretationConstPtr()) {
            answer->getStorage() -= program.getGroundProgram().mask->getStorage();
        }
        return answer;
    }
}


InternalGroundASPSolver::InternalGroundASPSolver(ProgramCtx& c, const AnnotatedGroundProgram& p, InterpretationConstPtr frozen) : CDNLSolver(c, NogoodSet()), program(p), bodyAtomPrefix(std::string("body_")), bodyAtomNumber(0), firstmodel(true), cntDetectedUnfoundedSets(0), modelCount(0)
{
    DBGLOG(DBG, "Internal Ground ASP Solver Init");

    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidsolvertime, "Solver time");
    reg = ctx.registry();

    resizeVectors();
    initializeLists(frozen);
    computeClarkCompletion();
    createSingularLoopNogoods(frozen);
    resizeVectors();
    initWatchingStructures();
    computeDepGraph();
    computeStronglyConnectedComponents();
    initSourcePointers();
    setEDB();
}


void InternalGroundASPSolver::addProgram(const AnnotatedGroundProgram& p, InterpretationConstPtr frozen)
{
    throw GeneralError("Internal grounder does not support incremental extension of the program");
}

/**
  * In order to compute an inconsistency reason the program must be inconsistent.
  * This condition is stronger than the condition of getNextModel() returning NULL:
  *   getNextModel() might also return if there are _no more_ models, while for inconsistent programs it would return NULL even after a restart.
  */
Nogood InternalGroundASPSolver::getInconsistencyCause(InterpretationConstPtr explanationAtoms){

    loadAddedNogoods();
    if ((!getNextModel() && modelCount == 0) ||
        /** @TODO: FOR TESTIGN PURPOSES ONLY
          * If (for whatever reason) there is a contradictory nogood after getNextModel() returns NULL, we can compute a reason why there is no *further* model (flipping all decision literals leads to this conflict);
          *   if all previous models have been rejected due to incompatibility (which needs to be checked by the caller), then this is also a reason for inconsistency. */
        (!getNextModel() && contradictoryNogoods.size() > 0)) {
        if (!explanationAtoms) return Nogood(); // explanation is trivial in this case

        // start from a conflicting nogood
        // heuristics: choose a conflicting nogood with minimal cardinality
        int conflictNogoodIndex = 0;
        BOOST_FOREACH (int i, contradictoryNogoods) {
            if (nogoodset.getNogood(i).size() < nogoodset.getNogood(contradictoryNogoods[conflictNogoodIndex]).size()) conflictNogoodIndex = i;
        }
        Nogood violatedNogood = nogoodset.getNogood(contradictoryNogoods[conflictNogoodIndex]);

#ifndef NDEBUG
        std::stringstream debugoutput;
        DBGLOG(DBG, "[IR] getInconsistencyCause, last interpretation before detecting inconsistency: " << *interpretation);
        DBGLOG(DBG, "[IR] getInconsistencyCause, current implication graph:" << std::endl << "[IR] " << getImplicationGraphAsDotString());
		DBGLOG(DBG, "[IR] getInconsistencyCause, explanation atoms: " << *explanationAtoms);
        std::string indent = "";
		debugoutput << "[IR] getInconsistencyCause, computation:" << std::endl << "[IR:INIT] " << violatedNogood.getStringRepresentation(reg) << std::endl;
#endif

        // resolve back to explanationAtoms
        std::vector<ID> removeLits;
        int resolveWithNogoodIndex = -1;
        ID resolvedLiteral = ID_FAIL;
        do {
            removeLits.clear();
            resolveWithNogoodIndex = -1;
            resolvedLiteral = ID_FAIL;
            // 1. check if the violated nogood contains implied literals; in this case mark for resolving
            // 2. check if the violated nogood contains not implied literals other than from explanationAtoms; in this case mark for removal
		    BOOST_FOREACH (ID lit, violatedNogood) {
                assert (decisionlevel[lit.address] == 0 && "found literal assigned on decisionlevel > 0");
                // check if there are literals other than from explanationAtoms
                if (!explanationAtoms->getFact(lit.address)) {
                    // if it is an implied literal, resolve with its reason, otherwise it is a fact and we remove it
				    if (cause[lit.address] != -1){
                        if (resolveWithNogoodIndex == -1){
                            resolveWithNogoodIndex = cause[lit.address];
                            resolvedLiteral = lit;
                        }
				    }else{
                        removeLits.push_back(lit);
                    }
                }
			}
            // remove the literals marked for removal (actually, since removal from vectors is expensive, create a new vector but skip removed literals)
            Nogood newNogood;
            int nextRemovedLit = 0;
			BOOST_FOREACH (ID lit, violatedNogood){
				if (nextRemovedLit < removeLits.size() && lit == removeLits[nextRemovedLit]){
                    nextRemovedLit++;
				}else{
                    // keep
                    newNogood.insert(lit);
                }
            }
            // possibly resolve
            if (resolveWithNogoodIndex != -1){
                violatedNogood = resolve(newNogood, nogoodset.getNogood(resolveWithNogoodIndex), resolvedLiteral.address);
#ifndef NDEBUG
                debugoutput << "[IR:RNEX] " << indent << newNogood.getStringRepresentation(reg) << std::endl;
				debugoutput << "[IR:RLIT] " << indent << (resolvedLiteral.isNaf() ? "-" : "") << printToString<RawPrinter>(ID::atomFromLiteral(resolvedLiteral), reg) << std::endl;
				debugoutput << "[IR:IMPL] " << indent << nogoodset.getNogood(resolveWithNogoodIndex).getStringRepresentation(reg) << std::endl;
                indent = indent + "     ";
                debugoutput << "[IR:RSVT] " << indent << violatedNogood.getStringRepresentation(reg) << std::endl;
#endif
			}else{
                violatedNogood = newNogood;
            }
		}while(removeLits.size() > 0 || resolveWithNogoodIndex != -1);

        // the nogood contains now only literals which have not been implied and which are from explanationAtoms
#ifndef NDEBUG
        debugoutput << "[IR:FINL] " << indent << violatedNogood.getStringRepresentation(reg) << std::endl;
        DBGLOG(DBG, debugoutput.str() << std::endl << "INIT ... Initially violated nogood" << std::endl << "RNEX ... Eliminated non-implied literals other than explanation atoms" << std::endl << "RLIT ... Resolved literal" << std::endl << "IMPL ... Implicant of the resolved literal" << std::endl << "RSVT ... Resolvent" << std::endl << "FINL ... Final explanation nogood");

        // the assertion could already be checked above, but we want to trace the algorithm also in case of errors
        BOOST_FOREACH (ID lit, violatedNogood) {
            assert(interpretation->getFact(lit.address) != lit.isNaf() && "nogood supposed to be violated is not");
        }
#endif
        return violatedNogood;
    }
    throw GeneralError("getInconsistencyCause can only be called for inconsistent instances (getNextModel() has to return NULL at first call)");
}

void InternalGroundASPSolver::addNogoodSet(const NogoodSet& ns, InterpretationConstPtr frozen)
{
    throw GeneralError("Internal CDNL solver does not support incremental extension of the instance");
}


void InternalGroundASPSolver::restartWithAssumptions(const std::vector<ID>& assumptions)
{

    // reset
    std::vector<IDAddress> toClear;
    bm::bvector<>::enumerator en = assignedAtoms->getStorage().first();
    bm::bvector<>::enumerator en_end = assignedAtoms->getStorage().end();
    while (en < en_end) {
        toClear.push_back(*en);
        en++;
    }
    BOOST_FOREACH (IDAddress adr, toClear) clearFact(adr);
    /*

      DBGLOG(DBG, "Resetting solver");
      interpretation.reset(new Interpretation(ctx.registry()));
      assigned.reset(new Interpretation(ctx.registry()));
      changed.reset(new Interpretation(ctx.registry()));
      currentDL = 0;
      exhaustedDL = 0;

      initWatchingStructures();
    */
    // set assumptions at DL=0
    DBGLOG(DBG, "Setting assumptions");
    BOOST_FOREACH (ID a, assumptions) {
        if (allAtoms.contains(a.address)) {
            setFact(a, 0);
        }
    }

    setEDB();
}


void InternalGroundASPSolver::addPropagator(PropagatorCallback* pb)
{
    propagator.insert(pb);
}


void InternalGroundASPSolver::removePropagator(PropagatorCallback* pb)
{
    propagator.erase(pb);
}


void InternalGroundASPSolver::setOptimum(std::vector<int>& optimum)
{
    // not supported: ignore the call
    LOG(INFO,"InternalGroundASPSolver::setOptimum not supported!");
}


InterpretationPtr InternalGroundASPSolver::getNextModel()
{
#ifndef NDEBUG
    DBGLOG(DBG, "getNextModel, current implication graph:" << std::endl << getImplicationGraphAsDotString());
#endif

    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidsolvertime, "Solver time");
    Nogood violatedNogood;

    if (!firstmodel && complete()) {
        if (currentDL == 0) {
            return InterpretationPtr();
        }
        else {
            flipDecisionLiteral();
        }
    }
    firstmodel = false;

    // if set to true, the loop will run even if the interpretation is already complete
    bool anotherIterationEvenIfComplete = false;
    // (needed to check if newly added nogood (e.g. by external learners) are satisfied)
    while (!complete() || anotherIterationEvenIfComplete) {
        anotherIterationEvenIfComplete = false;
        if (!unitPropagation(violatedNogood)) {
            if (currentDL == 0) {
                // no answer set
                return InterpretationPtr();
            }
            else {
                if (currentDL > exhaustedDL) {
                    DBGLOG(DBG, "Conflict analysis");
                    // backtrack
                    Nogood learnedNogood;
                    int k = currentDL;
                    analysis(violatedNogood, learnedNogood, k);
                    recentConflicts.push_back(addNogoodAndUpdateWatchingStructures(learnedNogood));
                                 // do not jump below exhausted level, this could lead to regeneration of models
                    currentDL = k > exhaustedDL ? k : exhaustedDL;
                    backtrack(currentDL);
                }
                else {
                    flipDecisionLiteral();
                }
            }
        }
        else {
            Set<ID> ufs = getUnfoundedSet();

            if (ufs.size() > 0) {
                DBGLOG(DBG, "Found UFS: " << toString(ufs));
                #ifndef NDEBUG
                ++cntDetectedUnfoundedSets;
                #endif

                Nogood loopNogood = getLoopNogood(ufs);
                addNogoodAndUpdateWatchingStructures(loopNogood);
                anotherIterationEvenIfComplete = true;
            }
            else {
                // no ufs
                DBGLOG(DBG, "No unfounded set exists");

                DBGLOG(DBG, "Calling external learner");
                int nogoodCount = nogoodset.getNogoodCount();
                BOOST_FOREACH (PropagatorCallback* cb, propagator) {
                    DBGLOG(DBG, "Calling external learners with interpretation: " << *interpretation);
                    cb->propagate(interpretation, assignedAtoms, changedAtoms);
                }
                // add new nogoods
                int ngc = nogoodset.getNogoodCount();
                loadAddedNogoods();
                if (ngc != nogoodset.getNogoodCount()) anotherIterationEvenIfComplete = true;
                changedAtoms->clear();

                if (nogoodset.getNogoodCount() != nogoodCount) {
                    DBGLOG(DBG, "Learned something");
                }
                else {
                    DBGLOG(DBG, "Did not learn anything");

                    if (assignedAtoms->getStorage().count() > allAtoms.size()){
                        std::cout << "All: ";
                        BOOST_FOREACH (IDAddress a, allAtoms) { std::cout << printToString<RawPrinter>(reg->ogatoms.getIDByAddress(a), reg) << " "; }
                        std::cout << " (" << allAtoms.size() << ")" << std::endl;
                    }

                    if (!complete()) {
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

    InterpretationPtr icp = outputProjection(interpretation);
    modelCount++;
    return icp;
}


int InternalGroundASPSolver::getModelCount()
{
    return modelCount;
}


std::string InternalGroundASPSolver::getStatistics()
{

    #ifndef NDEBUG
    std::stringstream ss;
    ss  << CDNLSolver::getStatistics() << std::endl
        << "Detected unfounded sets: " << cntDetectedUnfoundedSets;
    return ss.str();
    #else
    std::stringstream ss;
    ss << "Only available in debug mode";
    return ss.str();
    #endif
}

const NogoodSet& InternalGroundASPSolver::getNogoodStorage(){
    return nogoodset;
}

std::string InternalGroundASPSolver::getImplicationGraphAsDotString(){

    // create debug output graph
    std::stringstream dot;

    // export implication graph
    dot << "digraph G { ";
    BOOST_FOREACH (IDAddress adr, this->allAtoms){
        if (!assignedAtoms->getFact(adr)) continue;

        dot << adr << " [label=\"" << (this->interpretation->getFact(adr) ? "" : "-") << printToString<RawPrinter>(reg->ogatoms.getIDByAddress(adr), reg) << "@" << this->decisionlevel[adr] << " ";
        // decision literal?
        if (cause[adr] == -1 && decisionlevel[adr] == 0){
            dot << "(fact)" << "\"]; ";
        }else if (cause[adr] == -1 && decisionlevel[adr] > 0){
            dot << "(" << (flipped[adr] ? "flipped " : "")  << "decision)" << "\"]; ";
        }else{
            const Nogood& implicant = nogoodset.getNogood(cause[adr]);
            dot << "(" << implicant.getStringRepresentation(reg) << ")" << "\"]; ";
            // add edges from implicants
            BOOST_FOREACH (ID id, implicant) {
                if (id.address != adr){
                    dot << id.address << " -> " << adr << "; ";
                }
            }
        }
    }

    // add conflict nogood and edges if present
    BOOST_FOREACH (int conflictNogoodIndex, contradictoryNogoods) {
        // start from the conflicting nogood
        Nogood violatedNogood = nogoodset.getNogood(conflictNogoodIndex);
        dot << "c" << conflictNogoodIndex << " [label=\"conflict (" << violatedNogood.getStringRepresentation(reg) << ")\"]; ";
        BOOST_FOREACH (ID id, violatedNogood) {
            dot << id.address << " -> c" << conflictNogoodIndex << "; ";
        }
    }

    dot << "}" << std::endl;
    return dot.str();
}

DLVHEX_NAMESPACE_END

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
