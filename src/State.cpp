/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
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
 * @author Peter Schüller
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
#  include "config.h"
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
  std::ostream& printStatePtr(std::ostream& o, StatePtr ptr)
  {
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
  if( !ctx->config.getOption("Silent") )
  {
    BOOST_FOREACH(PluginInterfacePtr plugin, ctx->pluginContainer()->getPlugins())
    {
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
  BOOST_FOREACH(const std::string& name, ctx->inputProvider->contentNames())
  {
    // only use part after last / here
    inputName += "_" + name.substr(name.find_last_of("/") + 1);
  }
  LOG(INFO,"inputName='" << inputName << "'");

  // store it
  ctx->config.setStringOption("DebugPrefix","dbg" + inputName);
  LOG(DBG,"debugFilePrefix='" << ctx->config.getStringOption("DebugPrefix") << "'");

  std::vector<PluginConverterPtr> converters;
  BOOST_FOREACH(PluginInterfacePtr plugin, ctx->pluginContainer()->getPlugins())
  {
    BOOST_FOREACH(PluginConverterPtr pc, plugin->createConverters(*ctx))
    {
      LOG(PLUGIN,"got plugin converter from plugin " << plugin->getPluginName());
      converters.push_back(pc);
    }
  }

  if( converters.size() > 1 )
    LOG(WARNING,"got more than one plugin converter, using arbitrary order!");

  BOOST_FOREACH(PluginConverterPtr converter, converters)
  {
    DBGLOG(DBG,"calling input converter");
    std::stringstream out;
    converter->convert(ctx->inputProvider->getAsStream(), out);

    // debug output (if requested)
    if( ctx->config.doVerbose(Configuration::DUMP_CONVERTED_PROGRAM) )
    {
      LOG(DBG,"input conversion result:" << std::endl << out.str() << std::endl);
    }

    // replace input provider with converted input provider
    ctx->inputProvider.reset(new InputProvider);
    ctx->inputProvider->addStringInput(out.str(), "converted" + inputName);
	}

#warning TODO realize dlt as a plugin
// 	  //
// 	  // now call dlt if needed
// 	  //
// 	  if (optiondlt)
// 	    {
// 	      char tempfile[L_tmpnam];
// 	      mkstemp(tempfile);

// 	      std::ofstream dlttemp(tempfile);

// 	      //
// 	      // write program into tempfile
// 	      //
// 	      dlttemp << input.rdbuf();

// 	      dlttemp.close();

// 	      std::string execPreParser("dlt -silent -preparsing " + std::string(tempfile));

// 	      fp = popen(execPreParser.c_str(), "r");

// 	      if (fp == NULL)
// 		{
// 		  throw GeneralError("Unable to call Preparser dlt");
// 		}

// 	      __gnu_cxx::stdio_filebuf<char>* fb;
// 	      fb = new __gnu_cxx::stdio_filebuf<char>(fp, std::ios::in);

// 	      std::istream inpipe(fb);

// 	      //
// 	      // now we have a program rewriten by dlt - since it should
// 	      // be in the stream "input", we have to delete the old
// 	      // input-buffer and set input to the buffer from the
// 	      // dlt-call
// 	      //
// 	      delete input.rdbuf();
// 	      input.rdbuf(fb);
// 	    }

  boost::shared_ptr<State> next(new ParseState);
  changeState(ctx, next);
}

MANDATORY_STATE_CONSTRUCTOR(ParseState);

void ParseState::parse(ProgramCtx* ctx)
{
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"Parsing input");

  // use alternative parser from plugins, if applicable
  assert(!ctx->parser);
  BOOST_FOREACH(PluginInterfacePtr plugin, ctx->pluginContainer()->getPlugins())
  {
    HexParserPtr alternativeParser = plugin->createParser(*ctx);
    if( !!alternativeParser )
    {
      if( !!ctx->parser )
      {
        LOG(WARNING,"ignoring alternative parser provided by plugin " <<
            plugin->getPluginName() << " because parser already initialized");
      }
      else
      {
        LOG(INFO,"using alternative parser provided by plugin " <<
            plugin->getPluginName());
        ctx->parser = alternativeParser;
      }
    }
  }

  // use default parser if no alternative parsers given
  if( !ctx->parser )
  {
    LOG(INFO,"using default parser (no alternatives provided by plugins)");
    ctx->parser.reset(new ModuleHexParser);
  }
  
  // configure parser modules if possible
  {
    ModuleHexParserPtr mhp =
      boost::dynamic_pointer_cast<ModuleHexParser>(ctx->parser);
    BOOST_FOREACH(PluginInterfacePtr plugin, ctx->pluginContainer()->getPlugins())
    {
      std::vector<HexParserModulePtr> modules =
        plugin->createParserModules(*ctx);
      if( !modules.empty() )
      {
        if( !!mhp )
        {
          LOG(INFO,"got " << modules.size() <<
              " parser modules from plugin " << plugin->getPluginName());
          BOOST_FOREACH(HexParserModulePtr module, modules)
          {
            mhp->registerModule(module);
          }
          LOG(INFO,"registered successfully");
        }
        else
        {
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

	#warning namespaces were here!
#if 0

///@brief predicate returns true iff argument is not alpha-numeric and
///is not one of {_,-,.} characters, i.e., it returns true if
///characater does not belong to XML's NCNameChar character class.
struct NotNCNameChar : public std::unary_function<char, bool>
{
  bool
  operator() (char c)
  {
    c = std::toupper(c);
    return
      (c < 'A' || c > 'Z') &&
      (c < '0' || c > '9') &&
      c != '-' &&
      c != '_' &&
      c != '.';
  }
};
#endif

// TODO implement namespaces
#if 0
void
insertNamespaces()
{
  ///@todo move this stuff to Term, this has nothing to do here!

  if (Term::getNameSpaces().empty())
    return;

  std::string prefix;

  for (NamesTable<std::string>::const_iterator nm = Term::getNames().begin();
       nm != Term::getNames().end();
       ++nm)
    {
      for (std::vector<std::pair<std::string, std::string> >::iterator ns = Term::getNameSpaces().begin();
	   ns != Term::getNameSpaces().end();
	   ++ns)
	{
	  prefix = ns->second + ':';

	  //
	  // prefix must occur either at beginning or right after quote
	  //
	  unsigned start = 0;
	  unsigned end = (*nm).length();

	  if ((*nm)[0] == '"')
	    {
	      ++start;
	      --end;
	    }

	    
	  //
	  // accourding to http://www.w3.org/TR/REC-xml-names/ QNames
	  // consist of a prefix followed by ':' and a LocalPart, or
	  // just a LocalPart. In case of a single LocalPart, we would
	  // not find prefix and leave that Term alone. If we find a
	  // prefix in the Term, we must disallow non-NCNames in
	  // LocalPart, otw. we get in serious troubles when replacing
	  // proper Terms:
	  // NameChar ::= Letter | Digit | '.' | '-' | '_' | ':' | CombiningChar | Extender  
	  //

	  std::string::size_type colon = (*nm).find(":", start);
					  
	  if (colon != std::string::npos) // Prefix:LocalPart
	    {
	      std::string::const_iterator it =
		std::find_if((*nm).begin() + colon + 1, (*nm).begin() + end - 1, NotNCNameChar());

	      // prefix starts with ns->second, LocalPart does not
	      // contain non-NCNameChars, hence we can replace that
	      // Term
	      if ((*nm).find(prefix, start) == start &&
		  (it == (*nm).begin() + end - 1)
		  )
		{
		  std::string r(*nm);
	      
		  r.replace(start, prefix.length(), ns->first); // replace ns->first from start to prefix + 1
		  r.replace(0, 1, "\"<");
		  r.replace(r.length() - 1, 1, ">\"");
	      
		  Term::getNames().modify(nm, r);
		}
	    }
	}
    }
}

void
removeNamespaces()
{
  ///@todo move this stuff to Term, this has nothing to do here!

  if (Term::getNameSpaces().empty())
    return;

  std::string prefix;
  std::string fullns;

  for (NamesTable<std::string>::const_iterator nm = Term::getNames().begin();
       nm != Term::getNames().end();
       ++nm)
    {
      for (std::vector<std::pair<std::string, std::string> >::iterator ns = Term::getNameSpaces().begin();
	   ns != Term::getNameSpaces().end();
	   ++ns)
	{
	  fullns = ns->first;

	  prefix = ns->second + ":";

	  //
	  // original ns must occur either at beginning or right after quote
	  //
	  unsigned start = 0;
	  if ((*nm)[0] == '"')
	    start = 1;

	  if ((*nm).find(fullns, start) == start)
	    {
	      std::string r(*nm);

	      r.replace(start, fullns.length(), prefix);

	      Term::getNames().modify(nm, r);
	    }
	}
    }
}
#endif

	// be verbose if requested
	if( ctx->config.doVerbose(Configuration::DUMP_PARSED_PROGRAM) )
	{
    LOG(INFO,"parsed IDB:");
    RawPrinter rp(Logger::Instance().stream(), ctx->registry());
	  rp.printmany(ctx->idb, "\n");
    Logger::Instance().stream() << std::endl;

    LOG(INFO,"parsed EDB:");
    Logger::Instance().stream() << *(ctx->edb) << std::endl;
	}
  if( ctx->config.getOption("MLP") ) 
    {
      StatePtr next(new ModuleSyntaxCheckState);
      changeState(ctx, next);
    }
  else 
    {
      StatePtr next(new RewriteEDBIDBState);
      changeState(ctx, next);
    }
}

MANDATORY_STATE_CONSTRUCTOR(ModuleSyntaxCheckState);
// MLPSyntaxChecker ..
void ModuleSyntaxCheckState::moduleSyntaxCheck(ProgramCtx* ctx)
{
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"Module Syntax Check");
  MLPSyntaxChecker sC(*ctx);
  bool success = sC.verifySyntax();
  if (success)
    {
      StatePtr next(new MLPSolverState);
      changeState(ctx, next);
    }
  else
    {
      std::cout << "Does not solve the MLP because of syntax error" << std::endl;
      StatePtr next(new PostProcessState);
      changeState(ctx, next);
    }
}

MANDATORY_STATE_CONSTRUCTOR(MLPSolverState);
void MLPSolverState::mlpSolver(ProgramCtx* ctx)
{
  MLPSolver m(*ctx);
  m.setNASReturned(ctx->config.getOption("NumberOfModels"));
  m.setPrintLevel(ctx->config.getOption("Verbose"));
  m.setForget(ctx->config.getOption("Forget"));
  m.setInstSplitting(ctx->config.getOption("Split"));
  m.solve();
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
  BOOST_FOREACH(PluginInterfacePtr plugin, ctx->pluginContainer()->getPlugins())
  {
    PluginRewriterPtr rewriter = plugin->createRewriter(*ctx);
    if( !rewriter )
      continue;

    LOG(PLUGIN,"got plugin rewriter from plugin " << plugin->getPluginName());

	  rewriter->rewrite(*ctx);

    // be verbose if requested
    if( ctx->config.doVerbose(Configuration::DUMP_REWRITTEN_PROGRAM) )
    {
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

OPTIONAL_STATE_CONSTRUCTOR(SafetyCheckState,CreateDependencyGraphState);

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

  StatePtr next(new CreateDependencyGraphState);
  changeState(ctx, next);
}

MANDATORY_STATE_CONSTRUCTOR(CreateDependencyGraphState);

void CreateDependencyGraphState::createDependencyGraph(ProgramCtx* ctx)
{
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"building dependency graph");

  DependencyGraphPtr depgraph(new DependencyGraph(ctx->registry()));
  std::vector<dlvhex::ID> auxRules;
  depgraph->createDependencies(ctx->idb, auxRules);

  if( ctx->config.getOption("DumpDepGraph") )
  {
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
  BOOST_FOREACH(PluginInterfacePtr plugin, ctx->pluginContainer()->getPlugins())
  {
    PluginOptimizerPtr optimizer = plugin->createOptimizer(*ctx);
    if( !optimizer )
      continue;

    LOG(PLUGIN,"got plugin optimizer from plugin " << plugin->getPluginName());

	  optimizer->optimize(ctx->edb, ctx->depgraph);
    optimized = true;
  }

  if( optimized && ctx->config.getOption("DumpDepGraph") )
  {
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
      new ComponentGraph(*ctx->depgraph, ctx->registry()));

  if( ctx->config.getOption("DumpCompGraph") )
  {
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
	if( ctx->config.getOption("DumpEvaluationPlan") )
	{
		egbuilder.reset(new DumpingEvalGraphBuilder(
					*ctx, *ctx->compgraph, *evalgraph, ctx->aspsoftware,
					ctx->config.getStringOption("DumpEvaluationPlanFile")));
	}
	else
	{
		egbuilder.reset(new EvalGraphBuilder(
					*ctx, *ctx->compgraph, *evalgraph, ctx->aspsoftware));
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
  #warning TODO if we project answer sets, or do querying, we could reduce the number of units used here!
  FinalEvalGraph::EvalUnit ufinal =
    evalgraph->addUnit(FinalEvalGraph::EvalUnitPropertyBundle());
  LOG(DBG,"created virtual final unit ufinal = " << ufinal);

  FinalEvalGraph::EvalUnitIterator it, itend;
  boost::tie(it, itend) = evalgraph->getEvalUnits();
  for(; it != itend && *it != ufinal; ++it)
  {
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

  if( ctx->config.getOption("DumpEvalGraph") )
  {
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

  // default model outputting callback
  ModelCallbackPtr asprinter(new AnswerSetPrinterCallback(*ctx));
  ctx->modelCallbacks.push_back(asprinter);

  // setup printing of auxiliaries
  if( 1 == ctx->config.getOption("KeepAuxiliaryPredicates") )
  {
    AuxPrinterPtr plainAuxPrinter(new PlainAuxPrinter(ctx->registry()));
    ctx->registry()->registerUserDefaultAuxPrinter(plainAuxPrinter);
  }

  // let plugins setup the program ctx (removing the default hooks is permitted)
  ctx->setupByPlugins();

  StatePtr next(new EvaluateState);
  changeState(ctx, next);
}

MANDATORY_STATE_CONSTRUCTOR(EvaluateState);

void
EvaluateState::evaluate(ProgramCtx* ctx)
{
  typedef ModelBuilder<FinalEvalGraph>::Model Model;
  typedef ModelBuilder<FinalEvalGraph>::OptionalModel OptionalModel;
  typedef ModelBuilder<FinalEvalGraph>::MyModelGraph MyModelGraph;

  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"evaluate()");

  do
  {
    LOG(INFO,"creating model builder");
    {
      DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidmb, "create model builder");
      ctx->modelBuilder = ModelBuilderPtr(ctx->modelBuilderFactory(*ctx->evalgraph));
    }
    ModelBuilder<FinalEvalGraph>& mb = *ctx->modelBuilder;

    // get model and call all callbacks
    // abort if one callback returns false
    DLVHEX_BENCHMARK_REGISTER(sidgetnextmodel, "evaluate::get next model");
    unsigned mcount = 0;
    bool abort = false;
    bool gotModel;
    unsigned mcountLimit = ctx->config.getOption("NumberOfModels");
    std::vector<AnswerSetPtr> bestModels;	// model cache for weight minimization
    do
    {
      gotModel = false;
      DBGLOG(DBG,"requesting imodel");
      DLVHEX_BENCHMARK_START(sidgetnextmodel);
      OptionalModel om = mb.getNextIModel(ctx->ufinal);
      DLVHEX_BENCHMARK_STOP(sidgetnextmodel);
      if( !!om )
      {
        Model m = om.get();
        InterpretationConstPtr interpretation =
          mb.getModelGraph().propsOf(m).interpretation;

        // if the program is empty, we may get a NULL interpretation
        if( !interpretation )
        {
          assert(mb.getModelGraph().propsOf(m).dummy == true);
          interpretation.reset(new Interpretation(ctx->registry()));
        }

        if( ctx->config.getOption("DumpIModelGraph") )
        {
          throw std::runtime_error("DumpIModelGraph  not implemented!");
          #warning TODO individual eval/model graphviz output
        }
        #ifndef NDEBUG
        DBGLOG(DBG,"got model#" << mcount << ":" << *interpretation);
        /*
        std::set<Model> onlyFor;
        onlyFor.insert(m.get());
        GraphVizFunc func = boost::bind(&writeEgMgGraphViz<MyModelGraph>, _1,
            true, boost::cref(mb.getEvalGraph()), boost::cref(mb.getModelGraph()), onlyFor);
        std::stringstream smodel;
        smodel << fname << "PlainHEXOnlineModel" << mcount;
        writeGraphVizFunctors(func, func, smodel.str());
        */
        #endif

        // model callbacks
        AnswerSetPtr answerset(new AnswerSet(ctx->registry()));
        // copy interpretation! (callbacks can modify it)
        answerset->interpretation->getStorage() = interpretation->getStorage();
        answerset->computeWeightVector();

        // add EDB if configured that way
        if( !ctx->config.getOption("NoFacts") )
        {
          answerset->interpretation->getStorage() |=
            ctx->edb->getStorage();
        }

        // cost check
        // compare the solution to the best known model
        gotModel = true;
        if (ctx->currentOptimum.size() == 0 || answerset->betterThan(ctx->currentOptimum))
        {
          ctx->currentOptimum = answerset->getWeightVector();
#ifndef NDEBUG
          std::stringstream ss;
          answerset->printWeightVector(ss);
          DBGLOG(DBG, "New global optimum: " << ss.str());
#endif

          if (ctx->onlyBestModels)
          {
            // cache models and decide at the end upon optimality

            // is the new model better than the best known one?
            if (bestModels.size() > 0 && !bestModels[0]->betterThan(answerset->getWeightVector()))
            {
                    bestModels.clear();
                    mcount = 0;
            }
            bestModels.push_back(answerset);
          }
          else
          {
            // process models directly
            BOOST_FOREACH(ModelCallbackPtr mcb, ctx->modelCallbacks)
            {
              bool aborthere = !(*mcb)(answerset);
              abort |= aborthere;
              if( aborthere )
                LOG(DBG,"callback '" << typeid(*mcb).name() << "' signalled abort at model " << mcount);
            }
          }
          mcount++;

          #ifndef NDEBUG
          //mb.printEvalGraphModelGraph(std::cerr);
          #endif
          if( mcountLimit != 0 && mcount >= mcountLimit )
          {
            LOG(INFO,"breaking model enumeration loop because already enumerated " << mcount << " models!");
            break;
          }
        }
      }
    }
    while( gotModel && !abort );

    // process cached models
    if (ctx->onlyBestModels){
      BOOST_FOREACH (AnswerSetPtr answerset, bestModels){
        BOOST_FOREACH(ModelCallbackPtr mcb, ctx->modelCallbacks)
        {
          bool aborthere = !(*mcb)(answerset);
          abort |= aborthere;
          if( aborthere )
            LOG(DBG,"callback '" << typeid(*mcb).name() << "' signalled abort at model " << mcount);
        }
      }
    }

    LOG(INFO,"got " << mcount << " models");
    if( abort )
    {
      LOG(INFO,"model building was aborted by callback");
    }
    else
    {
      if( mcountLimit == 0 )
      {
        LOG(INFO,"model building finished after enumerating all models");
      }
      else
      {
        LOG(INFO,"model building finished after enumerating " << mcountLimit << " models");
      }
    }

    if( ctx->config.getOption("DumpModelGraph") )
    {
      throw std::runtime_error("DumpModelGraph  not implemented!");
      #warning TODO overall eval/model graphviz output
    }

    // call final callbacks
    BOOST_FOREACH(FinalCallbackPtr fcb, ctx->finalCallbacks)
    {
      DBGLOG(DBG,"calling final callback " << printptr(fcb));
      (*fcb)();
    }

    // if repetition counter is set, decrease it and repeat
    unsigned repeatEvaluation = ctx->config.getOption("RepeatEvaluation");
    if( repeatEvaluation > 0 )
    {
      LOG(INFO,"repeating evaluation because RepeatEvaluation=" << repeatEvaluation);
      ctx->config.setOption("RepeatEvaluation", repeatEvaluation-1);
      continue;
    }

    // default: do not repeat
    break;
  }
  while(true);

  #if 0
  #ifndef NDEBUG
  mb.printEvalGraphModelGraph(std::cerr);
  #endif
  #ifndef NDEBUG
  GraphVizFunc func = boost::bind(&writeEgMgGraphViz<MyModelGraph>, _1,
      true, boost::cref(mb.getEvalGraph()), boost::cref(mb.getModelGraph()), boost::none);
  writeGraphVizFunctors(func, func, fname+"PlainHEXOnlineEgMg");
  #endif
  #endif
  //std::cerr << __FILE__ << ":" << __LINE__ << std::endl << *ctx.registry() << std::endl;


  #if 0

  //
  // We don't have a depedency graph, so just dump the program to an
  // OrdinaryModelGenerator and see what happens
  //

  std::vector<AtomSet> models;

  OrdinaryModelGenerator omg(*ctx);

  //
  // The GraphProcessor starts its computation with the program's ground
  // facts as input.
  // But only if the original EDB is consistent, otherwise, we can skip it
  // anyway.
  //
  if (ctx->getEDB()->isConsistent())
    {
      omg.compute(*ctx->getIDB(), *ctx->getEDB(), models);
    }

  ///@todo weak contraint prefixes are a bit clumsy here. How can we do better?

  //
  // prepare result container
  //
  // if we had any weak constraints, we have to tell the result container the
  // prefix in order to be able to compute each asnwer set's costs!
  //
  std::string wcprefix;

  if (ctx->getIDB()->getWeakConstraints().size() > 0)
    {
      wcprefix = "wch__";
    }

  ResultContainer* result = new ResultContainer(wcprefix);
  ctx->setResultContainer(result);

  //
  // put GraphProcessor result into ResultContainer
  ///@todo we can do better, for sure
  //
  for (std::vector<AtomSet>::const_iterator it = models.begin();
       it != models.end(); ++it)
    {
      ctx->getResultContainer()->addSet(*it);
    }

  #endif

  #warning TODO dlt was here
  ///@todo quick hack for dlt
  //   if (optiondlt)
  //     {
  //       ctx->getResultContainer()->filterOutDLT();
  //     }


  #if 0
    std::cerr << "TIMING " << fname << " " << heurimode << " " << mbmode << " " << backend << " " <<
      ctx->evalgraph.countEvalUnits() << " evalunits " << ctx->evalgraph.countEvalUnitDeps() << " evalunitdeps " << mcount << " models ";
    benchmark::BenchmarkController::Instance().printDuration(std::cerr, sidoverall) << std::endl;
  #endif

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

  if( ctx->config.getOption("BenchmarkEAstderr") == 1 )
  {
    benchmark::BenchmarkController& bmc = benchmark::BenchmarkController::Instance();
    //benchmark::ID eeval = bmc.getInstrumentationID("evaluate external atom");
    benchmark::ID eeval = bmc.getInstrumentationID("PluginAtom retrieve");
    const benchmark::BenchmarkController::Stat& stat = bmc.getStat(eeval);
    std::cerr << stat.count << " ";
    bmc.printInSecs(std::cerr, stat.duration, 3);
    std::cerr << std::endl;
  }
}


DLVHEX_NAMESPACE_END

// vim:ts=4:

// Local Variables:
// mode: C++
// End:
