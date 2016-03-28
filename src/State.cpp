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
 * @file State.cpp
 * @author Thomas Krennwallner
 * @author Peter Schller
 * @date
 *
 * @brief State class.
 *
 *
 *
 */

#include "dlvhex2/State.h"

// activate benchmarking if activated by configure option --enable-debug
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Error.h"
#include "dlvhex2/Printhelpers.h"
#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/ASPSolverManager.h"
#include "dlvhex2/ASPSolver.h"
#include "dlvhex2/HexParser.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/PluginContainer.h"
#include "dlvhex2/LiberalSafetyChecker.h"
#include "dlvhex2/DependencyGraph.h"
#include "dlvhex2/ComponentGraph.h"
#include "dlvhex2/FinalEvalGraph.h"
#include "dlvhex2/EvalGraphBuilder.h"
#include "dlvhex2/DumpingEvalGraphBuilder.h"
#include "dlvhex2/AnswerSetPrinterCallback.h"
#include "dlvhex2/PlainAuxPrinter.h"
#include "dlvhex2/SafetyChecker.h"
#include "dlvhex2/MLPSyntaxChecker.h"
#include "dlvhex2/MLPSolver.h"

#include <boost/foreach.hpp>

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

DLVHEX_NAMESPACE_BEGIN

State::State(StatePtr failureState):
failureState(failureState)
{
}


State::~State()
{
}


namespace
{
    std::ostream& printStatePtr(std::ostream& o, StatePtr ptr) {
        if( !ptr )
            return o << "NULL";
        else
            return o << "'" << typeid(*ptr).name() << "'";
    }
}


void State::changeState(ProgramCtx* ctx, StatePtr s)
{
    LOG(INFO,
        "State::changeState from " <<
        print_function(boost::bind(&printStatePtr, _1, ctx->state)) <<
        " to " <<
        print_function(boost::bind(&printStatePtr, _1, s)));
    ctx->changeState(s);
}


// each of these functions skips to the "failureState" and executes the executed function on it
// this is useful for having optional states
// if no failureState is given, an exception is raised
// this is useful for non-optional states
#define STATE_FUNC_DEFAULT_IMPL(function) \
    void State:: function (ProgramCtx* ctx) \
    { \
        if( !!failureState ) \
        { \
            changeState(ctx, failureState); /* <-- this destructs *this */ \
            ctx->state-> function (ctx); \
        } \
        else \
        { \
            throw std::runtime_error("tried to skip execution of '" \
            #function "' in State!"); \
        } \
    }

        // all state methods get skipping possibility
        // derived classes will decide whether to set the failureState or not
        // if it is set, the state is skippable,
        // if not, execution of this state is mandatory
        STATE_FUNC_DEFAULT_IMPL(showPlugins);
STATE_FUNC_DEFAULT_IMPL(convert);
STATE_FUNC_DEFAULT_IMPL(parse);
STATE_FUNC_DEFAULT_IMPL(moduleSyntaxCheck);
STATE_FUNC_DEFAULT_IMPL(mlpSolver);
STATE_FUNC_DEFAULT_IMPL(rewriteEDBIDB);
STATE_FUNC_DEFAULT_IMPL(safetyCheck);
STATE_FUNC_DEFAULT_IMPL(checkLiberalSafety);
STATE_FUNC_DEFAULT_IMPL(createDependencyGraph);
STATE_FUNC_DEFAULT_IMPL(optimizeEDBDependencyGraph);
STATE_FUNC_DEFAULT_IMPL(createComponentGraph);
STATE_FUNC_DEFAULT_IMPL(strongSafetyCheck);
STATE_FUNC_DEFAULT_IMPL(createEvalGraph);
STATE_FUNC_DEFAULT_IMPL(setupProgramCtx);
STATE_FUNC_DEFAULT_IMPL(evaluate);
STATE_FUNC_DEFAULT_IMPL(postProcess);

#define MANDATORY_STATE_CONSTRUCTOR(state) \
    state :: state (): State() {}

#define OPTIONAL_STATE_CONSTRUCTOR(state,skiptostate) \
    state :: state (): State(StatePtr(new skiptostate)) {}

OPTIONAL_STATE_CONSTRUCTOR(ShowPluginsState,ConvertState);

void ShowPluginsState::showPlugins(ProgramCtx* ctx)
{
    if( !ctx->config.getOption("Silent") ) {
        BOOST_FOREACH(PluginInterfacePtr plugin, ctx->pluginContainer()->getPlugins()) {
            LOG(INFO,"opening plugin " << plugin->getPluginName() <<
                " version " <<
                plugin->getVersionMajor() << "." <<
                plugin->getVersionMinor() << "." <<
                plugin->getVersionMicro());
        }
    }

    boost::shared_ptr<State> next(new ConvertState);
    changeState(ctx, next);
}


OPTIONAL_STATE_CONSTRUCTOR(ConvertState,ParseState);

void ConvertState::convert(ProgramCtx* ctx)
{
    assert(!!ctx->inputProvider && ctx->inputProvider->hasContent() && "need input provider with content for converting");

    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"Calling plugin converters");

    // get combination of input filenames for creating debug output files and for naming converted input
    std::string inputName;
    BOOST_FOREACH(const std::string& name, ctx->inputProvider->contentNames()) {
        // only use part after last / here
        inputName += "_" + name.substr(name.find_last_of("/") + 1);
    }
    LOG(INFO,"inputName='" << inputName << "'");

    // store it
    ctx->config.setStringOption("DebugPrefix","dbg" + inputName);
    LOG(DBG,"debugFilePrefix='" << ctx->config.getStringOption("DebugPrefix") << "'");

    std::vector<PluginConverterPtr> converters;
    BOOST_FOREACH(PluginInterfacePtr plugin, ctx->pluginContainer()->getPlugins()) {
        BOOST_FOREACH(PluginConverterPtr pc, plugin->createConverters(*ctx)) {
            LOG(PLUGIN,"got plugin converter from plugin " << plugin->getPluginName());
            converters.push_back(pc);
        }
    }

    if( converters.size() > 1 )
        LOG(WARNING,"got more than one plugin converter, using arbitrary order!");

    BOOST_FOREACH(PluginConverterPtr converter, converters) {
        DBGLOG(DBG,"calling input converter");
        std::stringstream out;
        converter->convert(ctx->inputProvider->getAsStream(), out);

        // debug output (if requested)
        if( ctx->config.doVerbose(Configuration::DUMP_CONVERTED_PROGRAM) ) {
            LOG(DBG,"input conversion result:" << std::endl << out.str() << std::endl);
        }

        // replace input provider with converted input provider
        ctx->inputProvider.reset(new InputProvider);
        ctx->inputProvider->addStringInput(out.str(), "converted" + inputName);
    }

    WARNING("TODO realize dlt as a preparser plugin")

        boost::shared_ptr<State> next(new ParseState);
    changeState(ctx, next);
}


MANDATORY_STATE_CONSTRUCTOR(ParseState);

void ParseState::parse(ProgramCtx* ctx)
{
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"Parsing input");

    // use alternative parser from plugins, if applicable
    assert(!ctx->parser);
    BOOST_FOREACH(PluginInterfacePtr plugin, ctx->pluginContainer()->getPlugins()) {
        HexParserPtr alternativeParser = plugin->createParser(*ctx);
        if( !!alternativeParser ) {
            if( !!ctx->parser ) {
                LOG(WARNING,"ignoring alternative parser provided by plugin " <<
                    plugin->getPluginName() << " because parser already initialized");
            }
            else {
                LOG(INFO,"using alternative parser provided by plugin " <<
                    plugin->getPluginName());
                ctx->parser = alternativeParser;
            }
        }
    }

    // use default parser if no alternative parsers given
    if( !ctx->parser ) {
        LOG(INFO,"using default parser (no alternatives provided by plugins)");
        ctx->parser.reset(new ModuleHexParser);
    }

    // configure parser modules if possible
    {
        ModuleHexParserPtr mhp =
            boost::dynamic_pointer_cast<ModuleHexParser>(ctx->parser);
        BOOST_FOREACH(PluginInterfacePtr plugin, ctx->pluginContainer()->getPlugins()) {
            std::vector<HexParserModulePtr> modules =
                plugin->createParserModules(*ctx);
            if( !modules.empty() ) {
                if( !!mhp ) {
                    LOG(INFO,"got " << modules.size() <<
                        " parser modules from plugin " << plugin->getPluginName());
                    BOOST_FOREACH(HexParserModulePtr module, modules) {
                        mhp->registerModule(module);
                    }
                    LOG(INFO,"registered successfully");
                }
                else {
                    LOG(WARNING,"ignoring parser module from plugin '" <<
                        plugin->getPluginName() << "' as ModuleHexParser is not used");
                }
            }
        }
    }

    // parse
    assert(!!ctx->parser);
    ctx->parser->parse(ctx->inputProvider, *ctx);

    // free input provider memory
    assert(ctx->inputProvider.use_count() == 1);
    ctx->inputProvider.reset();

    WARNING("namespaces were here! (perhaps we should forget namespaces. in the best case implement in a rewriter plugin)")

    // be verbose if requested
        if( ctx->config.doVerbose(Configuration::DUMP_PARSED_PROGRAM) &&
    Logger::Instance().shallPrint(Logger::INFO) ) {
        LOG(INFO,"parsed IDB:");
        RawPrinter rp(Logger::Instance().stream(), ctx->registry());
        rp.printmany(ctx->idb, "\n");
        Logger::Instance().stream() << std::endl;

        LOG(INFO,"parsed EDB:");
        Logger::Instance().stream() << *(ctx->edb) << std::endl;
    }
    if( ctx->config.getOption("MLP") ) {
        StatePtr next(new ModuleSyntaxCheckState);
        changeState(ctx, next);
    }
    else {
        StatePtr next(new RewriteEDBIDBState);
        changeState(ctx, next);
    }
}


MANDATORY_STATE_CONSTRUCTOR(ModuleSyntaxCheckState);
// MLPSyntaxChecker ..
void ModuleSyntaxCheckState::moduleSyntaxCheck(ProgramCtx* ctx)
{
    #ifdef HAVE_MLP
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"Module Syntax Check");
    MLPSyntaxChecker sC(*ctx);
    bool success = sC.verifySyntax();
    #else
    bool success = true;
    #endif
    if (success) {
        StatePtr next(new MLPSolverState);
        changeState(ctx, next);
    }
    else {
        std::cout << "Does not solve the MLP because of syntax error" << std::endl;
        StatePtr next(new PostProcessState);
        changeState(ctx, next);
    }
}


MANDATORY_STATE_CONSTRUCTOR(MLPSolverState);
void MLPSolverState::mlpSolver(ProgramCtx* ctx)
{
    #ifdef HAVE_MLP
    MLPSolver m(*ctx);
    m.setNASReturned(ctx->config.getOption("NumberOfModels"));
    m.setPrintLevel(ctx->config.getOption("Verbose"));
    m.setForget(ctx->config.getOption("Forget"));
    m.setInstSplitting(ctx->config.getOption("Split"));
    m.solve();
    #endif
    StatePtr next(new PostProcessState);
    changeState(ctx, next);
}


OPTIONAL_STATE_CONSTRUCTOR(RewriteEDBIDBState,SafetyCheckState);

void
RewriteEDBIDBState::rewriteEDBIDB(ProgramCtx* ctx)
{
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"Calling plugin rewriters");
    DBGLOG_SCOPE(DBG,"rewrite",false);

    // get rewriter from each plugin
    BOOST_FOREACH(PluginInterfacePtr plugin, ctx->pluginContainer()->getPlugins()) {
        PluginRewriterPtr rewriter = plugin->createRewriter(*ctx);
        if( !rewriter )
            continue;

        LOG(PLUGIN,"got plugin rewriter from plugin " << plugin->getPluginName());

        rewriter->rewrite(*ctx);

        // be verbose if requested
        if( ctx->config.doVerbose(Configuration::DUMP_REWRITTEN_PROGRAM) &&
        Logger::Instance().shallPrint(Logger::INFO) ) {
            LOG(INFO,"rewritten IDB:");
            RawPrinter rp(Logger::Instance().stream(), ctx->registry());
            rp.printmany(ctx->idb, "\n");
            Logger::Instance().stream() << std::endl;

            LOG(INFO,"rewritten EDB:");
            Logger::Instance().stream() << *(ctx->edb) << std::endl;
        }
    }

    StatePtr next(new SafetyCheckState);
    changeState(ctx, next);
}


OPTIONAL_STATE_CONSTRUCTOR(SafetyCheckState,CheckLiberalSafetyState);

void
SafetyCheckState::safetyCheck(ProgramCtx* ctx)
{
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"Safety checking");

    //
    // Performing the safety check
    //
    SafetyChecker schecker(*ctx);
    // check by calling the object
    schecker();

    StatePtr next(new CheckLiberalSafetyState);
    changeState(ctx, next);
}


MANDATORY_STATE_CONSTRUCTOR(CheckLiberalSafetyState);

void CheckLiberalSafetyState::checkLiberalSafety(ProgramCtx* ctx)
{
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"checking liberal safety");

    ctx->liberalSafetyChecker = LiberalSafetyCheckerPtr(new LiberalSafetyChecker(ctx->registry(), ctx->idb, ctx->liberalSafetyPlugins));

    if( ctx->config.getOption("DumpAttrGraph") ) {
        std::string fnamev = ctx->config.getStringOption("DebugPrefix")+"_AttrGraphVerbose.dot";
        LOG(INFO,"dumping verbose attribute graph to " << fnamev);
        std::ofstream filev(fnamev.c_str());
        ctx->liberalSafetyChecker->writeGraphViz(filev, true);
    }

    if (ctx->config.getOption("LiberalSafety")) {
        if (!ctx->liberalSafetyChecker->isDomainExpansionSafe()) {
            throw SyntaxError("Program is not liberally domain-expansion safe");
        }
    }

    StatePtr next(new CreateDependencyGraphState);
    changeState(ctx, next);
}


MANDATORY_STATE_CONSTRUCTOR(CreateDependencyGraphState);

void CreateDependencyGraphState::createDependencyGraph(ProgramCtx* ctx)
{
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"building dependency graph");

    DependencyGraphPtr depgraph(new DependencyGraph(*ctx, ctx->registry()));
    std::vector<dlvhex::ID> auxRules;
    depgraph->createDependencies(ctx->idb, auxRules);

    if( ctx->config.getOption("DumpDepGraph") ) {
        std::string fnamev = ctx->config.getStringOption("DebugPrefix")+"_DepGraphVerbose.dot";
        LOG(INFO,"dumping verbose dependency graph to " << fnamev);
        std::ofstream filev(fnamev.c_str());
        depgraph->writeGraphViz(filev, true);

        std::string fnamet = ctx->config.getStringOption("DebugPrefix")+"_DepGraphTerse.dot";
        LOG(INFO,"dumping terse dependency graph to " << fnamet);
        std::ofstream filet(fnamet.c_str());
        depgraph->writeGraphViz(filet, false);
    }

    ctx->depgraph = depgraph;

    StatePtr next(new OptimizeEDBDependencyGraphState);
    changeState(ctx, next);
}


OPTIONAL_STATE_CONSTRUCTOR(OptimizeEDBDependencyGraphState,CreateComponentGraphState);

void
OptimizeEDBDependencyGraphState::optimizeEDBDependencyGraph(ProgramCtx* ctx)
{
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"Calling plugin optimizers");

    // get optimizer from each plugin
    bool optimized = false;
    BOOST_FOREACH(PluginInterfacePtr plugin, ctx->pluginContainer()->getPlugins()) {
        PluginOptimizerPtr optimizer = plugin->createOptimizer(*ctx);
        if( !optimizer )
            continue;

        LOG(PLUGIN,"got plugin optimizer from plugin " << plugin->getPluginName());

        optimizer->optimize(ctx->edb, ctx->depgraph);
        optimized = true;
    }

    if( optimized && ctx->config.getOption("DumpDepGraph") ) {
        std::string fnamev = ctx->config.getStringOption("DebugPrefix")+
            "_DepGraphOptimizedVerbose.dot";
        LOG(INFO,"dumping optimized verbose dependency graph to " << fnamev);
        std::ofstream filev(fnamev.c_str());
        ctx->depgraph->writeGraphViz(filev, true);

        std::string fnamet = ctx->config.getStringOption("DebugPrefix")+
            "_DepGraphOptimizedTerse.dot";
        LOG(INFO,"dumping optimized terse dependency graph to " << fnamet);
        std::ofstream filet(fnamet.c_str());
        ctx->depgraph->writeGraphViz(filet, false);
    }

    StatePtr next(new CreateComponentGraphState);
    changeState(ctx, next);
}


MANDATORY_STATE_CONSTRUCTOR(CreateComponentGraphState);

void CreateComponentGraphState::createComponentGraph(ProgramCtx* ctx)
{
    assert(!!ctx->depgraph && "need depgraph for building component graph");
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"building component graph");

    ComponentGraphPtr compgraph(
        new ComponentGraph(*ctx->depgraph, *ctx, ctx->registry()));

    if( ctx->config.getOption("DumpCompGraph") ) {
        std::string fnamev = ctx->config.getStringOption("DebugPrefix")+"_CompGraphVerbose.dot";
        LOG(INFO,"dumping verbose component graph to " << fnamev);
        std::ofstream filev(fnamev.c_str());
        compgraph->writeGraphViz(filev, true);

        std::string fnamet = ctx->config.getStringOption("DebugPrefix")+"_CompGraphTerse.dot";
        LOG(INFO,"dumping terse component graph to " << fnamet);
        std::ofstream filet(fnamet.c_str());
        compgraph->writeGraphViz(filet, false);
    }

    ctx->compgraph = compgraph;

    StatePtr next(new StrongSafetyCheckState);
    changeState(ctx, next);
}


OPTIONAL_STATE_CONSTRUCTOR(StrongSafetyCheckState,CreateEvalGraphState);

void StrongSafetyCheckState::strongSafetyCheck(ProgramCtx* ctx)
{
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"Strong safety checking");

    StrongSafetyChecker sschecker(*ctx);
    // check by calling the object
    sschecker();

    StatePtr next(new CreateEvalGraphState);
    changeState(ctx, next);
}


MANDATORY_STATE_CONSTRUCTOR(CreateEvalGraphState);

void CreateEvalGraphState::createEvalGraph(ProgramCtx* ctx)
{
    assert(!!ctx->compgraph &&
        "need component graph for creating evaluation graph");
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"creating evaluation graph");

    FinalEvalGraphPtr evalgraph(new FinalEvalGraph);

    EvalGraphBuilderPtr egbuilder;
    if( ctx->config.getOption("DumpEvaluationPlan") ) {
        egbuilder.reset(new DumpingEvalGraphBuilder(
            *ctx, *ctx->compgraph, *evalgraph, ctx->aspsoftware,
            ctx->config.getStringOption("DumpEvaluationPlanFile")));
    }
    else {
        egbuilder.reset(new EvalGraphBuilder(
            *ctx, *ctx->compgraph, *evalgraph, ctx->aspsoftware));
    }

    // dump component graph again, this time the cloned version
    // (it has different addresses which we might need for debugging)
    if( ctx->config.getOption("DumpCompGraph") ) {
        std::string fnamev = ctx->config.getStringOption("DebugPrefix")+"_ClonedCompGraphVerbose.dot";
        LOG(INFO,"dumping verbose cloned component graph to " << fnamev);
        std::ofstream filev(fnamev.c_str());
        egbuilder->getComponentGraph().writeGraphViz(filev, true);

        std::string fnamet = ctx->config.getStringOption("DebugPrefix")+"_ClonedCompGraphTerse.dot";
        LOG(INFO,"dumping terse cloned component graph to " << fnamet);
        std::ofstream filet(fnamet.c_str());
        egbuilder->getComponentGraph().writeGraphViz(filet, false);
    }

    // use configured eval heuristic
    assert(!!ctx->evalHeuristic && "need configured heuristic");
    DBGLOG(DBG,"invoking build() on eval heuristic");
    ctx->evalHeuristic->build(*egbuilder);
    // do not destruct heuristic because we might reuse it in evaluateSubprogram
    //DBGLOG(DBG,"destructing eval heuristic");
    //ctx->evalHeuristic.reset();
    // destruct eval graph builder
    egbuilder.reset();

    // setup final unit used to get full models
    WARNING("TODO if we project answer sets, or do querying, we could reduce the number of units used here!")
        FinalEvalGraph::EvalUnit ufinal =
        evalgraph->addUnit(FinalEvalGraph::EvalUnitPropertyBundle());
    LOG(DBG,"created virtual final unit ufinal = " << ufinal);

    FinalEvalGraph::EvalUnitIterator it, itend;
    boost::tie(it, itend) = evalgraph->getEvalUnits();
    for(; it != itend && *it != ufinal; ++it) {
        DBGLOG(DBG,"adding dependency from ufinal to unit " << *it <<
            " join order " << *it);
        // we can do this because we know that eval units
        // (= vertices of a vecS adjacency list) are unsigned integers
        evalgraph->addDependency(
            ufinal, *it,
            FinalEvalGraph::EvalUnitDepPropertyBundle(*it));
    }

    ctx->ufinal = ufinal;
    ctx->evalgraph = evalgraph;

    if( ctx->config.getOption("DumpEvalGraph") ) {
        std::string fnamev = ctx->config.getStringOption("DebugPrefix")+"_EvalGraphVerbose.dot";
        LOG(INFO,"dumping verbose eval graph to " << fnamev);
        std::ofstream filev(fnamev.c_str());
        ctx->evalgraph->writeGraphViz(filev, true);

        std::string fnamet = ctx->config.getStringOption("DebugPrefix")+"_EvalGraphTerse.dot";
        LOG(INFO,"dumping terse eval graph to " << fnamet);
        std::ofstream filet(fnamet.c_str());
        ctx->evalgraph->writeGraphViz(filet, false);
    }

    StatePtr next(new SetupProgramCtxState);
    changeState(ctx, next);
}


MANDATORY_STATE_CONSTRUCTOR(SetupProgramCtxState);

void SetupProgramCtxState::setupProgramCtx(ProgramCtx* ctx)
{
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"setupProgramCtx");

    // what to snapshot at first model
    ctx->benchmarksToSnapshotAtFirstModel.insert(std::make_pair("BenchmarkController lifetime", "time to first model"));
    ctx->benchmarksToSnapshotAtFirstModel.insert(std::make_pair("Grounder time", "Grounder time to first model"));
    ctx->benchmarksToSnapshotAtFirstModel.insert(std::make_pair("Solver time", "Solver time to first model"));
    ctx->benchmarksToSnapshotAtFirstModel.insert(std::make_pair("HEX grounder time", "HEX grounder time to first mdl"));
    ctx->benchmarksToSnapshotAtFirstModel.insert(std::make_pair("HEX solver time", "HEX solver time to first model"));
    ctx->benchmarksToSnapshotAtFirstModel.insert(std::make_pair("PluginAtom retrieve", "PluginAtom retr to first model"));
    ctx->benchmarksToSnapshotAtFirstModel.insert(std::make_pair("Candidate compatible sets", "CandCompat sets to first model"));

    // default model outputting callback
    if (ctx->modelCallbacks.size() == 0){
        ModelCallbackPtr asprinter(new AnswerSetPrinterCallback(*ctx));
        ctx->modelCallbacks.push_back(asprinter);
    }

    // setup printing of auxiliaries
    if( 1 == ctx->config.getOption("KeepAuxiliaryPredicates") ) {
        AuxPrinterPtr plainAuxPrinter(new PlainAuxPrinter(ctx->registry()));
        ctx->registry()->registerUserDefaultAuxPrinter(plainAuxPrinter);
    }

    // let plugins setup the program ctx (removing the default hooks is permitted)
    ctx->setupByPlugins();

    StatePtr next(new EvaluateState);
    changeState(ctx, next);
}


MANDATORY_STATE_CONSTRUCTOR(EvaluateState);

namespace
{
    typedef ModelBuilder<FinalEvalGraph>::Model Model;
    typedef ModelBuilder<FinalEvalGraph>::OptionalModel OptionalModel;
    typedef ModelBuilder<FinalEvalGraph>::MyModelGraph MyModelGraph;

    void snapShotBenchmarking(ProgramCtx& ctx) {
        static bool alreadyDidIt = false;

        // do this really only once in the lifetime of a dlvhex binary
        if( alreadyDidIt ) return;
        alreadyDidIt = true;

        std::map<std::string, std::string>::const_iterator snapit;
        for(snapit = ctx.benchmarksToSnapshotAtFirstModel.begin();
        snapit != ctx.benchmarksToSnapshotAtFirstModel.end(); ++snapit) {
            dlvhex::benchmark::BenchmarkController::Instance().snapshot(snapit->first, snapit->second);
        }
    }

