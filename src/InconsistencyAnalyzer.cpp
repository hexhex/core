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
 * @file   InconsistencyAnalyzer.cpp
 * @author Christoph Redl
 * @date Wed April 20 2016
 *
 * @brief  Computes a reason for the inconsistency in a program unit.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dlvhex2/BaseModelGenerator.h"
#include "dlvhex2/InconsistencyAnalyzer.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/InternalGrounder.h"
#include "dlvhex2/SATSolver.h"

#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/strong_components.hpp>
#include <boost/graph/filtered_graph.hpp>

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

InconsistencyAnalyzer::InconsistencyAnalyzer(ProgramCtx& ctx) : ctx(ctx){
}

Nogood InconsistencyAnalyzer::getInconsistencyReason(BaseModelGenerator* mg, InterpretationConstPtr explAtoms, InterpretationConstPtr unitInput, std::vector<ID>& innerEatoms, OrdinaryASPProgram& program, AnnotatedGroundProgram& annotatedOptimizedProgram, bool* haveInconsistencyReason){

#ifndef NDEBUG
    DBGLOG(DBG, "Performing inconsistency analyzis for program:" << std::endl << *program.edb << std::endl << printManyToString<RawPrinter>(program.idb, "\n", ctx.registry()));
#endif

    // ground the program without optimization
    InternalGrounder grounder(ctx, program, InternalGrounder::builtin);
    OrdinaryASPProgram gp = grounder.getGroundProgram();

#ifndef NDEBUG
    DBGLOG(DBG, "Unoptimized grounded program:" << std::endl << *gp.edb << std::endl << printManyToString<RawPrinter>(gp.idb, "\n", ctx.registry()));
#endif

    // Construct domain of the SAT instance.
    // It consists of all external atom input atoms and all ordinary atoms in the program (to be added below).
    // It can be constructed by using the program mask of the previous (optimized) grounding and adding newly introduced atoms in the non-optimized grounding.
    // All input atoms from predecessor components are already contained in the program mask of the optimized grounding.
    InterpretationPtr domain(new Interpretation(*annotatedOptimizedProgram.getProgramMask()));

    for (int eaIndex = 0; eaIndex < innerEatoms.size(); ++eaIndex) {
        const ExternalAtom& eatom = ctx.registry()->eatoms.getByID(innerEatoms[eaIndex]);
        eatom.updatePredicateInputMask();
        domain->add(*(eatom.getPredicateInputMask()));
    }

    bm::bvector<>::enumerator en, en_end;

    // remove input from predecessor units
    InterpretationPtr newEdb(new Interpretation(*gp.edb));
    newEdb->getStorage() -= unitInput->getStorage();
    gp.edb = newEdb;

#ifndef NDEBUG
    DBGLOG(DBG, "Program to be analyzed:" << std::endl << *gp.edb << std::endl << printManyToString<RawPrinter>(gp.idb, "\n", ctx.registry()));
#endif

    NogoodSet instance;

    // add input is pseudo-nogoods to make sure they are part of the problem
    en = unitInput->getStorage().first();
    en_end = unitInput->getStorage().end();
    while (en < en_end) {
        Nogood ng;
        ng.insert(NogoodContainer::createLiteral(*en, true));
        ng.insert(NogoodContainer::createLiteral(*en, false));
        instance.addNogood(ng);
        en++;
    }

    // interprete the ground program as a set of classical implications
    if (!!gp.edb) {
        en = gp.edb->getStorage().first();
        en_end = gp.edb->getStorage().end();
        while (en < en_end) {
            Nogood ng;
            ng.insert(NogoodContainer::createLiteral(*en, false));
            instance.addNogood(ng);
//            domain->setFact(*en);
            en++;
        }
    }
    BOOST_FOREACH (ID ruleID, gp.idb) {
        const Rule& rule = ctx.registry()->rules.getByID(ruleID);
	    Nogood ng;
        BOOST_FOREACH (ID h, rule.head) { ng.insert(NogoodContainer::createLiteral(h.address, false)); domain->setFact(h.address); }
        BOOST_FOREACH (ID b, rule.body) { ng.insert(NogoodContainer::createLiteral(b.address, !b.isNaf())); domain->setFact(b.address); }
        instance.addNogood(ng);
    }
    SATSolverPtr classicalSolver = SATSolver::getInstance(ctx, instance /*, domain*/);

    // explanation atoms are all atoms from the domain which are not defined in the given program, i.e., which do not unify with a head atom in program
/*
    InterpretationPtr explanationAtoms(new Interpretation(ctx.registry()));
    en = domain->getStorage().first();
    en_end = domain->getStorage().end();
    while (en < en_end) {
        const OrdinaryAtom& datom = ctx.registry()->ogatoms.getByAddress(*en);

        bool definedInProgram = false;
        BOOST_FOREACH (ID ruleID, program.idb) {
            const Rule& rule = ctx.registry()->rules.getByID(ruleID);
            BOOST_FOREACH (ID hatomID, rule.head) {
                const OrdinaryAtom& hatom = ctx.registry()->lookupOrdinaryAtom(hatomID);
                if (datom.unifiesWith(hatom)) {
                    definedInProgram = true;
                    break;
                }
                en++;
            }
        }

        if (!definedInProgram) explanationAtoms->setFact(*en);
    }
*/
    DBGLOG(DBG, "Explanation atoms: " << *explAtoms);

    // compute the classical models and an inconsistency reason
    Nogood inconsistencyReason;
    InterpretationPtr alreadyPos(new Interpretation(ctx.registry()));
    InterpretationPtr alreadyNeg(new Interpretation(ctx.registry()));
    InterpretationConstPtr model;
    while ( (model = classicalSolver->getNextModel()) != InterpretationConstPtr() ) {
        // compatibility check
        bool verified = true;
        for (int eaIndex = 0; eaIndex < innerEatoms.size() && verified; ++eaIndex){
            BaseModelGenerator::VerifyExternalAtomCB vcb(model, ctx.registry()->eatoms.getByID(innerEatoms[eaIndex]), *(annotatedOptimizedProgram.getEAMask(eaIndex)));
            mg->evaluateExternalAtom(ctx, innerEatoms[eaIndex], model, vcb, ctx.config.getOption("ExternalLearning") ? classicalSolver : NogoodContainerPtr());
            verified &= vcb.verify();
        }

        if (verified) {
            DBGLOG(DBG, "Got classical model: " << *model);

            // add a literal "l" over some explanation atom "a", where the sign of "l" is the opposite of "a"'s sign in the model
            en = explAtoms->getStorage().first();
            en_end = explAtoms->getStorage().end();
            while (en < en_end) {
                // Use its negation in the inconsistency explanation; this will either:
                // (i) prevent super sets of this model from becoming models of the program (namely if l="F a"; then adding "a" as fact will eliminate the model because "a" must be true); or
                // (ii) ensure that there is a non-empty unfounded set (namely if l=T a); then *not* adding "a" as fact will leave "a" unfounded)
                if (model->getFact(*en)) {
                    if (alreadyPos->getFact(*en)) { en++; continue; } // cannot use this literal because it needs to be added negatively but is already positive
                    if (alreadyNeg->getFact(*en)) break; // needs to be added negatively and is already negative --> done
                    DBGLOG(DBG, "Adding -" << printToString<RawPrinter>(ctx.registry()->ogatoms.getIDByAddress(*en), ctx.registry()) << " to reason nogood");
                    inconsistencyReason.insert(NogoodContainer::createLiteral(*en, false));
                    alreadyNeg->setFact(*en);

                    // eliminate all models with this literal
                    Nogood ng;
                    ng.insert(NogoodContainer::createLiteral(*en, true));
                    classicalSolver->addNogood(ng);
                    break;
                }else{
                    if (alreadyNeg->getFact(*en)) { en++; continue; } // cannot use this literal because it needs to be added positively but is already negative
                    if (alreadyPos->getFact(*en)) break; // needs to be added positively and is already positive --> done
                    DBGLOG(DBG, "Adding " << printToString<RawPrinter>(ctx.registry()->ogatoms.getIDByAddress(*en), ctx.registry()) << " to reason nogood");
                    inconsistencyReason.insert(NogoodContainer::createLiteral(*en, true));
                    alreadyPos->setFact(*en);

                    // eliminate all models with this literal
                    Nogood ng;
                    ng.insert(NogoodContainer::createLiteral(*en, false));
                    classicalSolver->addNogood(ng);
                    break;
                }
                en++;
            }
            if (en == en_end) {
                // could not find a reason
                DBGLOG(DBG, "No inconsistency reason found");
                *haveInconsistencyReason = false;
                return Nogood();
            }
        }
    }
    DBGLOG(DBG, "Found inconsistency reason: " << inconsistencyReason.getStringRepresentation(ctx.registry()) << std::endl << "for program " << *gp.edb << std::endl << printManyToString<RawPrinter>(gp.idb, "\n", ctx.registry()) << std::endl << "wrt. explanation atoms " << *explAtoms);
    *haveInconsistencyReason = true;
    return inconsistencyReason;
}

DLVHEX_NAMESPACE_END

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
