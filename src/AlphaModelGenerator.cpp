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
 * @file AlphaModelGenerator.cpp
 * @author 
 *
 * @brief 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#define DLVHEX_BENCHMARK

#include "dlvhex2/AlphaModelGenerator.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/ASPSolver.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/Benchmarking.h"

#include <boost/foreach.hpp>

#include <jni.h>

DLVHEX_NAMESPACE_BEGIN

AlphaModelGenerator* amgPointer;

AlphaModelGeneratorFactory::AlphaModelGeneratorFactory(
ProgramCtx& ctx,
const ComponentInfo& ci,
ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig):
BaseModelGeneratorFactory(),
externalEvalConfig(externalEvalConfig),
ctx(ctx),
outerEatoms(ci.outerEatoms),
innerEatoms(ci.innerEatoms),
idb(),
xidb(),
ridb(),
nonmonotonicinputs()
{
    RegistryPtr reg = ctx.registry();

    // copy rules and constraints to idb
    // TODO we do not need this except for debugging
    idb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
    idb.insert(idb.end(), ci.innerRules.begin(), ci.innerRules.end());
    idb.insert(idb.end(), ci.innerConstraints.begin(), ci.innerConstraints.end());
    
    // transform original innerRules and innerConstraints
    // to xidb with only auxiliaries
    xidb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
    std::back_insert_iterator<std::vector<ID> > inserter(xidb);
    std::transform(ci.innerRules.begin(), ci.innerRules.end(),
        inserter, boost::bind(
        &AlphaModelGeneratorFactory::convertRule, this, ctx, _1));
    std::transform(ci.innerConstraints.begin(), ci.innerConstraints.end(),
        inserter, boost::bind(
        &AlphaModelGeneratorFactory::convertRule, this, ctx, _1));

    #ifndef NDEBUG
    {
        {
            std::ostringstream s;
            RawPrinter printer(s,ctx.registry());
            printer.printmany(idb," ");
            DBGLOG(DBG,"AlphaModelGeneratorFactory got idb " << s.str());
        }
        {
            std::ostringstream s;
            RawPrinter printer(s,ctx.registry());
            printer.printmany(xidb," ");
            DBGLOG(DBG,"AlphaModelGeneratorFactory got xidb " << s.str());
        }
    }
    #endif

    boost::unordered_map<ID, bool> relevantinputpreds;
    
    std::set<ID> predicatesDefinedInComponent;
        
    BOOST_FOREACH (ID eaid, innerEatoms) {
        const ExternalAtom& ea = reg->eatoms.getByID(eaid);

        for (uint32_t i = 0; i < ea.inputs.size(); ++i) {
            if (ea.pluginAtom->getInputType(i) == PluginAtom::PREDICATE &&
                !ea.getExtSourceProperties().isMonotonic(i)) {
                relevantinputpreds[ea.inputs[i]] = true;
                nonmonotonicinputs.insert(ea.inputs[i]);
            }
        }
    }
    
    boost::unordered_map<ID, bool> relevantpart;
    typedef std::pair<ID, bool> Pair;
    bool changed = true;
    bool firstiteration = true;
    
    while(changed) {
        changed = false;
        BOOST_FOREACH (Pair p, relevantinputpreds) {
            std::vector<ID>::const_iterator itr = idb.begin();
            while( itr != idb.end() ) {
                const Rule& rule = reg->rules.getByID(*itr);
                bool inhead = false;
                Tuple::const_iterator itlit = rule.head.begin();
                while( itlit != rule.head.end() ) {
                    
                    if((*itlit).isOrdinaryAtom()) {
                        const OrdinaryAtom& atom = reg->lookupOrdinaryAtom(*itlit);
                        if(atom.tuple[0] == p.first) inhead = true;
                    }
                    
                    if( firstiteration && inhead) {
                        relevantguesses.insert(*itlit);
                    }
                    
                    itlit++;
                }
                

                
                if(inhead && !relevantpart.count(*itr)) {
                    changed = true;
                    relevantpart[*itr] = true;
                    
                    Tuple::const_iterator itlit = rule.body.begin();
                    while( itlit != rule.body.end() ) {
                        if(!(*itlit).isNaf() && !(*itlit).isExternalAtom()) {
                            if((*itlit).isOrdinaryAtom()) {
                                const OrdinaryAtom& atom = reg->lookupOrdinaryAtom(*itlit);
                                relevantinputpreds[atom.tuple[0]] = true;
                            }
                        } else if (!(*itlit).isNaf() && (*itlit).isExternalAtom()) {
                            const ExternalAtom& ea = reg->eatoms.getByID(*itlit);

                            for (uint32_t i = 0; i < ea.inputs.size(); ++i) {
                                if (ea.pluginAtom->getInputType(i) == PluginAtom::PREDICATE) {
                                    relevantinputpreds[ea.inputs[i]] = true;
                                }
                            }
                        }
                        itlit++;
                    }
                }
                itr++;
            }
        }
        firstiteration = false;
    }
    
    BOOST_FOREACH (Pair p, relevantpart) {
//        std::ostringstream s;
//        RawPrinter printer(s,ctx.registry());
//        printer.print(p.first);
//        std::cout << s.str() << std::endl;
        ridb.push_back(p.first);
    }
    
    // create program for domain exploration
    if (ctx.config.getOption("LiberalSafety")) {
        // add domain predicates for all external atoms which are necessary to establish liberal domain-expansion safety
        // and extract the domain-exploration program from the IDB
        addDomainPredicatesAndCreateDomainExplorationProgram(ci, ctx, ridb, deidb, deidbInnerEatoms, outerEatoms);
    }

}


std::ostream& AlphaModelGeneratorFactory::print(
std::ostream& o) const
{
    RawPrinter printer(o, ctx.registry());
    o << "outer eatoms:" << std::endl;
    if( !outerEatoms.empty() ) {
        printer.printmany(outerEatoms,"\n");
    }
    o << "inner eatoms:" << std::endl;
    if( !innerEatoms.empty() ) {
        printer.printmany(innerEatoms,"\n");
    }
    o << "xidb:" << std::endl;
    if( !xidb.empty() ) {
        printer.printmany(xidb,"\n");
    }
    return o;
}


AlphaModelGenerator::AlphaModelGenerator(
Factory& factory,
InterpretationConstPtr input):
BaseModelGenerator(input),
factory(factory)
{

}

bool AlphaModelGenerator::evaluateExternalAtomFacade(ProgramCtx& ctx,
        ID eatomID,
        InterpretationConstPtr inputi,
        ExternalAnswerTupleCallback& cb,
        NogoodContainerPtr nogoods,
        InterpretationConstPtr assigned,
        InterpretationConstPtr changed,
        bool* fromCache) const
{
    return evaluateExternalAtom(ctx, eatomID, inputi, cb, nogoods, assigned, changed, fromCache);
}


AlphaModelGenerator::InterpretationPtr
AlphaModelGenerator::generateNextModel()
{
    RegistryPtr reg = factory.ctx.registry();
    if( currentResults == 0 ) {
        do {                     // breakout possibility
            // we need to create currentResults

            // create new interpretation as copy
            Interpretation::Ptr newint;
            if( input == 0 ) {
                // empty construction
                newint.reset(new Interpretation(reg));
            }
            else {
                // copy construction
                newint.reset(new Interpretation(*input));
            }

            // augment input with edb
            newint->add(*factory.ctx.edb);
            
            // remember facts so far (we have to remove these from any output)
            InterpretationConstPtr mask(new Interpretation(*newint));

            // manage outer external atoms
            if( !factory.outerEatoms.empty() ) {
                // augment input with result of external atom evaluation
                // use newint as input and as output interpretation
                IntegrateExternalAnswerIntoInterpretationCB cb(newint);
                evaluateExternalAtoms(factory.ctx, factory.outerEatoms, newint, cb);
                DLVHEX_BENCHMARK_REGISTER(sidcountexternalanswersets,
                    "outer eatom computations");
                DLVHEX_BENCHMARK_COUNT(sidcountexternalanswersets,1);

                if( factory.xidb.empty() ) {
                    // we only have eatoms -> return singular result

                    // remove EDB and direct input from newint
                    // (keep local models as small as possible)
                    newint->getStorage() -= mask->getStorage();

                    PreparedResults* pr = new PreparedResults;
                    currentResults.reset(pr);
                    pr->add(AnswerSetPtr(new AnswerSet(newint)));
                    break;
                }
            }

            // store in model generator and store as const
            postprocessedInput = newint;

            DLVHEX_BENCHMARK_REGISTER_AND_START(sidaspsolve, "initiating external solver");
            OrdinaryASPProgram program(reg,factory.xidb, postprocessedInput, factory.ctx.maxint, mask);
            
            if (factory.ctx.config.getOption("LiberalSafety")) {
                InterpretationConstPtr relevantdomain = computeRelevantDomain(factory.ctx, postprocessedInput, factory.deidb, factory.deidbInnerEatoms);
            
                typedef bm::bvector<> Storage;
                Storage::enumerator it = (*relevantdomain).getStorage().first();
                for(; it != (*relevantdomain).getStorage().end(); ++it) {
                    const OrdinaryAtom& atom = reg->lookupOrdinaryAtom(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *it));
                    
                    BOOST_FOREACH (ID p, factory.nonmonotonicinputs) {
                        if(atom.tuple[0] == p)
                            factory.relevantatomextensions.insert(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *it));
                    }
                }
            }
            
            // keep global reference for callback from jvm
            amgPointer = this;
            ASPSolverManager mgr;
            currentResults = mgr.solve(*factory.externalEvalConfig, program);
            DLVHEX_BENCHMARK_STOP(sidaspsolve);
        }
        while(false);            // end of breakout possibility
    }

    assert(currentResults != 0);
    AnswerSet::Ptr ret = currentResults->getNextAnswerSet();
    if( ret == 0 ) {
        currentResults.reset();
        // the following is just for freeing memory early
        postprocessedInput.reset();
        return InterpretationPtr();
    }
    DLVHEX_BENCHMARK_REGISTER(sidcountalphaanswersets, "AlphaMG answer sets");
    DLVHEX_BENCHMARK_COUNT(sidcountalphaanswersets,1);

    return ret->interpretation;
}

InterpretationConstPtr AlphaModelGenerator::computeRelevantDomain(ProgramCtx& ctx, InterpretationConstPtr edb, std::vector<ID>& deidb, std::vector<ID>& deidbInnerEatoms, bool enumerateNonmonotonic)
{

    RegistryPtr reg = ctx.registry();

    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidcedp,"computeExtensionOfDomainPreds");
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidhexground, "HEX grounder time");

    // get the set of all predicates defined in deidb
    std::set<ID> predicatesDefinedInComponent;
    BOOST_FOREACH(ID rid, deidb) {
        const Rule& rule = reg->rules.getByID(rid);

        BOOST_FOREACH(ID hid, rule.head) {
            if (!hid.isOrdinaryAtom()) continue;
            const OrdinaryAtom& oatom = reg->lookupOrdinaryAtom(hid);
            predicatesDefinedInComponent.insert(oatom.tuple[0]);
        }
    }

    InterpretationPtr domintr = InterpretationPtr(new Interpretation(reg));
    domintr->getStorage() |= edb->getStorage();

    DBGLOG(DBG, "Computing fixpoint of extensions of domain predicates");
    DBGLOG(DBG, "" << deidbInnerEatoms.size() << " inner external atoms are necessary for establishing de-safety");

    InterpretationPtr auxinputs = InterpretationPtr(new Interpretation(reg));
    InterpretationPtr herbrandBase = InterpretationPtr(new Interpretation(reg));
    InterpretationPtr oldherbrandBase = InterpretationPtr(new Interpretation(reg));
    herbrandBase->getStorage() |= edb->getStorage();
    do {
        oldherbrandBase->getStorage() = herbrandBase->getStorage();

        // ground program
        OrdinaryASPProgram program(reg, deidb, domintr, ctx.maxint);
        GenuineGrounderPtr grounder = GenuineGrounder::getInstance(ctx, program);

        // retrieve the Herbrand base
        if (!!grounder->getGroundProgram().mask) {
            herbrandBase->getStorage() |= (grounder->getGroundProgram().edb->getStorage() - grounder->getGroundProgram().mask->getStorage());
        }
        else {
            herbrandBase->getStorage() |= grounder->getGroundProgram().edb->getStorage();
        }
        BOOST_FOREACH (ID rid, grounder->getGroundProgram().idb) {
            const Rule& r = reg->rules.getByID(rid);
            BOOST_FOREACH (ID h, r.head)
                if (!grounder->getGroundProgram().mask || !grounder->getGroundProgram().mask->getFact(h.address)) herbrandBase->setFact(h.address);
            BOOST_FOREACH (ID b, r.body)
                if (!grounder->getGroundProgram().mask || !grounder->getGroundProgram().mask->getFact(b.address)) herbrandBase->setFact(b.address);
        }

        // evaluate inner external atoms
        BaseModelGenerator::IntegrateExternalAnswerIntoInterpretationCB cb(herbrandBase);
        BOOST_FOREACH (ID eaid, deidbInnerEatoms) {
            const ExternalAtom& ea = reg->eatoms.getByID(eaid);

            // remove all atoms over antimonotonic parameters from the input interpretation (both in standard and in higher-order notation)
            // in order to maximize the output;
            // for nonmonotonic input atoms, enumerate all (exponentially many) possible assignments
            boost::unordered_map<IDAddress, bool> nonmonotonicinput;
            InterpretationPtr input(new Interpretation(reg));
            input->add(*herbrandBase);
            ea.updatePredicateInputMask();
            bm::bvector<>::enumerator en = ea.getPredicateInputMask()->getStorage().first();
            bm::bvector<>::enumerator en_end = ea.getPredicateInputMask()->getStorage().end();
            while (en < en_end) {
                const OrdinaryAtom& ogatom = reg->ogatoms.getByAddress(*en);

                for (uint32_t i = 0; i < ea.inputs.size(); ++i) {
                    if (ea.pluginAtom->getInputType(i) == PluginAtom::PREDICATE &&
                        ea.getExtSourceProperties().isAntimonotonic(i) &&
                    ogatom.tuple[0] == ea.inputs[i]) {
                        DBGLOG(DBG, "Setting " << *en << " to false because it is an antimonotonic input atom");
                        input->clearFact(*en);
                    }
                    if (ea.pluginAtom->getInputType(i) == PluginAtom::PREDICATE &&
                        !ea.getExtSourceProperties().isAntimonotonic(i) &&
                        !ea.getExtSourceProperties().isMonotonic(i) &&
                    ogatom.tuple[0] == ea.inputs[i]) {
                        // if the predicate is defined in this component, enumerate all possible assignments
                        if (predicatesDefinedInComponent.count(ea.inputs[i]) > 0) {
                            DBGLOG(DBG, "Must guess all assignments to " << *en << " because it is a nonmonotonic and unstratified input atom");
                            nonmonotonicinput[*en] = false;
                        }
                        // otherwise: take the truth value from the edb
                        else {
                            if (!edb->getFact(*en)) {
                                DBGLOG(DBG, "Setting " << *en << " to false because it is stratified and false in the edb");
                                input->clearFact(*en);
                            }
                        }
                    }
                }
                en++;
            }

            typedef std::pair<IDAddress, bool> Pair;
            if (!enumerateNonmonotonic) {
                // evalute external atom
                DBGLOG(DBG, "Evaluating external atom " << eaid << " under " << *input << " (do not enumerate nonmonotonic input assignments due to user request)");
                BOOST_FOREACH (Pair p, nonmonotonicinput) input->clearFact(p.first);
                evaluateExternalAtom(ctx, eaid, input, cb);
            }
            else {
                DBGLOG(DBG, "Enumerating nonmonotonic input assignments to " << eaid);
                bool allOnes;
                do {
                    // set nonmonotonic input
                    allOnes = true;
                    BOOST_FOREACH (Pair p, nonmonotonicinput) {
		        DBGLOG(DBG, "Checking input atom " << printToString<RawPrinter>(reg->ogatoms.getIDByAddress(p.first), reg) << " (current truth value: " << p.second << ")");
                        if (p.second) input->setFact(p.first);
                        else {
                            input->clearFact(p.first);
                            allOnes = false;
                        }
                    }

                    // evalute external atom
                    DBGLOG(DBG, "Evaluating external atom " << eaid << " under " << *input);
                    evaluateExternalAtom(ctx, eaid, input, cb);

                    // enumerate next assignment to nonmonotonic input atoms
                    if (!allOnes) {
                        std::vector<IDAddress> clear;
                        BOOST_FOREACH (Pair p, nonmonotonicinput) {
                            if (p.second) clear.push_back(p.first);
                            else {
                                nonmonotonicinput[p.first] = true;
                                break;
                            }
                        }
                        BOOST_FOREACH (IDAddress c, clear) nonmonotonicinput[c] = false;
                    }
                }while(!allOnes);

                DBGLOG(DBG, "Enumerated all nonmonotonic input assignments to " << eaid);
            }
        }

        // translate new EA-replacements to domain atoms
        bm::bvector<>::enumerator en = herbrandBase->getStorage().first();
        bm::bvector<>::enumerator en_end = herbrandBase->getStorage().end();
        while (en < en_end) {
            ID id = reg->ogatoms.getIDByAddress(*en);
            if (id.isExternalAuxiliary()) {
                DBGLOG(DBG, "Converting atom with address " << *en);

                const OrdinaryAtom& ogatom = reg->ogatoms.getByAddress(*en);
                BOOST_FOREACH (ID eaid, deidbInnerEatoms) {
                    const ExternalAtom ea = reg->eatoms.getByID(eaid);
                    if (ea.predicate == reg->getIDByAuxiliaryConstantSymbol(ogatom.tuple[0])) {

                        OrdinaryAtom domatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX);
                        domatom.tuple.push_back(reg->getAuxiliaryConstantSymbol('d', eaid));
                        int io = 1;
                        //							if (ea.auxInputPredicate != ID_FAIL && ctx.config.getOption("IncludeAuxInputInAuxiliaries")) io = 2;
                        for (uint32_t i = io; i < ogatom.tuple.size(); ++i) {
                            domatom.tuple.push_back(ogatom.tuple[i]);
                        }
                        domintr->setFact(reg->storeOrdinaryGAtom(domatom).address);
                    }
                }
            }
            en++;
        }
        herbrandBase->getStorage() |= domintr->getStorage();
        DBGLOG(DBG, "Domain extension interpretation (intermediate result, including EDB): " << *domintr);
    }while(herbrandBase->getStorage().count() != oldherbrandBase->getStorage().count());

    domintr->getStorage() -= edb->getStorage();
    DBGLOG(DBG, "Domain extension interpretation (final result): " << *domintr);
    return herbrandBase;
}

DLVHEX_NAMESPACE_END

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