    ModelBuilder<FinalEvalGraph>& createModelBuilder(ProgramCtx* ctx) {
        LOG(INFO,"creating model builder");
        {
            DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidmb, "create model builder");
            ModelBuilderConfig<FinalEvalGraph> cfg(*ctx->evalgraph);
            cfg.redundancyElimination = true;
            cfg.constantSpace = ctx->config.getOption("UseConstantSpace") == 1;
            ctx->modelBuilder = ModelBuilderPtr(ctx->modelBuilderFactory(cfg));
        }
        return *ctx->modelBuilder;
    }

    bool callModelCallbacks(ProgramCtx* ctx, AnswerSetPtr answerset) {
        // process all answer sets via callback mechanism
        // processing a model this way gives it as a result, so we snapshot the first model here
        snapShotBenchmarking(*ctx);

        bool abort = false;
        BOOST_FOREACH(ModelCallbackPtr mcb, ctx->modelCallbacks) {
            bool aborthere = !(*mcb)(answerset);
            abort |= aborthere;
            if( aborthere )
                LOG(DBG,"callback '" << typeid(*mcb).name() << "' signalled abort");
        }
        return abort;
    }

    // evaluate the hex program to find the optimum
    // (this will only be used for OptimizationTwoStep because in other cases it might not yield correct results)
    // * enumerate models better than current cost
    // * ignore model limits/callbacks
    // * remember last found model and its cost
    // * when not finding model, set current cost to last found model
    // returns the first optimal answer set or NULL if there is no answer set
    AnswerSetPtr evaluateFindOptimum(ProgramCtx* ctx) {

        DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "evaluateFindOptimum");
        DLVHEX_BENCHMARK_REGISTER(sidgetnextmodel, "evaluateFindOptimum::gNM");
        DBGLOG_SCOPE(DBG,"eFO",false);
        DBGLOG(DBG,"eFO = evaluateFindOptimum");

        assert(ctx->config.getOption("OptimizationTwoStep") == 1);
        AnswerSetPtr lastAnswerSet;
        ModelBuilder<FinalEvalGraph>& mb = createModelBuilder(ctx);
        OptionalModel om;
        do {
            DBGLOG(DBG,"requesting omodel");
            {
                DLVHEX_BENCHMARK_SCOPE(sidgetnextmodel);
                om = mb.getNextIModel(ctx->ufinal);
            }
            if( !!om ) {
                Model m = om.get();
                InterpretationConstPtr interpretation =
                    mb.getModelGraph().propsOf(m).interpretation;

                // if the program is empty, we may get a NULL interpretation
                if( !interpretation ) {
                    assert(mb.getModelGraph().propsOf(m).dummy == true);
                    interpretation.reset(new Interpretation(ctx->registry()));
                }

                AnswerSetPtr answerset(new AnswerSet(ctx->registry()));
                // copy interpretation! (we and callbacks might modify it after returning from this method)
                answerset->interpretation->getStorage() = interpretation->getStorage();
                answerset->computeWeightVector();
                LOG(INFO, "new global best weight vector: " << printvector(answerset->getWeightVector()) << ", old best: " << printvector(ctx->currentOptimum));
                assert(ctx->currentOptimum.empty() || answerset->strictlyBetterThan(ctx->currentOptimum));
                ctx->currentOptimum = answerset->getWeightVector();
                // if we have at least one weight we need to complete the vector
                // in order to obtain bounds for all levels
                // (if we do not do this, clasp will not set a boud if we find a cost-free model)
                // TODO set currentOptimumRelevantLevels not in ClaspSolver but in WeakPlugin during rewriting (should be possible!)
                while (ctx->currentOptimum.size() < (ctx->currentOptimumRelevantLevels+1))
                    ctx->currentOptimum.push_back(0);
                lastAnswerSet = answerset;
            }
            // exit if we get no model
            // if we get a model with zero cost, the next iteration will set 0 as bound in clasp, so no further model will be found
        } while( !!om );
        // we got no model so we left the loop:
        // * either there never was any model with any weight
        // * or we got models and found the optimum (ctx->currentOptimum) and lastAnswerSet is the first optimal one
        // our caller will handle these cases
        DBGLOG(DBG,"returning answer set " << reinterpret_cast<void*>(lastAnswerSet.get()));
        return lastAnswerSet;
    }

    // evaluate the hex program
    // * enumerate models
    // * honor model limits
    // * including model callbacks
    // * including final callbacks
    void evaluateOnce(ProgramCtx* ctx) {

        DLVHEX_BENCHMARK_REGISTER(sidgetnextmodel, "evaluate::get next model");
        DBGLOG_SCOPE(DBG,"eO",false);
        DBGLOG(DBG,"eO = evaluateOnce");

        // this implementation requires that there is no optimization OR
        // that the optimal cost has been found and set and that we use two-stop optimization mode
        assert(ctx->config.getOption("Optimization") == 0 ||
            (!ctx->currentOptimum.empty() && ctx->config.getOption("OptimizationTwoStep") == 2));

        ModelBuilder<FinalEvalGraph>& mb = createModelBuilder(ctx);
        unsigned mcount = 0;
        bool abort = false;
        const unsigned mcountLimit = ctx->config.getOption("NumberOfModels");
        OptionalModel om;
        do {
            DBGLOG(DBG,"requesting imodel");
            {
                DLVHEX_BENCHMARK_SCOPE(sidgetnextmodel);
                om = mb.getNextIModel(ctx->ufinal);
            }
            if( !!om ) {
                Model m = om.get();
                InterpretationConstPtr interpretation =
                    mb.getModelGraph().propsOf(m).interpretation;

                // if the program is empty, we may get a NULL interpretation
                if( !interpretation ) {
                    assert(mb.getModelGraph().propsOf(m).dummy == true);
                    interpretation.reset(new Interpretation(ctx->registry()));
                }

                DBGLOG(DBG,"got model#" << mcount << ":" << *interpretation);

                // model callbacks
                AnswerSetPtr answerset(new AnswerSet(ctx->registry()));
                // copy interpretation! (we and callbacks might modify it)
                answerset->interpretation->getStorage() = interpretation->getStorage();
                answerset->computeWeightVector();
                LOG(DBG, "weight vector of this answer set: " << printvector(answerset->getWeightVector()));
                // TODO this assertion should be done, but only if optimizing and perhaps even then we might have vector length difference problems
                //assert( ctx->currentOptimum == answerset->getWeightVector() );

                // add EDB if configured that way
                if( !ctx->config.getOption("NoFacts") )
                    answerset->interpretation->getStorage() |= ctx->edb->getStorage();

                abort |= callModelCallbacks(ctx, answerset);
                mcount++;
            }
        } while( !!om && !abort && (mcountLimit == 0 || mcount < mcountLimit) );

        LOG(INFO,"got " << mcount << " models");
        if( abort ) {
            LOG(INFO,"model building was aborted by callback");
        }
        else {
            if( mcountLimit == 0 ) {
                LOG(INFO,"model building finished after enumerating all models");
            }
            else {
                LOG(INFO,"model building finished after a maximum of " << mcountLimit << " models");
            }
        }
    }

    // evaluate the hex program using naive optimization
    // * enumerate all models of a certain cost or better
    //   store all models of the currently known best cost
    //   until no more models are found, then output
    // * during output:
    //   * honor model limits
    //   * call model callbacks
    // * then call  final callbacks
    void evaluateOnceExpspace(ProgramCtx* ctx) {

        DLVHEX_BENCHMARK_REGISTER(sidgetnextmodel, "evaluate::get next model");
        DBGLOG_SCOPE(DBG,"eOE",false);
        DBGLOG(DBG,"eOE = evaluateOnceExpspace");

        // this implementation should only be used for naive optimization
        assert(ctx->config.getOption("Optimization") == 1 &&
            ctx->config.getOption("OptimizationTwoStep") == 0 /*&&
            ctx->config.getOption("OptimizationByDlvhex") == 1*/);
        ModelBuilder<FinalEvalGraph>& mb = createModelBuilder(ctx);
        const unsigned mcountLimit = ctx->config.getOption("NumberOfModels");
        unsigned mcount = 0;
        bool abort = false;
        std::list<AnswerSetPtr> bestModels;
        OptionalModel om;
        do {
            DBGLOG(DBG,"requesting imodel");
            {
                DLVHEX_BENCHMARK_SCOPE(sidgetnextmodel);
                om = mb.getNextIModel(ctx->ufinal);
            }
            if( !!om ) {
                Model m = om.get();
                InterpretationConstPtr interpretation =
                    mb.getModelGraph().propsOf(m).interpretation;
                // if the program is empty, we may get a NULL interpretation
                if( !interpretation ) {
                    assert(mb.getModelGraph().propsOf(m).dummy == true);
                    interpretation.reset(new Interpretation(ctx->registry()));
                }

                DBGLOG(DBG,"got model#" << mcount << ":" << *interpretation);

                // model callbacks
                AnswerSetPtr answerset(new AnswerSet(ctx->registry()));
                // copy interpretation! (we and callbacks might modify it)
                answerset->interpretation->getStorage() = interpretation->getStorage();
                answerset->computeWeightVector();
                LOG(DBG, "weight vector of this answer set: " << printvector(answerset->getWeightVector()));

                // add EDB if configured that way
                if( !ctx->config.getOption("NoFacts") )
                    answerset->interpretation->getStorage() |= ctx->edb->getStorage();

                // cost check
                // compare the solution to the best known model
                // 3 Options:
                // - ctx->config.getOption("OptimizationByDlvhex"):
                //   Let dlvhex manage optimization. Setting this option to true suffices to get the correct result.
                // - ctx->config.getOption("OptimizationFilterNonOptimal"):
                //   Avoid that non-optimal models are printed before the best model appears; option is only relevant if "OptimizationByDlvhex" is also set.
                // - ctx->config.getOption("OptimizationByBackend"):
                //   Let solver backends manage optimization (if the specific backends supports it).
                //   This option is optional but might prune the search space already in single units while dlvhex can optimize only after the final models have been found.
                                 // betterThan does not necessarily mean strictly better, i.e., it includes solutions of the same quality!
                bool equalOrBetter = (ctx->currentOptimum.size() == 0 || answerset->betterThan(ctx->currentOptimum));

                // keep track of the current optimum
                if( equalOrBetter ) {
                    ctx->currentOptimum = answerset->getWeightVector();
                    LOG(DBG, "Current global optimum (equalOrBetter = True): " << printvector(answerset->getWeightVector()));
                }

                if (ctx->config.getOption("OptimizationByDlvhex")){
                    if( !equalOrBetter ) continue;

                    // in this block we do not need to count models as we need to enumerate all of them
                    // only afterwards the requested number of best models can be output

                    // is there a previous model and the new model is (strictly!) better than the best known one?
                    if( !bestModels.empty() && !bestModels.front()->betterThan(answerset->getWeightVector()) ) {
                        // new model is better than all previous ones --> clear cache
                        LOG(DBG, "clearing bestModels because new model is strictly better");
                        bestModels.clear();
                    }

                    // also show some non-optimal models?
                    if( ctx->config.getOption("OptimizationFilterNonOptimal") == 0 ) {
                        // yes: output model immediately
                        abort |= callModelCallbacks(ctx, answerset);
                        mcount++;
                    }else{
                        // store this one in cache and decide at the end upon optimality
                        LOG(DBG, "recording answer set in bestModels: " << *answerset);
                        bestModels.push_back(answerset);
                    }
                }else{
                    abort |= callModelCallbacks(ctx, answerset);
                    mcount++;
                }
                if (mcountLimit != 0 && mcount >= mcountLimit) abort = true;
            }
        }
        while( !!om && !abort );

        // process cached models
        BOOST_FOREACH(AnswerSetPtr answerset, bestModels) {
            mcount++;
            abort |= callModelCallbacks(ctx, answerset);
            // respect model count limit for cached models
            if( abort || (mcountLimit != 0 && mcount >= mcountLimit) )
                break;
        }

        LOG(INFO,"got " << mcount << " models");
        if( abort ) {
            LOG(INFO,"model building was aborted by callback");
        }
        else {
            if( mcountLimit == 0 ) {
                LOG(INFO,"model building finished after enumerating all models");
            }
            else {
                LOG(INFO,"model building finished after enumerating a maximum of " << mcountLimit << " models");
            }
        }
    }
}


void
EvaluateState::evaluate(ProgramCtx* ctx)
{
    do {
        if( ctx->config.getOption("Optimization") ) {
            if( ctx->config.getOption("OptimizationTwoStep") > 0 ) {
                // special optimization method (see dlvhex.cpp)
                AnswerSetPtr firstBest = evaluateFindOptimum(ctx);
                if( !!firstBest ) {
                    LOG(INFO,"first optimal answer set: " << *firstBest);
                    // enumerate all answer sets equal to previously found optimum
                    // TODO if we just want to find one answer set, do not call evaluateOnce but directly use firstBest
                    ctx->config.setOption("OptimizationTwoStep", 2);
                    evaluateOnce(ctx);
                }
            }
            else {
                evaluateOnceExpspace(ctx);
            }
        }
        else {
            // no optimization required
            evaluateOnce(ctx);
        }

        // call final callbacks
        BOOST_FOREACH(FinalCallbackPtr fcb, ctx->finalCallbacks) {
            DBGLOG(DBG,"calling final callback " << printptr(fcb));
            (*fcb)();
        }

        // TODO this repetition business should be solved in a more state-machine-ish way
        // if repetition counter is set, decrease it and repeat
        // this value might change in model/final callbacks, so we need to load it again here
        unsigned repeatEvaluation = ctx->config.getOption("RepeatEvaluation");
        if( repeatEvaluation > 0 ) {
            LOG(INFO,"repeating evaluation because RepeatEvaluation=" << repeatEvaluation);
            ctx->config.setOption("RepeatEvaluation", repeatEvaluation-1);
        } else
        break;
    } while(true);

    //mb.printEvalGraphModelGraph(std::cerr);

    StatePtr next(new PostProcessState);
    changeState(ctx, next);
}


MANDATORY_STATE_CONSTRUCTOR(PostProcessState);

void PostProcessState::postProcess(ProgramCtx* ctx)
{
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"postProcess");

    // cleanup some stuff that is better not automatically destructed
    DBGLOG(DBG,"usage count of model builder before reset is " <<
        ctx->modelBuilder.use_count());
    ctx->modelBuilder.reset();

    // use base State class with no failureState -> calling it will always throw an exception
    boost::shared_ptr<State> next(new State);
    changeState(ctx, next);

    if( ctx->config.getOption("BenchmarkEAstderr") == 1 ) {
        benchmark::BenchmarkController& bmc = benchmark::BenchmarkController::Instance();
        //benchmark::ID eeval = bmc.getInstrumentationID("evaluate external atom");
        benchmark::ID eeval = bmc.getInstrumentationID("PluginAtom retrieve");
        const benchmark::BenchmarkController::Stat& stat = bmc.getStat(eeval);
        std::cerr << stat.count << " ";
        bmc.printInSecs(std::cerr, stat.duration, 3);
        std::cerr << std::endl;
    }
    if( ctx->config.getOption("DumpStats") ) {
        // dump number of ground atoms, number of rules (in registry)
        // dump certain time stats
        benchmark::BenchmarkController& bmc = benchmark::BenchmarkController::Instance();
        unsigned noAtoms = 0;
        unsigned noRules = 0;
        if( ctx && ctx->registry() ) {
            noAtoms = ctx->registry()->ogatoms.getSize();
            noRules = ctx->registry()->rules.getSize();
        }

        const char* overallName = "BenchmarkController lifetime";
        benchmark::ID overall = bmc.getInstrumentationID(overallName);
        bmc.stop(overall);
        std::cerr << "STATS;ogatoms;" << noAtoms << ";rules;" << noRules;
        //std::cerr << ";plain_mg;" << bmc.duration("genuine plain mg construction", 3);
        //std::cerr << ";gnc_mg;" << bmc.duration("genuine g&c mg construction", 3);
        std::cerr << ";grounder;" << bmc.duration("Grounder time", 3);
        std::cerr << ";solver;" << bmc.duration("Solver time", 3);
        std::cerr << ";overall;" << bmc.duration(overallName, 3);
        std::cerr << std::endl;
    }
}


DLVHEX_NAMESPACE_END

// vim:tabstop=4:shiftwidth=4:noexpandtab:


// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
