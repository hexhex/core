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
 * @file ProgramCtx.cpp
 * @author Thomas Krennwallner
 * @date
 *
 * @brief Program context.
 *
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/PluginContainer.h"
#include "dlvhex2/State.h"
#include "dlvhex2/Printer.h"
//#include "dlvhex2/DLVProcess.h"
//#include "dlvhex2/EvalHeuristicEasy.h"

#include <boost/shared_ptr.hpp>

#include <sstream>
#include <iostream>

DLVHEX_NAMESPACE_BEGIN

ProgramCtx::ProgramCtx():
maxint(0), currentOptimumRelevantLevels(0), terminationRequest(false)
{
    config.setOption("AllowAggExtCycles",0);
    config.setOption("FLPDecisionCriterionHead", 1);
    config.setOption("FLPDecisionCriterionE", 1);
    config.setOption("FLPDecisionCriterionEM", 1);
    config.setOption("FLPDecisionCriterionEMI", 0);
    config.setOption("FLPCheck", 0);
    config.setOption("UFSCheck", 1);
    config.setOption("UFSCheckMonolithic", 0);
    config.setOption("UFSCheckAssumptionBased", 1);
    config.setOption("GenuineSolver", 0);
    config.setOption("AlphaSolver", 0);
    config.setOption("ExternalLearning", 1);
    config.setOption("UFSLearning", 1);
    config.setOption("UFSLearnStrategy", 2);
    config.setOption("ExternalLearningIOBehavior", 1);
    config.setOption("ExternalLearningMonotonicity", 1);
    config.setOption("ExternalLearningFunctionality", 1);
    config.setOption("ExternalLearningLinearity", 1);
    config.setOption("ExternalLearningNeg", 1);
    config.setOption("ExternalLearningUser", 1);
    config.setOption("ExternalLearningGeneralize", 0);
    config.setOption("AlwaysEvaluateAllExternalAtoms", 0);
    config.setOption("NongroundNogoodInstantiation", 0);
    config.setOption("UFSCheckHeuristics", 0);
    config.setOption("ModelQueueSize", 5);
    config.setOption("Silent", 0);
    config.setOption("Verbose", 0);
    config.setOption("UseExtAtomCache",1);
    config.setOption("KeepNamespacePrefix",0);
    config.setOption("DumpDepGraph",0);
    config.setOption("DumpCyclicPredicateInputAnalysisGraph",0);
    config.setOption("DumpCompGraph",0);
    config.setOption("DumpEvalGraph",0);
    config.setOption("DumpModelGraph",0);
    config.setOption("DumpIModelGraph",0);
    config.setOption("DumpAttrGraph",0);
    config.setOption("KeepAuxiliaryPredicates",0);
    config.setOption("NoFacts",0);
    config.setOption("NumberOfModels",0);
    config.setOption("RepeatEvaluation",0);
    config.setOption("LegacyECycleDetection",0);
    config.setOption("NMLP", 0);
    config.setOption("MLP", 0);
    config.setOption("Forget", 0);
    config.setOption("Split", 0);
    config.setOption("SkipStrongSafetyCheck",0);
    config.setOption("LiberalSafety",1);
    config.setOption("IncludeAuxInputInAuxiliaries",0);
    config.setOption("DumpEvaluationPlan",0);
    config.setOption("DumpStats",0);
                                 // perhaps only temporary
    config.setOption("BenchmarkEAstderr",0);
                                 // perhaps only temporary
    config.setOption("ExplicitFLPUnshift",0);
                                 // perhaps only temporary
    config.setOption("PrintLearnedNogoods",0);
    // frumpy is the name of the failsafe clasp config option
    config.setStringOption("ClaspConfiguration","frumpy");
    config.setOption("ClaspIncrementalInterpretationExtraction",1);
    config.setOption("ClaspSingletonLoopNogoods",0);
    config.setOption("ClaspInverseLiterals", 0);
    // propagate at least once per second, but also propagate all 10000 times we can propagate
    // TODO we should experiment with these
    config.setOption("ClaspDeferNPropagations", 10000);
    config.setOption("ClaspDeferMaxTMilliseconds",1000);
                                 // if 1, model generators will not register propagators for external atoms
    config.setOption("NoPropagator", 0);
                                 // see --help
    config.setOption("UseConstantSpace", 0);
    config.setOption("ClaspForceSingleThreaded", 0);
    config.setOption("LazyUFSCheckerInitialization", 0);
    config.setOption("SupportSets", 0);
    config.setOption("ExternalSourceInlining", 0);
    config.setOption("ForceGC", 0);
    config.setStringOption("PluginDirs", "");
    config.setOption("IncrementalGrounding", 0);
    config.setOption("MinimizationSize", 10000);

    // options related to WeakConstraintPlugin (we need to support this in the core for efficiency)
                                 // whether we handle answer set weights
    config.setOption("Optimization", 0);
    // whether we first find the optimum and then enumerate all optimal answer sets
    // this setting is used by the model builders:
    // if it is 0, enumeration finds models of same quality or better (the safe option)
    //   (clasp MinimizeMode_t::Mode::optimize and currentOptimum is decreased by 1)
    // if it is 1, enumeration must find a better model
    //   (clasp MinimizeMode_t::Mode::optimize and currentOptimum is used as it is)
    // if it is 2, enumeration finds all models of equal quality
    //   (clasp MinimizeMode_t::Mode::enumOpt and currentOptimum is used as it is)
    // (if we use two step optimization, we use it with 1 to find the optimum and with 2 to enumerate optimal models)
    config.setOption("OptimizationTwoStep", 0);
    config.setOption("OptimizationByDlvhex", 0);
    config.setOption("OptimizationByBackend", 0);
                                 // if 1 then we only show optimal results, otherwise before getting optimal results we might get nonoptimal ones
    config.setOption("OptimizationFilterNonOptimal", 1);

    config.setStringOption("DumpEANogoods", "");
    config.setOption("MinimizeNogoods", 0);
    config.setOption("MinimizeNogoodsOpt", 0);
    config.setOption("MinimizeNogoodsOnConflict", 0);
    // forces all external atoms to be treated as inner
    config.setOption("NoOuterExternalAtoms", 0);
    config.setOption("TransUnitLearning", 0);
    config.setOption("ExternalAtomVerificationFromLearnedNogoods", 0);
    config.setOption("WaitOnModel", 0);


    WARNING("TODO cleanup the setASPSoftware vs nGenuineSolver thing")
    // but if we have genuinegc, take genuinegc as default
    #if defined(HAVE_LIBGRINGO) && defined(HAVE_LIBCLASP)
    config.setOption("GenuineSolver", 4);
    #endif
}


ProgramCtx::~ProgramCtx()
{
    DBGLOG(DBG,"resetting custom model generator provider");
    if (!!customModelGeneratorProvider) customModelGeneratorProvider.reset();

    DBGLOG(DBG,"resetting state");
    state.reset();

    DBGLOG(DBG,"resetting callbacks");
    modelCallbacks.clear();
    finalCallbacks.clear();

    DBGLOG(DBG,"resetting modelBuilder");
    modelBuilder.reset();

    DBGLOG(DBG,"resetting parser");
    parser.reset();

    DBGLOG(DBG,"resetting evalgraph");
    evalgraph.reset();

    DBGLOG(DBG,"resetting compgraph");
    compgraph.reset();

    DBGLOG(DBG,"resetting depgraph");
    depgraph.reset();

    DBGLOG(DBG,"resetting edbList");
    std::vector<InterpretationPtr>::iterator it = edbList.begin();
    while ( it != edbList.end() ) {
        it->reset();
        it++;
    }
    edb.reset();

    DBGLOG(DBG,"resetting inputProvider");
    inputProvider.reset();

    DBGLOG(DBG,"resetting aspsoftware");
    aspsoftware.reset();

    DBGLOG(DBG,"resetting pluginData");
    pluginData.clear();

    DBGLOG(DBG,"resetting pluginEnvironment");
    pluginEnvironment.clear();

    DBGLOG(DBG,"resetting registry, usage count was " << _registry.use_count() << " (it should be 2)");
    // not printing, it creates too much clutter
    //if( Logger::Instance().shallPrint(Logger::DBG) )
    //	_registry->print(Logger::Instance().stream()) << std::endl;
    _registry.reset();

    DBGLOG(DBG,"resetting pluginAtoms");
    pluginAtoms.clear();

    DBGLOG(DBG,"resetting pluginContainer, usage count was " << _pluginContainer.use_count() << " (it should be 1)");
    _pluginContainer.reset();
}


void
ProgramCtx::changeState(const boost::shared_ptr<State>& s)
{
    state = s;
}


// cannot change registry if something is already stored here
void ProgramCtx::setupRegistry(
RegistryPtr registry)
{
    assert(
        (
        !_registry ||            // allow to set from nothing
                                 // allow to change if empty
        (idb.empty() && !edb && idbList.size()==0 && edbList.size()==0 && pluginAtoms.empty())
        )
        &&
        "cannot change registry once idb or edb or pluginAtoms contains data");
    _registry = registry;
    _registry->setupAuxiliaryGroundAtomMask();
}


void ProgramCtx::changeRegistry(RegistryPtr registry)
{
    // clear everything that depends on IDs of registry
    idb.clear();
    edb.reset();
    idbList.clear();
    edbList.clear();
    pluginAtoms.clear();

    // setup new registry
    setupRegistry(registry);

    // re-add plugin atoms (using new registry)
    addPluginAtomsFromPluginContainer();
}


void ProgramCtx::setupPluginContainer(
PluginContainerPtr pluginContainer)
{
    assert(
        (
        !_pluginContainer ||     // allow to set if unset
        pluginAtoms.empty()      // allow to change if no atoms stored
        )
        &&
        "cannot change pluginContainer once pluginAtoms are used");
    _pluginContainer = pluginContainer;
    WARNING("here we could reset the pointers in all ExternalAtoms if we unset the pluginContainer")
}


ASPSolverManager::SoftwareConfigurationPtr
ProgramCtx::getASPSoftware() const
{
    assert(aspsoftware != 0);
    return aspsoftware;
}


void ProgramCtx::setASPSoftware(ASPSolverManager::SoftwareConfigurationPtr software)
{
    aspsoftware = software;
}


void ProgramCtx::showPlugins() { state->showPlugins(this); }
void ProgramCtx::convert() { state->convert(this); }
void ProgramCtx::parse() { state->parse(this); }
void ProgramCtx::moduleSyntaxCheck() { state->moduleSyntaxCheck(this); }
void ProgramCtx::mlpSolver() { state->mlpSolver(this); }
void ProgramCtx::rewriteEDBIDB() { state->rewriteEDBIDB(this); }
void ProgramCtx::safetyCheck() { state->safetyCheck(this); }
void ProgramCtx::createDependencyGraph() { state->createDependencyGraph(this); }
void ProgramCtx::liberalSafetyCheck() { state->checkLiberalSafety(this); }
void ProgramCtx::optimizeEDBDependencyGraph() { state->optimizeEDBDependencyGraph(this); }
void ProgramCtx::createComponentGraph() { state->createComponentGraph(this); }
void ProgramCtx::strongSafetyCheck() { state->strongSafetyCheck(this); }
void ProgramCtx::createEvalGraph() { state->createEvalGraph(this); }
void ProgramCtx::setupProgramCtx() { state->setupProgramCtx(this); }
void ProgramCtx::evaluate() { state->evaluate(this); }
void ProgramCtx::postProcess() { state->postProcess(this); }

// ============================== subprogram handling ==============================

bool ProgramCtx::SubprogramAnswerSetCallback::operator()(AnswerSetPtr model)
{
    answersets.push_back(model->interpretation);
    return true;
}


ProgramCtx::SubprogramAnswerSetCallback::~SubprogramAnswerSetCallback(){}

std::vector<InterpretationPtr> ProgramCtx::evaluateSubprogram(InterpretationConstPtr edb, std::vector<ID>& idb)
{

    ProgramCtx pc = *this;
    pc.idb = idb;
    pc.edb = InterpretationPtr(new Interpretation(*edb));
    pc.currentOptimum.clear();
    pc.currentOptimumRelevantLevels = 0;
    return evaluateSubprogram(pc, false);
}


std::vector<InterpretationPtr> ProgramCtx::evaluateSubprogram(InputProviderPtr& ip, InterpretationConstPtr addFacts)
{

    ProgramCtx pc = *this;
    pc.idb.clear();
    pc.edb = InterpretationPtr(new Interpretation(this->registry()));
    pc.currentOptimum.clear();
    pc.currentOptimumRelevantLevels = 0;
    pc.config.setOption("NumberOfModels",0);
    if( !!addFacts )
        pc.edb->getStorage() |= addFacts->getStorage();
    pc.inputProvider = ip;
    ip.reset();

    return evaluateSubprogram(pc, true);
}


std::vector<InterpretationPtr> ProgramCtx::evaluateSubprogram(ProgramCtx& pc, bool parse)
{

    DBGLOG(DBG, "Resetting context");
    pc.state.reset();
    pc.modelBuilder.reset();
    pc.parser.reset();
    pc.evalgraph.reset();
    pc.compgraph.reset();
    pc.depgraph.reset();

    pc.config.setOption("DumpDepGraph",0);
    pc.config.setOption("DumpCyclicPredicateInputAnalysisGraph",0);
    pc.config.setOption("DumpCompGraph",0);
    pc.config.setOption("DumpEvalGraph",0);
    pc.config.setOption("DumpModelGraph",0);
    pc.config.setOption("DumpIModelGraph",0);
    pc.config.setOption("DumpAttrGraph",0);

    if( !pc.evalHeuristic ) {
        assert(false);
        throw GeneralError("No evaluation heuristics found");
    }

    DBGLOG(DBG, "Starting state pipeline " << (parse ? "with" : "without") << " parsing");
    if (parse) {
        pc.changeState(StatePtr(new ConvertState));
    }
    else {
        pc.changeState(StatePtr(new RewriteEDBIDBState));
    }

    if (parse) {
        pc.convert();
        if( pc.terminationRequest ) throw GeneralError("Conversion of subprogram failed");
        pc.parse();
        if( pc.terminationRequest ) throw GeneralError("Parsing of subprogram failed");
    }

    DBGLOG(DBG, "Associate PluginAtom instances with ExternalAtom instances");
    pc.associateExtAtomsWithPluginAtoms(pc.idb, true);
    if( pc.terminationRequest ) throw GeneralError("associateExtAtomsWithPluginAtoms(1) for subprogram failed");
    pc.rewriteEDBIDB();
    if( pc.terminationRequest ) throw GeneralError("rewrite EDBIDB for subprogram failed");
    pc.associateExtAtomsWithPluginAtoms(pc.idb, true);
    if( pc.terminationRequest ) throw GeneralError("associateExtAtomsWithPluginAtoms(2) for subprogram failed");
    pc.safetyCheck();
    if( pc.terminationRequest ) throw GeneralError("Safety check for subprogram failed");
    pc.liberalSafetyCheck();
    if( pc.terminationRequest ) throw GeneralError("Liberal safety check for subprogram failed");
    pc.createDependencyGraph();
    if( pc.terminationRequest ) throw GeneralError("Create dependency graph for subprogram failed");
    pc.optimizeEDBDependencyGraph();
    if( pc.terminationRequest ) throw GeneralError("Optimize EDB dependency graph for subprogram failed");
    pc.createComponentGraph();
    if( pc.terminationRequest ) throw GeneralError("Create component graph for subprogram failed");
    // use SCCs to do strong safety check
    if( !pc.config.getOption("SkipStrongSafetyCheck") ) {
        pc.strongSafetyCheck();
        if( pc.terminationRequest ) GeneralError("Strong safety check for subprogram failed");
    }
    pc.createEvalGraph();
    if( pc.terminationRequest ) throw GeneralError("Create evaluation graph for subprogram failed");
    pc.setupProgramCtx();
    if( pc.terminationRequest ) throw GeneralError("Setup ProgramCtx for subprogram failed");

    DBGLOG(DBG, "Setting AnswerSetCallback");
    pc.modelCallbacks.clear();
    pc.finalCallbacks.clear();
    SubprogramAnswerSetCallback* spasc = new SubprogramAnswerSetCallback();
    ModelCallbackPtr spascp = ModelCallbackPtr(spasc);
    pc.modelCallbacks.push_back(spascp);

    DBGLOG(DBG, "Evaluate subprogram");
    pc.evaluate();
    std::vector<InterpretationPtr> result;
    BOOST_FOREACH (InterpretationPtr intr, spasc->answersets) {
        result.push_back(intr);
    }

    return result;
}


// ============================== end subprogram handling ==============================

#if 0
PluginAtomPtr
PluginContainer::getAtom(const std::string& name) const
{
    PluginAtomMap::const_iterator pa = pluginAtoms.find(name);

    if (pa == pluginAtoms.end())
        return PluginAtomPtr();

    return pa->second;
}
#endif

void ProgramCtx::addPluginAtom(PluginAtomPtr atom)
{
    assert(!!atom);
    assert(!!_registry);
    const std::string& predicate = atom->getPredicate();
    LOG(PLUGIN,"adding PluginAtom '" << predicate << "'");
    if( pluginAtoms.find(predicate) == pluginAtoms.end() ) {
        atom->setRegistry(_registry);
        pluginAtoms[predicate] = atom;
    }
    else {
        LOG(WARNING,"External atom " << predicate << " is already loaded (skipping)");
    }
}


