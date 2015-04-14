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
 * @file   UnfoundedSetChecker.cpp
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 *
 * @brief  Unfounded set checker for programs with disjunctions and external atoms.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#include "dlvhex2/BaseModelGenerator.h"
#include "dlvhex2/UnfoundedSetChecker.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/ClaspSolver.h"

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/strong_components.hpp>

#include <fstream>

DLVHEX_NAMESPACE_BEGIN

/*
 * UnfoundedSetChecker
 * Base class for all unfounded set checkers
 */

UnfoundedSetChecker::UnfoundedSetChecker(
ProgramCtx& ctx,
const OrdinaryASPProgram& groundProgram,
InterpretationConstPtr componentAtoms,
SimpleNogoodContainerPtr ngc)
: mg(0),
ctx(ctx),
groundProgram(groundProgram),
agp(emptyagp),
componentAtoms(componentAtoms),
ngc(ngc),
domain(new Interpretation(ctx.registry()))
{

    reg = ctx.registry();

    mode = Ordinary;
    DBGLOG(DBG, "Starting UFS checker in ordinary mode, program idb has size " << groundProgram.idb.size());
}


UnfoundedSetChecker::UnfoundedSetChecker(
BaseModelGenerator* mg,
ProgramCtx& ctx,
const OrdinaryASPProgram& groundProgram,
const AnnotatedGroundProgram& agp,
InterpretationConstPtr componentAtoms,
SimpleNogoodContainerPtr ngc)
: mg(mg),
ctx(ctx),
groundProgram(groundProgram),
agp(agp),
componentAtoms(componentAtoms),
ngc(ngc),
domain(new Interpretation(ctx.registry()))
{

    reg = ctx.registry();

    mode = WithExt;
    DBGLOG(DBG, "Starting UFS checker in external atom mode, program idb has size " << groundProgram.idb.size());
}


bool UnfoundedSetChecker::isUnfoundedSet(InterpretationConstPtr compatibleSet, InterpretationConstPtr compatibleSetWithoutAux, InterpretationConstPtr ufsCandidate)
{

    // in debug mode we might want to do both checks (traditional and support set based)
    #ifndef NDEBUG
    //	#define DOBOTHCHECKS
    #endif

    // ordinary mode generates only real unfounded sets, hence there is no check required
    assert(mode == WithExt);

    DBGLOG(DBG, "Checking if " << *ufsCandidate << " is an unfounded set");

    // build indices
    UnfoundedSetVerificationStatus ufsVerStatus(agp, domain, ufsCandidate, compatibleSet, compatibleSetWithoutAux);

    // For check using support sets
    InterpretationPtr supportSetVerification;
    InterpretationPtr auxToVerify;
    if (ctx.config.getOption("SupportSets")) {
        // take external atom values from the ufsCandidate and ordinary atoms from I \ U
        DBGLOG(DBG, "Constructing interpretation for external atom evaluation from " << *ufsCandidate);
        supportSetVerification = InterpretationPtr(new Interpretation(reg));
        auxToVerify = InterpretationPtr(new Interpretation(reg));
        supportSetVerification->getStorage() = (compatibleSetWithoutAux->getStorage() - ufsCandidate->getStorage());
        BOOST_FOREACH (IDAddress adr, ufsVerStatus.auxiliariesToVerify) if (ufsCandidate->getFact(adr)) supportSetVerification->setFact(adr);
        BOOST_FOREACH (IDAddress adr, ufsVerStatus.auxiliariesToVerify) auxToVerify->setFact(adr);
    }

    // now evaluate one external atom after the other and check if the new truth value of the auxiliaries are justified
    #ifndef NDEBUG
    DBGLOG(DBG, "Verifying external atoms in UFS candidate");
    int evalCnt = 0;
    #endif

    bool isUFS = true;
    for (uint32_t eaIndex = 0; eaIndex < agp.getIndexedEAtoms().size(); ++eaIndex) {
        ID eaID = agp.getIndexedEAtom(eaIndex);

        // we only evaluate external atoms which are relevant for some auxiliaries
        if (eaID.address >= ufsVerStatus.externalAtomAddressToAuxIndices.size() ||
            ufsVerStatus.externalAtomAddressToAuxIndices[eaID.address].size() == 0) continue;
        const ExternalAtom& eatom = reg->eatoms.getByID(eaID);

        if  (ctx.config.getOption("SupportSets") &&
            (eatom.getExtSourceProperties().providesCompletePositiveSupportSets() || eatom.getExtSourceProperties().providesCompleteNegativeSupportSets()) &&
        agp.allowsForVerificationUsingCompleteSupportSets()) {
            DBGLOG(DBG, "Verifying " << eaID << " for UFS verification using complete support sets (" << *supportSetVerification << ")");
            if (!agp.verifyExternalAtomsUsingCompleteSupportSets(eaIndex, supportSetVerification, auxToVerify)) {
                isUFS = false;
                // if we should do both checks, then remember the result and continue with the explicit check,
                // otherwise we already know the result
                #ifndef DOBOTHCHECKS
                break;
                #endif
            }
        }
        #ifndef DOBOTHCHECKS
        else
        #else
            bool suppSetResult = isUFS;
        #endif
        {
            #ifndef NDEBUG
            evalCnt++;
            #endif
            // update the indices
            if (!verifyExternalAtomByEvaluation(eaID, ufsCandidate, compatibleSet, ufsVerStatus)) {
                #ifdef DOBOTHCHECKS
                // if we did already a support set-based check, assert that it also failed
                assert((!ctx.config.getOption("SupportSets") || isUFS == suppSetResult) &&
                    "Explicit and support set approach for UFS checking gave different answers");
                #endif
                isUFS = false;
                break;
            }

        }
    }

    DBGLOG(DBG, "Evaluated " << agp.getIndexedEAtoms().size() << " of " << agp.getIndexedEAtoms().size() << " external atoms");

    DBGLOG(DBG, "Candidate is " << (isUFS ? "" : "not ") << "an unfounded set (" << *ufsCandidate << ")");
    return isUFS;
}


UnfoundedSetChecker::UnfoundedSetVerificationStatus::UnfoundedSetVerificationStatus(
const AnnotatedGroundProgram& agp,
InterpretationConstPtr domain, InterpretationConstPtr ufsCandidate, InterpretationConstPtr compatibleSet, InterpretationConstPtr compatibleSetWithoutAux
)
{

    assert (ufsCandidate->getRegistry() == compatibleSet->getRegistry() && compatibleSet->getRegistry() == compatibleSetWithoutAux->getRegistry());

    // get all pairs of mappings of auxiliaries to external atoms
    BOOST_FOREACH (AnnotatedGroundProgram::AuxToExternalAtoms auxToEA, agp.getAuxToEA()) {

        // Check if this auxiliary needs to be verified; this is only the case if its truth value has changes over compatibleSet.
        // Note: If the truth value has not changed, then the guess could still be wrong.
        //       But it has been proven in the optimization part of
        //         "Efficient HEX-Program Evaluation based on Unfounded Sets" (Eiter, Fink, Krennwallner, Redl, Schller, JAIR, 2014)
        //       that in such cases this wrong guess is irrelevant for the unfounded set, i.e., even with a correct guess the interpretation would still be unfounded.
        IDAddress aux = auxToEA.first;
        if (ufsCandidate->getFact(aux) != compatibleSet->getFact(aux)) {
            if (domain->getFact(aux) && compatibleSet->getRegistry()->ogatoms.getIDByAddress(aux).isExternalAuxiliary()) {
                auxiliariesToVerify.push_back(aux);
                auxIndexToRemainingExternalAtoms.push_back(std::set<ID>(agp.getAuxToEA(aux).begin(), agp.getAuxToEA(aux).end()));
                BOOST_FOREACH (ID eaID, agp.getAuxToEA(aux)) {
                    while (externalAtomAddressToAuxIndices.size() <= eaID.address) externalAtomAddressToAuxIndices.push_back(std::vector<int>());
                    externalAtomAddressToAuxIndices[eaID.address].push_back(auxIndexToRemainingExternalAtoms.size() - 1);
                }
            }
        }
    }

    // For check using explicit evaluation of external atoms: prepare input to external atom evaluations
    //     Construct: compatibleSetWithoutAux - ufsCandidate
    //     Remove the UFS from the compatible set, but do not remove EA auxiliaries
    //     This does not hurt because EA auxiliaries can never be in the input to an external atom,
    //     but keeping them has the advantage that negative learning is more effective
    DBGLOG(DBG, "Constructing input interpretation for external atom evaluation");
    eaInput = InterpretationPtr(new Interpretation(compatibleSet->getRegistry()));
    eaInput->getStorage() = (compatibleSet->getStorage() - (ufsCandidate->getStorage() & compatibleSetWithoutAux->getStorage()));
}


// 1. evaluates an external atom and possibly learns during this step
// 2. tries to verify its auxiliaries and returns the result of this trial
// 3. updates the data structures used for unfounded set candidate verification
bool UnfoundedSetChecker::verifyExternalAtomByEvaluation(
ID eaID,                         // external atom
                                 // actual input to the check
InterpretationConstPtr ufsCandidate, InterpretationConstPtr compatibleSet,
                                 // indices
UnfoundedSetVerificationStatus& ufsVerStatus
)
{

    // evaluate
    DBGLOG(DBG, "Evaluate " << eaID << " for UFS verification " << (!!ngc ? "with" : "without") << " learning");

    const ExternalAtom& eatom = reg->eatoms.getByID(eaID);

    // prepare answer interpretation
    InterpretationPtr eaResult = InterpretationPtr(new Interpretation(reg));
    BaseModelGenerator::IntegrateExternalAnswerIntoInterpretationCB cb(eaResult);

    if (!!ngc && !!solver) {
        // evaluate the external atom with learned, and add the learned nogoods in transformed form to the UFS detection problem
        int oldNogoodCount = ngc->getNogoodCount();
        mg->evaluateExternalAtom(ctx, eaID, ufsVerStatus.eaInput, cb, ngc);
        DBGLOG(DBG, "O: Adding new valid input-output relationships from nogood container");
        for (int i = oldNogoodCount; i < ngc->getNogoodCount(); ++i) {

            const Nogood& ng = ngc->getNogood(i);
            if (ng.isGround()) {
                DBGLOG(DBG, "Processing learned nogood " << ng.getStringRepresentation(reg));

                std::vector<Nogood> transformed = nogoodTransformation(ng, compatibleSet);
                BOOST_FOREACH (Nogood tng, transformed) {
                    solver->addNogood(tng);
                }
            }
        }
    }
    else {
        mg->evaluateExternalAtom(ctx, eaID, ufsVerStatus.eaInput, cb);
    }

    // remove the external atom from the remaining lists of all auxiliaries which wait for the EA to be verified
    DBGLOG(DBG, "Updating data structures");
    assert (eaID.address < ufsVerStatus.externalAtomAddressToAuxIndices.size());
    BOOST_FOREACH (uint32_t i, ufsVerStatus.externalAtomAddressToAuxIndices[eaID.address]) {
        assert (i >= 0 && i < ufsVerStatus.auxIndexToRemainingExternalAtoms.size() && i < ufsVerStatus.auxiliariesToVerify.size());
        DBGLOG(DBG, "Updating auxiliary " << ufsVerStatus.auxiliariesToVerify[i]);
        if (!ufsVerStatus.auxIndexToRemainingExternalAtoms[i].empty()) {
            ufsVerStatus.auxIndexToRemainingExternalAtoms[i].erase(eaID);

            // if no external atoms remain to be verified, then the truth/falsity of the auxiliary is finally known
            if (ufsVerStatus.auxIndexToRemainingExternalAtoms[i].empty()) {
                // check if the auxiliary, which was assumed to be unfounded, is indeed _not_ in eaResult
                DBGLOG(DBG, "All relevant external atoms have been evaluated auxiliary, now checking if auxiliary " << ufsVerStatus.auxiliariesToVerify[i] << " is justified");
                if (eaResult->getFact(ufsVerStatus.auxiliariesToVerify[i]) != ufsCandidate->getFact(ufsVerStatus.auxiliariesToVerify[i])) {

                    // wrong guess: the auxiliary is _not_ unfounded
                    #ifndef NDEBUG
                    DBGLOG(DBG, "Truth value of auxiliary " << ufsVerStatus.auxiliariesToVerify[i] << " is not justified --> Candidate is not an unfounded set");
                    DBGLOG(DBG, "Candidate is not an unfounded set (" << *ufsCandidate << ")");
                    #endif

                    return false;
                }
                else {
                    DBGLOG(DBG, "Truth value of auxiliary " << ufsVerStatus.auxiliariesToVerify[i] << " is justified");
                }
            }
        }
    }

    return true;
}


Nogood UnfoundedSetChecker::getUFSNogood(
const std::vector<IDAddress>& ufs,
InterpretationConstPtr interpretation)
{

    #ifndef NDEBUG
    InterpretationPtr intr(new Interpretation(reg));
    BOOST_FOREACH (IDAddress adr, ufs) {
        intr->setFact(adr);
    }
    DBGLOG(DBG, "Constructing nogoods for UFS: " << *intr);
    #endif

    switch (ctx.config.getOption("UFSLearnStrategy")) {
        case 1: return getUFSNogoodReductBased(ufs, interpretation);
        case 2: return getUFSNogoodUFSBased(ufs, interpretation);
        default: throw GeneralError("Unknown UFSLern strategy");
    }
}


Nogood UnfoundedSetChecker::getUFSNogoodReductBased(
const std::vector<IDAddress>& ufs,
InterpretationConstPtr interpretation)
{

    // reduct-based stratey
    Nogood ng;

    #ifndef NDEBUG
    std::stringstream ss;
    bool first = true;
    ss << "{ ";
    BOOST_FOREACH (IDAddress adr, ufs) {
        ss << (!first ? ", " : "") << adr;
        first = false;
    }
    ss << " }";
    DBGLOG(DBG, "Constructing UFS nogood for UFS " << ss.str() << " wrt. " << *interpretation);
    #endif

    // for each rule with unsatisfied body
    BOOST_FOREACH (ID ruleId, groundProgram.idb) {
        const Rule& rule = reg->rules.getByID(ruleId);
        BOOST_FOREACH (ID b, rule.body) {
            if (interpretation->getFact(b.address) != !b.isNaf()) {
                // take an unsatisfied body literal
                ng.insert(NogoodContainer::createLiteral(b.address, interpretation->getFact(b.address)));
                break;
            }
        }
    }

    // add the smaller FLP model (interpretation minus unfounded set), restricted to ordinary atoms
    InterpretationPtr smallerFLPModel = InterpretationPtr(new Interpretation(*interpretation));
    BOOST_FOREACH (IDAddress adr, ufs) {
        smallerFLPModel->clearFact(adr);
    }
    bm::bvector<>::enumerator en = smallerFLPModel->getStorage().first();
    bm::bvector<>::enumerator en_end = smallerFLPModel->getStorage().end();
    while (en < en_end) {
        if (!reg->ogatoms.getIDByTuple(reg->ogatoms.getByAddress(*en).tuple).isAuxiliary()) {
            ng.insert(NogoodContainer::createLiteral(*en));
        }
        en++;
    }

    // add one atom which is in the original interpretation but not in the flp model
    en = interpretation->getStorage().first();
    en_end = interpretation->getStorage().end();
    while (en < en_end) {
        if (!smallerFLPModel->getFact(*en)) {
            ng.insert(NogoodContainer::createLiteral(*en));
            break;
        }
        en++;
    }

    DBGLOG(DBG, "Constructed UFS nogood " << ng);
    return ng;
}


Nogood UnfoundedSetChecker::getUFSNogoodUFSBased(
const std::vector<IDAddress>& ufs,
InterpretationConstPtr interpretation)
{

    // UFS-based strategy
    Nogood ng;

    // take an atom from the unfounded set which is true in the interpretation
    DBGLOG(DBG, "Constructing UFS nogood");
    BOOST_FOREACH (IDAddress adr, ufs) {
        if (interpretation->getFact(adr)) {
            ng.insert(NogoodContainer::createLiteral(adr, true));
            break;
        }
    }

    #ifndef NDEBUG
    int intersectionRules = 0;
    int nonExtRules = 0;
    #endif

    // find all rules r such that H(r) intersects with the unfounded set
    BOOST_FOREACH (ID ruleID, groundProgram.idb) {
        const Rule& rule = reg->rules.getByID(ruleID);
        if (mg && (rule.isEAGuessingRule() || (rule.head.size() == 1 && rule.head[0].isExternalAuxiliary()))) continue;

        bool intersects = false;
        BOOST_FOREACH (ID h, rule.head) {
            if (std::find(ufs.begin(), ufs.end(), h.address) != ufs.end()) {
                intersects = true;
                break;
            }
        }
        if (!intersects) continue;
        #ifndef NDEBUG
        intersectionRules++;
        #endif

        // Check if the rule is external ("external" to the UFS, has nothing to do with external atoms), i.e., it does _not_ contain an ordinary unfounded atom in its positive body
        // (if the rule is not external, then condition (ii) will always be satisfied wrt. this unfounded set)
        bool external = true;
        BOOST_FOREACH (ID b, rule.body) {
            if (interpretation->getFact(b.address) && !b.isNaf() && (!b.isExternalAuxiliary() || mode == Ordinary) && std::find(ufs.begin(), ufs.end(), b.address) != ufs.end()) {
                external = false;
                break;
            }
        }
        #ifndef NDEBUG
        if (external) {
            DBGLOG(DBG, "External rule: " << RawPrinter::toString(reg, ruleID));
        }
        else {
            DBGLOG(DBG, "Non-external rule: " << RawPrinter::toString(reg, ruleID));
            nonExtRules++;
        }
        #endif
        if (!external) continue;

        // If available, find a literal which satisfies this rule independently of ufs;
        // this is either
        // (i) an ordinary body atom is false in the interpretation
        // (iii) a head atom, which is true in the interpretation and not in the unfounded set
        // because then the rule is no justification for the ufs
        bool foundInd = false;
        // (iii)
        BOOST_FOREACH (ID h, rule.head) {
            if (interpretation->getFact(h.address) && std::find(ufs.begin(), ufs.end(), h.address) == ufs.end()) {
                ng.insert(NogoodContainer::createLiteral(h.address, true));
                DBGLOG(DBG, "Literal chosen by (iii)");
                foundInd = true;
                break;
            }
        }
        if (!foundInd) {
            // (i)
            BOOST_FOREACH (ID b, rule.body) {
                if (!b.isNaf() != interpretation->getFact(b.address) && (!b.isExternalAuxiliary() || mode == Ordinary)) {
                    ng.insert(NogoodContainer::createLiteral(b.address, false));
                    DBGLOG(DBG, "Literal chosen by (i)");
                    foundInd = true;
                    break;
                }
            }
        }
        if (!foundInd) {
            // (ii) alternatively: collect the truth values of all atoms relevant to the external atoms in the rule body
            bool extFound = false;
            BOOST_FOREACH (ID b, rule.body) {
                if (!b.isExternalAuxiliary()) {
                    // this atom is satisfied by the interpretation (otherwise we had already foundInd),
                    // therefore there must be another (external) atom which is false under I u -X
                    // ng.insert(NogoodContainer::createLiteral(b.address, interpretation->getFact(b.address)));
                }
                else {
                    assert(agp.mapsAux(b.address) && "mapping of auxiliary to EA not found");
                    const ExternalAtom& ea = reg->eatoms.getByID(agp.getAuxToEA(b.address)[0]);
                    ea.updatePredicateInputMask();
                    bm::bvector<>::enumerator en = ea.getPredicateInputMask()->getStorage().first();
                    bm::bvector<>::enumerator en_end = ea.getPredicateInputMask()->getStorage().end();
                    while (en < en_end) {
                        if (agp.getProgramMask()->getFact(*en) && domain->getFact(*en)) {
                            //						if (!ufs->getFact(*en)){	// atoms in the UFS will be always false under I u -X, thus their truth value in the interpretation is irrelevant
                            ng.insert(NogoodContainer::createLiteral(*en, interpretation->getFact(*en)));
                            //						}
                        }
                        extFound = true;
                        en++;
                    }
                }
            }
            assert (extFound);
            DBGLOG(DBG, "Literal chosen by (ii)");
        }
    }
    #ifndef NDEBUG
    DBGLOG(DBG, "During UFS nogood construction, " << intersectionRules << " of " << groundProgram.idb.size() << " rules intersected with the UFS and " << nonExtRules << " rules were non-external");
    DBGLOG(DBG, "Constructed UFS nogood " << ng.getStringRepresentation(reg));
    #endif

    return ng;
}


/*
 * EncodingBasedUnfoundedSetChecker
 *
 * Encoding-based unfounded set checker
 *
 * The current assignment is used on the meta-level during construction of the UFS search problem.
 * This requires the re-construction of the UFS subproblem for each UFS check (if the assignment has changed).
 */

EncodingBasedUnfoundedSetChecker::EncodingBasedUnfoundedSetChecker(
ProgramCtx& ctx,
const OrdinaryASPProgram& groundProgram,
InterpretationConstPtr componentAtoms,
SimpleNogoodContainerPtr ngc) :
UnfoundedSetChecker(ctx, groundProgram, componentAtoms, ngc)
{
}


EncodingBasedUnfoundedSetChecker::EncodingBasedUnfoundedSetChecker(
BaseModelGenerator& mg,
ProgramCtx& ctx,
const OrdinaryASPProgram& groundProgram,
const AnnotatedGroundProgram& agp,
InterpretationConstPtr componentAtoms,
SimpleNogoodContainerPtr ngc) :
UnfoundedSetChecker(&mg, ctx, groundProgram, agp, componentAtoms, ngc)
{
}


void EncodingBasedUnfoundedSetChecker::constructUFSDetectionProblem(
NogoodSet& ufsDetectionProblem,
InterpretationConstPtr compatibleSet,
InterpretationConstPtr compatibleSetWithoutAux,
const std::set<ID>& skipProgram,
std::vector<ID>& ufsProgram)
{

    int auxatomcnt = 0;
    constructUFSDetectionProblemNecessaryPart(ufsDetectionProblem, auxatomcnt, compatibleSet, compatibleSetWithoutAux, skipProgram, ufsProgram);
    constructUFSDetectionProblemOptimizationPart(ufsDetectionProblem, auxatomcnt, compatibleSet, compatibleSetWithoutAux, skipProgram, ufsProgram);
}


void EncodingBasedUnfoundedSetChecker::constructUFSDetectionProblemNecessaryPart(
NogoodSet& ufsDetectionProblem,
int& auxatomcnt,
InterpretationConstPtr compatibleSet,
InterpretationConstPtr compatibleSetWithoutAux,
const std::set<ID>& skipProgram,
std::vector<ID>& ufsProgram)
{

    #ifndef NDEBUG
    std::stringstream programstring;
    RawPrinter printer(programstring, reg);
    #endif

    DBGLOG(DBG, "Constructing necessary part of UFS detection problem");
    DBGLOG(DBG, "N: Facts");
    // facts cannot be in X
    if (groundProgram.edb) {
        bm::bvector<>::enumerator en = groundProgram.edb->getStorage().first();
        bm::bvector<>::enumerator en_end = groundProgram.edb->getStorage().end();
        while (en < en_end) {
            domain->setFact(*en);
            Nogood ng;
            ng.insert(NogoodContainer::createLiteral(*en, true));
            ufsDetectionProblem.addNogood(ng);
            en++;
        }
    }

    DBGLOG(DBG, "N: Rules");
    BOOST_FOREACH (ID ruleID, ufsProgram) {

        #ifndef NDEBUG
        programstring.str("");
        printer.print(ruleID);
        DBGLOG(DBG, "Processing rule " << programstring.str());
        #endif

        const Rule& rule = reg->rules.getByID(ruleID);

        if (ruleID.isWeightRule()) {
            // cycles through weight rules are not supported: the head atom must not be in the unfounded set
            if (ctx.config.getOption("AllowAggExtCycles")) {
                LOG(WARNING, "A cycle through weight rules was detected. This usually comes from cycles which involve both aggregates and external atoms and might result in non-minimal models. See aggregate options.");
            }
            else {
                throw GeneralError("A cycle through weight rules was detected. This usually comes from cycles which involve both aggregates and external atoms and is not allowed. See aggregate options.");
            }
            if (compatibleSet->getFact(rule.head[0].address)) {
                Nogood ng;
                ng.insert(NogoodContainer::createLiteral(rule.head[0].address, true));
                ufsDetectionProblem.addNogood(ng);
            }
            continue;
        }

        // condition 1 is handled directly: skip rules with unsatisfied body
        bool unsatisfied = false;
        BOOST_FOREACH (ID b, rule.body) {
            if (compatibleSet->getFact(b.address) != !b.isNaf()) {
                unsatisfied = true;
                break;
            }
        }
        if (unsatisfied) continue;

        // Compute the set of problem variables: this is the set of all atoms which (1) occur in the head of some rule; or (2) are external atom auxiliaries
        BOOST_FOREACH (ID h, rule.head) domain->setFact(h.address);
        BOOST_FOREACH (ID b, rule.body) domain->setFact(b.address);

        // Create two unique predicates and atoms for this rule
        OrdinaryAtom hratom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX | ID::PROPERTY_ATOM_HIDDEN);
        hratom.tuple.push_back(reg->getAuxiliaryConstantSymbol('x', ID(0, auxatomcnt++)));
        ID hr = reg->storeOrdinaryGAtom(hratom);

        // hr is true iff one of the rule's head atoms is in X
        {
            Nogood ng;
            ng.insert(NogoodContainer::createLiteral(hr.address, true));
            BOOST_FOREACH (ID h, rule.head) {
                ng.insert(NogoodContainer::createLiteral(h.address, false));
            }
            ufsDetectionProblem.addNogood(ng);
        }
        {
            BOOST_FOREACH (ID h, rule.head) {
                Nogood ng;
                ng.insert(NogoodContainer::createLiteral(hr.address, false));
                ng.insert(NogoodContainer::createLiteral(h.address, true));
                ufsDetectionProblem.addNogood(ng);
            }
        }

        {
            Nogood ng;
            // if hr is true, then it must not happen that neither Condition 2 nor Condition 3 is satisfied
            ng.insert(NogoodContainer::createLiteral(hr.address, true));

            // Condition 2: some body literal b, which is true in I, is false under I u -X
            // If b is ordinary (or considered to be ordinary), then this can only happen if b is positive because for a negative b, I \models b implies I u -X \models b
            // if b is external, then it can be either positive or negative because due to nonmonotonicity we might have I \models b but I u -X \not\models b (even if b is negative)
            // That is: It must not happen that
            //	1. all ordinary positive body atoms, which are true in I, are not in the unfounded set; and
            //	2. all external literals are true under I u -X
            BOOST_FOREACH (ID b, rule.body) {
                if (!b.isExternalAuxiliary() || mode == Ordinary) {
                    // ordinary literal
                    if (!b.isNaf() && compatibleSet->getFact(b.address)) {
                        ng.insert(NogoodContainer::createLiteral(b.address, false));
                    }
                }
                else {
                    // external literal
                    ng.insert(NogoodContainer::createLiteral(b.address, !b.isNaf()));
                }
            }

            // Condition 3: some head atom, which is true in I, is not in the unfounded set
            // That is: It must not happen, that all positive head atoms, which are true in I, are in the unfounded set (then the condition is not satisfied)
            BOOST_FOREACH (ID h, rule.head) {
                if (compatibleSet->getFact(h.address)) {
                    ng.insert(NogoodContainer::createLiteral(h.address, true));
                }
            }
            ufsDetectionProblem.addNogood(ng);
        }
    }

    // we want a UFS which intersects (wrt. the domain) with I
    DBGLOG(DBG, "N: Intersection with I ");
    {
        Nogood ng;
        bm::bvector<>::enumerator en = compatibleSetWithoutAux->getStorage().first();
        bm::bvector<>::enumerator en_end = compatibleSetWithoutAux->getStorage().end();
        while (en < en_end) {
            if ((!componentAtoms || componentAtoms->getFact(*en)) && domain->getFact(*en)) {
                ng.insert(NogoodContainer::createLiteral(*en, false));
            }
            en++;
        }
        ufsDetectionProblem.addNogood(ng);
    }

    // the ufs must not contain a head atom of an ignored rule
    // (otherwise we cannot guarantee that the ufs remains an ufs for completed interpretations)
    DBGLOG(DBG, "N: Ignored rules");
    {
        BOOST_FOREACH (ID ruleId, skipProgram) {
            const Rule& rule = reg->rules.getByID(ruleId);
            BOOST_FOREACH (ID h, rule.head) {
                Nogood ng;
                ng.insert(NogoodContainer::createLiteral(h.address, true));
                ufsDetectionProblem.addNogood(ng);
            }
        }
    }

    // the ufs must not contain an atom which is external to the component
    if (componentAtoms) {
        DBGLOG(DBG, "N: Restrict search to strongly connected component");
        bm::bvector<>::enumerator en = domain->getStorage().first();
        bm::bvector<>::enumerator en_end = domain->getStorage().end();
        while (en < en_end) {
            if ((!reg->ogatoms.getIDByAddress(*en).isExternalAuxiliary() || mode == Ordinary) && !componentAtoms->getFact(*en)) {
                Nogood ng;
                ng.insert(NogoodContainer::createLiteral(*en, true));
                ufsDetectionProblem.addNogood(ng);
            }
            en++;
        }
    }
}


void EncodingBasedUnfoundedSetChecker::constructUFSDetectionProblemOptimizationPart(
NogoodSet& ufsDetectionProblem,
int& auxatomcnt,
InterpretationConstPtr compatibleSet,
InterpretationConstPtr compatibleSetWithoutAux,
const std::set<ID>& skipProgram,
std::vector<ID>& ufsProgram)
{

    DBGLOG(DBG, "Constructing optimization part of UFS detection problem");
    constructUFSDetectionProblemOptimizationPartRestrictToCompatibleSet(ufsDetectionProblem, auxatomcnt, compatibleSet, compatibleSetWithoutAux, skipProgram, ufsProgram);
    if (mode == WithExt) {
        constructUFSDetectionProblemOptimizationPartBasicEAKnowledge(ufsDetectionProblem, auxatomcnt, compatibleSet, compatibleSetWithoutAux, skipProgram, ufsProgram);
        constructUFSDetectionProblemOptimizationPartLearnedFromMainSearch(ufsDetectionProblem, auxatomcnt, compatibleSet, compatibleSetWithoutAux, skipProgram, ufsProgram);

        // use this optimization only if external learning is off; the two optimizations can influence each other and cause spurious contradictions
        if (!ngc) constructUFSDetectionProblemOptimizationPartEAEnforement(ufsDetectionProblem, auxatomcnt, compatibleSet, compatibleSetWithoutAux, skipProgram, ufsProgram);
    }
}


void EncodingBasedUnfoundedSetChecker::constructUFSDetectionProblemOptimizationPartRestrictToCompatibleSet(
NogoodSet& ufsDetectionProblem,
int& auxatomcnt,
InterpretationConstPtr compatibleSet,
InterpretationConstPtr compatibleSetWithoutAux,
const std::set<ID>& skipProgram,
std::vector<ID>& ufsProgram)
{

    // ordinary atoms not in I must not be in the unfounded set
    DBGLOG(DBG, "O: Ordinary atoms not in I must not be in the unfounded set");
    BOOST_FOREACH (ID ruleID, ufsProgram) {
        const Rule& rule = reg->rules.getByID(ruleID);
        BOOST_FOREACH (ID h, rule.head) {
            if (!compatibleSet->getFact(h.address)) {
                Nogood ng;
                ng.insert(NogoodContainer::createLiteral(h.address, true));
                ufsDetectionProblem.addNogood(ng);
            }
        }
        BOOST_FOREACH (ID b, rule.body) {
            if ((!b.isExternalAuxiliary() || mode == Ordinary) && !compatibleSet->getFact(b.address)) {
                Nogood ng;
                ng.insert(NogoodContainer::createLiteral(b.address, true));
                ufsDetectionProblem.addNogood(ng);
            }
        }
    }
}


void EncodingBasedUnfoundedSetChecker::constructUFSDetectionProblemOptimizationPartBasicEAKnowledge(
NogoodSet& ufsDetectionProblem,
int& auxatomcnt,
InterpretationConstPtr compatibleSet,
InterpretationConstPtr compatibleSetWithoutAux,
const std::set<ID>& skipProgram,
std::vector<ID>& ufsProgram)
{

    // if none of the input atoms to an external atom, which are true in I, are in the unfounded set, then the truth value of the external atom cannot change
    DBGLOG(DBG, "O: Adding basic knowledge about external atom behavior");
    for (uint32_t eaIndex = 0; eaIndex < agp.getIndexedEAtoms().size(); ++eaIndex) {
        const ExternalAtom& eatom = reg->eatoms.getByID(agp.getIndexedEAtom(eaIndex));

        eatom.updatePredicateInputMask();

        // if none of the input atoms, which are true in I, are unfounded, then the output of the external atom does not change
        Nogood inputNogood;
        bm::bvector<>::enumerator en = eatom.getPredicateInputMask()->getStorage().first();
        bm::bvector<>::enumerator en_end = eatom.getPredicateInputMask()->getStorage().end();
        while (en < en_end) {
            if (compatibleSet->getFact(*en)) {
                // T a \in I
                if ( !domain->getFact(*en) ) {
                    // atom is true for sure in I u -X
                }
                else {
                    // atom might be true in I u -X
                    inputNogood.insert(NogoodContainer::createLiteral(*en, false));
                }
            }
            else {
                // F a \in I
                if ( !domain->getFact(*en) ) {
                    // atom is also false for sure in I u -X
                }
            }
            en++;
        }

        // go through the output atoms
        agp.getEAMask(eaIndex)->updateMask();
        en = agp.getEAMask(eaIndex)->mask()->getStorage().first();
        en_end = agp.getEAMask(eaIndex)->mask()->getStorage().end();
        while (en < en_end) {
            if (reg->ogatoms.getIDByAddress(*en).isExternalAuxiliary()) {
                // do not extend the variable domain (this is counterproductive)
                if ( domain->getFact(*en) ) {
                    Nogood ng = inputNogood;
                    ng.insert(NogoodContainer::createLiteral(*en, !compatibleSet->getFact(*en)));
                    ufsDetectionProblem.addNogood(ng);
                }
            }
            en++;
        }
    }
}


void EncodingBasedUnfoundedSetChecker::constructUFSDetectionProblemOptimizationPartLearnedFromMainSearch(
NogoodSet& ufsDetectionProblem,
int& auxatomcnt,
InterpretationConstPtr compatibleSet,
InterpretationConstPtr compatibleSetWithoutAux,
const std::set<ID>& skipProgram,
std::vector<ID>& ufsProgram)
{

    // add the learned nogoods (in transformed form)
    if (!!ngc) {
        DBGLOG(DBG, "O: Adding valid input-output relationships from nogood container");
        for (int i = 0; i < ngc->getNogoodCount(); ++i) {
            const Nogood& ng = ngc->getNogood(i);
            if (ng.isGround()) {
                DBGLOG(DBG, "Processing learned nogood " << ng.getStringRepresentation(reg));

                std::vector<Nogood> transformed = nogoodTransformation(ng, compatibleSet);
                BOOST_FOREACH (Nogood tng, transformed) {
                    ufsDetectionProblem.addNogood(tng);
                }
            }
        }
    }
}


void EncodingBasedUnfoundedSetChecker::constructUFSDetectionProblemOptimizationPartEAEnforement(
NogoodSet& ufsDetectionProblem,
int& auxatomcnt,
InterpretationConstPtr compatibleSet,
InterpretationConstPtr compatibleSetWithoutAux,
const std::set<ID>& skipProgram,
std::vector<ID>& ufsProgram)
{

    // if there is no necessity to change the truth value of an external atom compared to compatibleSet, then do not do it
    // (this makes the postcheck cheaper)
    DBGLOG(DBG, "O: Enforcement of external atom truth values");

    // make aux('x', r) false iff one of B^{+}_o(r), which is true in compatibleSet, is true or one of H(r), which is true in compatibleSet, is false
    boost::unordered_map<ID, IDAddress> ruleToAux;
    BOOST_FOREACH (ID ruleID, ufsProgram) {
        const Rule& rule = reg->rules.getByID(ruleID);

        OrdinaryAtom cratom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX | ID::PROPERTY_ATOM_HIDDEN);
        cratom.tuple.push_back(reg->getAuxiliaryConstantSymbol('x', ID(0, auxatomcnt++)));
        ID cr = reg->storeOrdinaryGAtom(cratom);
        ruleToAux[ruleID] = cr.address;

        // check if condition 1 applies for this rule
        bool condition1 = false;
        BOOST_FOREACH (ID b, rule.body) {
            if (compatibleSet->getFact(b.address) != !b.isNaf()) {
                // yes: set aux('x', r) to false
                condition1 = true;
                Nogood falsifyCr;
                falsifyCr.insert(NogoodContainer::createLiteral(cr.address, true));
                ufsDetectionProblem.addNogood(falsifyCr);
                break;
            }
        }

        if (!condition1) {
            Nogood ngnot;
            BOOST_FOREACH (ID b, rule.body) {
                if (!b.isNaf() && !b.isExternalAuxiliary() && compatibleSet->getFact(b.address)) {
                    DBGLOG(DBG, "Binding positive body atom to c " << cr);
                    Nogood ng;
                    ng.insert(NogoodContainer::createLiteral(cr.address, true));
                    ng.insert(NogoodContainer::createLiteral(b.address, true));
                    ufsDetectionProblem.addNogood(ng);

                    ngnot.insert(NogoodContainer::createLiteral(b.address, false));
                }
            }
            BOOST_FOREACH (ID h, rule.head) {
                if (compatibleSet->getFact(h.address)) {
                    DBGLOG(DBG, "Binding head atom to c " << cr);
                    Nogood ng;
                    ng.insert(NogoodContainer::createLiteral(cr.address, true));
                    ng.insert(NogoodContainer::createLiteral(h.address, false));
                    ufsDetectionProblem.addNogood(ng);

                    ngnot.insert(NogoodContainer::createLiteral(h.address, true));
                }
            }
            DBGLOG(DBG, "Negated nogood for c " << cr);
            ngnot.insert(NogoodContainer::createLiteral(cr.address, false));
            ufsDetectionProblem.addNogood(ngnot);
        }
    }

    // for all external atom auxiliaries
    std::set<IDAddress> eaAuxes;
    boost::unordered_map<IDAddress, std::vector<ID> > eaAuxToRule;

    BOOST_FOREACH (ID ruleID, ufsProgram) {
        const Rule& rule = reg->rules.getByID(ruleID);
        BOOST_FOREACH (ID b, rule.body) {
            if (b.isExternalAuxiliary()) {
                eaAuxes.insert(b.address);
                eaAuxToRule[b.address].push_back(ruleID);
            }
        }
    }
    BOOST_FOREACH (IDAddress eaAux, eaAuxes) {
        // if all aux('x', r) are false for all rules where eaAux occurs ...
        Nogood ng;
        BOOST_FOREACH (ID ruleID, eaAuxToRule[eaAux]) {
            ng.insert(NogoodContainer::createLiteral(ruleToAux[ruleID], false));
        }
        // then force aux to the same truth value as in compatibleSet
        ng.insert(NogoodContainer::createLiteral(eaAux, !compatibleSet->getFact(eaAux)));
        DBGLOG(DBG, "Enforcement of ea truth value");
        ufsDetectionProblem.addNogood(ng);
    }
}


std::vector<Nogood> EncodingBasedUnfoundedSetChecker::nogoodTransformation(Nogood ng, InterpretationConstPtr assignment)
{

    bool skip = false;
    Nogood ngAdd;

    BOOST_FOREACH (ID id, ng) {
                                 // we have to requery the ID because nogoods strip off unnecessary information (e.g. property flags)
        if (reg->ogatoms.getIDByAddress(id.address).isExternalAuxiliary()) {

            ID useID = id;

            // transform negative replacements to positive ones
            OrdinaryAtom ogatom = reg->ogatoms.getByID(id);
            if (ogatom.tuple[0] == reg->getAuxiliaryConstantSymbol('n', reg->getIDByAuxiliaryConstantSymbol(ogatom.tuple[0]))) {
                ogatom.tuple[0] = reg->getAuxiliaryConstantSymbol('r', reg->getIDByAuxiliaryConstantSymbol(ogatom.tuple[0]));
                useID = reg->storeOrdinaryGAtom(ogatom);
                                 // flip truth value
                useID.kind |= ID::NAF_MASK;
            }

            // do not add a nogood if it extends the variable domain (this is counterproductive)
            if ( !domain->getFact(useID.address) ) {
                DBGLOG(DBG, "Skipping because " << useID.address << " expands the domain");
                skip = true;
                break;
            }
            else {
                DBGLOG(DBG, "Inserting EA-Aux " << (useID.isNaf() ? "-" : "") << useID.address);
                ngAdd.insert(NogoodContainer::createLiteral(useID));
            }
        }
        else {
            // input atom

            // we have the following relations between sign S of the atom in the nogood, truth in assignment C and the unfounded set
            // S=positive, C=false --> nogood can never fire, skip it
            // S=positive, C=true --> nogood fires if the atom is NOT in the unfounded set (because it is not in the domain or it is false)
            // S=negative, C=true --> nogood fires if the atom IS in the unfounded set (because then it is false in I u -X)
            // S=negative, C=false --> nogood will always fire (wrt. this literal), skip the literal
            if (!id.isNaf()) {
                // positive
                if (assignment->getFact(id.address) == false) {
                    // false in I --> nogood can never fire unter I u -X
                    DBGLOG(DBG, "Skipping because " << id.address << " can never be true under I u -X");
                    skip = true;
                    break;
                }
                else {
                    // true in I --> nogood fires if X does not contain the atom
                    if ( domain->getFact(id.address) ) {
                        DBGLOG(DBG, "Inserting ordinary -" << id.address << " because it is true in I");
                        ngAdd.insert(NogoodContainer::createLiteral(id.address, false));
                    }
                    else {
                        DBGLOG(DBG, "Skipping ordinary " << id.address << " because it is not in the domain and can therefore never be in the unfounded set");
                    }
                }
            }
            else {
                // negative
                if (assignment->getFact(id.address) == true) {
                    // positive variant is true in I --> nogood fires if it is also in X
                    if ( !domain->getFact(id.address) ) {
                        DBGLOG(DBG, "Skipping because " << id.address << " can never be false under I u -X");
                        skip = true;
                        break;
                    }
                    else {
                        DBGLOG(DBG, "Inserting " << id.address << " because it is false in I u -X if it is in X");
                        ngAdd.insert(NogoodContainer::createLiteral(id.address, true));
                    }
                }
                else {
                    // positive variant is false in I --> it is also false in I u -X, skip literal
                    DBGLOG(DBG, "Skipping ordinary -" << id.address << " because it is false in I and therefore also in I u -X");
                }
            }
        }
    }
    if (skip) {
        return std::vector<Nogood>();
    }
    else {
        DBGLOG(DBG, "Adding transformed nogood " << ngAdd);
        std::vector<Nogood> result;
        result.push_back(ngAdd);
        return result;
    }
}


void EncodingBasedUnfoundedSetChecker::learnNogoodsFromMainSearch(bool reset)
{
    // nothing to do
    // (it is useless to learn nogoods now, because they will be forgetten anyway when the next UFS search is setup)
}


std::vector<IDAddress> EncodingBasedUnfoundedSetChecker::getUnfoundedSet(InterpretationConstPtr compatibleSet, const std::set<ID>& skipProgram)
{

    // remove external atom guessing rules and skipped rules from IDB
    std::vector<ID> ufsProgram;
    DBGLOG(DBG, "ch ");
    BOOST_FOREACH (ID ruleId, groundProgram.idb) {
        const Rule& rule = reg->rules.getByID(ruleId);
        if (mg &&
                                 // EA-guessing rule
            (rule.isEAGuessingRule() ||
                                 // ignored part of the program
        std::find(skipProgram.begin(), skipProgram.end(), ruleId) != skipProgram.end())) {
            // skip it
        }
        else {
            ufsProgram.push_back(ruleId);
        }
    }

    // we need the the compatible set with and without auxiliaries
    DBGLOG(DBG, "1");
    InterpretationConstPtr compatibleSetWithoutAux = compatibleSet->getInterpretationWithoutExternalAtomAuxiliaries();
    DBGLOG(DBG, "1");

    #ifndef NDEBUG
    std::stringstream programstring;
    RawPrinter printer(programstring, reg);
    if (groundProgram.edb) programstring << "EDB: " << *groundProgram.edb << std::endl;
    programstring << "IDB:" << std::endl;
    BOOST_FOREACH (ID ruleId, ufsProgram) {
        printer.print(ruleId);
        programstring << std::endl;
    }
    DBGLOG(DBG, "Computing unfounded set of program:" << std::endl << programstring.str() << std::endl << "with respect to interpretation" << std::endl << *compatibleSetWithoutAux << " (" << *compatibleSet << ")");
    #endif

    // construct the UFS detection problem
    {
        DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidcudp, "Construct UFS Detection Problem");
        NogoodSet ufsDetectionProblem;
        constructUFSDetectionProblem(ufsDetectionProblem, compatibleSet, compatibleSetWithoutAux, skipProgram, ufsProgram);

        // solve the ufs problem

        // We need to freeze the variables in the domain since their truth values in the models are relevant.
        // However, since leaned clauses can only constraint frozen variables,
        // it seems to be better to freeze all.
        solver = SATSolver::getInstance(ctx, ufsDetectionProblem /*, domain*/);
    }
    InterpretationConstPtr model;

    int mCnt = 0;

    #ifdef DLVHEX_BENCHMARK
    DLVHEX_BENCHMARK_REGISTER(ufscheck, "UFS Check");
    DLVHEX_BENCHMARK_REGISTER(oufscheck, "Ordinary UFS Check");
    if( mode == WithExt ) {
        DLVHEX_BENCHMARK_START(ufscheck);
    }
    else {
        DLVHEX_BENCHMARK_START(oufscheck);
    }
    BOOST_SCOPE_EXIT( (ufscheck)(oufscheck)(mode) ) {
        if( mode == WithExt ) {
            DLVHEX_BENCHMARK_STOP(ufscheck);
        }
        else {
            DLVHEX_BENCHMARK_STOP(oufscheck);
        }
    } BOOST_SCOPE_EXIT_END
        #endif

        DLVHEX_BENCHMARK_REGISTER(sidufsenum, "UFS-Detection Problem Solving");
    if (mode == WithExt) {
        DLVHEX_BENCHMARK_START(sidufsenum);
    }
    model = solver->getNextModel();
    if (mode == WithExt) {
        DLVHEX_BENCHMARK_STOP(sidufsenum);
    }
    while ( model != InterpretationConstPtr()) {
        if (mode == WithExt) {
            DLVHEX_BENCHMARK_REGISTER_AND_COUNT(ufscandidates, "Checked UFS candidates", 1);
        }

        // check if the model is actually an unfounded set
        DBGLOG(DBG, "Got UFS candidate: " << *model);
        mCnt++;

        if (mode == Ordinary || isUnfoundedSet(compatibleSet, compatibleSetWithoutAux, model)) {
            DBGLOG(DBG, "Found UFS: " << *model << " (interpretation: " << *compatibleSet << ")");

            std::vector<IDAddress> ufs;

            bm::bvector<>::enumerator en = model->getStorage().first();
            bm::bvector<>::enumerator en_end = model->getStorage().end();
            while (en < en_end) {
                if (domain->getFact(*en)) ufs.push_back(*en);
                en++;
            }

            DBGLOG(DBG, "Enumerated " << mCnt << " UFS candidates");

            solver.reset();

            if (mode == WithExt) {
                DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sidfailedufscheckcount, "Failed UFS Checks", 1);
            }

            return ufs;
        }
        else {
            DBGLOG(DBG, "No UFS: " << *model);
        }

        if (mode == WithExt) {
            DLVHEX_BENCHMARK_START(sidufsenum);
        }
        model = solver->getNextModel();
        if (mode == WithExt) {
            DLVHEX_BENCHMARK_STOP(sidufsenum);
        }
    }

    DBGLOG(DBG, "Enumerated " << mCnt << " UFS candidates");
    solver.reset();
    // no ufs
    std::vector<IDAddress> ufs;
    return ufs;
}


/*
 * AssumptionBasedUnfoundedSetChecker
 *
 * Assumption-based unfounded set checker
 *
 * The current assignment is used on the object-level (in the encoding) and can be inserted during the UFS check
 * by appropriate assumptions in the solver.
 * This allows for reusing of the UFS subproblem for each UFS check (even if the assignment has changed).
 */

void AssumptionBasedUnfoundedSetChecker::constructDomain()
{

    // EDB
    if (groundProgram.edb) {
        bm::bvector<>::enumerator en = groundProgram.edb->getStorage().first();
        bm::bvector<>::enumerator en_end = groundProgram.edb->getStorage().end();
        while (en < en_end) {
            domain->setFact(*en);
            en++;
        }
    }

    // IDB
    BOOST_FOREACH (ID ruleID, groundProgram.idb) {
        const Rule& rule = reg->rules.getByID(ruleID);
        if (mg && (rule.isEAGuessingRule() || (rule.head.size() == 1 && rule.head[0].isExternalAuxiliary()))) continue;
        BOOST_FOREACH (ID h, rule.head) domain->setFact(h.address);
        BOOST_FOREACH (ID b, rule.body) domain->setFact(b.address);
    }
}


void AssumptionBasedUnfoundedSetChecker::constructUFSDetectionProblemFacts(NogoodSet& ufsDetectionProblem)
{
    // facts cannot be in X
    DBGLOG(DBG, "N: Facts");
    if (groundProgram.edb) {
        bm::bvector<>::enumerator en = groundProgram.edb->getStorage().first();
        bm::bvector<>::enumerator en_end = groundProgram.edb->getStorage().end();
        while (en < en_end) {
            Nogood ng;
            ng.insert(NogoodContainer::createLiteral(*en, true));
            ufsDetectionProblem.addNogood(ng);
            en++;
        }
    }
}


void AssumptionBasedUnfoundedSetChecker::constructUFSDetectionProblemCreateAuxAtoms()
{

    bm::bvector<>::enumerator en = domain->getStorage().first();
    bm::bvector<>::enumerator en_end = domain->getStorage().end();
    while (en < en_end) {
        OrdinaryAtom interpretationShadowAtom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX | ID::PROPERTY_ATOM_HIDDEN);
        interpretationShadowAtom.tuple.push_back(reg->getAuxiliaryConstantSymbol('x', ID(0, atomcnt++)));
        interpretationShadow[*en] = reg->storeOrdinaryGAtom(interpretationShadowAtom).address;

        if (!reg->ogatoms.getIDByAddress(*en).isExternalAuxiliary() || mode == Ordinary) {
            OrdinaryAtom residualShadowAtom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX | ID::PROPERTY_ATOM_HIDDEN);
            residualShadowAtom.tuple.push_back(reg->getAuxiliaryConstantSymbol('x', ID(0, atomcnt++)));
            residualShadow[*en] = reg->storeOrdinaryGAtom(residualShadowAtom).address;

            OrdinaryAtom becomeFalseAtom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX | ID::PROPERTY_ATOM_HIDDEN);
            becomeFalseAtom.tuple.push_back(reg->getAuxiliaryConstantSymbol('x', ID(0, atomcnt++)));
            becomeFalse[*en] = reg->storeOrdinaryGAtom(becomeFalseAtom).address;

            OrdinaryAtom aIandU(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX | ID::PROPERTY_ATOM_HIDDEN);
            aIandU.tuple.push_back(reg->getAuxiliaryConstantSymbol('x', ID(0, atomcnt++)));
            IandU[*en] = reg->storeOrdinaryGAtom(aIandU).address;

            OrdinaryAtom anIorU(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX | ID::PROPERTY_ATOM_HIDDEN);
            anIorU.tuple.push_back(reg->getAuxiliaryConstantSymbol('x', ID(0, atomcnt++)));
            nIorU[*en] = reg->storeOrdinaryGAtom(anIorU).address;
        }

        en++;
    }
}


void AssumptionBasedUnfoundedSetChecker::constructUFSDetectionProblemRule(NogoodSet& ufsDetectionProblem, ID ruleID)
{

    const Rule& rule = reg->rules.getByID(ruleID);
    if (mg && (rule.isEAGuessingRule() || (rule.head.size() == 1 && rule.head[0].isExternalAuxiliary()))) return;

    #ifndef NDEBUG
    std::stringstream programstring;
    RawPrinter printer(programstring, reg);
    printer.print(ruleID);
    DBGLOG(DBG, "Processing rule " << programstring.str());
    #endif

    if (ruleID.isWeightRule()) {
        // cycles through weight rules are not supported: the head atom must not be in the unfounded set
        if (ctx.config.getOption("AllowAggExtCycles")) {
            LOG(WARNING, "A cycle through weight rules was detected. This usually comes from cycles which involve both aggregates and external atoms and might result in non-minimal models. See aggregate options.");
        }
        else {
            throw GeneralError("A cycle through weight rules was detected. This usually comes from cycles which involve both aggregates and external atoms and is not allowed. See aggregate options.");
        }
        Nogood ng;
        //		ng.insert(NogoodContainer::createLiteral(interpretationShadow[rule.head[0].address], true));
        ng.insert(NogoodContainer::createLiteral(rule.head[0].address, true));
        ufsDetectionProblem.addNogood(ng);
        return;
    }

    // Create a unique predicate and atom h_r for this rule
    ID hr;
    {
        OrdinaryAtom hratom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX | ID::PROPERTY_ATOM_HIDDEN);
        hratom.tuple.push_back(reg->getAuxiliaryConstantSymbol('x', ID(0, atomcnt++)));
        hr = reg->storeOrdinaryGAtom(hratom);
    }

    // hr is true iff one of the rule's head atoms is in X
    DBGLOG(DBG, "Binding hr to head atom");
    {
        Nogood ng;
        ng.insert(NogoodContainer::createLiteral(hr.address, true));
        BOOST_FOREACH (ID h, rule.head) {
            ng.insert(NogoodContainer::createLiteral(h.address, false));
        }
        ufsDetectionProblem.addNogood(ng);
    }
    {
        BOOST_FOREACH (ID h, rule.head) {
            Nogood ng;
            ng.insert(NogoodContainer::createLiteral(hr.address, false));
            ng.insert(NogoodContainer::createLiteral(h.address, true));
            ufsDetectionProblem.addNogood(ng);
        }
    }

    {
        Nogood ng;
        // if hr is true, then it must not happen that neither Condition 1 nor Condition 2 nor Condition 3 is satisfied
        ng.insert(NogoodContainer::createLiteral(hr.address, true));

        // Condition 1: some body literal b is unsatisfied by I
        // hence, it must not happen that all body literals are simultanously satisfied by I
        DBGLOG(DBG, "Condition 1");
        BOOST_FOREACH (ID b, rule.body) {
            ng.insert(NogoodContainer::createLiteral(interpretationShadow[b.address], !b.isNaf()));
        }

        // Condition 2: some body literal b, which is true in I, is false under I u -X
        // If b is ordinary (or considered to be ordinary), then this can only happen if b is positive because for a negative b, I \models b implies I u -X \models b
        // if b is external, then it can be either positive or negative because due to nonmonotonicity we might have I \models b but I u -X \not\models b (even if b is negative)
        // That is: It must not happen that
        //	1. all ordinary positive body atoms, which are true in I, are not in the unfounded set; and
        //	2. all external literals are true under I u -X
        DBGLOG(DBG, "Condition 2");
        BOOST_FOREACH (ID b, rule.body) {
            if (!b.isExternalAuxiliary() || mode == Ordinary) {
                if (!b.isNaf()) {
                    ng.insert(NogoodContainer::createLiteral(IandU[b.address], false));
                }
            }
            else {
                // external literal
                ng.insert(NogoodContainer::createLiteral(b.address, !b.isNaf()));
            }
        }

        // Condition 3: some head atom, which is true in I, is not in the unfounded set
        // That is: It must not happen, that all positive head atoms, which are true in I, are in the unfounded set (then the condition is not satisfied)
        DBGLOG(DBG, "Condition 3");
        BOOST_FOREACH (ID h, rule.head) {
            ng.insert(NogoodContainer::createLiteral(nIorU[h.address], true));
        }

        DBGLOG(DBG, "Checking conditions 1, 2, 3");
        ufsDetectionProblem.addNogood(ng);
    }
}


void AssumptionBasedUnfoundedSetChecker::constructUFSDetectionProblemDefineAuxiliaries(NogoodSet& ufsDetectionProblem)
{

    DBGLOG(DBG, "N: Define residual shadow");
    {
        bm::bvector<>::enumerator en = domain->getStorage().first();
        bm::bvector<>::enumerator en_end = domain->getStorage().end();
        while (en < en_end) {
            if (!reg->ogatoms.getIDByAddress(*en).isExternalAuxiliary() || mode == Ordinary) {
                // define: residual shadow rs=true iff en is true in I u -X
                ID is = reg->ogatoms.getIDByAddress(interpretationShadow[*en]);
                ID rs = reg->ogatoms.getIDByAddress(residualShadow[*en]);

                {
                    Nogood ng1;
                    ng1.insert(NogoodContainer::createLiteral(is.address, true));
                    ng1.insert(NogoodContainer::createLiteral(*en, false));
                    ng1.insert(NogoodContainer::createLiteral(rs.address, false));
                    ufsDetectionProblem.addNogood(ng1);
                }
                {
                    Nogood ng2;
                    ng2.insert(NogoodContainer::createLiteral(is.address, false));
                    ng2.insert(NogoodContainer::createLiteral(rs.address, true));
                    ufsDetectionProblem.addNogood(ng2);
                }
                {
                    Nogood ng3;
                    ng3.insert(NogoodContainer::createLiteral(*en, true));
                    ng3.insert(NogoodContainer::createLiteral(rs.address, true));
                    ufsDetectionProblem.addNogood(ng3);
                }
            }
            en++;
        }
    }

    DBGLOG(DBG, "N: Define \"became false\"");
    {
        bm::bvector<>::enumerator en = domain->getStorage().first();
        bm::bvector<>::enumerator en_end = domain->getStorage().end();
        while (en < en_end) {
            if (!reg->ogatoms.getIDByAddress(*en).isExternalAuxiliary() || mode == Ordinary) {
                // define: became false bf=true iff en is true in I and true in X
                ID is = reg->ogatoms.getIDByAddress(interpretationShadow[*en]);
                ID bf = reg->ogatoms.getIDByAddress(becomeFalse[*en]);

                {
                    Nogood ng1;
                    ng1.insert(NogoodContainer::createLiteral(is.address, true));
                    ng1.insert(NogoodContainer::createLiteral(*en, true));
                    ng1.insert(NogoodContainer::createLiteral(bf.address, false));
                    ufsDetectionProblem.addNogood(ng1);
                }
                {
                    Nogood ng2;
                    ng2.insert(NogoodContainer::createLiteral(is.address, false));
                    ng2.insert(NogoodContainer::createLiteral(bf.address, true));
                    ufsDetectionProblem.addNogood(ng2);
                }
                {
                    Nogood ng3;
                    ng3.insert(NogoodContainer::createLiteral(*en, false));
                    ng3.insert(NogoodContainer::createLiteral(bf.address, true));
                    ufsDetectionProblem.addNogood(ng3);
                }
            }
            en++;
        }
    }

    // for all ordinary atoms a
    // define: a_{IandU} := a_I \wedge a
    // define: a_{\overline{I}orU} := \neg a_I \or a
    DBGLOG(DBG, "N: Define a_{IandU} :- a_I \\wedge a   and   a_{\\overline{I}orU} :- \\neg a_I \\vee a");
    {
        bm::bvector<>::enumerator en = domain->getStorage().first();
        bm::bvector<>::enumerator en_end = domain->getStorage().end();
        while (en < en_end) {
            if (!reg->ogatoms.getIDByAddress(*en).isExternalAuxiliary() || mode == Ordinary) {
                ID is = reg->ogatoms.getIDByAddress(interpretationShadow[*en]);

                // define: a_{IandU} := a_I \wedge a
                // we define a new atom a_{IandU}, which serves as a replacement for a, as follows:
                //   1. if a_I is false, then a_{IandU} is false
                //   2. if a is false, then a_{IandU} is false
                //   3. otherwise (a_I is true and a is true) a_{IandU} is true
                {
                    IDAddress aIandU_IDA = IandU[*en];

                    // 1.
                    Nogood ng1;
                    ng1.insert(NogoodContainer::createLiteral(is.address, false));
                    ng1.insert(NogoodContainer::createLiteral(aIandU_IDA, true));
                    ufsDetectionProblem.addNogood(ng1);

                    // 2.
                    Nogood ng2;
                    //					ng2.insert(NogoodContainer::createLiteral(is.address, true));
                    ng2.insert(NogoodContainer::createLiteral(*en, false));
                    ng2.insert(NogoodContainer::createLiteral(aIandU_IDA, true));
                    ufsDetectionProblem.addNogood(ng2);

                    // 3.
                    Nogood ng3;
                    ng3.insert(NogoodContainer::createLiteral(is.address, true));
                    ng3.insert(NogoodContainer::createLiteral(*en, true));
                    ng3.insert(NogoodContainer::createLiteral(aIandU_IDA, false));
                    ufsDetectionProblem.addNogood(ng3);
                }

                // define: a_{\overline{I}orU} := \neg a_I \or a
                // we define a new atom a_{\overline{I}orU}, which serves as a replacement for a, as follows:
                //   1. if a_I is false, then a_{\overline{I}orU} is true
                //   2. if a is true, then a_{\overline{I}orU} is true
                //   3. otherwise (a_I is true and a is false) a_{\overline{I}orU} is false
                {
                    IDAddress anIorU_IDA = nIorU[*en];

                    // 1.
                    Nogood ng1;
                    ng1.insert(NogoodContainer::createLiteral(is.address, false));
                    ng1.insert(NogoodContainer::createLiteral(anIorU_IDA, false));
                    ufsDetectionProblem.addNogood(ng1);

                    // 2.
                    Nogood ng2;
                    //					ng2.insert(NogoodContainer::createLiteral(is.address, true));
                    ng2.insert(NogoodContainer::createLiteral(*en, true));
                    ng2.insert(NogoodContainer::createLiteral(anIorU_IDA, false));
                    ufsDetectionProblem.addNogood(ng2);

                    // 3.
                    Nogood ng3;
                    ng3.insert(NogoodContainer::createLiteral(is.address, true));
                    ng3.insert(NogoodContainer::createLiteral(*en, false));
                    ng3.insert(NogoodContainer::createLiteral(anIorU_IDA, true));
                    ufsDetectionProblem.addNogood(ng3);
                }
            }
            en++;
        }
    }
}


void AssumptionBasedUnfoundedSetChecker::constructUFSDetectionProblemNonempty(NogoodSet& ufsDetectionProblem)
{
    DBGLOG(DBG, "N: Nonempty");
    Nogood ng;
    bm::bvector<>::enumerator en = domain->getStorage().first();
    bm::bvector<>::enumerator en_end = domain->getStorage().end();
    while (en < en_end) {
        if (!reg->ogatoms.getIDByAddress(*en).isExternalAuxiliary() || mode == Ordinary) {
            ng.insert(NogoodContainer::createLiteral(*en, false));
        }
        en++;
    }

    // add the hook atom to allow for retracting this nogood later
    ng.insert(NogoodContainer::createLiteral(hookAtom.address, true));

    ufsDetectionProblem.addNogood(ng);
}


void AssumptionBasedUnfoundedSetChecker::constructUFSDetectionProblemRestrictToSCC(NogoodSet& ufsDetectionProblem)
{

    if (componentAtoms) {
        DBGLOG(DBG, "N: Restrict search to strongly connected component");
        bm::bvector<>::enumerator en = domain->getStorage().first();
        bm::bvector<>::enumerator en_end = domain->getStorage().end();
        while (en < en_end) {
            if ((!reg->ogatoms.getIDByAddress(*en).isExternalAuxiliary() || mode == Ordinary) && !componentAtoms->getFact(*en)) {
                Nogood ng;
                ng.insert(NogoodContainer::createLiteral(*en, true));

                // add the hook atom to allow for retracting this nogood later
                ng.insert(NogoodContainer::createLiteral(hookAtom.address, true));

                ufsDetectionProblem.addNogood(ng);
            }
            en++;
        }
    }
}


void AssumptionBasedUnfoundedSetChecker::constructUFSDetectionProblemBasicEABehavior(NogoodSet& ufsDetectionProblem)
{

    // if none of the input atoms to an external atom, which are true in I, are in the unfounded set, then the truth value of the external atom cannot change
    DBGLOG(DBG, "O: Adding basic knowledge about external atom behavior");
    for (uint32_t eaIndex = 0; eaIndex < agp.getIndexedEAtoms().size(); ++eaIndex) {
        const ExternalAtom& eatom = reg->eatoms.getByID(agp.getIndexedEAtom(eaIndex));

        eatom.updatePredicateInputMask();

        // if none of the input atoms in the scope of this UFS checker, which are true in I, are unfounded, then the output of the external atom does not change
        Nogood inputNogood;
        bm::bvector<>::enumerator en = eatom.getPredicateInputMask()->getStorage().first();
        bm::bvector<>::enumerator en_end = eatom.getPredicateInputMask()->getStorage().end();
        while (en < en_end) {
            if (domain->getFact(*en)) inputNogood.insert(NogoodContainer::createLiteral(becomeFalse[*en], false));
            en++;
        }
        // make sure that this nogood is invalidated over an extended domain
        inputNogood.insert(NogoodContainer::createLiteral(hookAtom.address, true));

        // go through the output atoms
        agp.getEAMask(eaIndex)->updateMask();
        en = agp.getEAMask(eaIndex)->mask()->getStorage().first();
        en_end = agp.getEAMask(eaIndex)->mask()->getStorage().end();
        while (en < en_end) {
            if (reg->ogatoms.getIDByAddress(*en).isExternalAuxiliary()) {
                // do not extend the variable domain (this is counterproductive)
                if ( domain->getFact(*en) ) { {
                        // avoid that EA is true in I and false in I u -X
                        Nogood ng = inputNogood;
                        ng.insert(NogoodContainer::createLiteral(*en, true));
                        ng.insert(NogoodContainer::createLiteral(interpretationShadow[*en], false));
                        ufsDetectionProblem.addNogood(ng);
                    }
                    {
                        // avoid that EA is false in I and true in I u -X
                        Nogood ng = inputNogood;
                        ng.insert(NogoodContainer::createLiteral(*en, false));
                        ng.insert(NogoodContainer::createLiteral(interpretationShadow[*en], true));
                        ufsDetectionProblem.addNogood(ng);
                    }
                }
            }
            en++;
        }
    }
}


void AssumptionBasedUnfoundedSetChecker::constructUFSDetectionProblemAndInstantiateSolver()
{

    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidcudp, "Construct UFS Detection Problem");
    NogoodSet ufsDetectionProblem;

    #ifndef NDEBUG
    std::stringstream programstring;
    RawPrinter printer(programstring, reg);
    #endif

    DBGLOG(DBG, "Constructing UFS detection problem");

    atomcnt = 0;

    // create a hook atom
    OrdinaryAtom hatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX | ID::PROPERTY_ATOM_HIDDEN);
    hatom.tuple.push_back(reg->getAuxiliaryConstantSymbol('x', ID(0, atomcnt++)));
    hookAtom = reg->storeOrdinaryGAtom(hatom);

    // create problem encoding
    constructDomain();
    constructUFSDetectionProblemFacts(ufsDetectionProblem);
    constructUFSDetectionProblemCreateAuxAtoms();
    constructUFSDetectionProblemDefineAuxiliaries(ufsDetectionProblem);
    constructUFSDetectionProblemNonempty(ufsDetectionProblem);
    constructUFSDetectionProblemRestrictToSCC(ufsDetectionProblem);
    constructUFSDetectionProblemBasicEABehavior(ufsDetectionProblem);

    DBGLOG(DBG, "N: Rules");
    BOOST_FOREACH (ID ruleID, groundProgram.idb) {
        constructUFSDetectionProblemRule(ufsDetectionProblem, ruleID);
    }

    // We need to freeze all variables which might be fixed by assumptions,
    // or whose truth values in the models are relevant.
    // This includes all domains atoms and their shadows.
    InterpretationPtr frozenInt;
    #if 0
    frozenInt.reset(new Interpretation(reg));
    bm::bvector<>::enumerator en = domain->getStorage().first();
    bm::bvector<>::enumerator en_end = domain->getStorage().end();
    while (en < en_end) {
        frozenInt->setFact(*en);
        frozenInt->setFact(interpretationShadow[*en]);
        en++;
    }
    // However, since leaned clauses can only constraint frozen variables,
    // it seems to be better to freeze all.
    #endif

    // finally, instantiate the solver for the constructed search problem
    DBGLOG(DBG, "Unfounded Set Detection Problem: " << ufsDetectionProblem);
    solver = SATSolver::getInstance(ctx, ufsDetectionProblem, frozenInt);

    // remember the number of rules respected to far to allow for incremental extension
    problemRuleCount = groundProgram.idb.size();
}


void AssumptionBasedUnfoundedSetChecker::expandUFSDetectionProblemAndReinstantiateSolver()
{

    DBGLOG(DBG, "Extend UFS detection problem");

    // remember old domain
    InterpretationPtr oldDomain = domain;

    // compute (strictly) new domain
    domain = InterpretationPtr(new Interpretation(reg));
    constructDomain();
    domain->getStorage() -= oldDomain->getStorage();

    // create a hook atom
    OrdinaryAtom hatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX | ID::PROPERTY_ATOM_HIDDEN);
    hatom.tuple.push_back(reg->getAuxiliaryConstantSymbol('x', ID(0, atomcnt++)));
    hookAtom = reg->storeOrdinaryGAtom(hatom);

    NogoodSet ufsDetectionProblem;

    // add domain-specific part for the new domain
    constructUFSDetectionProblemFacts(ufsDetectionProblem);
    constructUFSDetectionProblemCreateAuxAtoms();
    constructUFSDetectionProblemDefineAuxiliaries(ufsDetectionProblem);

    // add global part for the new overall program
    domain->add(*oldDomain);
    constructUFSDetectionProblemNonempty(ufsDetectionProblem);
    constructUFSDetectionProblemRestrictToSCC(ufsDetectionProblem);
    constructUFSDetectionProblemBasicEABehavior(ufsDetectionProblem);

    DBGLOG(DBG, "N: New Rules");
    while (problemRuleCount < groundProgram.idb.size()) {
        ID ruleID = groundProgram.idb[problemRuleCount];
        constructUFSDetectionProblemRule(ufsDetectionProblem, ruleID);
        problemRuleCount++;
    }

    // finally, reinstantiate the solver for the constructed search problem
    InterpretationPtr frozenInt;
    DBGLOG(DBG, "Unfounded Set Detection Problem Addition: " << ufsDetectionProblem);
    solver->addNogoodSet(ufsDetectionProblem);
}


void AssumptionBasedUnfoundedSetChecker::setAssumptions(InterpretationConstPtr compatibleSet, const std::set<ID>& skipProgram)
{

    std::vector<ID> assumptions;

    bm::bvector<>::enumerator en = domain->getStorage().first();
    bm::bvector<>::enumerator en_end = domain->getStorage().end();
    DBGLOG(DBG, "A: Encoding interpretation");
    while (en < en_end) {
        DBGLOG(DBG, interpretationShadow[*en] << "=" << compatibleSet->getFact(*en));
        assumptions.push_back(ID(compatibleSet->getFact(*en) ? 0 : ID::NAF_MASK, interpretationShadow[*en]));
        en++;
    }

    en = domain->getStorage().first();
    en_end = domain->getStorage().end();
    DBGLOG(DBG, "A: Intersection of U with I");
    while (en < en_end) {
        // do not set an ordinary atom which is false in I
        if (!reg->ogatoms.getIDByAddress(*en).isExternalAuxiliary() || mode == Ordinary) {
            if (!compatibleSet->getFact(*en)) {
                DBGLOG(DBG, *en << "=0");
                assumptions.push_back(ID::nafLiteralFromAtom(reg->ogatoms.getIDByAddress(*en)));
            }
        }
        en++;
    }

    // the ufs must not contain a head atom of an ignored rule
    // (otherwise we cannot guarantee that the ufs remains an ufs for completed interpretations)
    DBGLOG(DBG, "A: Ignored rules");
    {
        BOOST_FOREACH (ID ruleId, skipProgram) {
            const Rule& rule = reg->rules.getByID(ruleId);
            BOOST_FOREACH (ID h, rule.head) {
                if (domain->getFact(h.address)) {
                    DBGLOG(DBG, h.address << "=0");
                    assumptions.push_back(ID::nafLiteralFromAtom(reg->ogatoms.getIDByAddress(h.address)));
                }
            }
        }
    }

    // let the CURRENT(!) hook atom be true
    assumptions.push_back(ID::posLiteralFromAtom(hookAtom));

    #ifndef NDEBUG
    BOOST_FOREACH (ID a, assumptions) {
        DBGLOG(DBG, "Assumption: " << a.address << "=" << !a.isNaf());
    }
    #endif

    solver->restartWithAssumptions(assumptions);
}


AssumptionBasedUnfoundedSetChecker::AssumptionBasedUnfoundedSetChecker(
ProgramCtx& ctx,
const OrdinaryASPProgram& groundProgram,
InterpretationConstPtr componentAtoms,
SimpleNogoodContainerPtr ngc) :
UnfoundedSetChecker(ctx, groundProgram, componentAtoms, ngc)
{

    reg = ctx.registry();
    learnedNogoodsFromMainSearch = 0;

    #ifndef NDEBUG
    std::stringstream programstring;
    RawPrinter printer(programstring, reg);
    if (groundProgram.edb) programstring << "EDB: " << *groundProgram.edb << std::endl;
    programstring << "IDB:" << std::endl;
    BOOST_FOREACH (ID ruleId, groundProgram.idb) {
        printer.print(ruleId);
        programstring << std::endl;
    }
    DBGLOG(DBG, "Computing unfounded set of program:" << std::endl << programstring.str());
    #endif

    // construct the UFS detection problem
    constructUFSDetectionProblemAndInstantiateSolver();
}


AssumptionBasedUnfoundedSetChecker::AssumptionBasedUnfoundedSetChecker(
BaseModelGenerator& mg,
ProgramCtx& ctx,
const OrdinaryASPProgram& groundProgram,
const AnnotatedGroundProgram& agp,
InterpretationConstPtr componentAtoms,
SimpleNogoodContainerPtr ngc) :
UnfoundedSetChecker(&mg, ctx, groundProgram, agp, componentAtoms, ngc)
{

    reg = ctx.registry();
    learnedNogoodsFromMainSearch = 0;

    #ifndef NDEBUG
    std::stringstream programstring;
    RawPrinter printer(programstring, reg);
    if (groundProgram.edb) programstring << "EDB: " << *groundProgram.edb << std::endl;
    programstring << "IDB:" << std::endl;
    BOOST_FOREACH (ID ruleId, groundProgram.idb) {
        printer.print(ruleId);
        programstring << std::endl;
    }
    DBGLOG(DBG, "Computing unfounded set of program:" << std::endl << programstring.str());
    #endif

    // construct the UFS detection problem
    constructUFSDetectionProblemAndInstantiateSolver();
}


std::vector<Nogood> AssumptionBasedUnfoundedSetChecker::nogoodTransformation(Nogood ng, InterpretationConstPtr assignment)
{

    // Note: this transformation must not depend on the compatible set!

    bool skip = false;
    Nogood ngAdd;

    BOOST_FOREACH (ID id, ng) {
        // we have to requery the ID because nogoods strip off unnecessary information (e.g. property flags)
        if (reg->ogatoms.getIDByAddress(id.address).isExternalAuxiliary()) {

            ID useID = id;

            // transform negative replacements to positive ones
            OrdinaryAtom ogatom = reg->ogatoms.getByID(id);
            if (ogatom.tuple[0] == reg->getAuxiliaryConstantSymbol('n', reg->getIDByAuxiliaryConstantSymbol(ogatom.tuple[0]))) {
                ogatom.tuple[0] = reg->getAuxiliaryConstantSymbol('r', reg->getIDByAuxiliaryConstantSymbol(ogatom.tuple[0]));
                useID = reg->storeOrdinaryGAtom(ogatom);
                                 // flip truth value
                useID.kind |= ID::NAF_MASK;
            }

            // do not add a nogood if it extends the variable domain (this is counterproductive)
            if ( !domain->getFact(useID.address) ) {
                DBGLOG(DBG, "Skipping because " << useID.address << " expands the domain");
                skip = true;
                break;
            }
            else {
                DBGLOG(DBG, "Inserting EA-Aux " << (useID.isNaf() ? "-" : "") << useID.address);
                ngAdd.insert(NogoodContainer::createLiteral(useID));
            }
        }
        else {
            // input atom

            // we have the following relations between sign S of the atom in the nogood, truth in assignment C and the unfounded set
            // S=positive, C=false --> nogood can never fire, skip it
            // S=positive, C=true --> nogood fires if the atom is NOT in the unfounded set (because it is not in the domain or it is false)
            // S=negative, C=true --> nogood fires if the atom IS in the unfounded set (because then it is false in I u -X)
            // S=negative, C=false --> nogood will always fire (wrt. this literal), skip the literal
            if (!id.isNaf()) {
                // positive
                ngAdd.insert(NogoodContainer::createLiteral(interpretationShadow[id.address], true));
                // true in I --> nogood fires if X does not contain the atom
                if ( domain->getFact(id.address) ) {
                    DBGLOG(DBG, "Inserting ordinary -" << id.address << " because it is true in I");
                    ngAdd.insert(NogoodContainer::createLiteral(id.address, false));
                }
                else {
                    DBGLOG(DBG, "Skipping ordinary " << id.address << " because it is not in the domain and can therefore never be in the unfounded set");
                }
            }
            else {
                // negative
                DBGLOG(DBG, "Inserting " << id.address << " because it is false in I u -X if it is in X");
                ngAdd.insert(NogoodContainer::createLiteral(residualShadow[id.address], false));
            }
        }
    }
    if (skip) {
        return std::vector<Nogood>();
    }
    else {
        DBGLOG(DBG, "Adding transformed nogood " << ngAdd);
        std::vector<Nogood> result;
        result.push_back(ngAdd);
        return result;
    }
}


void AssumptionBasedUnfoundedSetChecker::learnNogoodsFromMainSearch(bool reset)
{

    // add newly learned nogoods from the main search (in transformed form)
    if (!!ngc) {
        // detect resets of the nogood container
        if (learnedNogoodsFromMainSearch > ngc->getNogoodCount() || reset) learnedNogoodsFromMainSearch = 0;
        DBGLOG(DBG, "O: Adding valid input-output relationships from nogood container");
        for (int i = learnedNogoodsFromMainSearch; i < ngc->getNogoodCount(); ++i) {
            const Nogood& ng = ngc->getNogood(i);
            if (ng.isGround()) {
                DBGLOG(DBG, "Processing learned nogood " << ng.getStringRepresentation(reg));

                // this transformation must not depend on the compatible set!
                std::vector<Nogood> transformed = nogoodTransformation(ng, InterpretationConstPtr());
                BOOST_FOREACH (Nogood tng, transformed) {
                    solver->addNogood(tng);
                }
            }
        }
        learnedNogoodsFromMainSearch = ngc->getNogoodCount();
    }
}


std::vector<IDAddress> AssumptionBasedUnfoundedSetChecker::getUnfoundedSet(InterpretationConstPtr compatibleSet, const std::set<ID>& skipProgram)
{

    DBGLOG(DBG, "Performing UFS Check wrt. " << *compatibleSet);

    // check if the instance needs to be extended
    DBGLOG(DBG, "Checking if problem encoding needs to be expanded");
    if (groundProgram.idb.size() > problemRuleCount) {
        DBGLOG(DBG, "Problem encoding needs to be expanded");
        expandUFSDetectionProblemAndReinstantiateSolver();
    }
    else {
        DBGLOG(DBG, "Problem encoding does not need to be expanded");
    }

    // learn from main search
    learnNogoodsFromMainSearch(true);

    // load assumptions
    setAssumptions(compatibleSet, skipProgram);

    // we need the compatible set also without external atom replacement atoms
    InterpretationConstPtr compatibleSetWithoutAux = compatibleSet->getInterpretationWithoutExternalAtomAuxiliaries();

    int mCnt = 0;

    #ifdef DLVHEX_BENCHMARK
    DLVHEX_BENCHMARK_REGISTER(ufscheck, "UFS Check");
    DLVHEX_BENCHMARK_REGISTER(oufscheck, "Ordinary UFS Check");
    if( mode == WithExt ) {
        DLVHEX_BENCHMARK_START(ufscheck);
    }
    else {
        DLVHEX_BENCHMARK_START(oufscheck);
    }
    BOOST_SCOPE_EXIT( (ufscheck)(oufscheck)(mode) ) {
        if( mode == WithExt ) {
            DLVHEX_BENCHMARK_STOP(ufscheck);
        }
        else {
            DLVHEX_BENCHMARK_STOP(oufscheck);
        }
    } BOOST_SCOPE_EXIT_END
        #endif

        DLVHEX_BENCHMARK_REGISTER(sidufsenum, "UFS-Detection Problem Solving");
    if (mode == WithExt) {
        DLVHEX_BENCHMARK_START(sidufsenum);
    }
    InterpretationConstPtr model = solver->getNextModel();
    if (mode == WithExt) {
        DLVHEX_BENCHMARK_STOP(sidufsenum);
    }
    while ( model != InterpretationConstPtr()) {
        if (mode == WithExt) {
            DLVHEX_BENCHMARK_REGISTER_AND_COUNT(ufscandidates, "Checked UFS candidates", 1);
        }

        // check if the model is actually an unfounded set
        DBGLOG(DBG, "Got UFS candidate: " << *model);
        mCnt++;

        if (mode == Ordinary || isUnfoundedSet(compatibleSet, compatibleSetWithoutAux, model)) {
            DBGLOG(DBG, "Found UFS: " << *model << " (interpretation: " << *compatibleSet << ")");

            std::vector<IDAddress> ufs;

            bm::bvector<>::enumerator en = model->getStorage().first();
            bm::bvector<>::enumerator en_end = model->getStorage().end();
            while (en < en_end) {
                if (domain->getFact(*en)) ufs.push_back(*en);
                en++;
            }

            DBGLOG(DBG, "Enumerated " << mCnt << " UFS candidates");

            if (mode == WithExt) {
                DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sidfailedufscheckcount, "Failed UFS Checks", 1);
            }

            return ufs;
        }
        else {
            DBGLOG(DBG, "No UFS: " << *model);
        }

        if (mode == WithExt) {
            DLVHEX_BENCHMARK_START(sidufsenum);
        }
        model = solver->getNextModel();
        if (mode == WithExt) {
            DLVHEX_BENCHMARK_STOP(sidufsenum);
        }
    }

    DBGLOG(DBG, "Enumerated " << mCnt << " UFS candidates");

    // no ufs
    std::vector<IDAddress> ufs;
    return ufs;
}


/*
 * Manager for unfounded set checkers
 *
 * The manager takes care of creating UFS checkers for all components of the program which require a UFS check.
 * During search, the manager will call all unfounded set checkers until a definite answer to the query can be given.
 */

UnfoundedSetCheckerManager::UnfoundedSetCheckerManager(
BaseModelGenerator& mg,
ProgramCtx& ctx,
const AnnotatedGroundProgram& agp,
bool choiceRuleCompatible,
SimpleNogoodContainerPtr ngc) :
mg(&mg), ctx(ctx), agp(agp), lastAGPComponentCount(0), choiceRuleCompatible(choiceRuleCompatible), ngc(ngc)
{

    computeChoiceRuleCompatibility(choiceRuleCompatible);
    if (!ctx.config.getOption("LazyUFSCheckerInitialization")) initializeUnfoundedSetCheckers();

    /*
    // Test incremental definition of an instance
    std::cout << "=================" << std::endl;
    OrdinaryAtom at1(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
    at1.tuple.push_back(ctx.registry()->storeConstantTerm("a"));
    ID at1ID = ctx.registry()->storeOrdinaryGAtom(at1);

    OrdinaryAtom at2(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
    at2.tuple.push_back(ctx.registry()->storeConstantTerm("b"));
    ID at2ID = ctx.registry()->storeOrdinaryGAtom(at2);

    OrdinaryAtom at3(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
    at3.tuple.push_back(ctx.registry()->storeConstantTerm("c"));
    ID at3ID = ctx.registry()->storeOrdinaryGAtom(at3);

    InterpretationPtr frozen(new Interpretation(ctx.registry()));
    //frozen->setFact(at1ID.address);
    //frozen->setFact(at2ID.address);
    frozen->setFact(at3ID.address);

    Rule r1(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);
    r1.head.push_back(at1ID);
    r1.body.push_back(ID::posLiteralFromAtom(at2ID));
    std::vector<ID> idb1;
    idb1.push_back(ctx.registry()->storeRule(r1));
    //OrdinaryASPProgram p1(ctx.registry(), idb1, InterpretationPtr(new Interpretation(ctx.registry())), ctx.maxint, InterpretationPtr());

    Rule r2(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);
    r2.head.push_back(at2ID);
    r2.body.push_back(ID::posLiteralFromAtom(at1ID));
    idb1.push_back(ctx.registry()->storeRule(r2));

    Rule r3(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);
    r3.head.push_back(at2ID);
    r3.body.push_back(ID::posLiteralFromAtom(at3ID));
    idb1.push_back(ctx.registry()->storeRule(r3));

    OrdinaryASPProgram p1(ctx.registry(), idb1, InterpretationPtr(new Interpretation(ctx.registry())), ctx.maxint, InterpretationPtr());

    GenuineGroundSolverPtr ss = GenuineGroundSolver::getInstance(ctx, p1, frozen);

    std::vector<ID> ass;
    ass.push_back(ID::nafLiteralFromAtom(at3ID));
    ss->restartWithAssumptions(ass);
    std::cout << "It 1" << std::endl;
    InterpretationConstPtr model;
    while (!!(model = ss->getNextModel())){
      std::cout << *model << std::endl;
    }

    std::cout << "=================" << std::endl;
    */
}


UnfoundedSetCheckerManager::UnfoundedSetCheckerManager(
ProgramCtx& ctx,
const AnnotatedGroundProgram& agp,
bool choiceRuleCompatible) :
ctx(ctx), mg(0), agp(agp), choiceRuleCompatible(choiceRuleCompatible), lastAGPComponentCount(0)
{

    computeChoiceRuleCompatibility(choiceRuleCompatible);
    if (!ctx.config.getOption("LazyUFSCheckerInitialization")) initializeUnfoundedSetCheckers();
}


void UnfoundedSetCheckerManager::initializeUnfoundedSetCheckers()
{

    bool flpdc_head = (ctx.config.getOption("FLPDecisionCriterionHead") != 0);
    bool flpdc_e = (ctx.config.getOption("FLPDecisionCriterionE") != 0);

    if (ctx.config.getOption("UFSCheckMonolithic")) {
        if (mg && (agp.hasECycles() || !flpdc_e)) {
            preparedUnfoundedSetCheckers.insert(std::pair<int, UnfoundedSetCheckerPtr>
                (0, instantiateUnfoundedSetChecker(*mg, ctx, agp.getGroundProgram(), agp, InterpretationConstPtr(), ngc))
                );
        }
        else {
            preparedUnfoundedSetCheckers.insert(std::pair<int, UnfoundedSetCheckerPtr>
                (0, instantiateUnfoundedSetChecker(ctx, agp.getGroundProgram(), InterpretationConstPtr(), ngc))
                );
        }
    }
    else {
        for (int comp = 0; comp < agp.getComponentCount(); ++comp) {
            if ((!agp.hasHeadCycles(comp) && flpdc_head) && !intersectsWithNonHCFDisjunctiveRules[comp] && (!mg || !agp.hasECycles(comp) && flpdc_e)) {
                DBGLOG(DBG, "Skipping component " << comp << " because it contains neither head-cycles nor e-cycles");
                continue;
            }

            if (mg && (agp.hasECycles(comp) || !flpdc_e)) {
                preparedUnfoundedSetCheckers.insert(std::pair<int, UnfoundedSetCheckerPtr>
                    (comp, instantiateUnfoundedSetChecker(*mg, ctx, agp.getProgramOfComponent(comp), agp, agp.getAtomsOfComponent(comp), ngc))
                    );
            }
            else {
                preparedUnfoundedSetCheckers.insert(std::pair<int, UnfoundedSetCheckerPtr>
                    (comp, instantiateUnfoundedSetChecker(ctx, agp.getProgramOfComponent(comp), agp.getAtomsOfComponent(comp), ngc))
                    );
            }
        }
    }
}


void UnfoundedSetCheckerManager::computeChoiceRuleCompatibility(bool choiceRuleCompatible)
{

    for (int comp = 0; comp < agp.getComponentCount(); ++comp) {
        computeChoiceRuleCompatibilityForComponent(choiceRuleCompatible, comp);
    }
}


void UnfoundedSetCheckerManager::computeChoiceRuleCompatibilityForComponent(bool choiceRuleCompatible, int comp)
{

    if (agp.hasHeadCycles(comp) || !choiceRuleCompatible) {
        intersectsWithNonHCFDisjunctiveRules.push_back(false);
    }
    else {
        // Check if the component contains a disjunctive non-HCF rule
        // Note that this does not necessarily mean that the component is non-NCF!
        // Example:
        //    a v b v c.
        //    a :- b.
        //    b :- a.
        //    a :- c.
        //    d :- c.
        //    c :- d.
        // The program has the following components:
        // {a, b} with rules  a v b v c.
        //                    a :- b.
        //                    b :- a.
        //                    a :- c.
        // {c, d} with rules  a v b v c.
        //                    d :- c.
        //                    c :- d.
        // Note that the component {c, d} contains a Non-HCF disjunctive rule (a v b v c.), but is not Non-HCF itself.
        // Therefore, the optimization would skip the UFS check for the component {c, d} (and do it for {a, b}).
        //
        // If disjunctive rules are considered as such, this is sufficient because the (polynomial) UFS check implemented
        // directly in the reasoner detects the unfounded set:
        //    A candidate is {c, a, b, d}, but none of these atoms can use the rule a v b v c. as source because it is satisfied independently of {a}, {b} and {c}.
        // However, if disjunctive rules are transformed to choice rules, then a v b v c. becomes 1{a, b, c} and MULTIPLE atoms may use it as source.
        //
        // Therefore, the (exponential) UFS check in this class is not only necessary for Non-HCF-components, but also for HCF-components which contain disjunctive rules
        // which also also in some other Non-HCF-components.
        bool dh = false;
        BOOST_FOREACH (ID ruleID, agp.getProgramOfComponent(comp).idb) {
            if (agp.containsHeadCycles(ruleID)) {
                dh = true;
                break;
            }
        }
        intersectsWithNonHCFDisjunctiveRules.push_back(dh);
    }
}


UnfoundedSetCheckerPtr UnfoundedSetCheckerManager::instantiateUnfoundedSetChecker(
ProgramCtx& ctx,
const OrdinaryASPProgram& groundProgram,
InterpretationConstPtr componentAtoms,
SimpleNogoodContainerPtr ngc)
{

    if (ctx.config.getOption("UFSCheckAssumptionBased") && false) {
        DBGLOG(DBG, "instantiateUnfoundedSetChecker ordinary/assumption-based");
        return UnfoundedSetCheckerPtr(new AssumptionBasedUnfoundedSetChecker(ctx, groundProgram, componentAtoms, ngc));
    }
    else {
        DBGLOG(DBG, "instantiateUnfoundedSetChecker ordinary/encoding-based");
        return UnfoundedSetCheckerPtr(new EncodingBasedUnfoundedSetChecker(ctx, groundProgram, componentAtoms, ngc));
    }
}


UnfoundedSetCheckerPtr UnfoundedSetCheckerManager::instantiateUnfoundedSetChecker(
BaseModelGenerator& mg,
ProgramCtx& ctx,
const OrdinaryASPProgram& groundProgram,
const AnnotatedGroundProgram& agp,
InterpretationConstPtr componentAtoms,
SimpleNogoodContainerPtr ngc)
{

    if (ctx.config.getOption("UFSCheckAssumptionBased")) {
        DBGLOG(DBG, "instantiateUnfoundedSetChecker external/assumption-based");
        return UnfoundedSetCheckerPtr(new AssumptionBasedUnfoundedSetChecker(mg, ctx, groundProgram, agp, componentAtoms, ngc));
    }
    else {
        DBGLOG(DBG, "instantiateUnfoundedSetChecker external/encoding-based");
        return UnfoundedSetCheckerPtr(new EncodingBasedUnfoundedSetChecker(mg, ctx, groundProgram, agp, componentAtoms, ngc));
    }
}


void UnfoundedSetCheckerManager::learnNogoodsFromMainSearch(bool reset)
{

    // notify all unfounded set checkers
    typedef std::pair<int, UnfoundedSetCheckerPtr> Pair;
    BOOST_FOREACH (Pair p, preparedUnfoundedSetCheckers) {
        p.second->learnNogoodsFromMainSearch(reset);
    }
}


std::vector<IDAddress> UnfoundedSetCheckerManager::getUnfoundedSet(
InterpretationConstPtr interpretation,
const std::set<ID>& skipProgram,
SimpleNogoodContainerPtr ngc)
{

    bool flpdc_head = (ctx.config.getOption("FLPDecisionCriterionHead") != 0);
    bool flpdc_e = (ctx.config.getOption("FLPDecisionCriterionE") != 0);

    // in incremental mode we need to proceed as we can decide the FLP criterion only after checking each component for necessary extensions
    if (!ctx.config.getOption("IncrementalGrounding")) {
        if ((!agp.hasHeadCycles() && flpdc_head) && (!mg || !agp.hasECycles() && flpdc_e)) {
            DBGLOG(DBG, "Skipping UFS check program it contains neither head-cycles nor e-cycles");
            return std::vector<IDAddress>();
        }
    }

    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "UnfoundedSetChkMgr::getUFS");
    std::vector<IDAddress> ufs;
    if (ctx.config.getOption("UFSCheckMonolithic")) {
        if (preparedUnfoundedSetCheckers.size() > 0 && agp.getComponentCount() > lastAGPComponentCount) {
        }
        lastAGPComponentCount = agp.getComponentCount();

        DBGLOG(DBG, "UnfoundedSetCheckerManager::getUnfoundedSet monolithic");
        if (mg && (agp.hasECycles() || !flpdc_e)) {
            DBGLOG(DBG, "Checking UFS under consideration of external atoms");
            if (preparedUnfoundedSetCheckers.size() == 0) {
                preparedUnfoundedSetCheckers.insert(std::pair<int, UnfoundedSetCheckerPtr>
                    (0, instantiateUnfoundedSetChecker(*mg, ctx, agp.getGroundProgram(), agp, InterpretationConstPtr(), ngc))
                    );
            }
            UnfoundedSetCheckerPtr ufsc = preparedUnfoundedSetCheckers.find(0)->second;
            ufs = ufsc->getUnfoundedSet(interpretation, skipProgram);
            if (ufs.size() > 0) {
                DBGLOG(DBG, "Found a UFS");
                ufsnogood = ufsc->getUFSNogood(ufs, interpretation);
            }
        }
        else {
            DBGLOG(DBG, "Checking UFS without considering external atoms");
            if (preparedUnfoundedSetCheckers.size() == 0) {
                preparedUnfoundedSetCheckers.insert(std::pair<int, UnfoundedSetCheckerPtr>
                    (0, instantiateUnfoundedSetChecker(ctx, agp.getGroundProgram(), InterpretationConstPtr(), ngc))
                    );
            }
            UnfoundedSetCheckerPtr ufsc = preparedUnfoundedSetCheckers.find(0)->second;
            ufs = ufsc->getUnfoundedSet(interpretation, skipProgram);
            if (ufs.size() > 0) {
                DBGLOG(DBG, "Found a UFS");
                ufsnogood = ufsc->getUFSNogood(ufs, interpretation);
            }
        }
    }
    else {
        for (int i = intersectsWithNonHCFDisjunctiveRules.size(); i < agp.getComponentCount(); ++i) {
            computeChoiceRuleCompatibilityForComponent(choiceRuleCompatible, i);
        }

        // search in each component for unfounded sets
        DBGLOG(DBG, "UnfoundedSetCheckerManager::getUnfoundedSet component-wise");
        for (int comp = 0; comp < agp.getComponentCount(); ++comp) {
            if ((!agp.hasHeadCycles(comp) && flpdc_head) && !intersectsWithNonHCFDisjunctiveRules[comp] && (!mg || !agp.hasECycles(comp) && flpdc_e)) {
                DBGLOG(DBG, "Skipping component " << comp << " because it contains neither head-cycles nor e-cycles");
                continue;
            }

            DBGLOG(DBG, "Checking for UFS in component " << comp);
            if (mg && (agp.hasECycles(comp) || !flpdc_e)) {
                DBGLOG(DBG, "Checking UFS under consideration of external atoms");
                if (preparedUnfoundedSetCheckers.find(comp) == preparedUnfoundedSetCheckers.end()) {
                    preparedUnfoundedSetCheckers.insert(std::pair<int, UnfoundedSetCheckerPtr>
                        (comp, instantiateUnfoundedSetChecker(*mg, ctx, agp.getProgramOfComponent(comp), agp, agp.getAtomsOfComponent(comp), ngc))
                        );
                }
                UnfoundedSetCheckerPtr ufsc = preparedUnfoundedSetCheckers.find(comp)->second;
                ufs = ufsc->getUnfoundedSet(interpretation, skipProgram);
                if (ufs.size() > 0) {
                    DBGLOG(DBG, "Found a UFS");
                    ufsnogood = ufsc->getUFSNogood(ufs, interpretation);
                    break;
                }
            }
            else {
                DBGLOG(DBG, "Checking UFS without considering external atoms");
                if (preparedUnfoundedSetCheckers.find(comp) == preparedUnfoundedSetCheckers.end()) {
                    preparedUnfoundedSetCheckers.insert(std::pair<int, UnfoundedSetCheckerPtr>
                        (comp, instantiateUnfoundedSetChecker(ctx, agp.getProgramOfComponent(comp), agp.getAtomsOfComponent(comp), ngc))
                        );
                }
                UnfoundedSetCheckerPtr ufsc = preparedUnfoundedSetCheckers.find(comp)->second;
                ufs = ufsc->getUnfoundedSet(interpretation, skipProgram);
                if (ufs.size() > 0) {
                    DBGLOG(DBG, "Found a UFS");
                    ufsnogood = ufsc->getUFSNogood(ufs, interpretation);
                    break;
                }
            }
        }
    }

    // no ufs found
    if (ufs.size() == 0) {
        DBGLOG(DBG, "No component contains a UFS");
    }
    else {
        #ifndef NDEBUG
        //		// all elements in the unfounded set must be in the domain
        //		BOOST_FOREACH (IDAddress adr, ufs){
        //			assert(domain->getFact(adr) && "UFS contains a non-domain atom");
        //		}
        #endif
    }
    return ufs;
}


std::vector<IDAddress> UnfoundedSetCheckerManager::getUnfoundedSet(InterpretationConstPtr interpretation)
{
    static std::set<ID> emptySkipProgram;
    return getUnfoundedSet(interpretation, emptySkipProgram);
}


Nogood UnfoundedSetCheckerManager::getLastUFSNogood() const
{
    #ifndef NDEBUG
    //	// all elements in the nogood must be in the domain
    //	BOOST_FOREACH (ID lit, ufsnogood){
    //		assert(domain->getFact(lit.address) && "UFS nogood contains a non-domain atom");
    //	}
    #endif
    return ufsnogood;
}


DLVHEX_NAMESPACE_END
// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
