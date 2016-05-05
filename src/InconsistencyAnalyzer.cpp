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

ID InconsistencyAnalyzer::getAuxiliaryAtom(char type, ID id){
    OrdinaryAtom oatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX);
    oatom.tuple.push_back(ctx.registry()->getAuxiliaryConstantSymbol(type, id));
    return ctx.registry()->storeOrdinaryAtom(oatom);
}

Nogood InconsistencyAnalyzer::getInconsistencyReason(BaseModelGenerator* mg, InterpretationConstPtr explAtoms, std::vector<ID>& innerEatoms, OrdinaryASPProgram& program, AnnotatedGroundProgram& annotatedOptimizedProgram, bool* haveInconsistencyReason){

#ifndef NDEBUG
    DBGLOG(DBG, "Performing inconsistency analyzis for program:" << std::endl << *program.edb << std::endl << printManyToString<RawPrinter>(program.idb, "\n", ctx.registry()));
#endif

    // ground the program without optimization
    InternalGrounder grounder(ctx, program, InternalGrounder::builtin);
    OrdinaryASPProgram gp = grounder.getGroundProgram();

#ifndef NDEBUG
    DBGLOG(DBG, "Unoptimized grounded program:" << std::endl << *gp.edb << std::endl << printManyToString<RawPrinter>(gp.idb, "\n", ctx.registry()));
#endif
    InterpretationPtr programAtoms(new Interpretation(*gp.edb));
    BOOST_FOREACH (ID ruleID, gp.idb) {
        const Rule& rule = ctx.registry()->rules.getByID(ruleID);
        BOOST_FOREACH (ID h, rule.head) programAtoms->setFact(h.address);
        BOOST_FOREACH (ID b, rule.body) programAtoms->setFact(b.address);
    }

    // construct analysis program
    InterpretationPtr analysisProgramEdb(new Interpretation(ctx.registry()));
    OrdinaryASPProgram analysisProgram(ctx.registry(), std::vector<ID>(), analysisProgramEdb, ctx.maxint);
    bm::bvector<>::enumerator en, en_end;
    int nextAtomID = 0;
    ID satAtom = getAuxiliaryAtom('x', ID::termFromInteger(nextAtomID++));

    // explanation guess
    DBGLOG(DBG, "Adding guessing rules for explanation atoms " << *explAtoms);
    InterpretationPtr negExplAtoms(new Interpretation(ctx.registry()));
    en = explAtoms->getStorage().first();
    en_end = explAtoms->getStorage().end();
    while (en < en_end) {
        Rule explanationGuess(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR | ID::PROPERTY_RULE_DISJ);
        ID eAtomID = ctx.registry()->ogatoms.getIDByAddress(*en);
        ID neAtomID = getAuxiliaryAtom('x', eAtomID);
        negExplAtoms->setFact(neAtomID.address);
        explanationGuess.head.push_back(eAtomID);                           // atom is in R+
        explanationGuess.head.push_back(neAtomID);                          // atom is in R-
        explanationGuess.head.push_back(getAuxiliaryAtom('y', eAtomID));    // atom is neither in R+ nor in R-
        analysisProgram.idb.push_back(ctx.registry()->storeRule(explanationGuess));
        en++;
    }

    // interpretation guess and saturation
    DBGLOG(DBG, "Adding guessing and saturation rules for program atoms " << *programAtoms);
    en = programAtoms->getStorage().first();
    en_end = programAtoms->getStorage().end();
    while (en < en_end) {
        // interpretation guess
        Rule interpretationGuess(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR | ID::PROPERTY_RULE_DISJ);
        ID atomID = ctx.registry()->ogatoms.getIDByAddress(*en);
        interpretationGuess.head.push_back(getAuxiliaryAtom('p', atomID));    // atom in interpretation
        interpretationGuess.head.push_back(getAuxiliaryAtom('n', atomID));    // -atom in interpretation
        analysisProgram.idb.push_back(ctx.registry()->storeRule(interpretationGuess));

        // saturation on discrepancy of interpretation guess from explanation guess
        {
            Rule discrepancy(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);
            discrepancy.head.push_back(satAtom);
            discrepancy.body.push_back(ID::posLiteralFromAtom(atomID));                                   // atom in explanation guess
            discrepancy.body.push_back(ID::posLiteralFromAtom(getAuxiliaryAtom('n', atomID)));            // -atom in interpretation
            analysisProgram.idb.push_back(ctx.registry()->storeRule(discrepancy));
        }{
            Rule discrepancy(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);
            discrepancy.head.push_back(satAtom);
            discrepancy.body.push_back(ID::posLiteralFromAtom(getAuxiliaryAtom('x', atomID)));            // atom in explanation guess
            discrepancy.body.push_back(ID::posLiteralFromAtom(getAuxiliaryAtom('p', atomID)));            // -atom in interpretation
            analysisProgram.idb.push_back(ctx.registry()->storeRule(discrepancy));
        }
        {
            Rule satInterpretation(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);
            satInterpretation.head.push_back(getAuxiliaryAtom('p', atomID));                              // -atom in interpretation
            satInterpretation.body.push_back(ID::posLiteralFromAtom(satAtom));
            analysisProgram.idb.push_back(ctx.registry()->storeRule(satInterpretation));
        }{
            Rule satInterpretation(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);
            satInterpretation.head.push_back(getAuxiliaryAtom('n', atomID));                              // -atom in interpretation
            satInterpretation.body.push_back(ID::posLiteralFromAtom(satAtom));
            analysisProgram.idb.push_back(ctx.registry()->storeRule(satInterpretation));
        }

        en++;
    }

    // saturation for non-models
    en = gp.edb->getStorage().first();
    en_end = gp.edb->getStorage().end();
    while (en < en_end) {
        // explanation atoms cannot be part of the EDB
//	    if (!explAtoms->getFact(*en)) {
                Rule satOnModelRule(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);
	        satOnModelRule.head.push_back(satAtom);
	        satOnModelRule.body.push_back(ID::posLiteralFromAtom(getAuxiliaryAtom('n', ctx.registry()->ogatoms.getIDByAddress(*en))));
                analysisProgram.idb.push_back(ctx.registry()->storeRule(satOnModelRule));
//	    }
        en++;
    }
    BOOST_FOREACH (ID ruleID, gp.idb) {
        DBGLOG(DBG, "Adding saturation rule for program rule " << printToString<RawPrinter>(ruleID, ctx.registry()));
        const Rule& rule = ctx.registry()->rules.getByID(ruleID);
        Rule satOnModelRule(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);
        satOnModelRule.head.push_back(satAtom);
        BOOST_FOREACH (ID h, rule.head) { satOnModelRule.body.push_back(getAuxiliaryAtom('n', h)); }
        BOOST_FOREACH (ID b, rule.body) {
            if (!b.isNaf()) {
                satOnModelRule.body.push_back(ID::posLiteralFromAtom(getAuxiliaryAtom('p', ID::atomFromLiteral(b))));
            }else{
                satOnModelRule.body.push_back(ID::posLiteralFromAtom(getAuxiliaryAtom('n', ID::atomFromLiteral(b))));
            }
        }
        analysisProgram.idb.push_back(ctx.registry()->storeRule(satOnModelRule));
    }

    // restrict search to models with atom sat
    {
        DBGLOG(DBG, "Adding sat constraint");
        Rule satConstraint(ID::MAINKIND_RULE | ID::SUBKIND_RULE_CONSTRAINT);
        satConstraint.body.push_back(ID::nafLiteralFromAtom(satAtom));
        analysisProgram.idb.push_back(ctx.registry()->storeRule(satConstraint));
    }

#ifndef NDEBUG
    if (!!gp.edb) {
        DBGLOG(DBG, "Analysis program:" << std::endl << *analysisProgramEdb.edb << std::endl << printManyToString<RawPrinter>(analysisProgram.idb, "\n", ctx.registry()));
    }else{
        DBGLOG(DBG, "Analysis program:" << std::endl << printManyToString<RawPrinter>(analysisProgram.idb, "\n", ctx.registry()));
    }
#endif

    // solve the instance
    GenuineGroundSolverPtr analysisSolver = GenuineGroundSolver::getInstance(ctx, analysisProgram);
    InterpretationConstPtr model;
    while ( (model = analysisSolver->getNextModel()) != InterpretationConstPtr() ) {
        DBGLOG(DBG, "Answer set of analysis program: " << *model);

        // extract explanation
        Nogood explanation;
        en = model->getStorage().first();
        en_end = model->getStorage().end();
        while (en < en_end) {
            if (explAtoms->getFact(*en)) explanation.insert(NogoodContainer::createLiteral(*en, true));
            else if (negExplAtoms->getFact(*en)) explanation.insert(NogoodContainer::createLiteral(ctx.registry()->getIDByAuxiliaryConstantSymbol(ctx.registry()->ogatoms.getByAddress(*en).tuple[0]).address, false));
            en++;
        }
        DBGLOG(DBG, "Explanation: " << explanation.getStringRepresentation(ctx.registry()));
        if (ctx.config.getOption("UserInconsistencyAnalysis")) {
            std::cout << "Inconsistency explanation: " << explanation.getStringRepresentation(ctx.registry()) << std::endl;
        }
    }

    return Nogood();
}

DLVHEX_NAMESPACE_END

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
