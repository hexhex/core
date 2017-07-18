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
 * \image html logo.png
 *
 * This is the sourcecode documentation of the dlvhex system.
 *
 * You will look into the documentation of dlvhex most likely to implement a
 * plugin. In this case, please continue with the \ref pluginframework
 * "Plugin Interface Module (for C++)" or the \ref pythonpluginframework "Python Plugin Interface Module", which contains all necessary information.
 *
 * If you are interested in the implementation of the dlvhex core itself, have a look at the \ref systemarchitecture for an overview.
 *
 * \defgroup systemarchitecture System Architecture
 *
 * This section describes the general dlvhex system architecture.
 * The following figure gives an overview, the numbers in the decription below refer to the numbers in this figure.
 * The arcs model both control and data flow within the system.
 *
 * \image html systemarchitecture.png
 * \image latex systemarchitecture.png
 *
 * The evaluation of a HEX-program works as follows.
 * First, the input program is read from the file system or from standard input
 * and passed to the <em>evaluation framework</em> (1).
 * The evaluation framework creates then a <em>generalized evaluation graph</em> depending on the chosen evaluation heuristics,
 * see EvalGraphBuilder and EvalHeuristicBase.
 * This results in a number of interconnected <em>generalized evaluation units</em> (see EvalGraph). While the interplay of the units
 * is managed by the evaluation framework, the individual units are handeled by <em>model generators</em> (see ModelGeneratorBase and FLPModelGeneratorBase)
 * of different kinds.
 *
 * dlvhex historically supports two interfaces for model generators. The legacy non-genuine interface is formed by the classes ASPSolver
 * and ASPSolverManager. Model generators built on top of this interface do not see the internals of the ordinary ASP solver but communicate
 * with them only as black boxes. This gives them little room for optimization, but can be used even for ordinary ASP solvers which come in binary format only.
 * Currently, the interface is used for DLV, which is run in a separate process and communication works via pipes (see Process, DLVProcess, and DLVResultParser).
 * The more modern genuine interface is mainly formed by the base classes GenuineSolver, GenuineGroundSolver and GenuineGrounder, and
 * allows for a much tighter coupling of dlvhex with ordinary ASP solvers and lets dlvhex access the solver internals.
 * This is usually much more efficient due to several possible optimizations.
 *
 * Besides the distintion between the non-genuine and genuine interface, there are also multiple model generators for different syntactic fragments of the HEX language.
 * All types of model generators are implemented for both interfaces.
 * General program components use a guess-and-check algorithm (see GuessAndCheckModelGenerator and GenuineGuessAndCheckModelGenerator),
 * while monotonic program components may use a more efficient fixpoint iteration (see WellfoundedModelGenerator and GenuineWellfoundedModelGenerator).
 * Programs without any cyclic external atoms can use the even simpler PlainModelGenerator or GenuinePlainModelGenerator.
 * This is realized as different model generators. Each instance of a model generator takes care of a single evaluation unit,
 * receives <em>input interpretations</em> from the framework (which are either output by predecessor units
 * or come from the input facts for leaf units), and sends output interpretations back to the framework (2),
 * which manages the integration of these interpretations to final answer sets.

 * Internally, the model generators make use of a <em>grounder</em> and a <em>solver</em> for ordinary ASP programs. The architecture of
 * our system is flexible and supports multiple concrete backends which can be plugged in. Currently it supports DLV, Gringo and clasp,
 * and an internal grounder (see InternalGrounder) and a solver (see InternalGroundASPSolver and InternalGroundDASPSolver) which do not depend on third-party software (mainly for testing purposes);
 * they use basically the same core algorithms as Gringo and clasp, but without any kind of optimizations.
 * The reasoner backends Gringo (see GringoGrounder) and clasp (see ClaspSolver) are statically linked to our system, thus no interprocess communication is necessary.
 * The model generator within the dlvhex core sends a non-ground program to the HEX-grounder,
 * and receives a ground program (3). The HEX-grounder in turn uses
 * an ordinary ASP grounder as submodule (4) and accesses external sources to handle value invention (5).
 * The ground-program is then sent to the solver and answer sets of the ground program (i.e. candidate compatible sets) are returned (6).
 * Note that the grounder and the solver are separated and communicate only through the model generator, which is in contrast
 * to previous implementations of dlvhex where the external grounder and solver were used as a single unit (i.e., the
 * non-ground program was sent and the answer sets were retrieved).
 * Separating the two units became necessary because the dlvhex core needs access to the ground-program. Otherwise
 * important structural information, such as cyclicity would be hidden.

 * For genuine solvers, the solver backend makes callbacks to the dlvhex core (where they are caught by a so-called <em>post propagator</em>) once a model has been found
 * or after unit and unfounded set propagation has been finished.
 * Once a complete or partial model candidate has arrived at the post propagator,
 * the dlvhex core performs two operations: <em>compatibility checking with learning</em> (driven by a heuristics, see ExternalAtomEvaluationHeuristics)
 * and <em>unfounded set detection</em> (also driven by a heuristics, see UnfoundedSetCheckHeuristics).
 *
 * Compatibility checks determine whether the guesses of the external atom replacements by the ordinary ASP solver coincide with the actual
 * semantics of the external source. This check also requires calls to the <em>plugins</em> (see PluginInterface), which implement the external sources.
 * The input list is sent to the external source and the truth values are returned
 * to the dlvhex core (9).
 * Additionally, the external source can use the current complete or partial interpretation
 * to create new learned nogoods in the sense of conflict-driven learning (see Nogood, NogoodContainer and PluginAtom::retrieve with learning functionality)
 * in order to enforce early backtracking in case of conflicts or guide the further search; these nogoods are sent back to the post propagator and to the ordinary ASP solver (7).
 *
 * <em>Unfounded set checking</em> is used
 * to find unfounded sets which are not detected by the ordinary ASP solver, i.e., unfounded sets caused by external sources (see UnfoundedSetChecker and UnfoundedSetCheckerManager).
 * For this purpose, the UFS checker employs a SAT solver (11) (see SATSolver), which can either be clasp or an internal SAT solver (see CDNLSolver).
 * The UFS checker possibly returns nogoods learned from unfounded sets to the post propagator (8).
 * UFS detection also needs to call the external sources for guess verification.
 * The post propagator sends all learned nogoods
 * (either directly from external sources or from unfounded sets) back to the ordinary ASP solver.
 * This makes sure that eventually only valid answer sets arrive at the model generator (6).

 * Finally, after the evaluation framework has built the final answer sets from the output interpretations
 * of the individual evaluation units, they are output to the user (12).
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#include "dlvhex2/Error.h"
#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/PluginContainer.h"
#include "dlvhex2/ASPSolverManager.h"
#include "dlvhex2/ASPSolver.h"
#include "dlvhex2/AnswerSetPrinterCallback.h"
#include "dlvhex2/State.h"
#include "dlvhex2/EvalGraphBuilder.h"
#include "dlvhex2/EvalHeuristicBase.h"
#include "dlvhex2/EvalHeuristicASP.h"
#include "dlvhex2/EvalHeuristicOldDlvhex.h"
#include "dlvhex2/EvalHeuristicTrivial.h"
#include "dlvhex2/EvalHeuristicEasy.h"
#include "dlvhex2/EvalHeuristicGreedy.h"
#include "dlvhex2/EvalHeuristicMonolithic.h"
#include "dlvhex2/EvalHeuristicFromFile.h"
#include "dlvhex2/ExternalAtomEvaluationHeuristics.h"
#include "dlvhex2/UnfoundedSetCheckHeuristics.h"
#include "dlvhex2/OnlineModelBuilder.h"
#include "dlvhex2/OfflineModelBuilder.h"

// internal plugins
#include "dlvhex2/QueryPlugin.h"
#include "dlvhex2/AggregatePlugin.h"
#include "dlvhex2/ChoicePlugin.h"
#include "dlvhex2/ConditionalLiteralPlugin.h"
#include "dlvhex2/StrongNegationPlugin.h"
#include "dlvhex2/HigherOrderPlugin.h"
#include "dlvhex2/WeakConstraintPlugin.h"
#include "dlvhex2/ManualEvalHeuristicsPlugin.h"
#include "dlvhex2/FunctionPlugin.h"
#ifdef HAVE_PYTHON
#include "dlvhex2/PythonPlugin.h"
#endif

#include <getopt.h>
#include <signal.h>
#include <sys/types.h>
#ifdef WIN32
#include <windows.h>
#undef ERROR                     // there is a clash with a Windows definition
#elif POSIX
#include <pwd.h>
#else
#error Either POSIX or WIN32 must be defined
#endif

#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>
#include <fstream>
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
        #endif                   // HAVE_CONFIG_H
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

    if (!full) {
        out << "Specify -h or --help for more detailed usage information." << std::endl
            << std::endl;

        return;
    }

    //
    // As soos as we have more options, we can introduce sections here!
    //
    //      123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
    out << "Input, Output and Reasoning Options (influence the results):" << std::endl
        << "     --               Parse from stdin." << std::endl
        << " -s, --silent         Do not display anything than the actual result." << std::endl
        << " -f, --filter=foo[,bar[,...]]" << std::endl
        << "                      Only display instances of the specified predicate(s)." << std::endl
        << "     --nofacts        Do not output EDB facts." << std::endl
        << " -n, --number=<num>   Limit number of displayed models to <num>, 0 (default) means all." << std::endl
        << " -N, --maxint=<num>   Set maximum integer (#maxint in the program takes precedence over the parameter)." << std::endl
        << "     --weaksafety     Skip strong safety check." << std::endl
        << "     --strongsafety   Applies traditional strong safety criteria." << std::endl
        << "     --liberalsafety  Uses more liberal safety conditions than strong safety (default)." << std::endl
        << "     --mlp            Use dlvhex+mlp solver (modular nonmonotonic logic programs)." << std::endl
        << "     --forget         Forget previous instantiations that are not involved in current computation (mlp setting)." << std::endl
        << "     --split          Use instantiation splitting techniques." << std::endl
        << "     --noeval         Just parse the program, don't evaluate it (only useful with --verbose)." << std::endl
        << "     --keepnsprefix   Keep specified namespace-prefixes in the result." << std::endl
        << "     --keepauxpreds   Keep auxiliary predicates in answer sets." << std::endl
        << "     --csvinput=PREDICATE,FILENAME" << std::endl
        << "                      Read from the given file in CSV format and add each line as fact" << std::endl
        << "                      in over the specified predicate (the original line number is added as first argument)." << std::endl
        << "     --csvoutput=PREDICATE" << std::endl
        << "                      Print the extension of the specified predicate in CSV format." << std::endl
        << "                      They are sorted by their first argument (should be numeric)." << std::endl
        << "                      Answer Sets are separated by empty lines." << std::endl
        << "     --waitonmodel    Wait for newline from stdin after each model." << std::endl

        << std::endl << "Plugin Options:" << std::endl
        << " -p, --plugindir=DIR  Specify additional directory where to look for plugin" << std::endl
        << "                      libraries (additionally to the installation plugin-dir" << std::endl
        << "                      and $HOME/.dlvhex/plugins). Start with ! to reset the" << std::endl
        << "                      preset plugin paths, e.g., '!:/lib' will use only /lib/." << std::endl

        << std::endl << "Performance Tuning Options:" << std::endl
        << "     --extlearn[=none,iobehavior,monotonicity,functionality,linearity,neg,user,generalize]" << std::endl
        << "                      Learn nogoods from external atom evaluation (only useful with --solver=genuineii or --solver=genuinegi)." << std::endl
        << "                         none             : Deactivate external learning" << std::endl
        << "                         iobehavior       : Apply generic rules to learn input-output behavior" << std::endl
        << "                         monotonicity     : Apply special rules for monotonic and antimonotonic external atoms" << std::endl
        << "                                            (only useful with iobehavior)" << std::endl
        << "                         functionality    : Apply special rules for functional external atoms" << std::endl
        << "                         linearity        : Apply special rules for external atoms which are linear in all(!) predicate parameters" << std::endl
        << "                         neg              : Learn negative information" << std::endl
        << "                         user             : Apply user-defined rules for nogood learning" << std::endl
        << "                         generalize       : Generalize learned ground nogoods to nonground nogoods" << std::endl
        << "                      By default, all options except \"generalize\" are enabled." << std::endl
        << "     --supportsets    Exploits support sets for evaluation." << std::endl
        << "     --extinlining[=post,re]" << std::endl
        << "                      Inlines external sources (based on support sets). The parameter specifies the integration method:" << std::endl
        << "                         post (default)   : Integrate after grounding; non-ground support sets are fully instantiated" << std::endl
        << "                         re               : Reground using the ASP grounder if non-ground support sets were used to inline external atoms" << std::endl
        << "                      Generally, the nogood grounding method employed by \"post\" might be slower, but is applied only to support sets," << std::endl
        << "                      while the one by \"re\" is faster but needs to reground the whole program. Usually, \"post\" is faster if the number of" << std::endl
        << "                      non-ground support sets is small compared to the grounding of the program; otherwise \"re\" might be faster." << std::endl
        << "     --evalall        Evaluate all external atoms in every compatibility check, even if previous external atoms already failed." << std::endl
        << "                      This makes nogood learning more independent of the sequence of external atom checks." << std::endl
        << "                      Only useful with --extlearn." << std::endl
        << "     --nongroundnogoods" << std::endl
        << "                      Automatically instantiate learned nonground nogoods." << std::endl
        << "     --flpcheck=[explicit,ufs,ufsm,aufs,aufsm,none]" << std::endl
        << "                      Sets the strategy used to check if a candidate is a subset-minimal model of the reduct." << std::endl
        << "                         explicit         : Compute the reduct and compare its models with the candidate" << std::endl
        << "                         ufs              : Use unfounded sets for minimality checking" << std::endl
        << "                         ufsm             : Use unfounded sets for minimality checking;" << std::endl
        << "                                            do not decompose the program for UFS checking" << std::endl
        << "                         aufs (default)   : Use unfounded sets for minimality checking by exploiting assumptions" << std::endl
        << "                         aufsm            : Use unfounded sets for minimality checking by exploiting assumptions;" << std::endl
        << "                                            do not decompose the program for UFS checking" << std::endl
        << "                         none             : Disable the check" << std::endl
        << "     --ufslearnpartial" << std::endl
        << "                      Enable learning during UFS check under partial input" << std::endl
        << "                      (only useful with --flpcheck=aufs[m])." << std::endl
        << "     --flpcriterion=[all,head,e,em,emi,none]" << std::endl
        << "                      Defines the kind of cycles whose absence is exploited for skipping minimality checks." << std::endl
        << "                         all (default)    : Exploit head- and e-cycles for skipping minimality checks" << std::endl
        << "                         head             : Exploit head-cycles for skipping minimality checks" << std::endl
        << "                         e                : Exploit e-cycles for skipping minimality checks" << std::endl
        << "                         em               : Exploit e-cycles and (anti)monotonicity of predicate parameters for skipping minimality checks" << std::endl
        << "                         emi              : Exploit e-cycles, (anti)monotonicity of predicate parameters and current interpretation for skipping minimality checks" << std::endl
        << "                         none             : Do not exploit head- or e-cycles for skipping minimality checks" << std::endl
        << "     --noflpcriterion Do no apply decision criterion to skip the FLP check." << std::endl
        << "                      (equivalent to --flpcriterion=none)" << std::endl
        << "     --ufslearn=[none,reduct,ufs]" << std::endl
        << "                      Enable learning from UFS checks (only useful with --flpcheck=[a]ufs[m])." << std::endl
        << "                         none             : No learning" << std::endl
        << "                         reduct           : Learning is based on the FLP-reduct" << std::endl
        << "                         ufs (default)    : Learning is based on the unfounded set" << std::endl
        << "     --eaevalheuristics=[always,periodic,inputcomplete,eacomplete,post,never]" << std::endl
        << "                      Selects the heuristic for external atom evaluation." << std::endl
        << "                         always           : Evaluate whenever possible" << std::endl
        << "                         periodic         : Evaluate in regular intervals" << std::endl
        << "                         inputcomplete    : Evaluate whenever the input to the external atom is complete" << std::endl
        << "                         eacomplete       : Evaluate whenever all atoms relevant for the external atom are assigned" << std::endl
        << "                         post (default)   : Only evaluate at the end" << std::endl
        << "                         never            : Only evaluate at the end and also ignore custom heuristics provided by plugins" << std::endl
        << "                      Except for heuristics \"never\", custom heuristics provided by external atoms overrule the" << std::endl
        << "                      global heuristics for the particular external atom." << std::endl
        << "     --ngminimization=[always,alwaysopt,onconflict,onconflictopt]" << std::endl
        << "                      Minimize positive and negative nogoods generated by external learning." << std::endl
        << "                         always           : Try to minimize every learned nogood" << std::endl
        << "                         alwaysopt        : Like always, but use cache for answers of external atom queries" << std::endl
        << "                         onconflict       : Only minimize nogoods that are violated by the current assignment" << std::endl
        << "                         onconflictopt    : Like onconflict, but use cache for answers of external atom queries" << std::endl
        << "     --ngminimizationlimit=N" << std::endl
        << "                      Maximum size of nogoods that will be minimized" << std::endl
        << "                      (only useful with ngminimization)" << std::endl
        << "     --ufscheckheuristic=[post,max,periodic]" << std::endl
        << "                      Specifies the frequency of unfounded set checks (only useful with --flpcheck=[a]ufs[m])." << std::endl
        << "                         post (default)   : Do UFS check only over complete interpretations" << std::endl
        << "                         max              : Do UFS check as frequent as possible and over maximal subprograms" << std::endl
        << "                         periodic         : Do UFS check in periodic intervals" << std::endl
        << "     --modelqueuesize=N" << std::endl
        << "                      Size of the model queue, i.e. number of models which can be computed in parallel." << std::endl
        << "                      Default value is 5. The option is only useful for clasp solver." << std::endl
        << "     --solver=S       Use S as ASP engine, where S is one of dlv, dlvdb, libdlv, libclingo, genuineii, genuinegi, genuineic, genuinegc" << std::endl
        << "                        (genuineii=(i)nternal grounder and (i)nternal solver; genuinegi=(g)ringo grounder and (i)nternal solver" << std::endl
        << "                         genuineic=(i)nternal grounder and (c)lasp solver; genuinegc=(g)ringo grounder and (c)lasp solver)." << std::endl
        << "     --claspconfig=C  If clasp is used, configure it with C where C is parsed by clasp config parser, or " << std::endl
        << "                      C is one of the predefined strings frumpy, jumpy, handy, crafty, or trendy." << std::endl
        << " -e, --heuristics=H   Use H as evaluation heuristics, where H is one of" << std::endl
        << "                         old              : Old dlvhex behavior" << std::endl
        << "                         trivial          : Use component graph as eval graph (much overhead)" << std::endl
        << "                         easy             : Simple heuristics, used for LPNMR2011" << std::endl
        << "                         greedy (default) : Heuristics with advantages for external behavior learning" << std::endl
        << "                         monolithic       : Put entire program into one unit" << std::endl
        << "                         manual:<file>    : Read 'collapse <idxs> share <idxs>' commands from <file>" << std::endl
        << "                                            where component indices <idx> are from '--graphviz=comp'" << std::endl
        << "                         asp:<script>     : Use asp program <script> as eval heuristic" << std::endl
        << "     --forcegc        Always use the guess and check model generator." << std::endl
        << " -m, --modelbuilder=M Use M as model builder, where M is one of (online,offline)." << std::endl
        << "     --nocache        Do not cache queries to and answers from external atoms." << std::endl
        << "     --iauxinaux      Keep auxiliary input predicates in auxiliary external atom predicates (can increase or decrease efficiency)." << std::endl
        << "     --constspace     Free partial models immediately after using them. This may cause some models." << std::endl
        << "                      to be computed multiple times. (Not with monolithic.)" << std::endl
        << "     --transunitlearning" << std::endl
        << "                      Analyze inconsistent units and propagate reasons to predecessor units." << std::endl
        << "     --transunitlearningpud" << std::endl
        << "                      Use more elaborated analysis method to determine possible more inconsistent reasons." << std::endl
        << "     --transunitlearningonestep" << std::endl
        << "                      Use unoptimized solver from the beginning (slower solving, but faster inconsistency analysis)." << std::endl
        << "     --transunitlearningoneanalysistreshold" << std::endl
        << "                      Analyze inconsistency only if percentage of unit evaluations with inconsistent result exceeds this value." << std::endl
        << "     --transunitlearningminimizenogoods" << std::endl
        << "                      Minimize nogoods for inconsistency analysis (does not activate nogood minimization otherwise)." << std::endl

        << std::endl << "Debugging and General Options:" << std::endl
        << "     --dumpevalplan=F Dump evaluation plan (usable as manual heuristics) to file F." << std::endl
        << "     --dumpeanogoods=F" << std::endl
        << "                      Dump learned EA nogoods to file F." << std::endl
        << " -v, --verbose[=N]    Specify verbose category (if option is used without [=N] then default is 1):" << std::endl
        << "                         1                : Program analysis information (including dot-file)" << std::endl
        << "                         2                : Program modifications by plugins" << std::endl
        << "                         4                : Intermediate model generation info" << std::endl
        << "                         8                : Timing information" << std::endl
        << "                                           (only if configured with --enable-benchmark)" << std::endl
        << "                      add values for multiple categories." << std::endl
        << "     --dumpstats      Dump certain benchmarking results and statistics in CSV format." << std::endl
        << "                      (Only if configured with --enable-benchmark.)" << std::endl
        << "     --graphviz=G     Specify comma separated list of graph types to export as .dot files." << std::endl
        << "                      Default is none, graph types are:" << std::endl
        << "                         dep              : Dependency Graph (once per program)" << std::endl
        << "                         cycinp           : Graph for analysis cyclic predicate inputs (once per G&C-eval unit)" << std::endl
        << "                         comp             : Component Graph (once per program)" << std::endl
        << "                         eval             : Evaluation Graph (once per program)" << std::endl
        << "                         model            : Model Graph (once per program, after end of computation)" << std::endl
        << "                         imodel           : Individual Model Graph (once per model)" << std::endl
        << "                         attr             : Attribute dependency graph (once per program)" << std::endl
        << "     --version        Show version information." << std::endl;
}


void
printVersion()
{
    std::cout << PACKAGE_TARNAME << " " << VERSION << std::endl;

    std::cout << "Copyright (C) 2005-2015 Roman Schindlauer, Thomas Krennwallner, Peter Sch\303\274ller, Christoph Redl, Tobias Kaminski, Antonius Weinzierl, Thomas Eiter, Michael Fink, Giovambattista Ianni" << std::endl
        << "License LGPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/lgpl.html>" << std::endl
        << "This is free software: you are free to change and redistribute it." << std::endl
        << "There is NO WARRANTY, to the extent permitted by law." << std::endl;

    std::cout << std::endl;

    std::cout << "Homepage: http://www.kr.tuwien.ac.at/research/systems/dlvhex/" << std::endl
        << "Support: dlvhex-devel@lists.sourceforge.net" << std::endl
        << "Bug reports: http://github.com/hexhex/core/issues/" << std::endl;

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

namespace
{
    ProgramCtx* exeCtx = NULL;
}


void signal_handler(int signum)
{
    // perform benchmarking shutdown to obtain benchmark output
    #ifdef WIN32
    LOG(ERROR,"dlvhex2 with pid " << GetCurrentProcessId() << " got termination signal" << signum << "!");
    #else
    LOG(ERROR,"dlvhex2 with pid " << getpid() << " got termination signal " << signum << "!");
    #endif

    benchmark::BenchmarkController::finish();

    // hard exit
    // (otherwise ctrl+c does not work for many situations, which is annoying!)
    exit(2);
}


int main(int argc, char *argv[])
{
    const char* whoAmI = argv[0];

    // pre-init logger
    // (we use more than 4 bits -> two digit loglevel)
    Logger::Instance().setPrintLevelWidth(2);

    // program context
    ProgramCtx pctx;
    exeCtx = &pctx;
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
    #if defined(HAVE_LIBGRINGO) && defined(HAVE_LIBCLASP)
    #else
    #ifndef WIN32
    #error no asp software configured! configure.ac should not allow this to happen!
    #endif
    #endif
    #endif
    #endif
    #endif
    #endif

    // default eval heuristic = "greedy" heuristic
    pctx.evalHeuristic.reset(new EvalHeuristicGreedy);
    // default model builder = "online" model builder
    pctx.modelBuilderFactory = boost::factory<OnlineModelBuilder<FinalEvalGraph>*>();

    // defaults of main
    Config config;

    // if we throw UsageError inside this, error and usage will be displayed, otherwise only error
    int returnCode = 1;
    try
    {
        // default logging priority = errors + warnings
        Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING);

        // manage options we can already manage
        // TODO use boost::program_options
        processOptionsPrePlugin(argc, argv, config, pctx);

        #ifdef HAVE_PYTHON
        PythonPlugin* pythonPlugin = new PythonPlugin();
        #endif

        // initialize internal plugins
        {
            PluginInterfacePtr manualEvalHeuristicsPlugin(new ManualEvalHeuristicsPlugin);
            pctx.pluginContainer()->addInternalPlugin(manualEvalHeuristicsPlugin);
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
            PluginInterfacePtr functionPlugin(new FunctionPlugin);
            pctx.pluginContainer()->addInternalPlugin(functionPlugin);
            PluginInterfacePtr choicePlugin(new ChoicePlugin);
            pctx.pluginContainer()->addInternalPlugin(choicePlugin);
            PluginInterfacePtr conditionalLiteralPlugin(new ConditionalLiteralPlugin);
            pctx.pluginContainer()->addInternalPlugin(conditionalLiteralPlugin);
            #ifdef HAVE_PYTHON
            PluginInterfacePtr _pythonPlugin(pythonPlugin);
            pctx.pluginContainer()->addInternalPlugin(_pythonPlugin);
            #endif
        }

        // before anything else we dump the logo
        if( !pctx.config.getOption("Silent") )
            printLogo();

        // initialize benchmarking (--verbose=8) with scope exit
        // (this cannot be outsourced due to the scope)
        benchmark::BenchmarkController& ctr =
            benchmark::BenchmarkController::Instance();
        if( pctx.config.doVerbose(dlvhex::Configuration::PROFILING) ) {
            LOG(INFO,"initializing benchmarking output");
            ctr.setOutput(&Logger::Instance().stream());
            // for continuous statistics output, display every 1000'th output
            ctr.setPrintInterval(999);
        }
        else
            ctr.setOutput(0);

        // also deconstruct & output at SIGTERM/SIGINT
        {
            if( SIG_ERR == signal(SIGTERM, signal_handler) )
                LOG(WARNING,"setting SIGTERM handler terminated with error '" << strerror(errno));
            if( SIG_ERR == signal(SIGINT, signal_handler) )
                LOG(WARNING,"setting SIINT handler terminated with error '" << strerror(errno));
        }

        // startup statemachine
        pctx.changeState(StatePtr(new ShowPluginsState));

        // load plugins
        {
            DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"loading plugins");
            pctx.pluginContainer()->loadPlugins(config.optionPlugindir);
            pctx.showPlugins();
        }

        // now we may offer help, including plugin help
        if( config.helpRequested ) {
            printUsage(std::cerr, whoAmI, true);
            pctx.pluginContainer()->printUsage(std::cerr);
            return 1;
        }

        // process plugin options using plugins
        // (this deletes processed options from config.pluginOptions)
        // TODO use boost::program_options
        pctx.processPluginOptions(config.pluginOptions);
        if( pctx.terminationRequest ) return 1;

        // handle options not recognized by dlvhex and not by plugins
        if( !config.pluginOptions.empty() ) {
            std::stringstream bad;
            bad << "Unknown option(s):";
            BOOST_FOREACH(const char* opt, config.pluginOptions) {
                bad << " " << opt;
            }
            throw UsageError(bad.str());
        }
        // use configured plugins to obtain plugin atoms
        pctx.addPluginAtomsFromPluginContainer();

        // check if this plugin provides a custom model generator
        BOOST_FOREACH(PluginInterfacePtr plugin, pctx.pluginContainer()->getPlugins()) {
            if (plugin->providesCustomModelGeneratorFactory(pctx)) {
                if (pctx.customModelGeneratorProvider == 0) {
                    LOG(DBG, "Plugin provides custom model generator factory");
                    pctx.customModelGeneratorProvider = plugin;
                }
                else {
                    throw PluginError("Multiple plugins prove alternative model generator factories. Do not know which one to use. Please change command-line options.");
                }
            }
        }

        #ifdef HAVE_PYTHON
        if (pctx.config.getOption("HavePythonMain")) {
            pythonPlugin->runPythonMain(pctx.config.getStringOption("PythonMain"));
            // display benchmark output
            benchmark::BenchmarkController::finish();
            return 1;
        }
        #endif

        // now we check if we got input
        if( !pctx.inputProvider || !pctx.inputProvider->hasContent() )
            throw UsageError("no input specified!");

        // convert input (only done if at least one plugin provides a converter)
        pctx.convert();
        if( pctx.terminationRequest ) return 1;

        // parse input (coming directly from inputprovider or from inputprovider provided by the convert() step)
        pctx.parse();
        if( pctx.terminationRequest ) return 1;

        // check if in mlp mode
        if( pctx.config.getOption("MLP") ) {
            // syntax check for mlp
            pctx.moduleSyntaxCheck();
            if( pctx.terminationRequest ) return 1;

            // solve mlp
            pctx.mlpSolver();
            if( pctx.terminationRequest ) return 1;
        }

        else {

            // associate PluginAtom instances with
            // ExternalAtom instances (in the IDB)
            pctx.associateExtAtomsWithPluginAtoms(pctx.idb, true);
            if( pctx.terminationRequest ) return 1;

            // rewrite program (plugins might want to do this, e.g., for partial grounding)
            pctx.rewriteEDBIDB();
            if( pctx.terminationRequest ) return 1;

            // associate PluginAtom instances with
            // ExternalAtom instances (in the IDB)
            // (again, rewrite might add external atoms)
            pctx.associateExtAtomsWithPluginAtoms(pctx.idb, true);

            // check weak safety
            pctx.safetyCheck();
            if( pctx.terminationRequest ) return 1;

            // check liberal safety
            pctx.liberalSafetyCheck();
            if( pctx.terminationRequest ) return 1;

            // create dependency graph (we need the previous step for this)
            pctx.createDependencyGraph();
            if( pctx.terminationRequest ) return 1;

            // optimize dependency graph (plugins might want to do this, e.g. by using domain information)
            pctx.optimizeEDBDependencyGraph();
            if( pctx.terminationRequest ) return 1;
            // everything in the following will be done using the dependency graph and EDB
            WARNING("IDB and dependencygraph could get out of sync! should we lock or empty the IDB to ensure that it is not directly used anymore after this step?")

            // create graph of strongly connected components of dependency graph
                pctx.createComponentGraph();
            if( pctx.terminationRequest ) return 1;

            // use SCCs to do strong safety check
            if( !pctx.config.getOption("SkipStrongSafetyCheck") ) {
                pctx.strongSafetyCheck();
                if( pctx.terminationRequest ) return 1;
            }

            // select heuristics and create eval graph
            pctx.createEvalGraph();
            if( pctx.terminationRequest ) return 1;

            // stop here if no evaluation was requested
            if( config.optionNoEval )
                return 0;

            // setup model builder and configure plugin/dlvhex model processing hooks
            pctx.setupProgramCtx();
            if( pctx.terminationRequest ) return 1;

            // evaluate (generally done in streaming mode, may exit early if indicated by hooks)
            // (individual model output should happen here)
            pctx.evaluate();
            if( pctx.terminationRequest ) return 1;

        }                        // end if (mlp) else ...

        // finalization plugin/dlvhex hooks (for accumulating model processing)
        // (accumulated model output/query answering should happen here)
        pctx.postProcess();

        // no error
        returnCode = 0;
    }
    catch(const UsageError &ue) {
        std::cerr << "UsageError: " << ue.getErrorMsg() << std::endl << std::endl;
        printUsage(std::cerr, whoAmI, true);
        if( !!pctx.pluginContainer() )
            pctx.pluginContainer()->printUsage(std::cerr);
    }
    catch(const GeneralError &ge) {
        pctx.modelBuilder.reset();
        std::cerr << "GeneralError: " << ge.getErrorMsg() << std::endl << std::endl;
    }
    catch(const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl << std::endl;
    }

    // display benchmark output
    benchmark::BenchmarkController::finish();

    // regular exit
    return returnCode;
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

    static const char* shortopts = "hsvf:p:are:m:n:N:";
    static struct option longopts[] = {
        { "help", no_argument, 0, 'h' },
        { "silent", no_argument, 0, 's' },
        { "verbose", optional_argument, 0, 'v' },
        { "filter", required_argument, 0, 'f' },
        { "plugindir", required_argument, 0, 'p' },
        { "reverse", no_argument, 0, 'r' },
        { "heuristics", required_argument, 0, 'e' },
        { "modelbuilder", required_argument, 0, 'm' },
        { "number", required_argument, 0, 'n' },
        { "maxint", required_argument, 0, 'N' },
        { "weaksafety", no_argument, &longid, 2 },
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
        { "evalall", no_argument, 0, 19 },
        { "flpcheck", required_argument, 0, 20 },
        { "ufslearn", optional_argument, 0, 23 },
        { "noflpcriterion", no_argument, 0, 35 },
        { "flpcriterion", optional_argument, 0, 42 },
        { "eaevalheuristic", required_argument, 0, 26 },
        {                        // compatibility
            "eaevalheuristics", required_argument, 0, 26
        },
        { "ufscheckheuristic", required_argument, 0, 27 },
        {                        // compatibility
            "ufscheckheuristics", required_argument, 0, 27
        },
        {                        // perhaps only temporary
            "benchmarkeastderr", no_argument, 0, 28
        },
        {                        // perhaps only temporary
            "explicitflpunshift", no_argument, 0, 29
        },
        {                        // perhaps only temporary
            "printlearnednogoodsstderr", no_argument, 0, 30
        },
        { "nongroundnogoods", no_argument, 0, 31 },
        { "modelqueuesize", required_argument, 0, 32 },
        { "liberalsafety", no_argument, 0, 33 },
        {                        // perhaps only temporary
            "claspconfig", required_argument, 0, 36
        },
        { "noclaspincremental", no_argument, 0, 43 },
        { "claspsingletonloopnogoods", no_argument, 0, 44 },
        { "claspinverseliterals", no_argument, 0, 45 },
        { "dumpstats", no_argument, 0, 37 },
        { "iauxinaux", optional_argument, 0, 38 },
        { "legacyecycledetection", no_argument, 0, 46 },
        { "constspace", no_argument, 0, 39 },
        { "forcesinglethreading", no_argument, 0, 40 },
        { "lazyufscheckerinitialization", no_argument, 0, 47 },
        { "supportsets", no_argument, 0, 48 },
        { "forcegc", no_argument, 0, 49 },
        { "incremental", no_argument, 0, 50 },
        { "strongsafety", no_argument, 0, 52 },
		{ "optmode", required_argument, 0, 54 },
		{ "claspdefernprop", required_argument, 0, 55 },
		{ "claspdefermsec", required_argument, 0, 56 },
        { "dumpeanogoods", required_argument, 0, 57 },
        { "ngminimization", required_argument, 0, 58 },
        { "ngminimizationlimit", required_argument, 0, 59 },
        { "csvinput", required_argument, 0, 60 },
        { "csvoutput", required_argument, 0, 61 },
        { "noouterexternalatoms", no_argument, 0, 62 },
        { "transunitlearning", no_argument, 0, 64 },
        { "transunitlearningpud", no_argument, 0, 68 },
        { "transunitlearningonestep", no_argument, 0, 69 },
        { "transunitlearningdumpnogoods", no_argument, 0, 70 },
        { "transunitlearninganalysistreshold", required_argument, 0, 71 },
        { "transunitlearningminimizenogoods", no_argument, 0, 72 },
        { "verifyfromlearned", no_argument, 0, 65 },
        { "waitonmodel", no_argument, 0, 66 },
        { "extinlining", optional_argument, 0, 67 },
        { "ufslearnpartial", no_argument, 0, 73 },
        { NULL, 0, NULL, 0 }
    };

    // default settings
    pctx.config.setOption("NoPropagator", 0);
    pctx.defaultExternalAtomEvaluationHeuristicsFactory.reset(new ExternalAtomEvaluationHeuristicsNeverFactory());
    pctx.unfoundedSetCheckHeuristicsFactory.reset(new UnfoundedSetCheckHeuristicsPostFactory());

    // start with new input provider
    pctx.inputProvider.reset(new InputProvider);

    bool specifiedModelQueueSize = false;
    bool defiaux = false;
    bool iaux = false;
    bool heuristicChosen = false;
    bool heuristicMonolithic = false;
    bool solverSet = false;
    bool forceoptmode = false;
    while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (ch) {
            case 'h':
                config.helpRequested = 1;
                break;

            case 's':
                pctx.config.setOption("Silent", 1);
                break;

            case 'v':
                if (optarg) {
                    int level = 1;
                    try
                    {
                        level = boost::lexical_cast<int>(optarg);
                    }
                    catch(const boost::bad_lexical_cast&) {
                        LOG(ERROR,"could not parse verbosity level '" << optarg << "' - using default=" << level << "!");
                    }
                    pctx.config.setOption("Verbose", level);
                    Logger::Instance().setPrintLevels(level);
                }
                else {
                    pctx.config.setOption("Verbose", 1);
                    Logger::Instance().setPrintLevels(Logger::ERROR | Logger::WARNING | Logger::INFO);
                }
                break;

            case 'f':
            {
                boost::char_separator<char> sep(",");
                                 // g++ 3.3 is unable to pass that at the ctor line below
                std::string oa(optarg);
                boost::tokenizer<boost::char_separator<char> > tok(oa, sep);

                for(boost::tokenizer<boost::char_separator<char> >::const_iterator f = tok.begin();
                    f != tok.end(); ++f)
                pctx.config.addFilter(*f);
            }
            break;

            case 'p':
                config.optionPlugindir = std::string(optarg);
                pctx.config.setStringOption("PluginDirs", std::string(optarg));
                break;

            case 'r':
                pctx.config.setOption("ReverseOrder", 1);
                break;

            case 'e':
                // heuristics={old,trivial,easy,manual:>filename>}
                {
                    heuristicChosen = true;
                    std::string heuri(optarg);
                    if( heuri == "old" ) {
                        pctx.evalHeuristic.reset(new EvalHeuristicOldDlvhex);
                    }
                    else if( heuri == "trivial" ) {
                        pctx.evalHeuristic.reset(new EvalHeuristicTrivial);
                    }
                    else if( heuri == "easy" ) {
                        pctx.evalHeuristic.reset(new EvalHeuristicEasy);
                    }
                    else if( heuri == "greedy" ) {
                        pctx.evalHeuristic.reset(new EvalHeuristicGreedy);
                    }
                    else if( heuri == "monolithic" ) {
                        pctx.evalHeuristic.reset(new EvalHeuristicMonolithic);
                        heuristicMonolithic = true;
                    }
                    else if( heuri.substr(0,7) == "manual:" ) {
                        pctx.evalHeuristic.reset(new EvalHeuristicFromFile(heuri.substr(7)));
                    }
                    else if( heuri.substr(0,4) == "asp:" ) {
                        pctx.evalHeuristic.reset(new EvalHeuristicASP(heuri.substr(4)));
                    }
                    else {
                        throw UsageError("unknown evaluation heuristic '" + heuri +"' specified!");
                    }
                    LOG(INFO,"selected '" << heuri << "' evaluation heuristics");
                }
                break;

            case 'm':
                // modelbuilder={offline,online}
                {
                    std::string modelbuilder(optarg);
                    if( modelbuilder == "offline" ) {
                        pctx.modelBuilderFactory =
                            boost::factory<OfflineModelBuilder<FinalEvalGraph>*>();
                    }
                    else if( modelbuilder == "online" ) {
                        pctx.modelBuilderFactory =
                            boost::factory<OnlineModelBuilder<FinalEvalGraph>*>();
                    }
                    else {
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
                catch(const boost::bad_lexical_cast&) {
                    LOG(ERROR,"could not parse model count '" << optarg << "' - using default=" << models << "!");
                }
                pctx.config.setOption("NumberOfModels", models);
            }
            break;

            case 'N':
            {
                int maxint = 0;
                try
                {
                    if( optarg[0] == '=' )
                        maxint = boost::lexical_cast<unsigned>(&optarg[1]);
                    else
                        maxint = boost::lexical_cast<unsigned>(optarg);
                }
                catch(const boost::bad_lexical_cast&) {
                    LOG(ERROR,"could not parse maxint '" << optarg << "' - using default=" << maxint << "!");
                }
                pctx.maxint = maxint;
            }
            break;

            case 0:
                switch (longid) {
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
                        if( solver == "dlv" ) {
                        #if defined(HAVE_DLV)
                            pctx.setASPSoftware(
                                ASPSolverManager::SoftwareConfigurationPtr(new ASPSolver::DLVSoftware::Configuration));
                            pctx.config.setOption("GenuineSolver", 0);
                        #else
                            throw GeneralError("sorry, no support for solver backend '"+solver+"' compiled into this binary");
                        #endif
                        }
                        else if( solver == "dlvdb" ) {
                        #if defined(HAVE_DLVDB)
                            WARNING("reactivate dlvhdb")
                            //pctx.setASPSoftware(
                            //	ASPSolverManager::SoftwareConfigurationPtr(new ASPSolver::DLVDBSoftware::Configuration));
                                pctx.config.setOption("GenuineSolver", 0);
                        #else
                            throw GeneralError("sorry, no support for solver backend '"+solver+"' compiled into this binary");
                        #endif
                        }
                        else if( solver == "libdlv" ) {
                        #if defined(HAVE_LIBDLV)
                            pctx.setASPSoftware(
                                ASPSolverManager::SoftwareConfigurationPtr(new ASPSolver::DLVLibSoftware::Configuration));
                            pctx.config.setOption("GenuineSolver", 0);
                        #else
                            throw GeneralError("sorry, no support for solver backend '"+solver+"' compiled into this binary");
                        #endif
                        }
                        else if( solver == "libclingo" ) {
                        #if defined(HAVE_LIBCLINGO)
                            pctx.setASPSoftware(
                                ASPSolverManager::SoftwareConfigurationPtr(new ASPSolver::ClingoSoftware::Configuration));
                            pctx.config.setOption("GenuineSolver", 0);
                        #else
                            throw GeneralError("sorry, no support for solver backend '"+solver+"' compiled into this binary");
                        #endif
                        }
                        else if( solver == "genuineii" ) {
                            pctx.config.setOption("GenuineSolver", 1);
                        }
                        else if( solver == "genuinegi" ) {
                        #if defined(HAVE_LIBGRINGO)
                            pctx.config.setOption("GenuineSolver", 2);
                        #else
                            throw GeneralError("sorry, no support for solver backend '"+solver+"' compiled into this binary");
                        #endif
                        }
                        else if( solver == "genuineic" ) {
                        #if defined(HAVE_LIBCLASP)
                            pctx.config.setOption("GenuineSolver", 3);
                        #else
                            throw GeneralError("sorry, no support for solver backend '"+solver+"' compiled into this binary");
                        #endif
                        }
                        else if( solver == "genuinegc" ) {
                        #if defined(HAVE_LIBGRINGO) && defined(HAVE_LIBCLASP)
                            pctx.config.setOption("GenuineSolver", 4);
                        #else
                            throw GeneralError("sorry, no support for solver backend '"+solver+"' compiled into this binary");
                        #endif
                        }
                        else {
                            throw UsageError("unknown solver backend '" + solver +"' specified!");
                        }
                        LOG(INFO,"selected '" << solver << "' solver backend");
                        solverSet = true;
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
                                 // g++ 3.3 is unable to pass that at the ctor line below
                        std::string oa(optarg);
                        boost::tokenizer<boost::char_separator<char> > tok(oa, sep);

                        for(boost::tokenizer<boost::char_separator<char> >::const_iterator f = tok.begin();
                        f != tok.end(); ++f) {
                            const std::string& token = *f;
                            if( token == "dep" ) {
                                pctx.config.setOption("DumpDepGraph",1);
                            }
                            else if( token == "cycinp" ) {
                                pctx.config.setOption("DumpCyclicPredicateInputAnalysisGraph",1);
                            }
                            else if( token == "comp" ) {
                                pctx.config.setOption("DumpCompGraph",1);
                            }
                            else if( token == "eval" ) {
                                pctx.config.setOption("DumpEvalGraph",1);
                            }
                            else if( token == "model" ) {
                                pctx.config.setOption("DumpModelGraph",1);
                                throw std::runtime_error("DumpModelGraph not implemented!");
                            }
                            else if( token == "imodel" ) {
                                pctx.config.setOption("DumpIModelGraph",1);
                                throw std::runtime_error("DumpIModelGraph not implemented!");
                            }
                            else if( token == "attr" ) {
                                pctx.config.setOption("DumpAttrGraph",1);
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
                // overwrite default settings: assume that nothing is enabled
                pctx.config.setOption("ExternalLearningIOBehavior", 0);
                pctx.config.setOption("ExternalLearningMonotonicity", 0);
                pctx.config.setOption("ExternalLearningFunctionality", 0);
                pctx.config.setOption("ExternalLearningLinearity", 0);
                pctx.config.setOption("ExternalLearningNeg", 0);
                pctx.config.setOption("ExternalLearningUser", 0);

                bool noneToken = false;
                bool enableToken = false;
                if (optarg) {
                    boost::char_separator<char> sep(",");
                                 // g++ 3.3 is unable to pass that at the ctor line below
                    std::string oa(optarg);
                    boost::tokenizer<boost::char_separator<char> > tok(oa, sep);

                    for(boost::tokenizer<boost::char_separator<char> >::const_iterator f = tok.begin();
                    f != tok.end(); ++f) {
                        const std::string& token = *f;
                        if (token == "none" ) {
                            noneToken = true;
                        }
                        else if (token == "iobehavior" ) {
                            pctx.config.setOption("ExternalLearningIOBehavior", 1);
                            enableToken = true;
                        }
                        else if( token == "monotonicity" ) {
                            pctx.config.setOption("ExternalLearningMonotonicity", 1);
                            enableToken = true;
                        }
                        else if( token == "functionality" ) {
                            pctx.config.setOption("ExternalLearningFunctionality", 1);
                            enableToken = true;
                        }
                        else if( token == "linearity" ) {
                            pctx.config.setOption("ExternalLearningLinearity", 1);
                            enableToken = true;
                        }
                        else if( token == "neg" ) {
                            pctx.config.setOption("ExternalLearningNeg", 1);
                            enableToken = true;
                        }
                        else if( token == "user" ) {
                            pctx.config.setOption("ExternalLearningUser", 1);
                            enableToken = true;
                        }
                        else if( token == "generalize" ) {
                            pctx.config.setOption("ExternalLearningGeneralize", 1);
                            enableToken = true;
                        }
                        else {
                            throw GeneralError("Unknown learning option: \"" + token + "\"");
                        }
                    }
                }
                else {
                    // by default, turn on all external learning rules
                    pctx.config.setOption("ExternalLearningIOBehavior", 1);
                    pctx.config.setOption("ExternalLearningMonotonicity", 1);
                    pctx.config.setOption("ExternalLearningFunctionality", 1);
                    pctx.config.setOption("ExternalLearningLinearity", 1);
                    pctx.config.setOption("ExternalLearningNeg", 1);
                    pctx.config.setOption("ExternalLearningUser", 1);
                    //pctx.config.setOption("ExternalLearningGeneralize", 1);	// do not activate by default (it is mostly counterproductive)
                }
                if (noneToken && enableToken) {
                    throw GeneralError("--extlearn: Value \"none\" cannot be used simultanously with other options");
                }

                pctx.config.setOption("ExternalLearning", noneToken ? 0 : 1);
            }

            LOG(DBG, "External learning: " << pctx.config.getOption("ExternalLearning") << " [iobehavior: " << pctx.config.getOption("ExternalLearningIOBehavior") << " [monotonicity: " << pctx.config.getOption("ExternalLearningMonotonicity") << ", functionlity: " << pctx.config.getOption("ExternalLearningFunctionality") << ", linearity: " << pctx.config.getOption("ExternalLearningLinearity") << ", user-defined: " << pctx.config.getOption("ExternalLearningUser") << "]");
            break;

            case 19:

                {
                    pctx.config.setOption("AlwaysEvaluateAllExternalAtoms", 1);
                    break;
                }

            case 20:
            {
                std::string check(optarg);
                if( check == "explicit" ) {
                    pctx.config.setOption("FLPCheck", 1);
                    pctx.config.setOption("UFSCheck", 0);
                }else if( check == "ufs" )
                {
                    pctx.config.setOption("FLPCheck", 0);
                    pctx.config.setOption("UFSCheck", 1);
                    pctx.config.setOption("UFSCheckMonolithic", 0);
                    pctx.config.setOption("UFSCheckAssumptionBased", 0);
                }else if( check == "ufsm" )
                {
                    pctx.config.setOption("FLPCheck", 0);
                    pctx.config.setOption("UFSCheck", 1);
                    pctx.config.setOption("UFSCheckMonolithic", 1);
                    pctx.config.setOption("UFSCheckAssumptionBased", 0);

                }else if( check == "aufs" )
                {
                    pctx.config.setOption("FLPCheck", 0);
                    pctx.config.setOption("UFSCheck", 1);
                    pctx.config.setOption("UFSCheckMonolithic", 0);
                    pctx.config.setOption("UFSCheckAssumptionBased", 1);
                }else if( check == "aufsm" )
                {
                    pctx.config.setOption("FLPCheck", 0);
                    pctx.config.setOption("UFSCheck", 1);
                    pctx.config.setOption("UFSCheckMonolithic", 1);
                    pctx.config.setOption("UFSCheckAssumptionBased", 1);
                }else if( check == "none" )
                {
                    pctx.config.setOption("FLPCheck", 0);
                    pctx.config.setOption("UFSCheck", 0);
                }
                else {
                    throw GeneralError("Invalid FLP check option: \"" + check + "\"");
                }

                LOG(INFO,"FLP Check: " << pctx.config.getOption("FLPCheck") << "; UFS Check: " << pctx.config.getOption("UFSCheck"));
            }

            break;

            case 23:
                if (!optarg) {
                    // use UFS-based learning
                    pctx.config.setOption("UFSLearning", 1);
                    pctx.config.setOption("UFSLearnStrategy", 2);
                }
                else {
                    std::string learn(optarg);
                    if (learn == "none") {
                        pctx.config.setOption("UFSLearning", 0);
                    }
                    else if (learn == "reduct") {
                        pctx.config.setOption("UFSLearning", 1);
                        pctx.config.setOption("UFSLearnStrategy", 1);
                    }
                    else if (learn == "ufs") {
                        pctx.config.setOption("UFSLearning", 1);
                        pctx.config.setOption("UFSLearnStrategy", 2);
                    }
                    else {
                        throw GeneralError("Unknown UFS Learning option: \"" + learn + "\"");
                    }
                }
                break;

            case 26:
            {
                std::string heur(optarg);
                if (heur == "always") {
                    pctx.defaultExternalAtomEvaluationHeuristicsFactory.reset(new ExternalAtomEvaluationHeuristicsAlwaysFactory());
                    pctx.config.setOption("NoPropagator", 0);
                }
               else  if (heur == "periodic") {
                    pctx.defaultExternalAtomEvaluationHeuristicsFactory.reset(new ExternalAtomEvaluationHeuristicsPeriodicFactory());
                    pctx.config.setOption("NoPropagator", 0);
                }
                else if (heur == "inputcomplete") {
                    pctx.defaultExternalAtomEvaluationHeuristicsFactory.reset(new ExternalAtomEvaluationHeuristicsInputCompleteFactory());
                    pctx.config.setOption("NoPropagator", 0);
                }
                else if (heur == "eacomplete") {
                    pctx.defaultExternalAtomEvaluationHeuristicsFactory.reset(new ExternalAtomEvaluationHeuristicsEACompleteFactory());
                    pctx.config.setOption("NoPropagator", 0);
                }
                else if (heur == "post") {
                    // here we evaluate only after the model candidate has been completed
                    pctx.defaultExternalAtomEvaluationHeuristicsFactory.reset(new ExternalAtomEvaluationHeuristicsNeverFactory());
                    pctx.config.setOption("NoPropagator", 0);
                }
                else if (heur == "never") {
                    // here we evaluate only after the model candidate has been completed
                    // differently from post, even the propagator is diabled, thus also custom heuristics provided by external atoms cannot overwrite this behavior
                    pctx.defaultExternalAtomEvaluationHeuristicsFactory.reset(new ExternalAtomEvaluationHeuristicsNeverFactory());
                    pctx.config.setOption("NoPropagator", 1);
                }
                else {
                    throw GeneralError(std::string("Unknown external atom evaluation heuristic: \"") + heur + std::string("\""));
                }
            }
            break;

            case 27:
            {
                std::string heur(optarg);
                if (heur == "post") {
                    pctx.unfoundedSetCheckHeuristicsFactory.reset(new UnfoundedSetCheckHeuristicsPostFactory());
                    pctx.config.setOption("UFSCheckHeuristics", 0);
                }
                else if (heur == "max") {
                    pctx.unfoundedSetCheckHeuristicsFactory.reset(new UnfoundedSetCheckHeuristicsMaxFactory());
                    pctx.config.setOption("UFSCheckHeuristics", 1);
                }
                else if (heur == "periodic") {
                    pctx.unfoundedSetCheckHeuristicsFactory.reset(new UnfoundedSetCheckHeuristicsPeriodicFactory());
                    pctx.config.setOption("UFSCheckHeuristics", 2);
                }
                else {
                    throw GeneralError(std::string("Unknown UFS check heuristic: \"") + heur + std::string("\""));
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
                catch(const boost::bad_lexical_cast&) {
                    LOG(ERROR,"could not parse size of model queue '" << optarg << "' - using default=" << queuesize << "!");
                }
                if (queuesize < 1) {
                    throw GeneralError(std::string("Model queue size must be > 0"));
                }
                pctx.config.setOption("ModelQueueSize", queuesize); break;
                specifiedModelQueueSize = true;
            }
            break;

            case 33:
                pctx.config.setOption("LiberalSafety", 1);
                break;

            case 35:
                pctx.config.setOption("FLPDecisionCriterionHead", 0);
                pctx.config.setOption("FLPDecisionCriterionE", 0);
                break;

            case 36: pctx.config.setStringOption("ClaspConfiguration",std::string(optarg)); break;

            case 37:
                pctx.config.setOption("DumpStats",1);
            #if !defined(DLVHEX_BENCHMARK)
                throw std::runtime_error("you can only use --dumpstats if you configured with --enable-benchmark");
            #endif
                break;

            case 38:
                defiaux = true;

                if (!optarg) {
                    iaux = true;
                }
                else {
                    if (std::string(optarg) == "true") iaux = true;
                    else if (std::string(optarg) == "false") iaux = false;
                    else throw GeneralError("Unknown option \"" + std::string(optarg) + "\" for iauxinaux");
                }
                break;

            case 39:
                pctx.config.setOption("UseConstantSpace",1);
                break;

            case '?':
                config.pluginOptions.push_back(argv[optind - 1]);
                break;
            case 40:
                pctx.config.setOption("ClaspForceSingleThreaded", 1);
                break;

            case 42:
                if (optarg) {
                    std::string cycle(optarg);
                    if (cycle == "all") {
                        pctx.config.setOption("FLPDecisionCriterionE", 1);
                        pctx.config.setOption("FLPDecisionCriterionEM", 1);
                        pctx.config.setOption("FLPDecisionCriterionEMI", 1);
                        pctx.config.setOption("FLPDecisionCriterionHead", 1);
                    }
                    else if (cycle == "head") {
                        pctx.config.setOption("FLPDecisionCriterionE", 0);
                        pctx.config.setOption("FLPDecisionCriterionEM", 0);
                        pctx.config.setOption("FLPDecisionCriterionEMI", 0);
                        pctx.config.setOption("FLPDecisionCriterionHead", 1);
                    }
                    else if (cycle == "e") {
                        pctx.config.setOption("FLPDecisionCriterionE", 1);
                        pctx.config.setOption("FLPDecisionCriterionEM", 0);
                        pctx.config.setOption("FLPDecisionCriterionEMI", 0);
                        pctx.config.setOption("FLPDecisionCriterionHead", 0);
                    }
                    else if (cycle == "em") {
                        pctx.config.setOption("FLPDecisionCriterionE", 1);
                        pctx.config.setOption("FLPDecisionCriterionEM", 1);
                        pctx.config.setOption("FLPDecisionCriterionEMI", 0);
                        pctx.config.setOption("FLPDecisionCriterionHead", 0);
                    }
                    else if (cycle == "emi") {
                        pctx.config.setOption("FLPDecisionCriterionE", 1);
                        pctx.config.setOption("FLPDecisionCriterionEM", 1);
                        pctx.config.setOption("FLPDecisionCriterionEMI", 1);
                        pctx.config.setOption("FLPDecisionCriterionHead", 0);
                    }
                    else if (cycle == "none") {
                        pctx.config.setOption("FLPDecisionCriterionE", 0);
                        pctx.config.setOption("FLPDecisionCriterionEM", 0);
                        pctx.config.setOption("FLPDecisionCriterionEMI", 0);
                        pctx.config.setOption("FLPDecisionCriterionHead", 0);
                    }
                    else {
                        throw GeneralError(std::string("Unknown cycle type: \"" + cycle + "\""));
                    }
                }
                else {
                    pctx.config.setOption("FLPDecisionCriterionE", 1);
                    pctx.config.setOption("FLPDecisionCriterionHead", 1);
                }
                break;
            case 43:
                pctx.config.setOption("ClaspIncrementalInterpretationExtraction", 0);
                break;
            case 44:
                pctx.config.setOption("ClaspSingletonLoopNogoods", 0);
                break;
            case 45:
                pctx.config.setOption("ClaspInverseLiterals", 1);
                break;
            case 46:
                pctx.config.setOption("LegacyECycleDetection", 1);
                break;
            case 47:
                pctx.config.setOption("LazyUFSCheckerInitialization", 1);
                break;
            case 48:
                pctx.config.setOption("SupportSets", 1);
                pctx.config.setOption("ExternalLearningUser", 1);
                pctx.config.setOption("ExternalLearning", 1);
                //	pctx.config.setOption("ForceGC", 1);
                pctx.config.setOption("LiberalSafety", 1);
                break;
            case 49:
                pctx.config.setOption("ForceGC", 1);
                pctx.config.setOption("LiberalSafety", 1);
                break;
            case 50:
                pctx.config.setOption("IncrementalGrounding", 1);
                break;
            case 52:
                pctx.config.setOption("LiberalSafety", 0);
                break;
            case 54:
                {
                    int optmode = 0;
                    try
                    {
                        if( optarg[0] == '=' )
                            optmode = boost::lexical_cast<unsigned>(&optarg[1]);
                        else
                            optmode = boost::lexical_cast<unsigned>(optarg);
                    }
                    catch(const boost::bad_lexical_cast&) {
                        LOG(ERROR,"could not parse optmode '" << optarg << "' - using default");
                    }
                    pctx.config.setOption("OptimizationTwoStep", optmode);
                    forceoptmode = true;
                }
                break;
            case 55:
                {
                    int deferval = 0;
                    try
                    {
                        if( optarg[0] == '=' )
                            deferval = boost::lexical_cast<unsigned>(&optarg[1]);
                        else
                            deferval = boost::lexical_cast<unsigned>(optarg);
                    }
                    catch(const boost::bad_lexical_cast&) {
                        LOG(ERROR,"claspdefernprop '" << optarg << "' does not specify an integer value");
                    }
                    pctx.config.setOption("ClaspDeferNPropagations", deferval);
                }
                break;
            case 56:
                {
                    int deferval = 0;
                    try
                    {
                        if( optarg[0] == '=' )
                            deferval = boost::lexical_cast<unsigned>(&optarg[1]);
                        else
                            deferval = boost::lexical_cast<unsigned>(optarg);
                    }
                    catch(const boost::bad_lexical_cast&) {
                        LOG(ERROR,"claspdefermsec '" << optarg << "' does not specify an integer value");
                    }
                    pctx.config.setOption("ClaspDeferMaxTMilliseconds", deferval);
                }
                break;
            case 57:
                {
                    pctx.config.setStringOption("DumpEANogoods", optarg);
                    std::ofstream filev(pctx.config.getStringOption("DumpEANogoods").c_str(), std::ios_base::out);
                }
                break;
            case 58:
                {
                    std::string heur(optarg);
                    if (heur == "always") {
                        pctx.config.setOption("MinimizeNogoods", 1);
                    }
                    else if (heur == "onconflict") {
                        pctx.config.setOption("MinimizeNogoods", 1);
                        pctx.config.setOption("MinimizeNogoodsOnConflict", 1);
                    }
		    else if (heur == "alwaysopt") {
                        pctx.config.setOption("MinimizeNogoods", 1);
			pctx.config.setOption("MinimizeNogoodsOpt", 1);
                    }
                    else if (heur == "onconflictopt") {
                        pctx.config.setOption("MinimizeNogoods", 1);
			pctx.config.setOption("MinimizeNogoodsOpt", 1);
                        pctx.config.setOption("MinimizeNogoodsOnConflict", 1);
                    }
                    else {
                        throw GeneralError(std::string("Unknown value for nogood minimization: \"") + heur + std::string("\""));
                    }
                }
                break;
            case 59:
                {
                    int minval = 0;
                    try
                    {
                        if( optarg[0] == '=' )
                            minval = boost::lexical_cast<unsigned>(&optarg[1]);
                        else
                            minval = boost::lexical_cast<unsigned>(optarg);
                    }
                    catch(const boost::bad_lexical_cast&) {
                        LOG(ERROR,"minimizesize '" << optarg << "' does not specify an integer value");
                    }
                    pctx.config.setOption("MinimizationSize", minval);
                }
                break;
            case 60:
                {
                    std::string arg(optarg);
                    if (arg.find(',', 0) == std::string::npos) throw GeneralError(std::string("Argument of \"--csvinput\" must be of type \"PREDICATE,FILENAME\""));
                    std::string pred = arg.substr(0, arg.find(',', 0));
                    std::string filename = arg.substr(arg.find(',', 0) + 1);
                    pctx.inputProvider->addCSVFileInput(pred, filename);
                }
                break;
            case 61:
                {
                    std::string pred(optarg);
                    pctx.modelCallbacks.push_back(ModelCallbackPtr(new CSVAnswerSetPrinterCallback(pctx, pred)));
                }
                break;
            case 62:
                {
                    pctx.config.setOption("NoOuterExternalAtoms", 1);
                }
                break;
            case 64:
                {
                    pctx.config.setOption("ForceGC", 1);
//                    pctx.config.setOption("NoOuterExternalAtoms", 1);
                    pctx.config.setOption("LiberalSafety", 1);
                    pctx.config.setOption("TransUnitLearning", 1);
                    pctx.config.setOption("TransUnitLearningPUD", 0);
                }
                break;
            case 68:
                {
                    pctx.config.setOption("TransUnitLearningPUD", 1);
                }
                break;
            case 69:
                {
                    pctx.config.setOption("TransUnitLearningOS", 1);
                }
                break;
            case 70:
                {
                    pctx.config.setOption("TransUnitLearningDN", 1);
                }
                break;
            case 71:
                {
                    int treshold = 0;
                    try
                    {
                        if( optarg[0] == '=' )
                            treshold = boost::lexical_cast<unsigned>(&optarg[1]);
                        else
                            treshold = boost::lexical_cast<unsigned>(optarg);
                    }
                    catch(const boost::bad_lexical_cast&) {
                        LOG(ERROR,"analysis treshold '" << optarg << "' does not specify an integer value");
                    }
                    pctx.config.setOption("TransUnitLearningAT", treshold);
                }
                break;
            case 72:
                {
                    pctx.config.setOption("TransUnitLearningMN", 1);
                }
                break;
            case 65:
                {
                    pctx.config.setOption("ExternalAtomVerificationFromLearnedNogoods", 1);
                }
                break;
            case 66:
                {
                    pctx.config.setOption("WaitOnModel", 1);
                }
                break;
            case 67:
                {
                    pctx.config.setOption("SupportSets", 1);
                    pctx.config.setOption("ExternalSourceInlining", 1);
                    if (optarg) {
                        std::string mode(optarg);
                        if (mode == "re") pctx.config.setOption("ExternalSourceInlining", 2);
                        else if (mode != "post") throw GeneralError("Unknown inlining mode \"" + mode + "\"");
                    }
                }
                break;
            case 73: pctx.config.setOption("UFSCheckPartial", 1); break;
        }
    }

    // global constraints
    if (pctx.config.getOption("UFSCheck") && !pctx.config.getOption("GenuineSolver")) {
        // if solver was not set by user, disable it silently, otherwise print a warning
        if (!solverSet) {
            DBGLOG(WARNING, "Unfounded Set Check is only supported for genuine solvers; will behave like flpcheck=explicit");
        }
        else {
            LOG(WARNING, "Unfounded Set Check is only supported for genuine solvers; will behave like flpcheck=explicit");
        }
        pctx.config.setOption("FLPCheck", 1);
        pctx.config.setOption("UFSCheck", 0);
    }
    if (pctx.config.getOption("LiberalSafety") && !pctx.config.getOption("GenuineSolver")) {
        // if solver was not set by user, disable it silently, otherwise print a warning
        if (!solverSet) {
            DBGLOG(WARNING, "Liberal safety is only supported for genuine solvers, will disable it");
        }
        else {
            LOG(WARNING, "Liberal safety is only supported for genuine solvers, will disable it");
        }
        pctx.config.setOption("LiberalSafety", 0);
    }
    if (specifiedModelQueueSize && pctx.config.getOption("GenuineSolver") <= 2) {
        LOG(WARNING, "Model caching (modelqueuesize) is only compatible with clasp backend");
    }
    if (pctx.config.getOption("GenuineSolver") || pctx.config.getOption("LiberalSafety") || heuristicMonolithic) {
        pctx.config.setOption("IncludeAuxInputInAuxiliaries", 1);
    }
    if (defiaux) {
        pctx.config.setOption("IncludeAuxInputInAuxiliaries", iaux);
    }
    if (!pctx.config.getOption("GenuineSolver") && pctx.config.getOption("ExternalLearning")) {
        // if solver was not set by user, disable it silently, otherwise print a warning
        if (!solverSet) {
            DBGLOG(WARNING, "Cannot use external learning with non-genuine solver, will disable it");
        }
        else {
            LOG(WARNING, "Cannot use external learning with non-genuine solver, will disable it");
        }
        pctx.config.setOption("ExternalLearning", 0);

    }
    bool usingClaspBackend = (pctx.config.getOption("GenuineSolver") == 3 || pctx.config.getOption("GenuineSolver") == 4);
    if( !forceoptmode && heuristicMonolithic && usingClaspBackend ) {
        // we can use this in a safe way if we use a monolithic evaluation unit
        // we can use this with the clasp backend (the other solver does not honor setOptimum() calls)
        // TODO we can also use this if we have all weight constraints in one evaluation unit, this can be detected automatically
        pctx.config.setOption("OptimizationTwoStep", 1);
    }
    // We cannot use strong safety if we treat all outer external atoms as inner ones.
    // We support two types of safety (strong and liberal) and two grounding approaches (decomposition-based and fixpoint-based). The two dimensions are related as follows:
    // - decomposition-based grounding is made for strong safety (it cannot handle liberally safe programs)
    // - fixpoint-based grounding is made for liberal safety (but can also handle strongly safe programs which are a special case of liberally safe ones)
    // Currently there are no separate options for the two dimensions: --strongsafety and --liberalsafety switch both the safety concept and the grounding approach (fixpoint-based with --liberalsafety and decomposition-based for --strongsafety).
    // Treating outer external atoms as inner ones does not destroy strong safety (because the structure of the program does not change) but destroys correctness of the decomposition-based grounder even for strongly safe programs,
    // while the fixpoint-based grounding approach is still correct (both for strongly and for liberally safe programs).
    // Thus, we can only handle outer external atoms as inner ones if --liberalsafety is used since then also the fixpoint-based grounding approach is used
    // (in principle, switching only the grounding approach to fixpoint-based but still using strong safety is possible but not implemented).
    if (!pctx.config.getOption("LiberalSafety") && pctx.config.getOption("NoOuterExternalAtoms")){
        throw GeneralError("Option --noouterexternalatoms can only be used with --liberalsafety");
    }

    // configure plugin path
    configurePluginPath(config.optionPlugindir);

    // check input files (stdin, file, or URI)

    // stdin requested, append it first
    if( std::string(argv[optind - 1]) == "--" )
        pctx.inputProvider->addStreamInput(std::cin, "<stdin>");

    // collect further filenames/URIs
    // if we use dlvdb, manage .typ files
    for (int i = optind; i < argc; ++i) {
        std::string arg(argv[i]);
        if( arg.size() > 4 && arg.substr(arg.size()-4) == ".typ" ) {
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
        #ifdef HAEV_LIBCURL
        else if( arg.find("http://") == 0 ) {
            pctx.inputProvider->addURLInput(arg);
        }
        #endif
        else {
            pctx.inputProvider->addFileInput(arg);
        }
    }
}


void configurePluginPath(std::string& userPlugindir)
{
    #ifdef WIN32
    bool reset = false;
    if( !userPlugindir.empty() && userPlugindir[0] == '!' ) {
        reset = true;
        if( userPlugindir.size() > 2 && userPlugindir[1] == ';' )
            userPlugindir.erase(0,2);
        else
            userPlugindir.erase(0,1);
    }

    std::stringstream searchpath;

    if( !userPlugindir.empty() )
        searchpath << userPlugindir << ';';

    #ifdef LOCAL_PLUGIN_DIR
        reset = true;
        searchpath << LOCAL_PLUGIN_DIR << ';';
    #endif

    if( !reset ) {
        // add LD_LIBRARY_PATH
        const char *envld = ::getenv("LD_LIBRARY_PATH");
        if( envld ) {
            searchpath << envld << ";";
        }

        #ifdef USER_PLUGIN_DIR
        const char* homedir = ::getenv("USERPROFILE");
        if (!!homedir) searchpath << homedir << "/" USER_PLUGIN_DIR << ';';
        #endif
        #ifdef SYS_PLUGIN_DIR
        searchpath << SYS_PLUGIN_DIR;
        #endif
    }
    userPlugindir = searchpath.str();
    #elif POSIX
    bool reset = false;
    if( !userPlugindir.empty() && userPlugindir[0] == '!' ) {
        reset = true;
        if( userPlugindir.size() > 2 && userPlugindir[1] == ':' )
            userPlugindir.erase(0,2);
        else
            userPlugindir.erase(0,1);
    }

    std::stringstream searchpath;

    if( !userPlugindir.empty() )
        searchpath << userPlugindir << ':';

    #ifdef LOCAL_PLUGIN_DIR
        reset = true;
        searchpath << LOCAL_PLUGIN_DIR << ':';
    #endif

    if( !reset ) {
        // add LD_LIBRARY_PATH
        const char *envld = ::getenv("LD_LIBRARY_PATH");
        if( envld ) {
            searchpath << envld << ":";
        }

        const char* homedir = ::getpwuid(::geteuid())->pw_dir;
        searchpath << homedir << "/" USER_PLUGIN_DIR << ':' << SYS_PLUGIN_DIR;
    }
    userPlugindir = searchpath.str();
    #else
    #error Either POSIX or WIN32 must be defined
    #endif
}


// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:

