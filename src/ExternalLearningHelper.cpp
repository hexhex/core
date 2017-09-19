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
 * @file   ExternalLearningHelper.cpp
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 *
 * @brief  Provides helper functions for writing learning functions.
 *         Consider TestPlugin.cpp to see how these methods are used.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#include "dlvhex2/ExternalLearningHelper.h"
#include "dlvhex2/HexParser.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/InternalGrounder.h"
#include "dlvhex2/Benchmarking.h"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/functional/hash.hpp>

#include <boost/unordered_set.hpp>

#include <fstream>

DLVHEX_NAMESPACE_BEGIN

ExternalLearningHelper::DefaultInputNogoodProvider::DefaultInputNogoodProvider(bool negateMonotonicity) : negateMonotonicity(negateMonotonicity) {
}

bool ExternalLearningHelper::DefaultInputNogoodProvider::dependsOnOutputTuple() const {
    return false;
}

Nogood ExternalLearningHelper::DefaultInputNogoodProvider::operator()(const PluginAtom::Query& query, const ExtSourceProperties& prop, bool contained, const Tuple tuple, int* weakenedPremiseLiterals) const {

    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(inputprovider, "InpNogoodProvider::operator()");

    // store for each predicate term ID the index of the corresponding parameter in input
    std::map<ID, int> inputPredicateTable;
    int index = 0;

    BOOST_FOREACH(ID inp, query.input) {
        inputPredicateTable[inp] = index++;
    }

    // find relevant input: by default, the predicate mask of the external source counts; this can however be overridden for queries
    bm::bvector<>::enumerator en = query.predicateInputMask == InterpretationPtr() ? query.ctx->registry()->eatoms.getByID(query.eatomID).getPredicateInputMask()->getStorage().first() : query.predicateInputMask->getStorage().first();
    bm::bvector<>::enumerator en_end = query.predicateInputMask == InterpretationPtr() ? query.ctx->registry()->eatoms.getByID(query.eatomID).getPredicateInputMask()->getStorage().end() : query.predicateInputMask->getStorage().end();
    Nogood extNgInput;

    while (en < en_end) {
        // get the predicate of the current input atom
        ID pred = query.interpretation->getRegistry()->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en)).tuple[0];

        // find the parameter index of this atom
        int index = inputPredicateTable.find(pred)->second;

        // positive atoms are only required for non-antimonotonic input parameters
        // negative atoms are only required for non-monotonic input parameters
        // unassigned input atoms are not needed if the external source provides partial answers (i.e., works over partial interpretations)
        if (!prop.doesProvidePartialAnswer() || !query.assigned || query.assigned->getFact(*en)) {
            if (query.interpretation->getFact(*en) != negateMonotonicity) {
                // positive
                if (!prop.isAntimonotonic(index) || !query.ctx->config.getOption("ExternalLearningMonotonicity")) {
                    extNgInput.insert(NogoodContainer::createLiteral(*en, query.interpretation->getFact(*en)));
                }
            } else {
                // negative
                if (!prop.isMonotonic(index) || !query.ctx->config.getOption("ExternalLearningMonotonicity")) {
                    extNgInput.insert(NogoodContainer::createLiteral(*en, query.interpretation->getFact(*en)));
                }
            }
        } else {
            if (!!query.assigned && !query.assigned->getFact(*en)) {
                if (weakenedPremiseLiterals != 0) {
                    DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sidweakenednumber, "Weakened EA-nogood premises", 1);
                    *weakenedPremiseLiterals = *weakenedPremiseLiterals + 1;
                }
            }
        }
        en++;
    }
    DBGLOG(DBG, "Input nogood: " << extNgInput.getStringRepresentation(query.ctx->registry()));
    return extNgInput;
}

Set<ID> ExternalLearningHelper::getOutputAtoms(const PluginAtom::Query& query, const PluginAtom::Answer& answer, bool sign) {

    Set<ID> out;

    // construct replacement atom
    OrdinaryAtom replacement(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX | ID::PROPERTY_EXTERNALAUX);
    replacement.tuple.resize(1);
    replacement.tuple[0] = query.ctx->registry()->getAuxiliaryConstantSymbol(sign ? 'r' : 'n', query.ctx->registry()->eatoms.getByID(query.eatomID).predicate);

    if (query.ctx->config.getOption("IncludeAuxInputInAuxiliaries") && query.ctx->registry()->eatoms.getByID(query.eatomID).auxInputPredicate != ID_FAIL) {
        //		replacement.tuple.push_back(query.ctx->registry()->storeVariableTerm("X"));
        //		replacement.kind |= ID::SUBKIND_ATOM_ORDINARYN;
        replacement.tuple.push_back(query.ctx->registry()->eatoms.getByID(query.eatomID).auxInputPredicate);
    }
    replacement.tuple.insert(replacement.tuple.end(), query.input.begin(), query.input.end());
    int s = replacement.tuple.size();

    const std::vector<Tuple>& otuples = answer.get();

    BOOST_FOREACH(Tuple otuple, otuples) {
        replacement.tuple.resize(s);
        // add current output
        replacement.tuple.insert(replacement.tuple.end(), otuple.begin(), otuple.end());
        // get ID of this replacement atom
        ID idreplacement = NogoodContainer::createLiteral(query.ctx->registry()->storeOrdinaryAtom(replacement));
        out.insert(idreplacement);
    }

    return out;
}