// call processOptions for each loaded plugin
// (this is supposed to remove "recognized" options from pluginOptions)
void ProgramCtx::processPluginOptions(
std::list<const char*>& pluginOptions)
{
    BOOST_FOREACH(PluginInterfacePtr plugin, pluginContainer()->getPlugins()) {
        LOG(DBG,"processing options for plugin " << plugin->getPluginName());
        LOG(DBG,"currently have " << printrange(pluginOptions));
        plugin->processOptions(pluginOptions, *this);
    }
}


void ProgramCtx::addPluginAtomsFromPluginContainer()
{
    assert(!!pluginContainer());
    assert(!!registry());

    BOOST_FOREACH(PluginInterfacePtr plugin, pluginContainer()->getPlugins()) {
        LOG(DBG,"adding plugin atoms from plugin " << plugin->getPluginName());
        // always freshly create! (pluginatoms are linked to a registry,
        // so when using multiple registries, you have to create multiple pluginatoms)
        BOOST_FOREACH(PluginAtomPtr pap, plugin->createAtoms(*this)) {
            assert(!!pap);
            const std::string& pred = pap->getPredicate();
            LOG(DBG,"  got plugin atom " << pred);
            if( pluginAtoms.count(pred) != 0 ) {
                LOG(WARNING,"warning: predicate '" << pred << "' already present in PluginAtomMap (skipping)");
            }
            else {
                pap->setRegistry(registry());
                pluginAtoms[pred] = pap;
            }
        }
    }
}


// associate plugins in container to external atoms in registry
void ProgramCtx::associateExtAtomsWithPluginAtoms(
const Tuple& idb, bool failOnUnknownAtom)
{
    assert(!!_registry);
    DBGLOG_SCOPE(DBG,"aEAwPA",false);
    DBGLOG(DBG,"= associateExtAtomsWithPluginAtoms");

    Tuple eatoms;

    // associate all rules
    for(Tuple::const_iterator it = idb.begin();
    it != idb.end(); ++it) {
        assert(it->isRule());
        // skip those without external atoms
        if( !it->doesRuleContainExtatoms() )
            continue;

        // associate all literals in rule body
        const Rule& rule = _registry->rules.getByID(*it);

        // get external atoms (recursively)
        _registry->getExternalAtomsInTuple(rule.body, eatoms);
    }

    // now associate
    for(Tuple::const_iterator it = eatoms.begin();
    it != eatoms.end(); ++it) {
        assert(it->isExternalAtom());

        const ExternalAtom& eatom = _registry->eatoms.getByID(*it);

        const std::string& predicate = _registry->getTermStringByID(eatom.predicate);
        // lookup pluginAtom to this eatom predicate
        PluginAtomMap::iterator itpa = pluginAtoms.find(predicate);
        if( itpa != pluginAtoms.end() ) {
            assert(!!itpa->second);
            // we store this as a POD pointer!
            eatom.pluginAtom = itpa->second.get();
            eatom.prop |= itpa->second->getExtSourceProperties();
            eatom.pluginAtom->setupProperties(eatom);
            if (!eatom.pluginAtom->checkOutputArity(eatom.getExtSourceProperties(), eatom.tuple.size())) {
                std::stringstream ss;
                ss << "External Atom " << RawPrinter::toString(_registry, *it) << " has a wrong output arity (should be " << eatom.pluginAtom->getOutputArity() << ")";
                throw GeneralError(ss.str());
            }
        }
        else {
            DBGLOG(DBG,"did not find plugin atom for predicate '" << predicate << "'");
            if( failOnUnknownAtom ) {
                throw FatalError("did not find plugin atom "
                    " for predicate '" + predicate + "'");
            }
        }
    }
}


// setup this ProgramCtx (using setupProgramCtx() for of all plugins)
void ProgramCtx::setupByPlugins()
{
    BOOST_FOREACH(PluginInterfacePtr plugin, pluginContainer()->getPlugins()) {
        LOG(DBG,"setting up program ctx for plugin " << plugin->getPluginName());
        plugin->setupProgramCtx(*this);
    }
}


// reset the cache of Plugins that use Environment
void ProgramCtx::resetCacheOfPlugins(bool resetOnlyIfUsesEnvironment)
{
    typedef std::pair<std::string, PluginAtomPtr> pairPluginAtomMap;
    BOOST_FOREACH(pairPluginAtomMap p, pluginAtoms)
        if( !resetOnlyIfUsesEnvironment ||
        p.second->getExtSourceProperties().doesItUseEnvironment())
        p.second->resetCache();
}


DLVHEX_NAMESPACE_END


// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:


