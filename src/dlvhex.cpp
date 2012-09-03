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
 * 02110-1301 USA
 */


/**
 * @file   dlvhex.cpp
 * @author Roman Schindlauer
 * @date   Thu Apr 28 15:00:10 2005
 * 
 * @brief  main().
 * 
 */

/** @mainpage dlvhex Source Documentation
 *
 * \section intro_sec Overview
 *
 * You will look into the documentation of dlvhex most likely to implement a
 * plugin. In this case, please continue with the \ref pluginframework
 * "Plugin Interface Module", which contains all necessary information.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/Error.h"
#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/PluginContainer.h"
#include "dlvhex2/ASPSolverManager.h"
#include "dlvhex2/ASPSolver.h"
#include "dlvhex2/State.h"
#include "dlvhex2/EvalGraphBuilder.h"
#include "dlvhex2/EvalHeuristicBase.h"
#include "dlvhex2/EvalHeuristicASP.h"
#include "dlvhex2/EvalHeuristicOldDlvhex.h"
#include "dlvhex2/EvalHeuristicTrivial.h"
#include "dlvhex2/EvalHeuristicEasy.h"
#include "dlvhex2/EvalHeuristicGreedy.h"
#include "dlvhex2/EvalHeuristicFromFile.h"
#include "dlvhex2/ExternalAtomEvaluationHeuristics.h"
#include "dlvhex2/UnfoundedSetCheckHeuristics.h"
#include "dlvhex2/OnlineModelBuilder.h"
#include "dlvhex2/OfflineModelBuilder.h"

// internal plugins
#include "dlvhex2/QueryPlugin.h"
#include "dlvhex2/AggregatePlugin.h"
#include "dlvhex2/StrongNegationPlugin.h"
#include "dlvhex2/HigherOrderPlugin.h"
#include "dlvhex2/WeakConstraintPlugin.h"

#include <getopt.h>
#include <sys/types.h>
#include <pwd.h>

#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>
#include <sstream>
#include <cstring>

DLVHEX_NAMESPACE_USE

/**
 * @brief Print logo.
 */
void
printLogo()
{
	std::cout
		<< "DLVHEX  "
#ifdef HAVE_CONFIG_H
		<< VERSION << " "
#endif // HAVE_CONFIG_H
		<< "[build "
		<< __DATE__ 
#ifdef __GNUC__
		<< "   gcc " << __VERSION__ 
#endif
		<< "]" << std::endl
		<< std::endl;
}


/**
 * @brief Print usage help.
 */
void
printUsage(std::ostream &out, const char* whoAmI, bool full)
{
  //      123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
  out << "Usage: " << whoAmI 
      << " [OPTION] FILENAME [FILENAME ...]" << std::endl
      << std::endl;
  
  out << "   or: " << whoAmI 
      << " [OPTION] --" << std::endl
      << std::endl;
  
  if (!full)
    {
      out << "Specify -h or --help for more detailed usage information." << std::endl
	  << std::endl;
      
      return;
    }
  
  //
  // As soos as we have more options, we can introduce sections here!
  //
  //      123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
  out << "     --               Parse from stdin." << std::endl
//      << "     --instantiate    Generate ground program without evaluating (only useful with --genuinesolver)" << std::endl
      << "     --extlearn[=iobehavior,monotonicity,functionality,linearity,neg,user,generalize]" << std::endl
      << "                      Learn nogoods from external atom evaluation (only useful with --solver=genuineii or --solver=genuinegi)" << std::endl
      << "                        iobehavior: Apply generic rules to learn input-output behavior" << std::endl
      << "                        monotonicity: Apply special rules for monotonic and antimonotonic external atoms (only useful with iobehavior)" << std::endl
      << "                        functionality: Apply special rules for functional external atoms" << std::endl
      << "                        linearity: Apply special rules for external atoms which are linear in all(!) predicate parameters" << std::endl
      << "                        neg: Learn negative information" << std::endl
      << "                        user: Apply user-defined rules for nogood learning" << std::endl
      << "                        generalize: Generalize learned ground nogoods to nonground nogoods" << std::endl
      << "                      By default, all options except \"generalize\" are enabled" << std::endl
      << "     --nongroundnogoods" << std::endl
      << "                      Automatically instantiate learned nonground nogoods" << std::endl
      << "     --flpcheck=[explicit,ufs,ufsm,none]" << std::endl
      << "                      Sets the strategy used to check if a candidate is a subset-minimal model of the reduct" << std::endl
      << "                        explicit (default): Compute the reduct and compare its models with the candidate" << std::endl
      << "                        ufs: Use unfounded sets for minimality checking" << std::endl
      << "                        ufsm: (monolithic) Use unfounded sets for minimality checking; do not decompose the program for UFS checking" << std::endl
      << "                        none: Disable the check" << std::endl
      << "     --ufslearn       Enable learning from UFS checks (only useful with --flpcheck=ufs)" << std::endl
      << "     --eaheuristics=[always,never]" << std::endl
      << "                      Selects the heuristics for external atom evaluation" << std::endl
      << "                      always: Evaluate whenever possible" << std::endl
      << "                      never: Only evaluate at the end" << std::endl
      << "     --ufscheckheuristics=[post,max,periodic]" << std::endl
      << "                      post: Do UFS check only over complete interpretations" << std::endl
      << "                      max: Do UFS check as frequent as possible and over maximal subprograms" << std::endl
      << "                      periodic: Do UFS check in periodic intervals" << std::endl
      << " --modelqueuesize=N   Size of the model queue, i.e. number of models which can be computed in parallel" << std::endl
      << "                        (only useful for clasp solver)" << std::endl
      << " -s, --silent         Do not display anything than the actual result." << std::endl
      << "     --mlp            Use dlvhex+mlp solver (modular nonmonotonic logic programs)" << std::endl
      << "     --forget         Forget previous instantiations that are not involved in current computation (mlp setting)." << std::endl
      << "     --split          Use instantiation splitting techniques" << std::endl
    //        << "--strongsafety     Check rules also for strong safety." << std::endl
      << "     --weaksafety     Skip strong safety check." << std::endl
      << "     --autostrongsafety" << std::endl
      << "                      Tries to automatically establish strong safety where the property is violated" << std::endl
      << "                      (only useful with --weaksafety)" << std::endl
      << "     --multithreading Parallelizes model candidate computation and external atom verification (experimental)" << std::endl
      << " -p, --plugindir=DIR  Specify additional directory where to look for plugin" << std::endl
      << "                      libraries (additionally to the installation plugin-dir" << std::endl
      << "                      and $HOME/.dlvhex/plugins). Start with ! to reset the" << std::endl
			<< "                      preset plugin paths, e.g., '!:/lib' will use only /lib/." << std::endl
      << " -f, --filter=foo[,bar[,...]]" << std::endl
      << "                      Only display instances of the specified predicate(s)." << std::endl
      << " -n, --number=<num>   Limit number of displayed models to <num>, 0 (default) means all." << std::endl
      << " -a, --allmodels      Display all models also under weak constraints." << std::endl
//      << " -r, --reverse        Reverse weak constraint ordering." << std::endl
//      << "     --ruleml         Output in RuleML-format (v0.9)." << std::endl
      << "     --noeval         Just parse the program, don't evaluate it (only useful" << std::endl
      << "                      with --verbose)." << std::endl
      << "     --keepnsprefix   Keep specified namespace-prefixes in the result." << std::endl
      << "     --solver=S       Use S as ASP engine, where S is one of (dlv,dlvdb,libdlv,libclingo,genuineii,genuinegi,genuineic,genuinegc)" << std::endl
      << "                        (genuineii=(i)nternal grounder and (i)nternal solver; genuinegi=(g)ringo grounder and (i)nternal solver" << std::endl
      << "                         genuineic=(i)nternal grounder and (c)lasp solver; genuinegc=(g)ringo grounder and (c)lasp solver)" << std::endl
      << "     --nofacts        Do not output EDB facts" << std::endl
      << " -e, --heuristics=H   Use H as evaluation heuristics, where H is one of" << std::endl
			<< "                      old - old dlvhex behavior" << std::endl
			<< "                      trivial - use component graph as eval graph (much overhead)" << std::endl
			<< "                      easy - simple heuristics, used for LPNMR2011" << std::endl
			<< "                      greedy - (default) heuristics with advantages for external behavior learning" << std::endl
			<< "                      manual:<file> - read 'collapse <idxs> share <idxs>' commands from <file>" << std::endl
			<< "                        where component indices <idx> are from '--graphviz=comp'" << std::endl
			<< "                      asp:<script> - use asp program <script> as eval heuristic" << std::endl
			<< "     --dumpevalplan=F dump evaluation plan (usable as manual heuristics) to file F" << std::endl
      << " -m, --modelbuilder=M Use M as model builder, where M is one of (online,offline)" << std::endl
      << "     --nocache        Do not cache queries to and answers from external atoms." << std::endl
      << " -v, --verbose[=N]    Specify verbose category (default: 1):" << std::endl
      << "                      1  - program analysis information (including dot-file)" << std::endl
      << "                      2  - program modifications by plugins" << std::endl
      << "                      4  - intermediate model generation info" << std::endl
      << "                      8  - timing information (only if configured with" << std::endl
      << "                                               --enable-debug)" << std::endl
      << "                      add values for multiple categories." << std::endl
      << "     --graphviz=G     Specify comma separated list of graph types to export as .dot files." << std::endl
      << "                      Default is none, graph types are:" << std::endl
      << "                      dep    - Dependency Graph (once per program)" << std::endl
      << "                      cycinp - Graph for analysis cyclic predicate inputs (once per G&C-eval unit)" << std::endl
      << "                      comp   - Component Graph (once per program)" << std::endl
      << "                      eval   - Evaluation Graph (once per program)" << std::endl
      << "                      model  - Model Graph (once per program, after end of computation)" << std::endl
      << "                      imodel - Individual Model Graph (once per model)" << std::endl
      << "     --welljustified  Uses well-justified FLP semantics instead of FLP semantics for G&C components (only useful with genuine solvers)" << std::endl
      << "     --keepauxpreds   Keep auxiliary predicates in answer sets" << std::endl
      << "     --version        Show version information." << std::endl;
}


void
printVersion()
{
  std::cout << PACKAGE_TARNAME << " " << VERSION << std::endl;

  std::cout << "Copyright (C) 2011 Roman Schindlauer, Thomas Krennwallner, Peter Schüller" << std::endl
	    << "License LGPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/lgpl.html>" << std::endl
	    << "This is free software: you are free to change and redistribute it." << std::endl
	    << "There is NO WARRANTY, to the extent permitted by law." << std::endl;

  std::cout << std::endl;

  std::cout << "Homepage: http://www.kr.tuwien.ac.at/research/systems/dlvhex/" << std::endl
	    << "Support: dlvhex-devel@lists.sourceforge.net" << std::endl
	    << "Bug reports: http://sourceforge.net/apps/trac/dlvhex/" << std::endl;

  exit(0);
}


/**
 * @brief Print a fatal error message and terminate.
 */
void
InternalError (const char *msg)
{
  std::cerr << std::endl
	    << "An internal error occurred (" << msg << ")."
	    << std::endl
	    << "Please contact <" PACKAGE_BUGREPORT ">." << std::endl;
  exit (99);
}

// config and defaults of dlvhex main
struct Config
{
  bool optionNoEval;
  bool helpRequested;
  std::string optionPlugindir;
  #if defined(HAVE_DLVDB)
	// dlvdb speciality
	std::string typFile;
  #endif
	// those options unhandled by dlvhex main
	std::list<const char*> pluginOptions;

	Config():
  	optionNoEval(false),
  	helpRequested(false),
		optionPlugindir(""),
		#if defined(HAVE_DLVDB)
		typFile(),
		#endif
		pluginOptions() {}
};

void processOptionsPrePlugin(int argc, char** argv, Config& config, ProgramCtx& pctx);

extern dlvhex::ProgramCtx* globalpc = 0;

int main(int argc, char *argv[])
{
  const char* whoAmI = argv[0];

	// pre-init logger
	// (we use more than 4 bits -> two digit loglevel)
	Logger::Instance().setPrintLevelWidth(2);

	// program context
  ProgramCtx pctx;
	globalpc = &pctx;
	{
		RegistryPtr registry(new Registry);
		PluginContainerPtr pcp(new PluginContainer);
		pctx.setupRegistry(registry);
		pctx.setupPluginContainer(pcp);
	}

  // default external asp solver to first one that has been configured
	#if HAVE_DLV
  pctx.setASPSoftware(
		ASPSolverManager::SoftwareConfigurationPtr(new ASPSolver::DLVSoftware::Configuration));
	#else
		#if HAVE_DLVDB
		#error reactivate dlvdb
		//pctx.setASPSoftware(
		//	ASPSolverManager::SoftwareConfigurationPtr(new ASPSolver::DLVDBSoftware::Configuration));
		#else
			#if HAVE_LIBDLV
			pctx.setASPSoftware(
				ASPSolverManager::SoftwareConfigurationPtr(new ASPSolver::DLVLibSoftware::Configuration));
			#else
				#if HAVE_LIBCLINGO
				pctx.setASPSoftware(
					ASPSolverManager::SoftwareConfigurationPtr(new ASPSolver::ClingoSoftware::Configuration));
				#else
					#error no asp software configured! configure.ac should not allow this to happen!
				#endif
			#endif
		#endif
	#endif

	// default eval heuristic = "greedy" heuristic
	pctx.evalHeuristic.reset(new EvalHeuristicGreedy);
	// default model builder = "online" model builder
	pctx.modelBuilderFactory = boost::factory<OnlineModelBuilder<FinalEvalGraph>*>();

  pctx.config.setOption("FLPCheck", 1);
  pctx.config.setOption("UFSCheck", 0);
  pctx.config.setOption("UFSCheckMonolithic", 0);
  pctx.config.setOption("GenuineSolver", 0);
  pctx.config.setOption("Instantiate", 0);
  pctx.config.setOption("ExternalLearning", 0);
  pctx.config.setOption("UFSLearning", 0);
  pctx.config.setOption("ExternalLearningIOBehavior", 0);
  pctx.config.setOption("ExternalLearningMonotonicity", 0);
  pctx.config.setOption("ExternalLearningFunctionality", 0);
  pctx.config.setOption("ExternalLearningLinearity", 0);
  pctx.config.setOption("ExternalLearningNeg", 0);
  pctx.config.setOption("ExternalLearningUser", 0);
  pctx.config.setOption("ExternalLearningGeneralize", 0);
  pctx.config.setOption("NongroundNogoodInstantiation", 0);
  pctx.config.setOption("UFSCheckHeuristics", 0);
  pctx.config.setOption("ModelQueueSize", 5);
  pctx.config.setOption("Silent", 0);
  pctx.config.setOption("Verbose", 0);
  pctx.config.setOption("WeakAllModels", 0);
  // TODO was/is not implemented: pctx.config.setOption("WeakReverseAllModels", 0);
  pctx.config.setOption("UseExtAtomCache",1);
  pctx.config.setOption("KeepNamespacePrefix",0);
  pctx.config.setOption("DumpDepGraph",0);
  pctx.config.setOption("DumpCyclicPredicateInputAnalysisGraph",0);
  pctx.config.setOption("DumpCompGraph",0);
  pctx.config.setOption("DumpEvalGraph",0);
  pctx.config.setOption("DumpModelGraph",0);
  pctx.config.setOption("DumpIModelGraph",0);
  pctx.config.setOption("KeepAuxiliaryPredicates",0);
  pctx.config.setOption("NoFacts",0);
  pctx.config.setOption("NumberOfModels",0);
  pctx.config.setOption("RepeatEvaluation",0);
  pctx.config.setOption("NMLP", 0);
  pctx.config.setOption("MLP", 0);
  pctx.config.setOption("Forget", 0);
  pctx.config.setOption("Split", 0);
  pctx.config.setOption("SkipStrongSafetyCheck",0);
  pctx.config.setOption("AutoStrongSafety",0);
  pctx.config.setOption("MultiThreading",0);
  pctx.config.setOption("WellJustified",0);
	pctx.config.setOption("DumpEvaluationPlan",0);
	pctx.config.setOption("BenchmarkEAstderr",0); // perhaps only temporary
	pctx.config.setOption("ExplicitFLPUnshift",0); // perhaps only temporary
	pctx.config.setOption("PrintLearnedNogoods",0); // perhaps only temporary

	#warning TODO cleanup the setASPSoftware vs nGenuineSolver thing
	// but if we have genuinegc, take genuinegc as default
	#if defined(HAVE_LIBGRINGO) && defined(HAVE_LIBCLASP)
	pctx.config.setOption("GenuineSolver", 4);
	#endif

	// defaults of main
	Config config;

	// if we throw UsageError inside this, error and usage will be displayed, otherwise only error
	try
	{
		// default logging priority = errors + warnings
		Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);

		// manage options we can already manage
		// TODO use boost::program_options
		processOptionsPrePlugin(argc, argv, config, pctx);

		// initialize internal plugins
		{
			PluginInterfacePtr queryPlugin(new QueryPlugin);
			pctx.pluginContainer()->addInternalPlugin(queryPlugin);
			PluginInterfacePtr aggregatePlugin(new AggregatePlugin);
			pctx.pluginContainer()->addInternalPlugin(aggregatePlugin);
			PluginInterfacePtr strongNegationPlugin(new StrongNegationPlugin);
			pctx.pluginContainer()->addInternalPlugin(strongNegationPlugin);
			PluginInterfacePtr higherOrderPlugin(new HigherOrderPlugin);
			pctx.pluginContainer()->addInternalPlugin(higherOrderPlugin);
			PluginInterfacePtr weakConstraintPlugin(new WeakConstraintPlugin);
			pctx.pluginContainer()->addInternalPlugin(weakConstraintPlugin);
		}

		// before anything else we dump the logo
		if( !pctx.config.getOption("Silent") )
			printLogo();

		// initialize benchmarking (--verbose=8) with scope exit
		// (this cannot be outsourced due to the scope)
		benchmark::BenchmarkController& ctr =
			benchmark::BenchmarkController::Instance();
		if( pctx.config.doVerbose(Configuration::PROFILING) )
		{
			LOG(INFO,"initializing benchmarking output");
			ctr.setOutput(&Logger::Instance().stream());
			// for continuous statistics output, display every 1000'th output
			ctr.setPrintInterval(999);
		}
		else
			ctr.setOutput(0);
		// deconstruct benchmarking (= output results) at scope exit 
		int dummy; // this is needed, as SCOPE_EXIT is not defined for no arguments
		BOOST_SCOPE_EXIT( (dummy) ) {
			(void)dummy;
			benchmark::BenchmarkController::finish();
		}
		BOOST_SCOPE_EXIT_END

		// startup statemachine
		pctx.changeState(StatePtr(new ShowPluginsState));

		// load plugins
		{
			DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"loading plugins");
			pctx.pluginContainer()->loadPlugins(config.optionPlugindir);
			pctx.showPlugins();
		}

		// now we may offer help, including plugin help
		if( config.helpRequested )
		{
			printUsage(std::cerr, whoAmI, true);
			pctx.pluginContainer()->printUsage(std::cerr);
			return 1;
		}

		// process plugin options using plugins
		// (this deletes processed options from config.pluginOptions)
		// TODO use boost::program_options
		pctx.processPluginOptions(config.pluginOptions);
			
		// handle options not recognized by dlvhex and not by plugins
		if( !config.pluginOptions.empty() )
		{
			std::stringstream bad;
			bad << "Unknown option(s):";
			BOOST_FOREACH(const char* opt, config.pluginOptions)
			{
				bad << " " << opt;
			}
			throw UsageError(bad.str());
		}
		// use configured plugins to obtain plugin atoms
		pctx.addPluginAtomsFromPluginContainer();

		// now we check if we got input
		if( !pctx.inputProvider || !pctx.inputProvider->hasContent() )
			throw UsageError("no input specified!");

		// convert input (only done if at least one plugin provides a converter)
		pctx.convert();
			
		// parse input (coming directly from inputprovider or from inputprovider provided by the convert() step)
		pctx.parse();
		
		// check if in mlp mode	
		if( pctx.config.getOption("MLP") ) 
		  {
			// syntax check for mlp
			pctx.moduleSyntaxCheck();

			// solve mlp
			pctx.mlpSolver();
		  }

		else 

		  {	

		// associate PluginAtom instances with
		// ExternalAtom instances (in the IDB)
		pctx.associateExtAtomsWithPluginAtoms(pctx.idb, true);
			
		// rewrite program (plugins might want to do this, e.g., for partial grounding)
		pctx.rewriteEDBIDB();
			
		// associate PluginAtom instances with
		// ExternalAtom instances (in the IDB)
		// (again, rewrite might add external atoms)
		pctx.associateExtAtomsWithPluginAtoms(pctx.idb, true);

		// check weak safety
		pctx.safetyCheck();

		// create dependency graph (we need the previous step for this)
		pctx.createDependencyGraph();

		// optimize dependency graph (plugins might want to do this, e.g. by using domain information)
		pctx.optimizeEDBDependencyGraph();
		// everything in the following will be done using the dependency graph and EDB
		#warning IDB and dependencygraph could get out of sync! should we lock or empty the IDB to ensure that it is not directly used anymore after this step?
			
		// create graph of strongly connected components of dependency graph
		pctx.createComponentGraph();

		// use SCCs to do strong safety check
		if( !pctx.config.getOption("SkipStrongSafetyCheck") )
			pctx.strongSafetyCheck();
		
		// select heuristics and create eval graph
		pctx.createEvalGraph();

		// stop here if no evaluation was requested
		if( config.optionNoEval )
			return 0;

		// setup model builder and configure plugin/dlvhex model processing hooks
		pctx.setupProgramCtx();
			
		// evaluate (generally done in streaming mode, may exit early if indicated by hooks)
		// (individual model output should happen here)
		pctx.evaluate();

		} // end if (mlp) else ...

		// finalization plugin/dlvhex hooks (for accumulating model processing)
		// (accumulated model output/query answering should happen here)
		pctx.postProcess();
	}
  catch(const UsageError &ue)
	{
		std::cerr << "UsageError: " << ue.getErrorMsg() << std::endl << std::endl;
		printUsage(std::cerr, whoAmI, true);
		if( !!pctx.pluginContainer() )
			pctx.pluginContainer()->printUsage(std::cerr);
		return 1;
	}
  catch(const GeneralError &ge)
	{
		std::cerr << "GeneralError: " << ge.getErrorMsg() << std::endl << std::endl;
		return 1;
	}
	catch(const std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl << std::endl;
		return 1;
	}

	// regular exit
	return 0;
}

void configurePluginPath(std::string& userPlugindir);

// process whole commandline:
// * recognized arguments are stored into some config
// * non-option arguments are interpreted as input files
//   and used to configure config.inputProvider (exception: .typ files)
// * non-recognized option arguments are stored into config.pluginOptions
void processOptionsPrePlugin(
		int argc, char** argv,
		Config& config, ProgramCtx& pctx)
{
  extern char* optarg;
  extern int optind;
  extern int opterr;

  //
  // prevent error message for unknown options - they might be known to
  // plugins later!
  //
  opterr = 0;

  int ch;
  int longid;
  
  static const char* shortopts = "hsvf:p:are:m:n:";
  static struct option longopts[] =
	{
		{ "help", no_argument, 0, 'h' },
		{ "silent", no_argument, 0, 's' },
		{ "verbose", optional_argument, 0, 'v' },
		{ "filter", required_argument, 0, 'f' },
		{ "plugindir", required_argument, 0, 'p' },
		{ "allmodels", no_argument, 0, 'a' },
		{ "reverse", no_argument, 0, 'r' },
		{ "heuristics", required_argument, 0, 'e' },
		{ "modelbuilder", required_argument, 0, 'm' },
		{ "number", required_argument, 0, 'n' },
		//{ "firstorder", no_argument, &longid, 1 },
		{ "weaksafety", no_argument, &longid, 2 },
		//{ "ruleml",     no_argument, &longid, 3 },
		//{ "dlt",        no_argument, &longid, 4 },
		{ "noeval",     no_argument, &longid, 5 },
		{ "keepnsprefix", no_argument, &longid, 6 },
		{ "solver", required_argument, &longid, 7 },
		{ "nocache",    no_argument, &longid, 8 },
		{ "version",    no_argument, &longid, 9 },
		{ "graphviz", required_argument, &longid, 10 },
		{ "keepauxpreds", no_argument, &longid, 11 },
		{ "nofacts", no_argument, &longid, 12 },
		{ "mlp", no_argument, &longid, 13 },
		{ "forget", no_argument, &longid, 15 },
		{ "split", no_argument, &longid, 16 },
		{ "dumpevalplan", required_argument, &longid, 17 },
		{ "extlearn", optional_argument, 0, 18 },
		{ "flpcheck", required_argument, 0, 20 },
		{ "ufslearn", no_argument, 0, 23 },
		{ "welljustified", optional_argument, 0, 25 },
		{ "eaevalheuristics", required_argument, 0, 26 },
		{ "ufscheckheuristics", required_argument, 0, 27 },
		{ "benchmarkeastderr", no_argument, 0, 28 }, // perhaps only temporary
		{ "explicitflpunshift", no_argument, 0, 29 }, // perhaps only temporary
		{ "printlearnednogoodsstderr", no_argument, 0, 30 }, // perhaps only temporary
		{ "nongroundnogoods", no_argument, 0, 31 },
		{ "modelqueuesize", required_argument, 0, 32 },
		{ "autostrongsafety", no_argument, 0, 33 },
		{ "multithreading", no_argument, 0, 34 },
		{ NULL, 0, NULL, 0 }
	};

  // default settings
  pctx.externalAtomEvaluationHeuristicsFactory.reset(new ExternalAtomEvaluationHeuristicsNeverFactory());
  pctx.unfoundedSetCheckHeuristicsFactory.reset(new UnfoundedSetCheckHeuristicsPostFactory());

  bool specifiedModelQueueSize = false;
  while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1)
	{
		switch (ch)
		{
		case 'h':
			config.helpRequested = 1;
			break;

		case 's':
			pctx.config.setOption("Silent", 1);
			break;

		case 'v':
			if (optarg)
			{
				int level = 1;
				try
				{
					level = boost::lexical_cast<int>(optarg);
				}
				catch(const boost::bad_lexical_cast& e)
				{
					LOG(ERROR,"could not parse verbosity level '" << optarg << "' - using default=" << level << "!");
				}
				pctx.config.setOption("Verbose", level);
				Logger::Instance().setPrintLevels(level);
			}
			else
			{
				pctx.config.setOption("Verbose", 1);
				Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING | Logger::INFO);
			}
			break;

		case 'f':
			{
				boost::char_separator<char> sep(",");
				std::string oa(optarg); // g++ 3.3 is unable to pass that at the ctor line below
				boost::tokenizer<boost::char_separator<char> > tok(oa, sep);
				
				for(boost::tokenizer<boost::char_separator<char> >::const_iterator f = tok.begin();
						f != tok.end(); ++f)
					pctx.config.addFilter(*f);
			}
			break;
			
		case 'p':
			config.optionPlugindir = std::string(optarg);
			break;

		case 'a':
			pctx.config.setOption("AllModels", 1);
			break;

		case 'r':
			pctx.config.setOption("ReverseOrder", 1);
			break;

		case 'e':
			// heuristics={old,trivial,easy,manual:>filename>}
			{
				std::string heuri(optarg);
				if( heuri == "old" )
				{
					pctx.evalHeuristic.reset(new EvalHeuristicOldDlvhex);
				}
				else if( heuri == "trivial" )
				{
					pctx.evalHeuristic.reset(new EvalHeuristicTrivial);
				}
				else if( heuri == "easy" )
				{
					pctx.evalHeuristic.reset(new EvalHeuristicEasy);
				}
				else if( heuri == "greedy" )
				{
					pctx.evalHeuristic.reset(new EvalHeuristicGreedy);
				}
				else if( heuri.substr(0,7) == "manual:" )
				{
					pctx.evalHeuristic.reset(new EvalHeuristicFromFile(heuri.substr(7)));
				}
				else if( heuri.substr(0,4) == "asp:" )
				{
					pctx.evalHeuristic.reset(new EvalHeuristicASP(heuri.substr(4)));
				}
				else
				{
					throw UsageError("unknown evaluation heuristic '" + heuri +"' specified!");
				}
				LOG(INFO,"selected '" << heuri << "' evaluation heuristics");
			}
			break;

		case 'm':
			// modelbuilder={offline,online}
			{
				std::string modelbuilder(optarg);
				if( modelbuilder == "offline" )
				{
					pctx.modelBuilderFactory =
						boost::factory<OfflineModelBuilder<FinalEvalGraph>*>();
				}
				else if( modelbuilder == "online" )
				{
					pctx.modelBuilderFactory =
						boost::factory<OnlineModelBuilder<FinalEvalGraph>*>();
				}
				else
				{
					throw UsageError("unknown model builder '" + modelbuilder +"' specified!");
				}
				LOG(INFO,"selected '" << modelbuilder << "' model builder");
			}
			break;

		case 'n':
			{
				int models = 0;
				try
				{
					if( optarg[0] == '=' )
						models = boost::lexical_cast<unsigned>(&optarg[1]);
					else
						models = boost::lexical_cast<unsigned>(optarg);
				}
				catch(const boost::bad_lexical_cast& e)
				{
					LOG(ERROR,"could not parse model count '" << optarg << "' - using default=" << models << "!");
				}
				pctx.config.setOption("NumberOfModels", models);
			}
			break;

		case 0:
			switch (longid)
				{
				case 2:
					pctx.config.setOption("SkipStrongSafetyCheck",1);
					break;

				//case 3:
				 // pctx.setOutputBuilder(new RuleMLOutputBuilder);
					// XML output makes only sense with silent:
				 // pctx.config.setOption("Silent", 1);
				 // break;

				//case 4:
				//  optiondlt = true;
				//  break;

				case 5:
					config.optionNoEval = true;
					break;

				case 6:
					pctx.config.setOption("KeepNamespacePrefix",1);
					break;

				case 7:
					{
						std::string solver(optarg);
						if( solver == "dlv" )
						{
							#if defined(HAVE_DLV)
							pctx.setASPSoftware(
								ASPSolverManager::SoftwareConfigurationPtr(new ASPSolver::DLVSoftware::Configuration));
							#else
							throw GeneralError("sorry, no support for solver backend '"+solver+"' compiled into this binary");
							#endif
						}
						else if( solver == "dlvdb" )
						{
							#if defined(HAVE_DLVDB)
							#warning reactivate dlvhdb
							//pctx.setASPSoftware(
							//	ASPSolverManager::SoftwareConfigurationPtr(new ASPSolver::DLVDBSoftware::Configuration));
							#else
							throw GeneralError("sorry, no support for solver backend '"+solver+"' compiled into this binary");
							#endif
						}
						else if( solver == "libdlv" )
						{
							#if defined(HAVE_LIBDLV)
							pctx.setASPSoftware(
								ASPSolverManager::SoftwareConfigurationPtr(new ASPSolver::DLVLibSoftware::Configuration));
							#else
							throw GeneralError("sorry, no support for solver backend '"+solver+"' compiled into this binary");
							#endif
						}
						else if( solver == "libclingo" )
						{
							#if defined(HAVE_LIBCLINGO)
							pctx.setASPSoftware(
								ASPSolverManager::SoftwareConfigurationPtr(new ASPSolver::ClingoSoftware::Configuration));
							#else
							throw GeneralError("sorry, no support for solver backend '"+solver+"' compiled into this binary");
							#endif
						}
						else if( solver == "genuineii" )
						{
							pctx.config.setOption("GenuineSolver", 1);
						}
						else if( solver == "genuinegi" )
						{
							#if defined(HAVE_LIBGRINGO)
							pctx.config.setOption("GenuineSolver", 2);
							#else
							throw GeneralError("sorry, no support for solver backend '"+solver+"' compiled into this binary");
							#endif
						}
						else if( solver == "genuineic" )
						{
							#if defined(HAVE_LIBCLASP)
							pctx.config.setOption("GenuineSolver", 3);
							#else
							throw GeneralError("sorry, no support for solver backend '"+solver+"' compiled into this binary");
							#endif
						}
						else if( solver == "genuinegc" )
						{
							#if defined(HAVE_LIBGRINGO) && defined(HAVE_LIBCLASP)
							pctx.config.setOption("GenuineSolver", 4);
							#else
							throw GeneralError("sorry, no support for solver backend '"+solver+"' compiled into this binary");
							#endif
						}
						else
						{
							throw UsageError("unknown solver backend '" + solver +"' specified!");
						}
						LOG(INFO,"selected '" << solver << "' solver backend");
					}
					break;

				case 8:
					pctx.config.setOption("UseExtAtomCache",0);
					break;

				case 9:
					printVersion();
					break;

				case 10:
					{
						boost::char_separator<char> sep(",");
						std::string oa(optarg); // g++ 3.3 is unable to pass that at the ctor line below
						boost::tokenizer<boost::char_separator<char> > tok(oa, sep);
						
						for(boost::tokenizer<boost::char_separator<char> >::const_iterator f = tok.begin();
								f != tok.end(); ++f)
						{
							const std::string& token = *f;
							if( token == "dep" )
							{
								pctx.config.setOption("DumpDepGraph",1);
							}
							else if( token == "cycinp" )
							{
								pctx.config.setOption("DumpCyclicPredicateInputAnalysisGraph",1);
							}
							else if( token == "comp" )
							{
								pctx.config.setOption("DumpCompGraph",1);
							}
							else if( token == "eval" )
							{
								pctx.config.setOption("DumpEvalGraph",1);
							}
							else if( token == "model" )
							{
								pctx.config.setOption("DumpModelGraph",1);
							}
							else if( token == "imodel" )
							{
								pctx.config.setOption("DumpIModelGraph",1);
							}
							else
								throw UsageError("unknown graphviz graph type '"+token+"'");
						}
					}
					break;
				case 11:
					pctx.config.setOption("KeepAuxiliaryPredicates",1);
					break;
				case 12:
					pctx.config.setOption("NoFacts",1);
					break;
				case 13:
					pctx.config.setOption("MLP",1);
					break;
				// unused case 14:
				case 15:
					pctx.config.setOption("Forget",1);
					break;
				case 16:
					pctx.config.setOption("Split",1);
					break;
				case 17:
					{
						std::string fname(optarg);
						pctx.config.setOption("DumpEvaluationPlan",1);
						pctx.config.setStringOption("DumpEvaluationPlanFile",fname);
					}
					break;
				}
			break;

		case 18:
			{
				if (optarg){
					boost::char_separator<char> sep(",");
					std::string oa(optarg); // g++ 3.3 is unable to pass that at the ctor line below
					boost::tokenizer<boost::char_separator<char> > tok(oa, sep);

					for(boost::tokenizer<boost::char_separator<char> >::const_iterator f = tok.begin();
							f != tok.end(); ++f)
					{
						const std::string& token = *f;
						if (token == "iobehavior" )
						{
							pctx.config.setOption("ExternalLearningIOBehavior", 1);
						}
						else if( token == "monotonicity" )
						{
							pctx.config.setOption("ExternalLearningMonotonicity", 1);
						}
						else if( token == "functionality" )
						{
							pctx.config.setOption("ExternalLearningFunctionality", 1);
						}
						else if( token == "linearity" )
						{
							pctx.config.setOption("ExternalLearningLinearity", 1);
						}
                                                else if( token == "neg" )
						{
							pctx.config.setOption("ExternalLearningNeg", 1);
						}
                                                else if( token == "user" )
						{
							pctx.config.setOption("ExternalLearningUser", 1);
						}
                                                else if( token == "generalize" )
						{
							pctx.config.setOption("ExternalLearningGeneralize", 1);
						}
						else
						{
							throw GeneralError("Unknown learning option: \"" + token + "\"");
						}
					}
				}else{
					// by default, turn on all external learning rules
					pctx.config.setOption("ExternalLearningIOBehavior", 1);
					pctx.config.setOption("ExternalLearningMonotonicity", 1);
					pctx.config.setOption("ExternalLearningFunctionality", 1);
					pctx.config.setOption("ExternalLearningLinearity", 1);
					pctx.config.setOption("ExternalLearningNeg", 1);
					pctx.config.setOption("ExternalLearningUser", 1);
					//pctx.config.setOption("ExternalLearningGeneralize", 1);	// do not activate by default (it is mostly counterproductive)
				}
			}

			pctx.config.setOption("ExternalLearning", 1);

			DBGLOG(DBG, "External learning: " << pctx.config.getOption("ExternalLearning") << " [iobehavior: " << pctx.config.getOption("ExternalLearningIOBehavior") << " [monotonicity: " << pctx.config.getOption("ExternalLearningMonotonicity") << ", functionlity: " << pctx.config.getOption("ExternalLearningFunctionality") << ", linearity: " << pctx.config.getOption("ExternalLearningLinearity") << ", user-defined: " << pctx.config.getOption("ExternalLearningUser") << "]");
			break;

		case 20:
			{
				std::string check(optarg);
				if( check == "explicit" )
				{
					pctx.config.setOption("FLPCheck", 1);
					pctx.config.setOption("UFSCheck", 0);
				}else if( check == "ufs" )
				{
					pctx.config.setOption("FLPCheck", 0);
					pctx.config.setOption("UFSCheck", 1);
					pctx.config.setOption("UFSCheckMonolithic", 0);
				}else if( check == "ufsm" )
				{
					pctx.config.setOption("FLPCheck", 0);
					pctx.config.setOption("UFSCheck", 1);
					pctx.config.setOption("UFSCheckMonolithic", 1);
				}else{
					pctx.config.setOption("FLPCheck", 0);
					pctx.config.setOption("UFSCheck", 0);
				}

				LOG(INFO,"FLP Check: " << pctx.config.getOption("FLPCheck") << "; UFS Check: " << pctx.config.getOption("UFSCheck"));
			}

			break;

		case 23:
			pctx.config.setOption("UFSLearning", 1);
			break;

		case 25:
			pctx.config.setOption("WellJustified", 1);
			break;

		case 26:
			{
				std::string heur(optarg);
				if (heur == "always")
				{
					pctx.externalAtomEvaluationHeuristicsFactory.reset(new ExternalAtomEvaluationHeuristicsAlwaysFactory());
				}
				else if (heur == "never")
				{
					pctx.externalAtomEvaluationHeuristicsFactory.reset(new ExternalAtomEvaluationHeuristicsNeverFactory());
				}
				else
				{
					throw GeneralError(std::string("Unknown external atom evaluation heuristics: \"") + heur + std::string("\""));
				}
			}
			break;

		case 27:
			{
				std::string heur(optarg);
				if (heur == "post")
				{
					pctx.unfoundedSetCheckHeuristicsFactory.reset(new UnfoundedSetCheckHeuristicsPostFactory());
					pctx.config.setOption("UFSCheckHeuristics", 0);
				}
				else if (heur == "max")
				{
					pctx.unfoundedSetCheckHeuristicsFactory.reset(new UnfoundedSetCheckHeuristicsMaxFactory());
					pctx.config.setOption("UFSCheckHeuristics", 1);
				}
				else if (heur == "periodic")
				{
					pctx.unfoundedSetCheckHeuristicsFactory.reset(new UnfoundedSetCheckHeuristicsPeriodicFactory());
					pctx.config.setOption("UFSCheckHeuristics", 2);
				}
				else
				{
					throw GeneralError(std::string("Unknown UFS check heuristics: \"") + heur + std::string("\""));
				}
			}
			break;

		case 28: pctx.config.setOption("BenchmarkEAstderr",1); break;
		case 29: pctx.config.setOption("ExplicitFLPUnshift",1); break;

		case 30: pctx.config.setOption("PrintLearnedNogoods",1); break;

		case 31: pctx.config.setOption("NongroundNogoodInstantiation", 1); break;

		case 32:
			{
				int queuesize = 5;
				try
				{
					if( optarg[0] == '=' )
						queuesize = boost::lexical_cast<unsigned>(&optarg[1]);
					else
						queuesize = boost::lexical_cast<unsigned>(optarg);
				}
				catch(const boost::bad_lexical_cast& e)
				{
					LOG(ERROR,"could not parse size of model queue '" << optarg << "' - using default=" << queuesize << "!");
				}
				if (queuesize < 1){
					throw GeneralError(std::string("Model queue size must be > 0"));
				}
				pctx.config.setOption("ModelQueueSize", queuesize); break;
				specifiedModelQueueSize = true;
			}
			break;

		case 33: pctx.config.setOption("AutoStrongSafety", 1); break;

		case 34: pctx.config.setOption("MultiThreading", 1); break;

		case '?':
			config.pluginOptions.push_back(argv[optind - 1]);
			break;
		}
	}

	// global constraints
	if (pctx.config.getOption("UFSCheck") && !pctx.config.getOption("GenuineSolver")){
		LOG(WARNING, "Unfounded Set Check is only supported for genuine solvers; will behave like flpcheck=none");
	}
	if (specifiedModelQueueSize && pctx.config.getOption("GenuineSolver") <= 2){
		LOG(WARNING, "Model caching (modelqueuesize) is only compatible with clasp backend");
	}

	// configure plugin path
	configurePluginPath(config.optionPlugindir);

	// check input files (stdin, file, or URI)

	// start with new input provider
	pctx.inputProvider.reset(new InputProvider);

	// stdin requested, append it first
	if( std::string(argv[optind - 1]) == "--" )
		pctx.inputProvider->addStreamInput(std::cin, "<stdin>");

	// collect further filenames/URIs
	// if we use dlvdb, manage .typ files
	for (int i = optind; i < argc; ++i)
	{
		std::string arg(argv[i]);
		if( arg.size() > 4 && arg.substr(arg.size()-4) == ".typ" )
		{
			#if defined(HAVE_DLVDB)
			boost::shared_ptr<ASPSolver::DLVDBSoftware::Configuration> ptr =
				boost::dynamic_pointer_cast<ASPSolver::DLVDBSoftware::Configuration>(pctx.getASPSoftware());
			if( ptr == 0 )
				throw GeneralError(".typ files can only be used if dlvdb backend is used");

			if( !ptr->options.typFile.empty() )
				throw GeneralError("cannot use more than one .typ file with dlvdb");
			
			ptr->options.typFile = arg;
			#endif
		}
		else if( arg.find("http://") == 0 )
		{
			pctx.inputProvider->addURLInput(arg);
		}
		else
		{
			pctx.inputProvider->addFileInput(arg);
		}
	}
}

void configurePluginPath(std::string& userPlugindir)
{
	bool reset = false;
	if( !userPlugindir.empty() && userPlugindir[0] == '!' )
	{
		reset = true;
		if( userPlugindir.size() > 2 && userPlugindir[1] == ':' )
			userPlugindir.erase(0,2);
		else
			userPlugindir.erase(0,1);
	}

  std::stringstream searchpath;

	if( !userPlugindir.empty() )
		searchpath << userPlugindir << ':';
  
	if( !reset )
	{
		// add LD_LIBRARY_PATH
		const char *envld = ::getenv("LD_LIBRARY_PATH");
		if( envld )
		{
			searchpath << envld << ":";
		}

		const char* homedir = ::getpwuid(::geteuid())->pw_dir;
		searchpath << homedir << "/" USER_PLUGIN_DIR << ':' << SYS_PLUGIN_DIR;
	}
	userPlugindir = searchpath.str();
}

/* vim: set expandtab sw=2 ts=2 tw=80: */

// Local Variables:
// mode: C++
// End:
