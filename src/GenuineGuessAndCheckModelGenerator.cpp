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
 * @file GenuineGuessAndCheckModelGenerator.cpp
 * @author Christoph Redl
 *
 * @brief Implementation of the model generator for "GuessAndCheck" components.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fstream>

#include "dlvhex2/GenuineGuessAndCheckModelGenerator.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/ASPSolver.h"
#include "dlvhex2/ASPSolverManager.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/InternalGroundDASPSolver.h"
#include "dlvhex2/UnfoundedSetChecker.h"
#include "dlvhex2/InternalGrounder.h"

#include <bm/bmalgo.h>

#include <boost/foreach.hpp>
#include <boost/unordered_map.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/properties.hpp>
#include <boost/scoped_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

GenuineGuessAndCheckModelGeneratorFactory::GenuineGuessAndCheckModelGeneratorFactory(
ProgramCtx& ctx,
const ComponentInfo& ci,
ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig):
FLPModelGeneratorFactoryBase(ctx),
externalEvalConfig(externalEvalConfig),
ctx(ctx),
ci(ci),
outerEatoms(ci.outerEatoms)
{
    // this model generator can handle any components
    // (and there is quite some room for more optimization)

    // just copy all rules and constraints to idb
    idb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
    idb.insert(idb.end(), ci.innerRules.begin(), ci.innerRules.end());
    idb.insert(idb.end(), ci.innerConstraints.begin(), ci.innerConstraints.end());

    // create program for domain exploration
    if (ctx.config.getOption("LiberalSafety")) {
        // add domain predicates for all external atoms which are necessary to establish liberal domain-expansion safety
        // and extract the domain-exploration program from the IDB
        addDomainPredicatesAndCreateDomainExplorationProgram(ci, ctx, idb, deidb, deidbInnerEatoms, outerEatoms);
    }

    innerEatoms = ci.innerEatoms;
    // create guessing rules "gidb" for innerEatoms in all inner rules and constraints
    createEatomGuessingRules(ctx);

    // transform original innerRules and innerConstraints to xidb with only auxiliaries
    xidb.reserve(idb.size());
    std::back_insert_iterator<std::vector<ID> > inserter(xidb);
    std::transform(idb.begin(), idb.end(),
        inserter, boost::bind(&GenuineGuessAndCheckModelGeneratorFactory::convertRule, this, ctx, _1));

    // transform xidb for flp calculation
    if (ctx.config.getOption("FLPCheck")) createFLPRules();

    // output rules
    {
        std::ostringstream s;
        print(s, true);
        LOG(DBG,"GenuineGuessAndCheckModelGeneratorFactory(): " << s.str());
    }
}

void GenuineGuessAndCheckModelGeneratorFactory::addInconsistencyCauseFromSuccessor(const Nogood* cause){
    DBGLOG(DBG, "Inconsistency cause was added to model generator factory: " << cause->getStringRepresentation(ctx.registry()) << ", ogatoms in registry: " << ctx.registry()->ogatoms.getSize());

    // Store the nogood for future reinstantiations of the model generator.
    // To this end, store also the current maximum index of ground atoms in the registry;
    //   all atoms which are added later must be added with negative sign to the nogood because it was generated as inconsistency cause under the assumption that these atoms do not exist.
    succNogoods.push_back(std::pair<Nogood, int>(*cause, ctx.registry()->ogatoms.getSize()));
}

GenuineGuessAndCheckModelGeneratorFactory::ModelGeneratorPtr
GenuineGuessAndCheckModelGeneratorFactory::createModelGenerator(
InterpretationConstPtr input)
{
    return ModelGeneratorPtr(new GenuineGuessAndCheckModelGenerator(*this, input));
}


std::ostream& GenuineGuessAndCheckModelGeneratorFactory::print(
std::ostream& o) const
{
    return print(o, true);
}


std::ostream& GenuineGuessAndCheckModelGeneratorFactory::print(
std::ostream& o, bool verbose) const
{
    // item separator
    std::string isep(" ");
    // group separator
    std::string gsep(" ");
    if( verbose ) {
        isep = "\n";
        gsep = "\n";
    }
    RawPrinter printer(o, ctx.registry());
    if( !outerEatoms.empty() ) {
        o << "outer Eatoms={" << gsep;
        printer.printmany(outerEatoms,isep);
        o << gsep << "}" << gsep;
    }
    if( !innerEatoms.empty() ) {
        o << "inner Eatoms={" << gsep;
        printer.printmany(innerEatoms,isep);
        o << gsep << "}" << gsep;
    }
    if( !gidb.empty() ) {
        o << "gidb={" << gsep;
        printer.printmany(gidb,isep);
        o << gsep << "}" << gsep;
    }
    if( !idb.empty() ) {
        o << "idb={" << gsep;
        printer.printmany(idb,isep);
        o << gsep << "}" << gsep;
    }
    if( !xidb.empty() ) {
        o << "xidb={" << gsep;
        printer.printmany(xidb,isep);
        o << gsep << "}" << gsep;
    }
    if( !xidbflphead.empty() ) {
        o << "xidbflphead={" << gsep;
        printer.printmany(xidbflphead,isep);
        o << gsep << "}" << gsep;
    }
    if( !xidbflpbody.empty() ) {
        o << "xidbflpbody={" << gsep;
        printer.printmany(xidbflpbody,isep);
        o << gsep << "}" << gsep;
    }
    return o;
}

//
// the model generator
//

GenuineGuessAndCheckModelGenerator::GenuineGuessAndCheckModelGenerator(
Factory& factory,
InterpretationConstPtr input):
FLPModelGeneratorBase(factory, input),
factory(factory),
reg(factory.reg),
cmModelCount(0),
haveInconsistencyCause(false),
unitInput(input),
guessingProgram(factory.reg)
{
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidconstruct, "genuine g&c mg constructor");
    DBGLOG(DBG, "Genuine GnC-ModelGenerator is instantiated for a " << (factory.ci.disjunctiveHeads ? "" : "non-") << "disjunctive component");

    RegistryPtr reg = factory.reg;

    // create new interpretation as copy
    InterpretationPtr postprocInput;
    if( input == 0 ) {
        // empty construction
        postprocInput.reset(new Interpretation(reg));
    }
    else {
        // copy construction
        postprocInput.reset(new Interpretation(*input));
    }

    // augment input with edb
    WARNING("perhaps we can pass multiple partially preprocessed input edb's to the external solver and save a lot of processing here")
        postprocInput->add(*factory.ctx.edb);

    // remember which facts we must remove
    mask.reset(new Interpretation(*postprocInput));

    // manage outer external atoms
    if( !factory.outerEatoms.empty() ) {
        DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidhexground, "HEX grounder out EA GenGnCMG");

        // augment input with result of external atom evaluation
        // use newint as input and as output interpretation
        IntegrateExternalAnswerIntoInterpretationCB cb(postprocInput);
        evaluateExternalAtoms(factory.ctx,
            factory.outerEatoms, postprocInput, cb);
        DLVHEX_BENCHMARK_REGISTER(sidcountexternalatomcomps,
            "outer eatom computations");
        DLVHEX_BENCHMARK_COUNT(sidcountexternalatomcomps,1);
    }

    // assign to const member -> this value must stay the same from here on!
    postprocessedInput = postprocInput;

    // construct guessing program
    guessingProgram = OrdinaryASPProgram(reg, factory.xidb, postprocessedInput, factory.ctx.maxint);
    guessingProgram.idb.insert(guessingProgram.idb.end(), factory.gidb.begin(), factory.gidb.end());

    // identify explanation atoms
    InterpretationPtr deinput(new Interpretation(reg));
    deinput->add(*postprocInput);
    std::vector<ID> solverAssumptions;
    std::vector<ID> deidb = factory.deidb;
    if (factory.ctx.config.getOption("TransUnitLearning")){
        initializeInconsistencyExplanationAtoms();

        // we add a guess of the truth value of all explanation atoms and enforce its truth value in the facts using assumptions.
        // (this is in order to make the grounding exhaustive also for the case that these atoms change their truth value)
        bm::bvector<>::enumerator en = explAtoms->getStorage().first();
        bm::bvector<>::enumerator en_end = explAtoms->getStorage().end();
        while (en < en_end) {
            Rule explanationGuess(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR | ID::PROPERTY_RULE_DISJ);
            explanationGuess.head.push_back(reg->ogatoms.getIDByAddress(*en));
            OrdinaryAtom oat(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX);
            oat.tuple.push_back(reg->getAuxiliaryConstantSymbol('x', reg->ogatoms.getIDByAddress(*en)));
            explanationGuess.head.push_back(reg->storeOrdinaryAtom(oat));
            mask->setFact(explanationGuess.head[1].address);
            ID explanationGuessID = reg->storeRule(explanationGuess);
            guessingProgram.idb.push_back(explanationGuessID);
            deidb.push_back(explanationGuessID);
//            deinput->setFact(*en);
            if (postprocessedInput->getFact(*en)) {
                solverAssumptions.push_back(explanationGuess.head[0]);
                postprocInput->clearFact(*en);
            }else{
                solverAssumptions.push_back(ID::nafLiteralFromAtom(explanationGuess.head[0]));
            }
            en++;
        }
    }

    // compute extensions of domain predicates and add it to the input
    if (factory.ctx.config.getOption("LiberalSafety")) {
        InterpretationConstPtr domPredictaesExtension = computeExtensionOfDomainPredicates(factory.ctx, postprocInput, deidb, factory.deidbInnerEatoms /*, true, factory.ctx.config.getOption("TransUnitLearning")*/);
        postprocInput->add(*domPredictaesExtension);
    }

    // evaluate edb+xidb+gidb
    {
        DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"genuine g&c init guessprog");
        DBGLOG(DBG,"evaluating guessing program");

        DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidhexground, "HEX grounder GuessPr GenGnCMG");
        
        grounder = GenuineGrounder::getInstance(factory.ctx, guessingProgram);
        OrdinaryASPProgram gp = grounder->getGroundProgram();
        // do not project within the solver as auxiliaries might be relevant for UFS checking (projection is done in G&C mg)
        if (!!gp.mask) mask->add(*gp.mask);
        gp.mask = InterpretationConstPtr();
        annotatedGroundProgram = AnnotatedGroundProgram(factory.ctx, gp, factory.innerEatoms);

        // external source inlining
        if (factory.ctx.config.getOption("ExternalSourceInlining")) {
            inlineExternalAtoms(gp, grounder, annotatedGroundProgram, activeInnerEatoms);
        }else{
            activeInnerEatoms = factory.innerEatoms;
        }

        // run solver
        solver = GenuineGroundSolver::getInstance(factory.ctx, annotatedGroundProgram);
	    if (solverAssumptions.size() > 0) solver->restartWithAssumptions(solverAssumptions);
    }

    {
        // update nogoods learned from successor (add all ground atoms which have been added to the registry in the meantime in negative form) and add their atoms to the explanation atoms
        if (factory.ctx.config.getOption("TransUnitLearning")){
            typedef std::pair<Nogood, int> NogoodIntegerPair;
            printUnitInfo("[IR] ");
            DBGLOG(DBG, "[IR] Solving program");
            DBGLOG(DBG, "[IR]     " << *annotatedGroundProgram.getGroundProgram().edb << std::endl << "[IR]     " << printManyToString<RawPrinter>(annotatedGroundProgram.getGroundProgram().idb, "\n[IR]     ", factory.ctx.registry()));

            printUnitInfo("[IR] ");
            DBGLOG(DBG, "[IR] Updating nogoods from successor");
            BOOST_FOREACH (NogoodIntegerPair nip, factory.succNogoods){
                for (int i = nip.second; i < factory.ctx.registry()->ogatoms.getSize(); i++){
                    if (annotatedGroundProgram.getProgramMask()->getFact(i)) nip.first.insert(NogoodContainer::createLiteral(i, false));
                }
                nip.second = factory.ctx.registry()->ogatoms.getSize();

                // atoms occurring the added nogood, but not defined in the component, are also explanation atoms (the nogood can be seen as a constraint)
                BOOST_FOREACH (ID l, nip.first) {
                    if (factory.ci.predicatesDefinedInComponent.find(factory.ctx.registry()->ogatoms.getByAddress(l.address).tuple[0]) == factory.ci.predicatesDefinedInComponent.end()) {
                        explAtoms->setFact(l.address);
                    }
                }
            }

            typedef std::pair<Nogood, int> NogoodIntegerPair;
            DBGLOG(DBG, "[IR] Adding nogoods from successor to main solver");
            BOOST_FOREACH (NogoodIntegerPair nip, factory.succNogoods){
                DBGLOG(DBG, "[IR] Adding nogood from successor " << nip.first.getStringRepresentation(factory.ctx.registry()));
                solver->addNogood(nip.first);
            }
        }
    }

    // external learning related initialization
    learnedEANogoods = SimpleNogoodContainerPtr(new SimpleNogoodContainer());
    analysissolverNogoods = SimpleNogoodContainerPtr(new SimpleNogoodContainer());
    learnedEANogoodsTransferredIndex = 0;
    nogoodGrounder = NogoodGrounderPtr(new ImmediateNogoodGrounder(factory.ctx.registry(), learnedEANogoods, learnedEANogoods, annotatedGroundProgram));
    if(factory.ctx.config.getOption("NoPropagator") == 0) {
        DBGLOG(DBG, "Adding propagator to solver");
        solver->addPropagator(this);
    }
    learnSupportSets();

    // external atom evaluation and unfounded set checking
    //   initialize UFS checker
    //     Concerning the last parameter, note that clasp backend uses choice rules for implementing disjunctions:
    //     this must be regarded in UFS checking (see examples/trickyufs.hex)
    ufscm = UnfoundedSetCheckerManagerPtr(new UnfoundedSetCheckerManager(*this, factory.ctx, annotatedGroundProgram,
        factory.ctx.config.getOption("GenuineSolver") >= 3,
        factory.ctx.config.getOption("ExternalLearning") ? learnedEANogoods : SimpleNogoodContainerPtr()));

    initializeHeuristics();
    initializeVerificationWatchLists();
}

GenuineGuessAndCheckModelGenerator::~GenuineGuessAndCheckModelGenerator()
{
    DBGLOG(DBG, "Removing propagator to solver");
    solver->removePropagator(this);
    DBGLOG(DBG, "Final Statistics:" << std::endl << solver->getStatistics());
}

ID GenuineGuessAndCheckModelGenerator::getAuxiliaryAtom(char type, ID id){
    OrdinaryAtom oatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX);
    oatom.tuple.push_back(factory.ctx.registry()->getAuxiliaryConstantSymbol(type, id));
    return factory.ctx.registry()->storeOrdinaryAtom(oatom);
}

// debugging of inconsistency analysis
void GenuineGuessAndCheckModelGenerator::printUnitInfo(std::string prefix) {
    std::stringstream inp;
    if (!!unitInput) inp << *unitInput;
    DBGLOG(DBG, prefix << std::endl << prefix << std::endl << prefix << std::endl <<
                prefix << "====================" << std::endl <<
                prefix << "Unit: " << std::endl <<
                prefix << "    " << printManyToString<RawPrinter>(factory.idb, "\n" + prefix + "    ", factory.ctx.registry()) << std::endl <<
                prefix << "Unit input: " + inp.str() << std::endl <<
                prefix << "--------------------");
}

void GenuineGuessAndCheckModelGenerator::inlineExternalAtoms(OrdinaryASPProgram& program, GenuineGrounderPtr& grounder, AnnotatedGroundProgram& annotatedGroundProgram, std::vector<ID>& activeInnerEatoms) {

#ifndef NDEBUG
        DBGLOG(DBG, "External source inlining in mode " << (factory.ctx.config.getOption("ExternalSourceInlining") == 2 ? "re" : "post"));
        DBGLOG(DBG, "Inlining in program:" << std::endl << *program.edb << std::endl)
        BOOST_FOREACH (ID rID, program.idb) {
            DBGLOG(DBG, printToString<RawPrinter>(rID, reg));
        }
#endif

    // remember the number of rules in the program before the rewriting
    int origRules = program.idb.size();
    bool groundAgain = false;

    InterpretationPtr eliminatedExtAuxes(new Interpretation(reg));
    for(unsigned eaIndex = 0; eaIndex < factory.innerEatoms.size(); ++eaIndex) {
        // evaluate the external atom if it provides support sets
        const ExternalAtom& eatom = reg->eatoms.getByID(factory.innerEatoms[eaIndex]);
        if (eatom.getExtSourceProperties().providesSupportSets() && eatom.getExtSourceProperties().providesCompletePositiveSupportSets()) {
            DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidinlined, "Inlined external atoms");

            DBGLOG(DBG, "Learning support sets for " << printToString<RawPrinter>(factory.innerEatoms[eaIndex], reg));
            SimpleNogoodContainerPtr supportSets = SimpleNogoodContainerPtr(new SimpleNogoodContainer());
            if (eatom.getExtSourceProperties().providesOnlySafeSupportSets() && factory.ctx.config.getOption("ExternalSourceInlining") == 2) {
                learnSupportSetsForExternalAtom(factory.ctx, factory.innerEatoms[eaIndex], supportSets);
                groundAgain = true;
            }else{
                SimpleNogoodContainerPtr potentialSupportSets = SimpleNogoodContainerPtr(new SimpleNogoodContainer());
                learnSupportSetsForExternalAtom(factory.ctx, factory.innerEatoms[eaIndex], potentialSupportSets);
                NogoodGrounderPtr nogoodgrounder = NogoodGrounderPtr(new ImmediateNogoodGrounder(factory.ctx.registry(), potentialSupportSets, potentialSupportSets, annotatedGroundProgram));
                int nc = 0;
                while (nc < potentialSupportSets->getNogoodCount()) {
                    nc = potentialSupportSets->getNogoodCount();
                    nogoodgrounder->update();
                }

                bool keep;
                for (int i = 0; i < potentialSupportSets->getNogoodCount(); ++i) {
                    const Nogood& ng = potentialSupportSets->getNogood(i);
                    if (ng.isGround()) {
                        // determine the external atom replacement in ng
                        ID eaAux = ID_FAIL;
                        BOOST_FOREACH (ID lit, ng) {
                            if (reg->ogatoms.getIDByAddress(lit.address).isExternalAuxiliary()) {
                                if (eaAux != ID_FAIL) throw GeneralError("Set " + ng.getStringRepresentation(reg) + " is not a valid support set because it contains multiple external literals");
                                eaAux = lit;
                            }
                        }
                        if (eaAux == ID_FAIL) throw GeneralError("Set " + ng.getStringRepresentation(reg) + " is not a valid support set because it contains no external literals");

                        // determine the according external atom
                        if (annotatedGroundProgram.mapsAux(eaAux.address)) {
                            DBGLOG(DBG, "Evaluating guards of " << ng.getStringRepresentation(reg));
                            keep = true;
                            Nogood ng2 = ng;
                            reg->eatoms.getByID(annotatedGroundProgram.getAuxToEA(eaAux.address)[0]).pluginAtom->guardSupportSet(keep, ng2, eaAux);
                            if (keep) {
                                #ifndef NDEBUG
                                // ng2 must be a subset of ng and still a valid support set
                                ID aux = ID_FAIL;
                                BOOST_FOREACH (ID id, ng2) {
                                    if (reg->ogatoms.getIDByAddress(id.address).isExternalAuxiliary()) aux = id;
                                    assert(ng.count(id) > 0);
                                }
                                assert(aux != ID_FAIL);
                                #endif
                                DBGLOG(DBG, "Keeping in form " << ng2.getStringRepresentation(reg));
                                supportSets->addNogood(ng2);
                                #ifdef DEBUG
                            }
                            else {
                                assert(ng == ng2);
                                DBGLOG(DBG, "Rejecting " << ng2.getStringRepresentation(reg));
                                #endif
                            }
                        }
                    }
                }
            }

            // external atom support rules
            DBGLOG(DBG, "Constructing support rules for " << printToString<RawPrinter>(factory.innerEatoms[eaIndex], reg));
            for (int i = 0; i < supportSets->getNogoodCount(); ++i) {
                const Nogood& ng = supportSets->getNogood(i);
                DBGLOG(DBG, "Processing support set " << ng.getStringRepresentation(reg));
                Rule supportRule(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);

                // find unique auxiliary and create head of support rule
                ID auxID = ID_FAIL;
                BOOST_FOREACH (ID lit, ng){
                    ID flit = (lit.isOrdinaryGroundAtom() ? reg->ogatoms.getIDByAddress(lit.address) : reg->onatoms.getIDByAddress(lit.address));
                    if (flit.isExternalAuxiliary()) {
                        if (reg->isNegativeExternalAtomAuxiliaryAtom(flit)) {
                            flit = reg->swapExternalAtomAuxiliaryAtom(flit);
                            flit.kind &= (ID::ALL_ONES ^ ID::NAF_MASK);
                        }
                        if (auxID != ID_FAIL) throw GeneralError("Invalid support set detected (contains multiple auxiliaries)");
                        auxID = flit;
                        supportRule.head.push_back(replacePredForInlinedEAs(flit, InterpretationConstPtr()));
                    }
                }
                if (auxID == ID_FAIL) throw GeneralError("Invalid support set detected (contains no auxiliary)");
                const OrdinaryAtom& aux = reg->lookupOrdinaryAtom(auxID);

                // create body of support rule
                BOOST_FOREACH (ID lit, ng){
                    ID flit = (lit.isOrdinaryGroundAtom() ? reg->ogatoms.getIDByAddress(lit.address) : reg->onatoms.getIDByAddress(lit.address));
                    flit.kind |= (lit.kind & ID::NAF_MASK);
                    if (!flit.isExternalAuxiliary()) {
                        // replace default-negated body atoms by the atoms which explicitly represent falsehood
                        if (flit.isNaf()){
                            const OrdinaryAtom& a = reg->ogatoms.getByID(flit);
                            OrdinaryAtom negA(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX);
                            negA.tuple = aux.tuple;
                            negA.tuple[0] = reg->getAuxiliaryConstantSymbol('F', aux.tuple[0]);
                            negA.tuple.insert(negA.tuple.end(), a.tuple.begin(), a.tuple.end());
                            ID negAID = reg->storeOrdinaryAtom(negA);
                            supportRule.body.push_back(negAID);
                        }else{
                            supportRule.body.push_back(flit);
                        }
                    }
                }

                // add support rule
                ID supportRuleID = reg->storeRule(supportRule);
                DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sidsupprulecount, "Added support rules", 1);
                DBGLOG(DBG, "Adding support rule " << printToString<RawPrinter>(supportRuleID, reg));
                program.idb.push_back(supportRuleID);
            }

            // explicit representation of negative input atoms
            typedef boost::unordered_map<IDAddress, std::vector<ID> > AuxToEAType;
            const AuxToEAType& auxToEA = annotatedGroundProgram.getAuxToEA();
            BOOST_FOREACH (AuxToEAType::value_type currentAux, auxToEA) {
                ID auxID = reg->ogatoms.getIDByAddress(currentAux.first);
                DBGLOG(DBG, "Processing external atom auxiliary " << printToString<RawPrinter>(auxID, reg));
                eliminatedExtAuxes->setFact(auxID.address);
                eliminatedExtAuxes->setFact(reg->swapExternalAtomAuxiliaryAtom(auxID).address);
                const OrdinaryAtom& aux = reg->ogatoms.getByAddress(currentAux.first);
                if (reg->getTypeByAuxiliaryConstantSymbol(aux.tuple[0]) == 'r') {
                    // for all input atoms
                    int eaIndex = annotatedGroundProgram.getIndexOfEAtom(currentAux.second[0]);
                    const boost::shared_ptr<ExternalAtomMask> mask = annotatedGroundProgram.getEAMask(eaIndex);
                    bm::bvector<>::enumerator en = mask->mask()->getStorage().first();
                    bm::bvector<>::enumerator en_end = mask->mask()->getStorage().end();
                    while (en < en_end) {
                        // atom "a"
                        ID aID = reg->ogatoms.getIDByAddress(*en);
                        if (!aID.isAuxiliary()) { // only input atoms from predicate input
                            const OrdinaryAtom& a = reg->ogatoms.getByID(aID);
                            DBGLOG(DBG, "Processing input atom: " << printToString<RawPrinter>(aID, reg));

                            // create an atom "af" which represents the negation of "a" when input to "aux"
                            OrdinaryAtom negA(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX);
                            negA.tuple = aux.tuple;
                            negA.tuple[0] = reg->getAuxiliaryConstantSymbol('F', aux.tuple[0]);
                            negA.tuple.insert(negA.tuple.end(), a.tuple.begin(), a.tuple.end());
                            ID negAID = reg->storeOrdinaryAtom(negA);
                            DBGLOG(DBG, "Negated atom: " << printToString<RawPrinter>(negAID, reg));

                            // "af" is true if "a" is false
                            Rule falsehoodRule(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);
                            falsehoodRule.head.push_back(negAID);
                            falsehoodRule.body.push_back(ID::nafLiteralFromAtom(aID));
                            ID falsehoodRuleID = reg->storeRule(falsehoodRule);
                            DBGLOG(DBG, "Falsehood rule for input atom: " << printToString<RawPrinter>(falsehoodRuleID, reg));
                            program.idb.push_back(falsehoodRuleID);

                            // "af" is also true if "aux" is true
                            Rule saturationRule(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);
                            saturationRule.head.push_back(negAID);
                            saturationRule.body.push_back(auxID);
                            ID saturationRuleID = reg->storeRule(saturationRule);
                            DBGLOG(DBG, "Saturation rule: " << printToString<RawPrinter>(saturationRuleID, reg));
                            program.idb.push_back(saturationRuleID);

                            // one of "a" or "af" must be true whenever "naux" is not false
                            Rule guessAorAF(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR | ID::PROPERTY_RULE_DISJ);
                            guessAorAF.head.push_back(aID);
                            guessAorAF.head.push_back(negAID);
                            guessAorAF.body.push_back(ID::nafLiteralFromAtom(reg->swapExternalAtomAuxiliaryAtom(auxID)));
                            ID guessAorAFID = reg->storeRule(guessAorAF);
                            DBGLOG(DBG, "Guessing rule: " << printToString<RawPrinter>(guessAorAFID, reg));
                            program.idb.push_back(guessAorAFID);

                            DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sidinlinedauxrules, "Added auxiliary rules", 4);
                        }

                        en++;
                    }
                }
            }
        }else{
            activeInnerEatoms.push_back(factory.innerEatoms[eaIndex]);
        }
    }

    // 1. substitute external atom guessing rule "e v ne :- B" by "ne :- not a"
    // 2. replace external atom auxiliaries 'r'/'n' by 'R'/'N' in all rules
    OrdinaryASPProgram inlinedProgram(reg, std::vector<ID>(), program.edb, factory.ctx.maxint);
    {
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidextreplace, "Rewrite to inlined ext. atoms");
    DBGLOG(DBG, "Replacing external atom auxiliaries");
    for (int rIndex = 0; rIndex < program.idb.size(); ++rIndex) {
        DBGLOG(DBG, "Processing rule " << printToString<RawPrinter>(program.idb[rIndex], reg));
        const Rule& rule = reg->rules.getByID(program.idb[rIndex]);
        if (rule.isEAGuessingRule() && eliminatedExtAuxes->getFact(rule.head[0].address)){
            int posIndex = (reg->getTypeByAuxiliaryConstantSymbol(rule.head[0]) == 'r' ? 0 : 1);
            Rule simplifiedGuessingRule(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);
            simplifiedGuessingRule.head.push_back(replacePredForInlinedEAs(rule.head[1 - posIndex], eliminatedExtAuxes));
            simplifiedGuessingRule.body = rule.body;
            simplifiedGuessingRule.body.push_back(ID::nafLiteralFromAtom(replacePredForInlinedEAs(rule.head[posIndex], eliminatedExtAuxes)));
            ID simplifiedGuessingRuleID = reg->storeRule(simplifiedGuessingRule);
            DBGLOG(DBG, "Simplified guessing rule " << printToString<RawPrinter>(program.idb[rIndex], reg) << " to " << printToString<RawPrinter>(simplifiedGuessingRuleID, reg));
            inlinedProgram.idb.push_back(simplifiedGuessingRuleID);
        }else{
            Rule* newRule = 0;  // in most cases we do not actually need to create one (if it is the same as the existing one)

            // substitute in all head atoms
            for (int hIndex = 0; hIndex < rule.head.size(); ++hIndex) {
                ID newAtomID = replacePredForInlinedEAs(rule.head[hIndex], eliminatedExtAuxes);
                if (newAtomID != rule.head[hIndex]) {
                    if (!newRule) {
                        newRule = new Rule(rule);
                    }
                    newRule->head[hIndex] = newAtomID;
                }
            }

            // substitute in all body atoms
            for (int bIndex = 0; bIndex < rule.body.size(); ++bIndex) {
                ID newAtomID = replacePredForInlinedEAs(rule.body[bIndex], eliminatedExtAuxes);
                if (newAtomID != rule.body[bIndex]) {
                    if (!newRule) {
                        newRule = new Rule(rule);
                        if (rIndex < origRules && rule.body[bIndex].isNaf()) throw GeneralError("Cannot inline negated external atom " + printToString<RawPrinter>(rule.body[bIndex], reg) + ": please rewrite");
                    }
                    newRule->body[bIndex] = (newRule->body[bIndex].isNaf() ? ID::nafLiteralFromAtom(newAtomID) : ID::posLiteralFromAtom(newAtomID));
                }
            }
            // was the rule modified?
            if (!!newRule){
                inlinedProgram.idb.push_back(reg->storeRule(*newRule));
                delete newRule;
            }else{
                inlinedProgram.idb.push_back(program.idb[rIndex]);
            }
        }
    }

#ifndef NDEBUG
    DBGLOG(DBG, "Inlined program:" << std::endl << *inlinedProgram.edb << std::endl);
    BOOST_FOREACH (ID rID, inlinedProgram.idb) {
        DBGLOG(DBG, printToString<RawPrinter>(rID, reg));
    }
#endif
    }

    {
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidextregrounding, "Regrounding after inlining");
    // reground and reanalyze extended program
    if (groundAgain) {
        grounder = GenuineGrounder::getInstance(factory.ctx, inlinedProgram);
        OrdinaryASPProgram gp = grounder->getGroundProgram();

        // do not project within the solver as auxiliaries might be relevant for UFS checking (projection is done in G&C mg)
        if (!!gp.mask) mask->add(*gp.mask);
        gp.mask = InterpretationConstPtr();
        annotatedGroundProgram = AnnotatedGroundProgram(factory.ctx, gp, activeInnerEatoms);
    }else{
        annotatedGroundProgram = AnnotatedGroundProgram(factory.ctx, inlinedProgram, activeInnerEatoms);
    }
    }
}

ID GenuineGuessAndCheckModelGenerator::replacePredForInlinedEAs(ID atomID, InterpretationConstPtr eliminatedExtAuxes) {

    DBGLOG(DBG, "replacePredForInlinedEAs called for " << printToString<RawPrinter>(atomID, factory.ctx.registry()));

    // only external predicates are inlined
    if (!atomID.isExternalAuxiliary() || (!!eliminatedExtAuxes && !eliminatedExtAuxes->getFact(atomID.address))) {
        DBGLOG(DBG, "--> not an eliminated external atom auxiliary; aborting");
        return atomID;
    }

    const OrdinaryAtom& oatom = factory.ctx.registry()->lookupOrdinaryAtom(atomID);
    char type = reg->getTypeByAuxiliaryConstantSymbol(oatom.tuple[0]);
    ID id = reg->getIDByAuxiliaryConstantSymbol(oatom.tuple[0]);
    DBGLOG(DBG, "replacePredForInlinedEAs: type=" << type << "; id=" << id << " (" << printToString<RawPrinter>(id, reg) << ")");
    if ((type == 'r' || type == 'n')) {
        // change 'r' to 'R' and 'n' to 'N'
        type -= 32;
        OrdinaryAtom inlined = oatom;
        inlined.kind &= (ID::ALL_ONES ^ ID::PROPERTY_EXTERNALAUX);
        inlined.tuple[0] = reg->getAuxiliaryConstantSymbol(type, id);
        ID newID = reg->storeOrdinaryAtom(inlined);
        DBGLOG(DBG, "replacePredForInlinedEAs returns " << printToString<RawPrinter>(newID, factory.ctx.registry()));
        return newID;
    }
}

void GenuineGuessAndCheckModelGenerator::initializeInconsistencyExplanationAtoms(){

    printUnitInfo("[IR] ");
    DBGLOG(DBG, "[IR] initializeInconsistencyExplanationAtoms");
    PredicateMaskPtr explAtomMask(new PredicateMask());
    PredicateMaskPtr unitMask(new PredicateMask());
    explAtoms.reset(new Interpretation(factory.ctx.registry()));

    // Explanation atoms are all ground atoms from the registry which are not defined in this unit.
    // This captures exactly the atoms which *could* be derivable in some predecessor unit.
    explAtomMask->setRegistry(factory.ctx.registry());
    unitMask->setRegistry(factory.ctx.registry());
    DBGLOG(DBG, "[IR] Computing set of explanation atoms");
    BOOST_FOREACH (ID predInComp, factory.ci.predicatesOccurringInComponent) {
        if (factory.ci.predicatesDefinedInComponent.find(predInComp) == factory.ci.predicatesDefinedInComponent.end()) {
             DBGLOG(DBG, "[IR] +EP " << printToString<RawPrinter>(predInComp, factory.ctx.registry()) << ": predicate occurs but is not defined in unit --> atoms over predicate are explanation atoms");
             explAtomMask->addPredicate(predInComp);
        }else{
             DBGLOG(DBG, "[IR] -EP " << printToString<RawPrinter>(predInComp, factory.ctx.registry()) << ": predicate occurs and is defined in unit --> atoms over predicate are no explanation atoms");
        }
        unitMask->addPredicate(predInComp);
    }
    explAtomMask->updateMask();
    unitMask->updateMask();
    explAtoms->getStorage() |= explAtomMask->mask()->getStorage();
    if (!!unitInput) explAtoms->getStorage() |= unitInput->getStorage();
    DBGLOG(DBG, "[IR] Explanation atoms for inconsistency analysis: " << *explAtoms);
}

void GenuineGuessAndCheckModelGenerator::initializeHeuristics()
{

    defaultExternalAtomEvalHeuristics = factory.ctx.defaultExternalAtomEvaluationHeuristicsFactory->createHeuristics(reg);

    // set external atom evaluation strategy according to selected heuristics
    for (uint32_t i = 0; i < activeInnerEatoms.size(); ++i) {
        const ExternalAtom& eatom = reg->eatoms.getByID(activeInnerEatoms[i]);

        eaEvaluated.push_back(false);
        eaVerified.push_back(false);
        changedAtomsPerExternalAtom.push_back(eatom.getExtSourceProperties().doesCareAboutChanged() ? InterpretationPtr(new Interpretation(reg)) : InterpretationPtr());

        // custom or default heuristics?
        if (eatom.pluginAtom->providesCustomExternalAtomEvaluationHeuristicsFactory()) {
            DBGLOG(DBG, "Using custom external atom heuristics for external atom " << activeInnerEatoms[i]);
            eaEvalHeuristics.push_back(eatom.pluginAtom->getCustomExternalAtomEvaluationHeuristicsFactory()->createHeuristics(reg));
        }
        else {
            DBGLOG(DBG, "Using default external atom heuristics for external atom " << activeInnerEatoms[i]);
            eaEvalHeuristics.push_back(defaultExternalAtomEvalHeuristics);
        }
    }

    // create ufs check heuristics as selected
    ufsCheckHeuristics = factory.ctx.unfoundedSetCheckHeuristicsFactory->createHeuristics(annotatedGroundProgram, reg);
    verifiedAuxes = InterpretationPtr(new Interpretation(reg));
}


void GenuineGuessAndCheckModelGenerator::initializeVerificationWatchLists()
{

    // set external atom evaluation strategy according to selected heuristics
    verifyWatchList.clear();
    unverifyWatchList.clear();
    for (uint32_t i = 0; i < activeInnerEatoms.size(); ++i) {

        // watch all atoms in the scope of the external atom for watch one input atom for verification
        bm::bvector<>::enumerator en = annotatedGroundProgram.getEAMask(i)->mask()->getStorage().first();
        bm::bvector<>::enumerator en_end = annotatedGroundProgram.getEAMask(i)->mask()->getStorage().end();
        if (en < en_end) {
            verifyWatchList[*en].push_back(i);
        }

        // watch all atoms in the scope of the external atom for unverification
        en = annotatedGroundProgram.getEAMask(i)->mask()->getStorage().first();
        en_end = annotatedGroundProgram.getEAMask(i)->mask()->getStorage().end();
        while (en < en_end) {
            unverifyWatchList[*en].push_back(i);
            en++;
        }
    }
}


InterpretationPtr GenuineGuessAndCheckModelGenerator::generateNextModel()
{
    // now we have postprocessed input in postprocessedInput
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidgcsolve, "genuine guess and check loop");
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidhexsolve, "HEX solver time (gNM GenGnC)");
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidhexsolve2, "HEX solver time");

    InterpretationPtr modelCandidate;
    do {
        LOG(DBG,"asking for next model");

        // Search space pruning: the idea is to set the current global optimum as upper limit in the solver instance (of this unit) to eliminate interpretations with higher costs.
        // Note that this optimization is conservative such that the algorithm remains complete even when the program is split. Because costs can be only positive,
        // if the costs of a partial model are greater than the current global optimum then also any completion of this partial model (by combining it with other units)
        // would be non-optimal.
        if (factory.ctx.config.getOption("OptimizationByBackend")) solver->setOptimum(factory.ctx.currentOptimum);
        modelCandidate = solver->getNextModel();

        DBGLOG(DBG, "Statistics:" << std::endl << solver->getStatistics());
        if( !modelCandidate ) {
            // compute reasons
            if (factory.ctx.config.getOption("TransUnitLearning") && cmModelCount == 0) {
                identifyInconsistencyCause();
            }

            LOG(DBG,"unsatisfiable -> returning no model");
            return InterpretationPtr();
        }

        DLVHEX_BENCHMARK_REGISTER_AND_COUNT(ssidmodelcandidates, "Candidate compatible sets", 1);
        LOG_SCOPE(DBG,"gM", false);
        LOG(DBG,"got guess model, will do compatibility check on " << *modelCandidate);
        if (!finalCompatibilityCheck(modelCandidate)) {
            LOG(DBG,"compatibility failed");

/*
            if (!!analysissolver){
                InterpretationPtr rematoms(new Interpretation(factory.ctx.registry()));
                InterpretationPtr relevantatoms(new Interpretation(*annotatedGroundProgram.getProgramMask()));
                relevantatoms->getStorage() |= postprocessedInput->getStorage();
                bm::bvector<>::enumerator en = relevantatoms->getStorage().first();
                bm::bvector<>::enumerator en_end = relevantatoms->getStorage().end();
                while (en < en_end) {
                    if (factory.ctx.registry()->ogatoms.getIDByAddress(*en).isAuxiliary()) rematoms->setFact(*en);
                    en++;
                }
                relevantatoms->getStorage() -= rematoms->getStorage();
                Nogood ng(relevantatoms, modelCandidate);
                DBGLOG(DBG, "Adding model candidate " << ng.getStringRepresentation(factory.ctx.registry()) << " to inconsistency analyzer");
                analysissolver->addNogood(ng);
            }
*/
            continue;
        }

        LOG(DBG, "Checking if model candidate is a model");
        if (!isModel(modelCandidate)) {
            LOG(DBG,"isModel failed");
            continue;
        }

        // remove edb and the guess (from here we don't need the guess anymore)
        {
            DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidcc, "GenuineGnCMG::gNM postproc");
            DBGLOG(DBG, "Got a model, removing replacement atoms");
            modelCandidate->getStorage() -= factory.gpMask.mask()->getStorage();
            modelCandidate->getStorage() -= factory.gnMask.mask()->getStorage();
            modelCandidate->getStorage() -= mask->getStorage();
        }

        LOG(DBG,"returning model without guess: " << *modelCandidate);

        printUnitInfo("[IR] ");
        DBGLOG(DBG, "[IR] Unit model: " << *modelCandidate);

        cmModelCount++;
        return modelCandidate;
    }while(true);
}

void GenuineGuessAndCheckModelGenerator::identifyInconsistencyCause() {

    printUnitInfo("[IR] ");
    DBGLOG(DBG, "[IR] identifyInconsistencyCause");
    DBGLOG(DBG, "[IR] Explanation atoms: " << *explAtoms << std::endl);








#if 0




// analysis
InterpretationPtr nogoodAtoms(new Interpretation(factory.ctx.registry()));

InterpretationPtr analysisProgramEdb(new Interpretation(factory.ctx.registry()));
OrdinaryASPProgram analysisProgram(factory.ctx.registry(), std::vector<ID>(), analysisProgramEdb, factory.ctx.maxint);
bm::bvector<>::enumerator en, en_end;
int nextAtomID = 0;
ID satAtom = getAuxiliaryAtom('x', ID::termFromInteger(nextAtomID++));

// explanation guess
DBGLOG(DBG, "[IR] Adding guessing rules for explanation atoms " << *explAtoms);
InterpretationPtr negExplAtoms(new Interpretation(factory.ctx.registry()));
en = explAtoms->getStorage().first();
en_end = explAtoms->getStorage().end();
while (en < en_end) {
    Rule explanationGuess(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR | ID::PROPERTY_RULE_DISJ);
    ID eAtomID = factory.ctx.registry()->ogatoms.getIDByAddress(*en);
    ID neAtomID = getAuxiliaryAtom('x', eAtomID);
    negExplAtoms->setFact(neAtomID.address);
    explanationGuess.head.push_back(eAtomID);                           // atom is in R+
    explanationGuess.head.push_back(neAtomID);                          // atom is in R-
    explanationGuess.head.push_back(getAuxiliaryAtom('y', eAtomID));    // atom is neither in R+ nor in R-
    analysisProgram.idb.push_back(factory.ctx.registry()->storeRule(explanationGuess));
    en++;
}

// add nogoods as rules such that the saturation atom is derived if a nogood is violated
const NogoodSet& nogoodStorage = analysissolver->getNogoodStorage();
for (int i = 0; i < nogoodStorage.getNogoodCount(); ++i){
    const Nogood& ng = nogoodStorage.getNogood(i);
    Rule nogoodCheckingRule(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);
    nogoodCheckingRule.head.push_back(satAtom);
    BOOST_FOREACH (ID lit, ng) {
        if (lit.isNaf()) {
            nogoodCheckingRule.body.push_back(ID::nafLiteralFromAtom(factory.ctx.registry()->ogatoms.getIDByAddress(lit.address)));
        }else{
            nogoodCheckingRule.body.push_back(ID::posLiteralFromAtom(factory.ctx.registry()->ogatoms.getIDByAddress(lit.address)));
        }
        nogoodAtoms->setFact(lit.address);
    }
    analysisProgram.idb.push_back(factory.ctx.registry()->storeRule(nogoodCheckingRule));
}

// interpretation guess and saturation
DBGLOG(DBG, "[IR] Adding guess of interpretation over " << *nogoodAtoms);
en = nogoodAtoms->getStorage().first();
en_end = nogoodAtoms->getStorage().end();
while (en < en_end) {
    // interpretation guess
    Rule interpretationGuess(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR | ID::PROPERTY_RULE_DISJ);
    ID atomID = factory.ctx.registry()->ogatoms.getIDByAddress(*en);
    interpretationGuess.head.push_back(getAuxiliaryAtom('p', atomID));    // atom in interpretation
    interpretationGuess.head.push_back(getAuxiliaryAtom('n', atomID));    // -atom in interpretation
    analysisProgram.idb.push_back(factory.ctx.registry()->storeRule(interpretationGuess));

    // saturation on discrepancy of interpretation guess from explanation guess
    {
        Rule discrepancy(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);
        discrepancy.head.push_back(satAtom);
        discrepancy.body.push_back(ID::posLiteralFromAtom(atomID));                                   // atom in positive explanation guess
        discrepancy.body.push_back(ID::posLiteralFromAtom(getAuxiliaryAtom('n', atomID)));            // -atom in interpretation
        analysisProgram.idb.push_back(factory.ctx.registry()->storeRule(discrepancy));
    }{
        Rule discrepancy(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);
        discrepancy.head.push_back(satAtom);
        discrepancy.body.push_back(ID::posLiteralFromAtom(getAuxiliaryAtom('x', atomID)));            // atom in negative explanation guess
        discrepancy.body.push_back(ID::posLiteralFromAtom(getAuxiliaryAtom('p', atomID)));            // atom in interpretation
        analysisProgram.idb.push_back(factory.ctx.registry()->storeRule(discrepancy));
    }
    {
        Rule satInterpretation(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);
        satInterpretation.head.push_back(getAuxiliaryAtom('p', atomID));                              // atom in interpretation if saturated
        satInterpretation.body.push_back(ID::posLiteralFromAtom(satAtom));
        analysisProgram.idb.push_back(factory.ctx.registry()->storeRule(satInterpretation));
    }{
        Rule satInterpretation(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);
        satInterpretation.head.push_back(getAuxiliaryAtom('n', atomID));                              // -atom in interpretation if saturated
        satInterpretation.body.push_back(ID::posLiteralFromAtom(satAtom));
        analysisProgram.idb.push_back(factory.ctx.registry()->storeRule(satInterpretation));
    }

    en++;
}

/*
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
*/

// restrict search to models with atom sat
{
    DBGLOG(DBG, "[IR] Adding sat constraint");
    Rule satConstraint(ID::MAINKIND_RULE | ID::SUBKIND_RULE_CONSTRAINT);
    satConstraint.body.push_back(ID::nafLiteralFromAtom(satAtom));
    analysisProgram.idb.push_back(factory.ctx.registry()->storeRule(satConstraint));
}

#ifndef NDEBUG
if (!!analysisProgram.edb) {
    DBGLOG(DBG, "[IR] Analysis program:" << std::endl << *analysisProgramEdb << std::endl << printManyToString<RawPrinter>(analysisProgram.idb, "\n     ", factory.ctx.registry()));
}else{
    DBGLOG(DBG, "[IR] Analysis program:" << std::endl << printManyToString<RawPrinter>(analysisProgram.idb, "\n     ", factory.ctx.registry()));
}
#endif

// solve the instance
GenuineSolverPtr analysisSolver = GenuineSolver::getInstance(factory.ctx, analysisProgram);
InterpretationConstPtr model;
while ( (model = analysisSolver->getNextModel()) != InterpretationConstPtr() ) {
    DBGLOG(DBG, "[IR] Answer set of analysis program: " << *model);

    // extract explanation
    Nogood explanation;
    en = model->getStorage().first();
    en_end = model->getStorage().end();
    while (en < en_end) {
        if (explAtoms->getFact(*en)) {
            explanation.insert(NogoodContainer::createLiteral(*en, true));
        }else if (negExplAtoms->getFact(*en)) {
            explanation.insert(NogoodContainer::createLiteral(factory.ctx.registry()->getIDByAuxiliaryConstantSymbol(factory.ctx.registry()->ogatoms.getByAddress(*en).tuple[0]).address, false));
        }
        en++;
    }
    DBGLOG(DBG, "[IR] Explanation: " << explanation.getStringRepresentation(factory.ctx.registry()));
}




#endif














    // non-optimized grounding
//        InterpretationPtr enrichedEdb(new Interpretation(factory.ctx.registry())); // add explanation atoms to EDB to make sure that the grounding is complete wrt. these atoms
//        for (int i = 0; i < factory.ctx.registry()->ogatoms.getSize(); ++i) enrichedEdb->setFact(i);
//        enrichedEdb->getStorage() |= program.edb->getStorage();
//        enrichedEdb->getStorage() |= unitMask->mask()->getStorage();
//        enrichedEdb->getStorage() |= explAtoms->getStorage();
//        InterpretationConstPtr originalEdb = program.edb;
    OrdinaryASPProgram enrichedProgram = guessingProgram;
//        enrichedProgram.edb = enrichedEdb;
    DBGLOG(DBG, "[IR] Grounding program for inconsistency analysis without optimizations:" << std::endl <<
                "[IR]     " << *enrichedProgram.edb << std::endl <<
                "[IR]     " << printManyToString<RawPrinter>(enrichedProgram.idb, "\n[IR]     ", factory.ctx.registry()));
    InternalGrounder nonOptimizedGrounder(factory.ctx, enrichedProgram, InternalGrounder::builtin);
    OrdinaryASPProgram nonoptgp = nonOptimizedGrounder.getGroundProgram();
    if (!!nonoptgp.mask) mask->add(*nonoptgp.mask);
    nonoptgp.mask = InterpretationConstPtr();
    DBGLOG(DBG, "[IR] Unoptimized ground program for inconsistency analysis:" << std::endl <<
                "[IR]     " << *nonoptgp.edb << std::endl <<
                "[IR]     " << printManyToString<RawPrinter>(nonoptgp.idb, "\n[IR]     ", factory.ctx.registry()));
    AnnotatedGroundProgram nonoptagp(factory.ctx, nonoptgp, factory.innerEatoms);
//        program.edb = originalEdb;

    // start analysis solver
    analysissolver.reset(new InternalGroundDASPSolver(factory.ctx, nonoptagp, explAtoms));
    std::vector<ID> assumptions;
    bm::bvector<>::enumerator en = explAtoms->getStorage().first();
    bm::bvector<>::enumerator en_end = explAtoms->getStorage().end();
    while (en < en_end) {
        assumptions.push_back(unitInput->getFact(*en) ? ID::posLiteralFromAtom(factory.ctx.registry()->ogatoms.getIDByAddress(*en)) : ID::nafLiteralFromAtom(factory.ctx.registry()->ogatoms.getIDByAddress(*en)));
        en++;
    }
    DBGLOG(DBG, "[IR] Adding assumptions " << printManyToString<RawPrinter>(assumptions, ",", factory.ctx.registry()));
    analysissolver->restartWithAssumptions(assumptions);
    for (int i = 0; i < analysissolverNogoods->getNogoodCount(); ++i) {
        DBGLOG(DBG, "[IR] Adding learned nogood from to inconsistency analyzer: " << analysissolverNogoods->getNogood(i).getStringRepresentation(factory.ctx.registry()));
        analysissolver->addNogood(analysissolverNogoods->getNogood(i));
    }

    // learn from successor units
    if (factory.ctx.config.getOption("TransUnitLearning")){
        typedef std::pair<Nogood, int> NogoodIntegerPair;
        DBGLOG(DBG, "[IR] Adding nogoods from successor to inconsistency analyzer");
        BOOST_FOREACH (NogoodIntegerPair nip, factory.succNogoods){
            DBGLOG(DBG, "[IR] Adding nogood from successor " << nip.first.getStringRepresentation(factory.ctx.registry()));
            analysissolver->addNogood(nip.first);
        }
    }

    // imodel must always be NULL, but we still have to call analysissolver->getNextModel() to make sure that it propagates to derive the conflict
    InterpretationConstPtr imodel = analysissolver->getNextModel();
#ifndef NDEBUG
    if (!!imodel) { DBGLOG(DBG, "[IR] Error: Inconsistency analysis program has model " << *imodel << " but should be inconsistent"); }
    assert (!imodel && "Instance did not yield models, but after restart it is not inconsistent!");
#endif

    haveInconsistencyCause = true;
    inconsistencyCause = analysissolver->getInconsistencyCause(explAtoms);
    DBGLOG(DBG, "[IR] Inconsistency of program and inconsistence cause have been detected: " << inconsistencyCause.getStringRepresentation(factory.ctx.registry()));
    DBGLOG(DBG, "[IR] Explanation: " << inconsistencyCause.getStringRepresentation(factory.ctx.registry()));

}

const Nogood* GenuineGuessAndCheckModelGenerator::getInconsistencyCause(){
    DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sidic, "Unit inconsistency causes", (haveInconsistencyCause ? 1 : 0));
    printUnitInfo("[IR] ");
    DBGLOG(DBG, "[IR] Inconsistency cause was requested: " << (haveInconsistencyCause ? "" : "not") << " available");
    return (factory.ctx.config.getOption("TransUnitLearning") && haveInconsistencyCause ? &inconsistencyCause : 0);
}

void GenuineGuessAndCheckModelGenerator::addNogood(const Nogood* cause){
    DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sidna, "Nogoods added from outside to GnC mg", 1);
    printUnitInfo("[IR] ");
    DBGLOG(DBG, "[IR] Adding nogood to model generator: " << cause->getStringRepresentation(factory.ctx.registry()));
    if (factory.ctx.config.getOption("TransUnitLearning")){
        if (!!solver) solver->addNogood(*cause);
//        if (!!analysissolver) analysissolver->addNogood(*cause);
    }
}

void GenuineGuessAndCheckModelGenerator::generalizeNogood(Nogood ng)
{

    if (!ng.isGround()) return;

    DBGLOG(DBG, "Generalizing " << ng.getStringRepresentation(reg));

    // find the external atom related to this nogood
    ID eaid = ID_FAIL;
    BOOST_FOREACH (ID l, ng) {
        if (reg->ogatoms.getIDByAddress(l.address).isExternalAuxiliary() && annotatedGroundProgram.mapsAux(l.address)) {
            eaid = l;
            break;
        }
    }
    if (eaid == ID_FAIL) return;

    assert(annotatedGroundProgram.getAuxToEA(eaid.address).size() > 0);
    DBGLOG(DBG, "External atom is " << annotatedGroundProgram.getAuxToEA(eaid.address)[0]);
    const ExternalAtom& ea = reg->eatoms.getByID(annotatedGroundProgram.getAuxToEA(eaid.address)[0]);

    // learn related nonground nogoods
    int oldCount = learnedEANogoods->getNogoodCount();
    ea.pluginAtom->generalizeNogood(ng, &factory.ctx, learnedEANogoods);
}


void GenuineGuessAndCheckModelGenerator::learnSupportSets()
{

    if (factory.ctx.config.getOption("SupportSets")) {
        SimpleNogoodContainerPtr potentialSupportSets = SimpleNogoodContainerPtr(new SimpleNogoodContainer());
        SimpleNogoodContainerPtr supportSets = SimpleNogoodContainerPtr(new SimpleNogoodContainer());
        for(unsigned eaIndex = 0; eaIndex < activeInnerEatoms.size(); ++eaIndex) {
            // evaluate the external atom if it provides support sets
            const ExternalAtom& eatom = reg->eatoms.getByID(activeInnerEatoms[eaIndex]);
            if (eatom.getExtSourceProperties().providesSupportSets()) {
                DBGLOG(DBG, "Evaluating external atom " << activeInnerEatoms[eaIndex] << " for support set learning");
                learnSupportSetsForExternalAtom(factory.ctx, activeInnerEatoms[eaIndex], potentialSupportSets);
            }
        }

        DLVHEX_BENCHMARK_REGISTER(sidnongroundpsupportsets, "nonground potential supportsets");
        DLVHEX_BENCHMARK_COUNT(sidnongroundpsupportsets, potentialSupportSets->getNogoodCount());

        // ground the support sets exhaustively
        NogoodGrounderPtr nogoodgrounder = NogoodGrounderPtr(new ImmediateNogoodGrounder(factory.ctx.registry(), potentialSupportSets, potentialSupportSets, annotatedGroundProgram));

        int nc = 0;
        while (nc < potentialSupportSets->getNogoodCount()) {
            nc = potentialSupportSets->getNogoodCount();
            nogoodgrounder->update();
        }
        DLVHEX_BENCHMARK_REGISTER(sidgroundpsupportsets, "ground potential supportsets");
        DLVHEX_BENCHMARK_COUNT(sidgroundpsupportsets, supportSets->getNogoodCount());

        // all support sets are also learned nogoods
        bool keep;
        for (int i = 0; i < potentialSupportSets->getNogoodCount(); ++i) {
            const Nogood& ng = potentialSupportSets->getNogood(i);
            if (ng.isGround()) {
                // determine the external atom replacement in ng
                ID eaAux = ID_FAIL;
                BOOST_FOREACH (ID lit, ng) {
                    if (reg->ogatoms.getIDByAddress(lit.address).isExternalAuxiliary()) {
                        if (eaAux != ID_FAIL) throw GeneralError("Set " + ng.getStringRepresentation(reg) + " is not a valid support set because it contains multiple external literals");
                        eaAux = lit;
                    }
                }
                if (eaAux == ID_FAIL) throw GeneralError("Set " + ng.getStringRepresentation(reg) + " is not a valid support set because it contains no external literals");

                // determine the according external atom
                if (annotatedGroundProgram.mapsAux(eaAux.address)) {
                    DBGLOG(DBG, "Evaluating guards of " << ng.getStringRepresentation(reg));
                    keep = true;
                    Nogood ng2 = ng;
                    reg->eatoms.getByID(annotatedGroundProgram.getAuxToEA(eaAux.address)[0]).pluginAtom->guardSupportSet(keep, ng2, eaAux);
                    if (keep) {
                        #ifdef DEBUG
                        // ng2 must be a subset of ng and still a valid support set
                        ID aux = ID_FAIL;
                        BOOST_FOREACH (ID id, ng2) {
                            if (reg->ogatoms.getIDByAddress(id.address).isExternalAuxiliary()) aux = id;
                            assert(ng.count(id) > 0);
                        }
                        assert(aux != ID_FAIL);
                        #endif
                        DBGLOG(DBG, "Keeping in form " << ng2.getStringRepresentation(reg));
                        learnedEANogoods->addNogood(ng2);
                        supportSets->addNogood(ng2);
                        #ifdef DEBUG
                    }
                    else {
                        assert(ng == ng2);
                        DBGLOG(DBG, "Rejecting " << ng2.getStringRepresentation(reg));
                        #endif
                    }
                }
            }
        }

        DLVHEX_BENCHMARK_REGISTER(sidgroundsupportsets, "final ground supportsets");
        DLVHEX_BENCHMARK_COUNT(sidgroundsupportsets, supportSets->getNogoodCount());

        // add them to the annotated ground program to make use of them for verification
        DBGLOG(DBG, "Adding " << supportSets->getNogoodCount() << " support sets to annotated ground program");
        annotatedGroundProgram.setCompleteSupportSetsForVerification(supportSets);
    }
}


void GenuineGuessAndCheckModelGenerator::updateEANogoods(
InterpretationConstPtr compatibleSet,
InterpretationConstPtr assigned,
InterpretationConstPtr changed)
{
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(siduean, "updateEANogoods");
    DBGLOG(DBG, "updateEANogoods");

    // generalize ground nogoods to nonground ones
    if (factory.ctx.config.getOption("ExternalLearningGeneralize")) {
        int max = learnedEANogoods->getNogoodCount();
        for (int i = learnedEANogoodsTransferredIndex; i < max; ++i) {
            generalizeNogood(learnedEANogoods->getNogood(i));
        }
    }

    // instantiate nonground nogoods
    if (factory.ctx.config.getOption("NongroundNogoodInstantiation")) {
        nogoodGrounder->update(compatibleSet, assigned, changed);
    }

    // transfer nogoods to the solver
    DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sidcompatiblesets, "Learned EA-Nogoods", learnedEANogoods->getNogoodCount() - learnedEANogoodsTransferredIndex);
    for (int i = learnedEANogoodsTransferredIndex; i < learnedEANogoods->getNogoodCount(); ++i) {
        const Nogood& ng = learnedEANogoods->getNogood(i);
        DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sidnogoodsizes, "Sum of learned EA-Nogood sizes", ng.size());
        if (factory.ctx.config.getOption("PrintLearnedNogoods")) {
            // we cannot use i==1 because of learnedEANogoods.clear() below in this function
            static bool first = true;
            if( first ) {
                if (factory.ctx.config.getOption("GenuineSolver") >= 3) {
                    LOG(DBG, "( NOTE: With clasp backend, learned nogoods become effective with a delay because of multithreading! )");
                }
                else {
                    LOG(DBG, "( NOTE: With i-backend, learned nogoods become effective AFTER the next model was printed ! )");
                }
                first = false;
            }
            LOG(DBG,"learned nogood " << ng.getStringRepresentation(reg));
        }
        if (ng.isGround()) {
            DBGLOG(DBG, "Adding learned nogood " << ng.getStringRepresentation(reg) << " to solver");
            if (factory.ctx.config.getStringOption("DumpEANogoods")[0] != '\0'){
		        std::ofstream filev(factory.ctx.config.getStringOption("DumpEANogoods").c_str(), std::ios_base::app);
                filev << ng.getStringRepresentation(reg) << std::endl;
            }
            solver->addNogood(ng);
            if (factory.ctx.config.getOption("TransUnitLearning")) {
                DBGLOG(DBG, "[IR] Adding learned nogood to inconsistency analyzer: " << ng.getStringRepresentation(reg));
                analysissolverNogoods->addNogood(ng);
            }

            if ( factory.ctx.config.getOption("ExternalAtomVerificationFromLearnedNogoods") ) {
                eavTree.addNogood(ng, reg, true);
                DBGLOG(DBG, "Adding nogood " << ng.getStringRepresentation(reg) << "; to verification tree; updated tree:" << std::endl << eavTree.toString(reg));
            }
        }
    }

    // for encoding-based UFS checkers and explicit FLP checks, we need to keep learned nogoods (otherwise future UFS searches will not be able to use them)
    // for assumption-based UFS checkers we can delete them as soon as nogoods were added both to the main search and to the UFS search
    if ( factory.ctx.config.getOption("UFSCheckAssumptionBased") ||
         (annotatedGroundProgram.hasECycles() == 0 && factory.ctx.config.getOption("FLPDecisionCriterionE")) ) {
        ufscm->learnNogoodsFromMainSearch(true);
        if (factory.ctx.config.getOption("NongroundNogoodInstantiation")) nogoodGrounder->resetWatched(learnedEANogoods);
        learnedEANogoods->clear();
    }
    else {
        learnedEANogoods->forgetLeastFrequentlyAdded();
    }
    learnedEANogoodsTransferredIndex = learnedEANogoods->getNogoodCount();
}


bool GenuineGuessAndCheckModelGenerator::finalCompatibilityCheck(InterpretationConstPtr modelCandidate)
{

    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidcc, "GenuineGnCMG: finalCompat");
    // did we already verify during model construction or do we have to do the verification now?
    bool compatible;

    compatible = true;
    for (uint32_t eaIndex = 0; eaIndex < activeInnerEatoms.size(); ++eaIndex) {
        DBGLOG(DBG, "NoPropagator: " << factory.ctx.config.getOption("NoPropagator") << ", eaEvaluated[" << eaIndex << "]=" << eaEvaluated[eaIndex]);
        assert(!(factory.ctx.config.getOption("NoPropagator") && eaEvaluated[eaIndex]) && "Verification result was stored for later usage although NoPropagator property was set");
        if (eaEvaluated[eaIndex] == true && eaVerified[eaIndex] == true) {
        }
        if (eaEvaluated[eaIndex] == true && eaVerified[eaIndex] == false) {
            DBGLOG(DBG, "External atom " << activeInnerEatoms[eaIndex] << " was evaluated but falsified");
            compatible = false;
            if (!factory.ctx.config.getOption("AlwaysEvaluateAllExternalAtoms")) break;
        }
        if (eaEvaluated[eaIndex] == false) {
            // try to verify
            DBGLOG(DBG, "External atom " << activeInnerEatoms[eaIndex] << " is not verified, trying to do this now");
            verifyExternalAtom(eaIndex, modelCandidate);
            DBGLOG(DBG, "Verification result: " << eaVerified[eaIndex]);

            if (eaVerified[eaIndex] == false) {
                compatible = false;
                if (!factory.ctx.config.getOption("AlwaysEvaluateAllExternalAtoms")) break;
            }
        }
    }
    DBGLOG(DBG, "Compatible: " << compatible);

    return compatible;
}


bool GenuineGuessAndCheckModelGenerator::isModel(InterpretationConstPtr compatibleSet)
{

    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidcc, "GenuineGnCMG: isModel");
    // FLP: ensure minimality of the compatible set wrt. the reduct (if necessary)
    if (annotatedGroundProgram.hasECycles() == 0 && factory.ctx.config.getOption("FLPDecisionCriterionE")) {
        DBGLOG(DBG, "No e-cycles --> No FLP/UFS check necessary");
        return true;
    }
    else {
        DBGLOG(DBG, "e-cycles --> FLP/UFS check necessary");

        // Explicit FLP check
        if (factory.ctx.config.getOption("FLPCheck")) {
            DBGLOG(DBG, "FLP Check");
            // do FLP check (possibly with nogood learning) and add the learned nogoods to the main search
            bool result = isSubsetMinimalFLPModel<GenuineSolver>(compatibleSet, postprocessedInput, factory.ctx,
                factory.ctx.config.getOption("ExternalLearning") ? learnedEANogoods : SimpleNogoodContainerPtr());
            updateEANogoods(compatibleSet);
            return result;
        }

        // UFS check
        if (factory.ctx.config.getOption("UFSCheck")) {
            DBGLOG(DBG, "UFS Check");
            bool result = unfoundedSetCheck(compatibleSet);
            updateEANogoods(compatibleSet);
            return result;
        }

        // no check
        return true;
    }

    assert (false);
}


namespace
{
    // collect all components on the way
    struct DFSVisitor:
    public boost::default_dfs_visitor
    {
        const ComponentGraph& cg;
        ComponentGraph::ComponentSet& comps;
        DFSVisitor(const ComponentGraph& cg, ComponentGraph::ComponentSet& comps): boost::default_dfs_visitor(), cg(cg), comps(comps) {}
        DFSVisitor(const DFSVisitor& other): boost::default_dfs_visitor(), cg(other.cg), comps(other.comps) {}
        template<typename GraphT>
        void discover_vertex(ComponentGraph::Component comp, const GraphT&) {
            comps.insert(comp);
        }
    };

    void transitivePredecessorComponents(const ComponentGraph& compgraph, ComponentGraph::Component from, ComponentGraph::ComponentSet& preds) {
        // we need a hash map, as component graph is no graph with vecS-storage
        //
        typedef boost::unordered_map<ComponentGraph::Component, boost::default_color_type> CompColorHashMap;
        typedef boost::associative_property_map<CompColorHashMap> CompColorMap;
        CompColorHashMap ccWhiteHashMap;
        // fill white hash map
        ComponentGraph::ComponentIterator cit, cit_end;
        for(boost::tie(cit, cit_end) = compgraph.getComponents();
        cit != cit_end; ++cit) {
            //boost::put(ccWhiteHashMap, *cit, boost::white_color);
            ccWhiteHashMap[*cit] = boost::white_color;
        }
        CompColorHashMap ccHashMap(ccWhiteHashMap);

        //
        // do DFS
        //
        DFSVisitor dfs_vis(compgraph, preds);
        //LOG("doing dfs visit for root " << *itr);
        boost::depth_first_visit(
            compgraph.getInternalGraph(),
            from,
            dfs_vis,
            CompColorMap(ccHashMap));
        DBGLOG(DBG,"predecessors of " << from << " are " << printrange(preds));
    }
}


bool GenuineGuessAndCheckModelGenerator::unfoundedSetCheck(InterpretationConstPtr partialInterpretation, InterpretationConstPtr assigned, InterpretationConstPtr changed, bool partial)
{

    assert ( (!partial || (!!assigned && !!changed)) && "partial UFS checks require information about the assigned atoms");

    DBGLOG(DBG, "GenuineGuessAndCheckModelGenerator::unfoundedSetCheck");
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "genuine g&c unfoundedSetCheck");

    DBGLOG(DBG, "unfoundedSetCheck was called to perform " << (partial ? "partial" : "full") << " UFS check");

    bool performCheck = false;
    static std::set<ID> emptySkipProgram;

    if (partial) {
        assert (!!assigned && !!changed);

        DBGLOG(DBG, "Calling UFS check heuristic");

        if (ufsCheckHeuristics->doUFSCheck(verifiedAuxes, partialInterpretation, assigned, changed)) {

            if (!factory.ctx.config.getOption("UFSCheck") && !factory.ctx.config.getOption("UFSCheckAssumptionBased")) {
                LOG(WARNING, "Partial unfounded set checks are only possible if FLP check method is set to unfounded set check; will skip the check");
                return true;
            }

            // ufs check without nogood learning makes no sense if the interpretation is not complete
            if (!factory.ctx.config.getOption("UFSLearning")) {
                LOG(WARNING, "Partial unfounded set checks is useless if unfounded set learning is not enabled; will perform the check anyway, but result does not have any effect");
            }

            DBGLOG(DBG, "Heuristic decides to do a partial UFS check");
            performCheck = true;
        }
        else {
            DBGLOG(DBG, "Heuristic decides not to do an UFS check");
        }
    }
    else {
        DBGLOG(DBG, "Since the method was called for a full check, it will be performed");
        performCheck = true;
    }

    if (performCheck) {
        std::vector<IDAddress> ufs = ufscm->getUnfoundedSet(partialInterpretation,
            (partial ? ufsCheckHeuristics->getSkipProgram() : emptySkipProgram),
            factory.ctx.config.getOption("ExternalLearning") ? learnedEANogoods : SimpleNogoodContainerPtr());
        bool ufsFound = (ufs.size() > 0);
        #ifndef NDEBUG
        std::stringstream ss;
        ss << "UFS result: " << (ufsFound ? "" : "no ") << "UFS found (interpretation: " << *partialInterpretation;
        if (!!assigned) ss << ", assigned: " << *assigned;
        ss << ")";
        DBGLOG(DBG, ss.str());
        #endif

        if (ufsFound && factory.ctx.config.getOption("UFSLearning")) {
            Nogood ng = ufscm->getLastUFSNogood();
            DBGLOG(DBG, "Adding UFS nogood: " << ng);

            #ifndef NDEBUG
            // the learned nogood must not talk about unassigned atoms
            if (!!assigned) {
                BOOST_FOREACH (ID lit, ng) {
                    assert(assigned->getFact(lit.address));
                }
            }
            #endif
            solver->addNogood(ng);
            if (factory.ctx.config.getOption("TransUnitLearning")) analysissolverNogoods->addNogood(ng);
        }
        return !ufsFound;
    }
    else {
        return true;
    }
}


IDAddress GenuineGuessAndCheckModelGenerator::getWatchedLiteral(int eaIndex, InterpretationConstPtr search, bool truthValue)
{

    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "getWatchedLiteral");
    bm::bvector<>::enumerator eaDepAtoms = annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage().first();
    bm::bvector<>::enumerator eaDepAtoms_end = annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage().end();
    bm::bvector<>::enumerator searchb = search->getStorage().first();
    bm::bvector<>::enumerator searchb_end = search->getStorage().end();

    // go through eamask
    while (eaDepAtoms < eaDepAtoms_end) {
        // if search bitset has correct truth value
        if (search->getFact(*eaDepAtoms) == truthValue) {
            DBGLOG(DBG, "Found watch " << *eaDepAtoms << " for atom " << activeInnerEatoms[eaIndex]);
            return *eaDepAtoms;
        }
        eaDepAtoms++;
    }

    return ID::ALL_ONES;
}


void GenuineGuessAndCheckModelGenerator::unverifyExternalAtoms(InterpretationConstPtr changed)
{
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "genuine g&c unverifyEAtoms");

    DBGLOG(DBG, "Unverify External Atoms");

    // for all changed atoms
    bm::bvector<>::enumerator en = changed->getStorage().first();
    bm::bvector<>::enumerator en_end = changed->getStorage().end();
    while (en < en_end) {
        // for all external atoms which watch this atom for unverification
        BOOST_FOREACH (int eaIndex, unverifyWatchList[*en]) {
            if (eaEvaluated[eaIndex]) {
                DBGLOG(DBG, "Unverifying external atom " << eaIndex);

                // unverify
                eaVerified[eaIndex] = false;
                eaEvaluated[eaIndex] = false;
                verifiedAuxes->getStorage() -= annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage();

                // *en is our new watch (as it is either undefined or was recently changed)
                verifyWatchList[*en].push_back(eaIndex);
            }
        }
        en++;
    }
    DBGLOG(DBG, "Unverify External Atoms finished");
}


bool GenuineGuessAndCheckModelGenerator::verifyExternalAtoms(InterpretationConstPtr partialInterpretation, InterpretationConstPtr assigned, InterpretationConstPtr changed)
{

    // If there is no information about assigned or changed atoms, then we do not do anything.
    // This is because we would need to assume the worst (any atom might have changed and no atom is currently assigned).
    // Under these assumptions we cannot do any useful computation since we could only blindly evaluate any external atom,
    // but this can also be done later (when we have a concrete compatible set).
    if (!assigned || !changed) return false;

    DBGLOG(DBG, "Updating changed atoms sets");
    // update set of changed input atoms
    for (int eaIndex = 0; eaIndex < activeInnerEatoms.size(); ++eaIndex) {
        const ExternalAtom& eatom = reg->eatoms.getByID(activeInnerEatoms[eaIndex]);
        if (eatom.getExtSourceProperties().doesCareAboutChanged()) {
            assert (!!changedAtomsPerExternalAtom[eaIndex]);
            changedAtomsPerExternalAtom[eaIndex]->add(*changed);
        }
    }

    DBGLOG(DBG, "Verify External Atoms");
    // go through all changed atoms which are now assigned
    bool conflict = false;
    bm::bvector<>::enumerator en = changed->getStorage().first();
    bm::bvector<>::enumerator en_end = changed->getStorage().end();
    while (en < en_end) {
        if (assigned->getFact(*en)) {

            // first call high and then low frquency heuristics
            for (int hf = 1; hf >= 0; hf--) {
                bool highFrequency = (hf == 1);
                DBGLOG(DBG, "Calling " << (highFrequency ? "high" : "low") << " frequency heuristics");

                // for all external atoms which watch this atom
                // for low frquency heuristics we use unverifyWatchList by intend as it contains all atoms relevant to this external atom
                std::vector<int>& watchlist = (highFrequency ? unverifyWatchList[*en] : verifyWatchList[*en]);
                BOOST_FOREACH (int eaIndex, watchlist) {
                    assert (!!eaEvalHeuristics[eaIndex]);

                    if ((highFrequency == eaEvalHeuristics[eaIndex]->frequent()) && !eaEvaluated[eaIndex]) {
                        const ExternalAtom& eatom = reg->eatoms.getByID(activeInnerEatoms[eaIndex]);

                        // evaluate external atom if the heuristics decides so
                        DBGLOG(DBG, "Calling " << (highFrequency ? "high" : "low") << " frequency heuristics for external atom " << eaEvalHeuristics[eaIndex]);
                        if (eaEvalHeuristics[eaIndex]->doEvaluate(
                            eatom,
                            annotatedGroundProgram.getEAMask(eaIndex)->mask(),
                            annotatedGroundProgram.getProgramMask(),
                        partialInterpretation, assigned, changed)) {
                            // evaluate it
                            bool answeredFromCacheOrSupportSets;
                            DBGLOG(DBG, "Heuristic decides to evaluate external atom " << activeInnerEatoms[eaIndex]);
                            conflict |= verifyExternalAtom(eaIndex, partialInterpretation, assigned,
                                eatom.getExtSourceProperties().doesCareAboutChanged() ? changedAtomsPerExternalAtom[eaIndex] : InterpretationConstPtr(),
                                &answeredFromCacheOrSupportSets);

                            // if the external source was actually called, then clear the set of changed atoms (otherwise keep them until the source is actually called)
                            if (!answeredFromCacheOrSupportSets && eatom.getExtSourceProperties().doesCareAboutChanged()) {
                                assert (!!changedAtomsPerExternalAtom[eaIndex]);
                                DBGLOG(DBG, "Resetting changed atoms of external atom " << activeInnerEatoms[eaIndex]);
                                changedAtomsPerExternalAtom[eaIndex]->clear();
                            }
                        }

                        // if the external atom is still not verified then find a new yet unassigned atom to watch
                        // (only necessary for low frequency heuristics since high frequency ones always watch all atoms)
                        if (!highFrequency && !eaEvaluated[eaIndex]) {
                            IDAddress id = getWatchedLiteral(eaIndex, assigned, false);
                            if (id != ID::ALL_ONES) verifyWatchList[id].push_back(eaIndex);
                        }
                    }
                }
                // current atom was set, so remove all watches
                verifyWatchList[*en].clear();
            }
        }

        en++;
    }

    DBGLOG(DBG, "Verify External Atoms finished " << (conflict ? "with" : "without") << " conflict");

    return conflict;
}


bool GenuineGuessAndCheckModelGenerator::verifyExternalAtom(int eaIndex, InterpretationConstPtr partialInterpretation, InterpretationConstPtr assigned, InterpretationConstPtr changed, bool* answeredFromCacheOrSupportSets)
{

    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "genuine g&c verifyEAtom");

    const ExternalAtom& eatom = reg->eatoms.getByID(activeInnerEatoms[eaIndex]);

    // if support sets are enabled, and the external atom provides complete support sets, we use them for verification
    if (!assigned && !changed && factory.ctx.config.getOption("SupportSets") &&
        (eatom.getExtSourceProperties().providesCompletePositiveSupportSets() || eatom.getExtSourceProperties().providesCompleteNegativeSupportSets()) &&
    annotatedGroundProgram.allowsForVerificationUsingCompleteSupportSets()) {
        if (answeredFromCacheOrSupportSets) *answeredFromCacheOrSupportSets = true;
        return verifyExternalAtomBySupportSets(eaIndex, partialInterpretation, assigned, changed);
    }
    else {
        return verifyExternalAtomByEvaluation(eaIndex, partialInterpretation, assigned, changed, answeredFromCacheOrSupportSets);
    }
}


bool GenuineGuessAndCheckModelGenerator::verifyExternalAtomByEvaluation(int eaIndex, InterpretationConstPtr partialInterpretation, InterpretationConstPtr assigned, InterpretationConstPtr changed, bool* answeredFromCache)
{
    assert (!!partialInterpretation && "interpretation not set");

    if (factory.ctx.config.getOption("ExternalAtomVerificationFromLearnedNogoods")) {
        DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sideav, "gen. g&c verifyEAtom by eav (attempt)");
        InterpretationConstPtr verifiedAuxes = eavTree.getVerifiedAuxiliaries(partialInterpretation, assigned, factory.ctx.registry());

        // check if all auxes are verified
        bm::bvector<>::enumerator en = annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage().first();
        bm::bvector<>::enumerator en_end = annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage().end();
        while (en < en_end) {
            if (factory.ctx.registry()->ogatoms.getIDByAddress(*en).isExternalAuxiliary()) {
                if (!verifiedAuxes->getFact(*en)) break;
            }
            en++;
        }
        if (en == en_end) {
            // verified
            DBGLOG(DBG, "Verified external atom without evaluation");
            DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sideavs, "gen. g&c verifyEAtom by eav (succeed)", 1);
            eaEvaluated[eaIndex] = true;
            eaVerified[eaIndex] = true;
            return true;
        }else{
            DBGLOG(DBG, "Could not verify external atom without evaluation --> will evaluate");
        }
    }

    {
        DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "gen. g&c verifyEAtom by eval.");

        // prepare EA evaluation
        InterpretationConstPtr mask = (annotatedGroundProgram.getEAMask(eaIndex)->mask());
        DBGLOG(DBG, "Initializing VerifyExternalAtomCB");
        VerifyExternalAtomCB vcb(partialInterpretation, factory.ctx.registry()->eatoms.getByID(activeInnerEatoms[eaIndex]), *(annotatedGroundProgram.getEAMask(eaIndex)));

        DBGLOG(DBG, "Assigning all auxiliary inputs");
        InterpretationConstPtr evalIntr = partialInterpretation;
        if (!factory.ctx.config.getOption("IncludeAuxInputInAuxiliaries")) {
            // make sure that ALL input auxiliary atoms are true, otherwise we might miss some output atoms and consider true output atoms wrongly as unfounded
            // clone and extend
            InterpretationPtr ncevalIntr(new Interpretation(*partialInterpretation));
            ncevalIntr->getStorage() |= annotatedGroundProgram.getEAMask(eaIndex)->getAuxInputMask()->getStorage();
            evalIntr = ncevalIntr;
        }

        // evaluate the external atom and learn nogoods if external learning is used
        if (!!assigned) {
             DBGLOG(DBG, "Verifying external Atom " << activeInnerEatoms[eaIndex] << " under " << *evalIntr << " (assigned: " << *assigned << ")");
        }else{
            DBGLOG(DBG, "Verifying external Atom " << activeInnerEatoms[eaIndex] << " under " << *evalIntr << " (assigned: all)");
        }
        evaluateExternalAtom(factory.ctx, activeInnerEatoms[eaIndex], evalIntr, vcb,
            factory.ctx.config.getOption("ExternalLearning") ? learnedEANogoods : NogoodContainerPtr(), assigned, changed, answeredFromCache);
        updateEANogoods(partialInterpretation, assigned, changed);

        // if the input to the external atom was complete, then remember the verification result;
        // for incomplete input we cannot yet decide this yet, evaluation is only done for learning purposes in this case
        DBGLOG(DBG, "Checking whether verification result is to be stored");
        if( !assigned ||
            (annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage() & annotatedGroundProgram.getProgramMask()->getStorage()).count() == (assigned->getStorage() & annotatedGroundProgram.getProgramMask()->getStorage()).count()) {
            eaVerified[eaIndex] = vcb.verify();
            DBGLOG(DBG, "Verifying " << activeInnerEatoms[eaIndex] << " (Result: " << eaVerified[eaIndex] << ")");

            // generate nogoods for falsified external atom auxiliaries
            if (!eaVerified[eaIndex] && factory.ctx.config.getOption("TransUnitLearning")){
                Nogood ng;
                bm::bvector<>::enumerator en = annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage().first();
                bm::bvector<>::enumerator en_end = annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage().end();
                while (en < en_end) {
                    if (!factory.ctx.registry()->ogatoms.getIDByAddress(*en).isExternalAuxiliary()) {
                        ng.insert(NogoodContainer::createLiteral(*en, partialInterpretation->getFact(*en)));
                    }
                    en++;
                }
                ng.insert(NogoodContainer::createLiteral(vcb.getFalsifiedAtom().address, partialInterpretation->getFact(vcb.getFalsifiedAtom().address)));
                DBGLOG(DBG, "[IR] Adding nogood for falsified external atom: " << ng.getStringRepresentation(factory.ctx.registry()));
                if (factory.ctx.config.getOption("TransUnitLearning")) analysissolverNogoods->addNogood(ng);
            }

            // we remember that we evaluated, only if there is a propagator that can undo this memory (that can unverify an eatom during model search)
            if(factory.ctx.config.getOption("NoPropagator") == 0) {
                DBGLOG(DBG, "Setting external atom status of " << eaIndex << " to evaluated");
                eaEvaluated[eaIndex] = true;
               if (eaVerified[eaIndex]) verifiedAuxes->getStorage() |= annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage();
            }

            return !eaVerified[eaIndex];
        }
        else {
            return false;
        }
    }
}


bool GenuineGuessAndCheckModelGenerator::verifyExternalAtomBySupportSets(int eaIndex, InterpretationConstPtr partialInterpretation, InterpretationConstPtr assigned, InterpretationConstPtr changed)
{

    assert (!assigned && !changed && " verification using complete support sets is only possible wrt. complete interpretations");
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "genuine g&c verifyEAtom by suoport sets");

    eaVerified[eaIndex] = annotatedGroundProgram.verifyExternalAtomsUsingCompleteSupportSets(eaIndex, partialInterpretation, InterpretationPtr());
    if (eaVerified[eaIndex]) verifiedAuxes->getStorage() |= annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage();

    // we remember that we evaluated, only if there is a propagator that can undo this memory (that can unverify an eatom during model search)
    if( factory.ctx.config.getOption("NoPropagator") == 0 ) {
        DBGLOG(DBG, "Setting external atom status of " << eaIndex << " to evaluated");
        eaEvaluated[eaIndex] = true;
    }

    return !eaVerified[eaIndex];
}


const OrdinaryASPProgram& GenuineGuessAndCheckModelGenerator::getGroundProgram()
{
    return annotatedGroundProgram.getGroundProgram();
}


void GenuineGuessAndCheckModelGenerator::propagate(InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed)
{

    assert (!!partialAssignment && !!assigned && !!changed);

    // update external atom verification results
    // (1) unverify external atoms if atoms, which are relevant to this external atom, have (potentially) changed
    unverifyExternalAtoms(changed);
    // (2) now verify external atoms (driven by a heuristic)
    bool conflict = verifyExternalAtoms(partialAssignment, assigned, changed);

    // UFS check can in principle also applied to conflicting assignments
    // since the heuristic knows which external atoms are correct and which ones not.
    // The check can be restricted to the non-conflicting part of the program.
    // Although there is already another reason for backtracking,
    // we still need to notify the heuristics such that it can update its internal information about assigned atoms.
    assert (!!ufsCheckHeuristics);
    ufsCheckHeuristics->updateSkipProgram(verifiedAuxes, partialAssignment, assigned, changed);
    if (!conflict) {
        if (annotatedGroundProgram.hasHeadCycles() == 0 && annotatedGroundProgram.hasECycles() == 0 &&
        factory.ctx.config.getOption("FLPDecisionCriterionHead") && factory.ctx.config.getOption("FLPDecisionCriterionE")) {
            DBGLOG(DBG, "No head- or e-cycles --> No FLP/UFS check necessary");
        }else{
            unfoundedSetCheck(partialAssignment, assigned, changed, true);
        }
    }else{
        ufsCheckHeuristics->notify(verifiedAuxes, partialAssignment, assigned, changed);
    }
}


DLVHEX_NAMESPACE_END

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