ID ExternalLearningHelper::getOutputAtom(const PluginAtom::Query& query, Tuple otuple, bool sign) {

    bool ground = true;
    BOOST_FOREACH(ID o, otuple) if (o.isVariableTerm()) ground = false;

    // construct replacement atom with input from query
    OrdinaryAtom replacement(ID::MAINKIND_ATOM | ID::PROPERTY_AUX | ID::PROPERTY_EXTERNALAUX);
    if (ground) replacement.kind |= ID::SUBKIND_ATOM_ORDINARYG;
    else replacement.kind |= ID::SUBKIND_ATOM_ORDINARYN;
    replacement.tuple.resize(1);
    replacement.tuple[0] = query.ctx->registry()->getAuxiliaryConstantSymbol(sign ? 'r' : 'n', query.ctx->registry()->eatoms.getByID(query.eatomID).predicate);
    if (query.ctx->config.getOption("IncludeAuxInputInAuxiliaries") && query.ctx->registry()->eatoms.getByID(query.eatomID).auxInputPredicate != ID_FAIL) {
        //		replacement.tuple.push_back(query.ctx->registry()->storeVariableTerm("X"));
        //		replacement.kind |= ID::SUBKIND_ATOM_ORDINARYN;
        replacement.tuple.push_back(query.ctx->registry()->eatoms.getByID(query.eatomID).auxInputPredicate);
    }
    replacement.tuple.insert(replacement.tuple.end(), query.input.begin(), query.input.end());
    int s = replacement.tuple.size();

    // add output tuple
    replacement.tuple.insert(replacement.tuple.end(), otuple.begin(), otuple.end());

    ID idreplacement = NogoodContainer::createLiteral(query.ctx->registry()->storeOrdinaryAtom(replacement));
    return idreplacement;
}


#if 0

ID ExternalLearningHelper::getOutputAtom(const PluginAtom::Query& query, const Tuple& ituple, const Tuple& otuple, bool sign) {

    bool ground = true;
    BOOST_FOREACH(ID i, ituple) if (i.isVariableTerm()) ground = false;
    BOOST_FOREACH(ID o, otuple) if (o.isVariableTerm()) ground = false;

    // construct replacement atom with input from query
    OrdinaryAtom replacement(ID::MAINKIND_ATOM | ID::PROPERTY_AUX | ID::PROPERTY_EXTERNALAUX);
    if (ground) replacement.kind |= ID::SUBKIND_ATOM_ORDINARYG;
    else replacement.kind |= ID::SUBKIND_ATOM_ORDINARYN;
    replacement.tuple.resize(1);
    replacement.tuple[0] = query.ctx->registry()->getAuxiliaryConstantSymbol(sign ? 'r' : 'n', query.ctx->registry()->eatoms.getByID(query.eatomID).predicate);
    if (query.ctx->config.getOption("IncludeAuxInputInAuxiliaries") && query.ctx->registry()->eatoms.getByID(query.eatomID).auxInputPredicate != ID_FAIL) {
        //		replacement.tuple.push_back(query.ctx->registry()->storeVariableTerm("X"));
        //		replacement.kind |= ID::SUBKIND_ATOM_ORDINARYN;
        replacement.tuple.push_back(query.ctx->registry()->eatoms.getByID(query.eatomID).auxInputPredicate);
    }
    replacement.tuple.insert(replacement.tuple.end(), ituple.begin(), ituple.end());
    int s = replacement.tuple.size();

    // add output tuple
    replacement.tuple.insert(replacement.tuple.end(), otuple.begin(), otuple.end());

    ID idreplacement = NogoodContainer::createLiteral(query.ctx->registry()->storeOrdinaryAtom(replacement));
    return idreplacement;
}
#endif

ID ExternalLearningHelper::getIDOfLearningRule(ProgramCtx* ctx, std::string learningrule) {

    RegistryPtr reg = ctx->registry();

    // parse rule
    DBGLOG(DBG, "Parsing learning rule " << learningrule);
    InputProviderPtr ip(new InputProvider());
    ip->addStringInput(learningrule, "rule");
    ProgramCtx pc = *ctx;
    pc.edb = InterpretationPtr(new Interpretation(ctx->registry()));
    pc.idb.clear();
    ModuleHexParser hp;
    hp.parse(ip, pc);

    if (pc.edb->getStorage().count() > 0) {
        DBGLOG(DBG, "Learning Rule Error: Learning rule must not be a fact");
        return ID_FAIL;
    } else if (pc.idb.size() != 1) {
        DBGLOG(DBG, "Error: Got " << pc.idb.size() << " rules; must be 1");
        return ID_FAIL;
    } else {
        DBGLOG(DBG, "Got 1 learning rule");
        ID rid = pc.idb[0];
        const Rule& r = reg->rules.getByID(rid);

        // learning rules must not be constraints or disjunctive
        if (r.head.size() != 1) {
            DBGLOG(DBG, "Learning Rule Error: Learning rule is not ordinary (head size must be 1)");
            return ID_FAIL;
        }

        // learning rules must use only predicates "out" or "nout" (in head) and in[i] (in body)

        BOOST_FOREACH(ID hLit, r.head) {
            const OrdinaryAtom& oatom = hLit.isOrdinaryGroundAtom() ? reg->ogatoms.getByID(hLit) : reg->onatoms.getByID(hLit);
            std::string hPred = reg->terms.getByID(oatom.tuple[0]).getUnquotedString();
            if (hPred != "out" && hPred != "nout") {
                DBGLOG(DBG, "Learning Rule Error: Head predicate of learning rule must be \"out\" or \"nout\"");
                return ID_FAIL;
            }
        }

        BOOST_FOREACH(ID bLit, r.body) {
            const OrdinaryAtom& oatom = bLit.isOrdinaryGroundAtom() ? reg->ogatoms.getByID(bLit) : reg->onatoms.getByID(bLit);
            std::string bPred = reg->terms.getByID(oatom.tuple[0]).getUnquotedString();

            try {
                if (boost::starts_with(bPred, "in")) {
                    boost::lexical_cast<int>(bPred.c_str() + 2);
                } else {
                    throw std::bad_cast();
                }
            } catch (std::bad_cast) {
                DBGLOG(DBG, "Learning Rule Error: Body predicates must be of kind \"in[integer]\"");
                return ID_FAIL;
            }
        }

        return rid;
    }
}

Nogood ExternalLearningHelper::getMinimalConflict(Tuple t, const PluginAtom::Query& query, ID ansID, Nogood accNogood, int deltaSize, Nogood nogood, bool neg) {
    
    if (deltaSize != 0) {
        InterpretationPtr interpretation(new Interpretation(query.interpretation->getRegistry()));
        InterpretationPtr assigned(new Interpretation(query.interpretation->getRegistry()));

        set_iterator<ID> it = accNogood.begin();
        while (it != accNogood.end()) {
            if (!it->isNaf()) {
                interpretation->setFact(it->address);
            }
            assigned->setFact(it->address);
            it++;
        }

        PluginAtom::Query qa = query;
        qa.interpretation = interpretation;
        qa.assigned = assigned;

        PluginAtom::Answer ans;

        query.ctx->registry()->eatoms.getByID(query.eatomID).pluginAtom->retrieveFacade(qa, ans, NogoodContainerPtr(), query.ctx->config.getOption("UseExtAtomCache"));

        Set<ID> ansout = ExternalLearningHelper::getOutputAtoms(qa, ans, false);

        if (neg) {
            if (!(std::find(ans.get().begin(), ans.get().end(), t) != ans.get().end()) &&
                    !(std::find(ans.getUnknown().begin(), ans.getUnknown().end(), t) != ans.getUnknown().end())) {
                return Nogood();
            }
        } else {
            if (ansout.contains(ansID)) {
                return Nogood();
            }
        }

    }

    if (nogood.size() == 1) {
        return nogood;
    }

    Nogood c1;
    Nogood c2;
    int c = 0;

    // split nogood
    BOOST_FOREACH(ID& iid, nogood) {
        if (c < nogood.size() / 2) {
            c2.insert(iid);
        } else {
            c1.insert(iid);
        }
        c++;
    }

    Nogood accNogoodC1;

    accNogoodC1.insert(accNogood.begin(), accNogood.end());
    accNogoodC1.insert(c1.begin(), c1.end());

    Nogood d2 = ExternalLearningHelper::getMinimalConflict(t, query, ansID, accNogoodC1, c1.size(), c2, neg);

    accNogood.insert(d2.begin(), d2.end());

    Nogood d1 = ExternalLearningHelper::getMinimalConflict(t, query, ansID, accNogood, d2.size(), c1, neg);

    d1.insert(d2.begin(), d2.end());

    return d1;
}

void ExternalLearningHelper::learnFromInputOutputBehavior(const PluginAtom::Query& query, const PluginAtom::Answer& answer, const ExtSourceProperties& prop, NogoodContainerPtr nogoods, InputNogoodProviderConstPtr inp) {
    if (nogoods) {
        DBGLOG(DBG, "External Learning: IOBehavior" << (query.ctx->config.getOption("ExternalLearningMonotonicity") ? " by exploiting monotonicity" : ""));

        // containers for storing nogoods that still have to be minimized 
        SimpleNogoodContainer newNogoodsContainer;
        std::vector<std::pair<Nogood, ID> > newNogoods;

        Nogood extNgInput;
        int weakenedPremiseLiterals = 0;
        if (!inp->dependsOnOutputTuple()) extNgInput = (*inp)(query, prop, true, Tuple(), &weakenedPremiseLiterals);
        Set<ID> out = ExternalLearningHelper::getOutputAtoms(query, answer, false);

        BOOST_FOREACH(ID oid, out) {
            int weakenedPremiseLiterals2 = 0;
            Nogood extNg = !inp->dependsOnOutputTuple()
                    ? extNgInput
                    : (*inp)(query, prop, true, query.ctx->registry()->ogatoms.getByID(oid).tuple, &weakenedPremiseLiterals2);
            weakenedPremiseLiterals += weakenedPremiseLiterals2;

            extNg.insert(oid);
            DBGLOG(DBG, "Learned nogood " << extNg.getStringRepresentation(query.ctx->registry()) << " from input-output behavior");

            DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sidweakenednumber, "EA-Nogoods from weakened intr.", (weakenedPremiseLiterals > 0 ? 1 : 0));

            if (query.ctx->config.getOption("MinimizeNogoods") && !query.ctx->config.getOption("MinimizeNogoodsOpt") && !inp->dependsOnOutputTuple() && prop.doesProvidePartialAnswer()) {
                // if nogoods should be minimized store them in intermediary container
                DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidmin, "Nogood minimization");
                newNogoodsContainer.addNogood(extNg);
            } else if (query.ctx->config.getOption("MinimizeNogoods") && query.ctx->config.getOption("MinimizeNogoodsOpt") && !inp->dependsOnOutputTuple() && prop.doesProvidePartialAnswer()) {
                DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidmin, "Nogood minimization");
                // if answers w.r.t. the inputs should be cached, input and output atoms have to be stored separately
                std::pair<Nogood, ID> newNogood(extNgInput, oid);
                newNogoods.push_back(newNogood);
            } else {
                nogoods->addNogood(extNg);
            }
        }

        // nogood minimization without caching answers of external atom
        if (query.ctx->config.getOption("MinimizeNogoods") && !query.ctx->config.getOption("MinimizeNogoodsOpt") && !inp->dependsOnOutputTuple() && prop.doesProvidePartialAnswer()) {
            DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidmin, "Nogood minimization");
            // iterate through all newly added nogoods
            for (int i = 0; i < newNogoodsContainer.getNogoodCount(); ++i) {
                if (newNogoodsContainer.getNogood(i).size() <= query.ctx->config.getOption("MinimizationSize")) {
                    // copy the respective nogood
                    Nogood testNg = newNogoodsContainer.getNogood(i);
                    // store the ID of answer atom that should still be contained in answer after minimization
                    ID ansID;

                    BOOST_FOREACH(ID& iid, newNogoodsContainer.getNogood(i)) {
                        if (query.ctx->registry()->ogatoms.getIDByAddress(iid.address).isExternalAuxiliary()) {
                            ansID = iid;
                        }
                    }

                    if (!query.ctx->config.getOption("MinimizeNogoodsOnConflict") || (!query.inputi || query.inputi->getFact(ansID.address))) {
                        DBGLOG(DBG, "Conflicting nogood");

                        testNg.erase(ansID);

                        if (query.ctx->config.getOption("MinimizeNogoodsQXP")) {
                            if (testNg.size() > 0) {
                                testNg = ExternalLearningHelper::getMinimalConflict(Tuple(), query, ansID, Nogood(), 0, testNg, false);
                            }
                        } else {
                            InterpretationPtr interpretation(new Interpretation(query.interpretation->getRegistry()));
                            InterpretationPtr assigned(new Interpretation(query.interpretation->getRegistry()));

                            // only add true atoms from nogood to the query interpretation
                            set_iterator<ID> it = testNg.begin();
                            while (it != testNg.end()) {
                                if (!it->isNaf()) {
                                    interpretation->setFact(it->address);
                                }
                                assigned->setFact(it->address);
                                it++;
                            }

                            PluginAtom::Query qa = query;
                            qa.interpretation = interpretation;
                            qa.assigned = assigned;

                            // iteratively remove each literal from nogood

                            BOOST_FOREACH(ID& iid, newNogoodsContainer.getNogood(i)) {
                                // only for non-auxiliaries
                                if (iid != ansID) {
                                    PluginAtom::Answer ans;

                                    if (!iid.isNaf()) {
                                        interpretation->clearFact(iid.address);
                                    }
                                    assigned->clearFact(iid.address);

                                    // query
                                    //if (query.ctx->config.getOption("UseExtAtomCache")) query.ctx->registry()->eatoms.getByID(query.eatomID).pluginAtom->retrieveCached(qa, ans, NogoodContainerPtr());
                                    //else query.ctx->registry()->eatoms.getByID(query.eatomID).pluginAtom->retrieve(qa, ans, NogoodContainerPtr());
                                    query.ctx->registry()->eatoms.getByID(query.eatomID).pluginAtom->retrieveFacade(qa, ans, NogoodContainerPtr(), query.ctx->config.getOption("UseExtAtomCache"));

                                    // get all answer atoms
                                    Set<ID> ansout = ExternalLearningHelper::getOutputAtoms(qa, ans, false);
                                    // and check if expected answer is still contained
                                    if (!ansout.contains(ansID)) {
                                        // if it isn't, add the atom to the nogood again
                                        if (!iid.isNaf()) {
                                            interpretation->setFact(iid.address);
                                        }
                                        assigned->setFact(iid.address);
                                    } else {
                                        testNg.erase(iid);
                                    }
                                }
                            }
                        }
                        testNg.insert(ansID);
                    }

                    // add minimized nogood
                    nogoods->addNogood(testNg);
                    DBGLOG(DBG, "Learned minimized nogood " << testNg.getStringRepresentation(query.ctx->registry()) << " from input-output behavior");
                } else {
                    nogoods->addNogood(newNogoodsContainer.getNogood(i));
                }
            }
        }


        // nogood minimization with caching answers of external atom:
        if (query.ctx->config.getOption("MinimizeNogoods") && query.ctx->config.getOption("MinimizeNogoodsOpt") && !inp->dependsOnOutputTuple() && prop.doesProvidePartialAnswer()) {
            DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidmin, "Nogood minimization");

            BOOST_FOREACH(ID& iid, extNgInput) {
                // cache for answers of external atom
                std::map<std::size_t, PluginAtom::Answer> externalEvaluationsCache;

                for (int i = 0; i < newNogoods.size(); ++i) {
                    if ((!query.ctx->config.getOption("MinimizeNogoodsOnConflict") || (!query.inputi || query.inputi->getFact(newNogoods[i].second.address)))
                            && (newNogoods[i].first.size() <= query.ctx->config.getOption("MinimizationSize"))) {
                        Nogood testNg = newNogoods[i].first;

                        testNg.erase(iid);

                        PluginAtom::Answer ans;
                        PluginAtom::Query qa = query;

                        if (externalEvaluationsCache.find(testNg.getHash()) != externalEvaluationsCache.end()) {
                            ans = externalEvaluationsCache[testNg.getHash()];
                        } else {
                            InterpretationPtr interpretation(new Interpretation(query.interpretation->getRegistry()));
                            InterpretationPtr assigned(new Interpretation(query.interpretation->getRegistry()));

                            // only add true atoms from nogood to the query interpretation
                            set_iterator<ID> it = testNg.begin();
                            while (it != testNg.end()) {
                                if (!it->isNaf()) {
                                    interpretation->setFact(it->address);
                                }
                                assigned->setFact(it->address);
                                it++;
                            }

                            qa.interpretation = interpretation;
                            qa.assigned = assigned;

                            DBGLOG(DBG, "minimizing nogood " << testNg.getStringRepresentation(query.ctx->registry()) << " from input-output behavior");

                            // query
                            //if (query.ctx->config.getOption("UseExtAtomCache")) query.ctx->registry()->eatoms.getByID(query.eatomID).pluginAtom->retrieveCached(qa, ans, NogoodContainerPtr());
                            //else query.ctx->registry()->eatoms.getByID(query.eatomID).pluginAtom->retrieve(qa, ans, NogoodContainerPtr());
                            query.ctx->registry()->eatoms.getByID(query.eatomID).pluginAtom->retrieveFacade(qa, ans, NogoodContainerPtr(), query.ctx->config.getOption("UseExtAtomCache"));

                            externalEvaluationsCache[testNg.getHash()] = ans;
                        }
                        // get all answer atoms
                        Set<ID> ansout = ExternalLearningHelper::getOutputAtoms(qa, ans, false);
                        // and check if expected answer is still contained
                        if (ansout.contains(newNogoods[i].second)) {
                            newNogoods[i].first = testNg;
                        }
                    }
                }
            }

            for (int i = 0; i < newNogoods.size(); ++i) {
                Nogood newNg = newNogoods[i].first;
                newNg.insert(newNogoods[i].second);
                DBGLOG(DBG, "Learned minimized nogood " << newNg.getStringRepresentation(query.ctx->registry()) << " from input-output behavior");
                nogoods->addNogood(newNg);
            }
        }
    }
}

void ExternalLearningHelper::learnFromFunctionality(const PluginAtom::Query& query, const PluginAtom::Answer& answer, const ExtSourceProperties& prop, std::vector<Tuple>& recordedTuples, NogoodContainerPtr nogoods) {

    if (nogoods) {
        DBGLOG(DBG, "External Learning: Functionality");

        // there is a unique output
        const std::vector<Tuple>& otuples = answer.get();

        if (otuples.size() > 0) {
            ID uniqueOut = ExternalLearningHelper::getOutputAtom(query, otuples[0], true);

            // go through all output tuples which have been generated so far

            BOOST_FOREACH(Tuple t, recordedTuples) {
                // compare the non-functional prefix
                bool match = true;
                for (int i = 0; i < prop.functionalStart; ++i) {
                    if (otuples[0][i] != t[i]) {
                        match = false;
                        break;
                    }
                }
                if (!match) continue;

                ID id = ExternalLearningHelper::getOutputAtom(query, t, true);
                if (id != uniqueOut) {
                    Nogood excludeOthers;
                    excludeOthers.insert(uniqueOut);
                    excludeOthers.insert(id);
                    DBGLOG(DBG, "Learned nogood " << excludeOthers.getStringRepresentation(query.ctx->registry()) << " from functionality");
                    nogoods->addNogood(excludeOthers);
                }
            }

            // remember that otuples[0] was generated
            recordedTuples.push_back(otuples[0]);
        }
    }
}

namespace {

    struct TupleHash {

        std::size_t operator()(const Tuple& t) const {
            std::size_t seed = 0;
            boost::hash_combine(seed, t);
            return seed;
        }
    };
}

void ExternalLearningHelper::learnFromNegativeAtoms(const PluginAtom::Query& query, const PluginAtom::Answer& answer, const ExtSourceProperties& prop, NogoodContainerPtr nogoods, InputNogoodProviderConstPtr inp) {
    // learning of negative information
    if (nogoods) {
        // transform output into set for faster lookup
        Nogood extNgInput;
        int weakenedPremiseLiterals = 0;
        if (!inp->dependsOnOutputTuple()) extNgInput = (*inp)(query, prop, false, Tuple(), &weakenedPremiseLiterals);

        if (weakenedPremiseLiterals > 0) {
            DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sidweakenedpositive, "Positive gr.inst. after weakened EA-eval", answer.get().size());
            DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sidweakenedunknown, "Unknown gr.inst. after weakened EA-eval", answer.getUnknown().size());
        }

        // containers for storing nogoods that still have to be minimized 
        SimpleNogoodContainer newNogoodsContainer;
        std::map<ID, Tuple> externalAuxiliaryTable;
        std::vector<std::pair<Nogood, ID> > newNogoods;

        // iterate over negative output atoms
        bm::bvector<>::enumerator en = query.ctx->registry()->eatoms.getByID(query.eatomID).pluginAtom->getReplacements()->mask()->getStorage().first();
        bm::bvector<>::enumerator en_end = query.ctx->registry()->eatoms.getByID(query.eatomID).pluginAtom->getReplacements()->mask()->getStorage().end();
        ID negOutPredicate = query.ctx->registry()->getAuxiliaryConstantSymbol('n', query.ctx->registry()->eatoms.getByID(query.eatomID).predicate);
        ID posOutPredicate = query.ctx->registry()->getAuxiliaryConstantSymbol('r', query.ctx->registry()->eatoms.getByID(query.eatomID).predicate);

        boost::unordered_set<Tuple, TupleHash> toutput, tunknown;
        BOOST_FOREACH(Tuple t, answer.get()) toutput.insert(t);
        BOOST_FOREACH(Tuple t, answer.getUnknown()) tunknown.insert(t);

        while (en < en_end) {
            const OrdinaryAtom& atom = query.ctx->registry()->ogatoms.getByAddress(*en);
            if (atom.tuple[0] == negOutPredicate || atom.tuple[0] == posOutPredicate) {
                bool paramMatch = true;

                // compare auxiliary predicate input
                int aux = 0;
                if (query.ctx->config.getOption("IncludeAuxInputInAuxiliaries")) {
                    if (query.ctx->registry()->eatoms.getByID(query.eatomID).auxInputPredicate != ID_FAIL) {
                        aux = 1;
                        if (atom.tuple[1] != query.ctx->registry()->eatoms.getByID(query.eatomID).auxInputPredicate) paramMatch = false;
                    }
                }

                // compare other inputs
                for (uint32_t i = 0; i < query.input.size(); i++) {
                    if (atom.tuple[aux + 1 + i] != query.input[i]) {
                        paramMatch = false;
                        break;
                    }
                }

                // compare arity: total number of input and output elements in the replacement atom is (atom.tuple.size() - 1 - aux)
                if ((atom.tuple.size() - 1 - aux) != query.input.size() + query.pattern.size()) paramMatch = false;

                if (paramMatch) {
                    // check if this tuple is _not_ in the answer (if the external atom provides partial answers, it also must not be in the unknown list)
                    Tuple t;
                    for (uint32_t i = aux + 1 + query.input.size(); i < atom.tuple.size(); i++) {
                        t.push_back(atom.tuple[i]);
                    }

#ifndef NDEBUG
                    DBGLOG(DBG, "Output of external atom:");

                    BOOST_FOREACH(Tuple t, answer.get()) {
                        DBGLOG(DBG, "+" << printManyToString<RawPrinter>(t, ",", query.ctx->registry()));
                    }

                    BOOST_FOREACH(Tuple t, answer.getUnknown()) {
                        DBGLOG(DBG, "~" << printManyToString<RawPrinter>(t, ",", query.ctx->registry()));
                    }
#endif

                    if (weakenedPremiseLiterals > 0) {
                        DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sidweakenedpositive, "Total gr.inst. after weakened EA-eval", 1);
                    }
                    if (weakenedPremiseLiterals > 0 && toutput.find(t) == toutput.end() /*std::find(answer.get().begin(), answer.get().end(), t) == answer.get().end()*/) {
                        DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sidweakenedpositive, "Gr.inst. not in out after weakened EA-eval", 1);
                    }
                    if (weakenedPremiseLiterals > 0 && (!prop.doesProvidePartialAnswer() || tunknown.find(t) == tunknown.end() /*std::find(answer.getUnknown().begin(), answer.getUnknown().end(), t) == answer.getUnknown().end()*/)) {
                        DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sidweakenedpositive, "Gr.inst. not in unknown after weakened EA-eval", 1);
                    }

                    if (toutput.find(t) == toutput.end() && //std::find(answer.get().begin(), answer.get().end(), t) == answer.get().end() &&
                            (!prop.doesProvidePartialAnswer() || tunknown.find(t) == tunknown.end())) { //std::find(answer.getUnknown().begin(), answer.getUnknown().end(), t) == answer.getUnknown().end())) {
                        // construct positive output atom
                        OrdinaryAtom posatom = atom;
                        posatom.tuple[0] = posOutPredicate;
                        ID posAtomID = query.ctx->registry()->storeOrdinaryGAtom(posatom);

                        // construct nogood
                        Nogood ng = !inp->dependsOnOutputTuple()
                                ? extNgInput
                                : (*inp)(query, prop, false, t);

                        if (query.ctx->config.getOption("MinimizeNogoods") && !inp->dependsOnOutputTuple() && prop.doesProvidePartialAnswer()) {
                            // store the output tuples of the external auxiliary atom for minimization queries
                            ID externalAuxiliaryID = NogoodContainer::createLiteral(posAtomID.address);
                            externalAuxiliaryTable[externalAuxiliaryID] = t;

                            if (query.ctx->config.getOption("MinimizeNogoodsOpt")) {
                                // if answers w.r.t. the inputs should be cached, input and output atoms have to be stored separately
                                std::pair<Nogood, ID> newNogood(extNgInput, externalAuxiliaryID);
                                newNogoods.push_back(newNogood);
                            } else {
                                ng.insert(externalAuxiliaryID);
                                newNogoodsContainer.addNogood(ng);
                            }
                        } else {
                            ng.insert(NogoodContainer::createLiteral(posAtomID.address));
                            nogoods->addNogood(ng);
                        }
                        DBGLOG(DBG, "Learned negative nogood " << ng.getStringRepresentation(query.ctx->registry()));
                        DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sidweakenednumber, "EA-Nogoods from weakened intr.", (weakenedPremiseLiterals > 0 ? 1 : 0));
                    }
                }
            }
            en++;
        }

        // nogood minimization without caching answers of external atom
        if (query.ctx->config.getOption("MinimizeNogoods") && !query.ctx->config.getOption("MinimizeNogoodsOpt") && !inp->dependsOnOutputTuple() && prop.doesProvidePartialAnswer()) {
            DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidmin, "Nogood minimization");
            // iterate through all newly added nogoods
            for (int i = 0; i < newNogoodsContainer.getNogoodCount(); ++i) {
                if (newNogoodsContainer.getNogood(i).size() <= query.ctx->config.getOption("MinimizationSize")) {
                    // copy the respective nogood
                    Nogood testNg = newNogoodsContainer.getNogood(i);
                    // store the ID of answer atom that should still not be contained in answer after minimization
                    ID ansID;

                    BOOST_FOREACH(ID& iid, newNogoodsContainer.getNogood(i)) {
                        if (externalAuxiliaryTable.find(iid) != externalAuxiliaryTable.end()) {
                            ansID = iid;
                        }
                    }

                    if (!query.ctx->config.getOption("MinimizeNogoodsOnConflict") || (!query.inputi || query.inputi->getFact(ansID.address))) {
                        DBGLOG(DBG, "Conflicting nogood");

                        testNg.erase(ansID);

                        if (query.ctx->config.getOption("MinimizeNogoodsQXP")) {
                            if (testNg.size() > 0) {
                                Tuple t = externalAuxiliaryTable[ansID];
                                testNg = ExternalLearningHelper::getMinimalConflict(t, query, ansID, Nogood(), 0, testNg, true);
                            }
                        } else {
                            InterpretationPtr interpretation(new Interpretation(query.interpretation->getRegistry()));
                            InterpretationPtr assigned(new Interpretation(query.interpretation->getRegistry()));

                            // only add true atoms from nogood to the query interpretation
                            set_iterator<ID> it = testNg.begin();
                            while (it != testNg.end()) {
                                if (!it->isNaf()) {
                                    interpretation->setFact(it->address);
                                }
                                assigned->setFact(it->address);
                                it++;
                            }

                            PluginAtom::Query qa = query;
                            qa.interpretation = interpretation;
                            qa.assigned = assigned;

                            // iteratively remove each literal from nogood

                            BOOST_FOREACH(ID& iid, newNogoodsContainer.getNogood(i)) {
                                // only for non-auxiliaries
                                if (iid != ansID) {
                                    PluginAtom::Answer ans;

                                    if (!iid.isNaf()) {
                                        interpretation->clearFact(iid.address);
                                    }
                                    assigned->clearFact(iid.address);

                                    // query
                                    //if (query.ctx->config.getOption("UseExtAtomCache")) query.ctx->registry()->eatoms.getByID(query.eatomID).pluginAtom->retrieveCached(qa, ans, NogoodContainerPtr());
                                    //else query.ctx->registry()->eatoms.getByID(query.eatomID).pluginAtom->retrieve(qa, ans, NogoodContainerPtr());
                                    query.ctx->registry()->eatoms.getByID(query.eatomID).pluginAtom->retrieveFacade(qa, ans, NogoodContainerPtr(), query.ctx->config.getOption("UseExtAtomCache"));

                                    Tuple t = externalAuxiliaryTable[ansID];

                                    // check if answer tuple is still false
                                    if ((std::find(ans.get().begin(), ans.get().end(), t) != ans.get().end()) ||
                                            (std::find(ans.getUnknown().begin(), ans.getUnknown().end(), t) != ans.getUnknown().end())) {
                                        if (!iid.isNaf()) {
                                            interpretation->setFact(iid.address);
                                        }
                                        assigned->setFact(iid.address);
                                    } else {
                                        testNg.erase(iid);
                                    }
                                }
                            }
                        }
                        testNg.insert(ansID);
                    }

                    // add minimized nogood
                    nogoods->addNogood(testNg);
                    DBGLOG(DBG, "Learned minimized negative nogood " << testNg.getStringRepresentation(query.ctx->registry()) << " from input-output behavior");
                } else {
                    nogoods->addNogood(newNogoodsContainer.getNogood(i));
                }
            }
        }

        // nogood minimization with caching answers of external atom:
        if (query.ctx->config.getOption("MinimizeNogoods") && query.ctx->config.getOption("MinimizeNogoodsOpt") && !inp->dependsOnOutputTuple() && prop.doesProvidePartialAnswer()) {
            DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidmin, "Nogood minimization");

            BOOST_FOREACH(ID& iid, extNgInput) {
                // cache for answers of external atom
                std::map<std::size_t, PluginAtom::Answer> externalEvaluationsCache;

                for (int i = 0; i < newNogoods.size(); ++i) {
                    if (!query.ctx->config.getOption("MinimizeNogoodsOnConflict") || ((!query.inputi || query.inputi->getFact(newNogoods[i].second.address)))
                            && (newNogoods[i].first.size() <= query.ctx->config.getOption("MinimizationSize"))) {
                        Nogood testNg = newNogoods[i].first;

                        testNg.erase(iid);

                        PluginAtom::Answer ans;
                        PluginAtom::Query qa = query;

                        if (externalEvaluationsCache.find(testNg.getHash()) != externalEvaluationsCache.end()) {
                            ans = externalEvaluationsCache[testNg.getHash()];
                        } else {
                            InterpretationPtr interpretation(new Interpretation(query.interpretation->getRegistry()));
                            InterpretationPtr assigned(new Interpretation(query.interpretation->getRegistry()));

                            // only add true atoms from nogood to the query interpretation
                            set_iterator<ID> it = testNg.begin();
                            while (it != testNg.end()) {
                                if (!it->isNaf()) {
                                    interpretation->setFact(it->address);
                                }
                                assigned->setFact(it->address);
                                it++;
                            }

                            qa.interpretation = interpretation;
                            qa.assigned = assigned;

                            // query
                            //if (query.ctx->config.getOption("UseExtAtomCache")) query.ctx->registry()->eatoms.getByID(query.eatomID).pluginAtom->retrieveCached(qa, ans, NogoodContainerPtr());
                            //else query.ctx->registry()->eatoms.getByID(query.eatomID).pluginAtom->retrieve(qa, ans, NogoodContainerPtr());
                            query.ctx->registry()->eatoms.getByID(query.eatomID).pluginAtom->retrieveFacade(qa, ans, NogoodContainerPtr(), query.ctx->config.getOption("UseExtAtomCache"));

                            externalEvaluationsCache[testNg.getHash()] = ans;
                        }

                        Tuple t = externalAuxiliaryTable[newNogoods[i].second];

                        // check if answer tuple is still false
                        if ((std::find(ans.get().begin(), ans.get().end(), t) == ans.get().end()) &&
                                (std::find(ans.getUnknown().begin(), ans.getUnknown().end(), t) == ans.getUnknown().end())) {
                            newNogoods[i].first = testNg;
                        }
                    }
                }
            }

            for (int i = 0; i < newNogoods.size(); ++i) {
                Nogood newNg = newNogoods[i].first;
                newNg.insert(newNogoods[i].second);
                DBGLOG(DBG, "Learned minimized nogood " << newNg.getStringRepresentation(query.ctx->registry()) << " from input-output behavior");
                nogoods->addNogood(newNg);
            }
        }
    }
}

void ExternalLearningHelper::learnFromGroundRule(const PluginAtom::Query& query, ID groundRule, NogoodContainerPtr nogoods) {

    RegistryPtr reg = query.ctx->registry();

    if (nogoods) {
        DBGLOG(DBG, "External Learning: Ground Rule");

        const Rule& rule = query.ctx->registry()->rules.getByID(groundRule);

        Nogood ng;

        BOOST_FOREACH(ID hId, rule.head) {
            const OrdinaryAtom& oat = query.ctx->registry()->ogatoms.getByID(hId);
            Tuple t;
            t.insert(t.end(), oat.tuple.begin() + 1, oat.tuple.end());
            if (reg->terms.getByID(oat.tuple[0]).getUnquotedString() == "out") {
                // output atom is positive, i.e. it must not be false
                ng.insert(getOutputAtom(query, t, false));
            } else {
                // output atom is negative, i.e. it must not be true
                ng.insert(getOutputAtom(query, t, true));
            }
        }

        BOOST_FOREACH(ID bId, rule.body) {
            ng.insert(bId);
        }
        DBGLOG(DBG, "Learned nogood " << ng.getStringRepresentation(query.ctx->registry()) << " from rule");
        nogoods->addNogood(ng);
    }
}

void ExternalLearningHelper::learnFromRule(const PluginAtom::Query& query, ID rid, ProgramCtx* ctx, NogoodContainerPtr nogoods) {

    if (nogoods) {
        DBGLOG(DBG, "External Learning: Rule");

        // prepare map for replacing body predicates:
        // "in[i+1]" is replaced by the predicate passed as parameter number "i"
        std::map<ID, ID> predReplacementMap;
        for (uint32_t p = 0; p < query.input.size(); p++) {
            std::stringstream inPredStr;
            inPredStr << "in" << (p + 1);
            Term inPredTerm(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, inPredStr.str());
            ID inPredID = query.ctx->registry()->storeTerm(inPredTerm);
            predReplacementMap[inPredID] = query.input[p];
        }

        DBGLOG(DBG, "Rewriting rule");
        const Rule& rule = query.ctx->registry()->rules.getByID(rid);

        Rule rrule(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);
        rrule.head = rule.head;

        BOOST_FOREACH(ID batom, rule.body) {

            const OrdinaryAtom& oatom = batom.isOrdinaryGroundAtom() ? query.ctx->registry()->ogatoms.getByID(batom) : query.ctx->registry()->onatoms.getByID(batom);

            // replace predicate name by parameter from query.input
            OrdinaryAtom roatom = oatom;
            bool found = false;
            for (uint32_t inp = 0; inp < query.input.size(); inp++) {
                std::stringstream inPredStr;
                inPredStr << "in" << (inp + 1);
                Term inPredTerm(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, inPredStr.str());
                ID inPredID = query.ctx->registry()->storeTerm(inPredTerm);

                if (roatom.tuple[0] == inPredID) {
                    roatom.tuple[0] = query.input[inp];
                    found = true;
                    break;
                }
            }
            assert(found);

            ID batomId = batom.isOrdinaryGroundAtom() ? query.ctx->registry()->storeOrdinaryGAtom(roatom) : query.ctx->registry()->storeOrdinaryNAtom(roatom);
            ID mask(ID::MAINKIND_LITERAL, 0);
            if (batom.isNaf()) mask = mask | ID(ID::NAF_MASK, 0);
            if (batom.isOrdinaryGroundAtom()) mask = mask | ID(ID::SUBKIND_ATOM_ORDINARYG, 0);
            if (batom.isOrdinaryNongroundAtom()) mask = mask | ID(ID::SUBKIND_ATOM_ORDINARYN, 0);
            rrule.body.push_back(ID(mask.kind, batomId.address));
        }
        ID rruleId = query.ctx->registry()->storeRule(rrule);

        DBGLOG(DBG, "Building ASP Program");
        InterpretationConstPtr edb = query.interpretation;
        std::vector<ID> idb;
        idb.push_back(rruleId);
        OrdinaryASPProgram program(query.ctx->registry(), idb, edb);

        DBGLOG(DBG, "Grounding learning rule");
        GenuineGrounderPtr grounder = GenuineGrounderPtr(new InternalGrounder(*ctx, program, InternalGrounder::builtin));
        const OrdinaryASPProgram& gprogram = grounder->getGroundProgram();

        DBGLOG(DBG, "Generating nogoods for all ground rules");

        BOOST_FOREACH(ID rid, gprogram.idb) {
            learnFromGroundRule(query, rid, nogoods);
        }
    }
}


DLVHEX_NAMESPACE_END

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:


