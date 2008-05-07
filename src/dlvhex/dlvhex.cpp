/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
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
 * plugin. In this case, please continue with the \ref pluginframework "Plugin
 * Interface Module", which contains all necessary information.
 *
 * For an overview on the logical primitives and datatypes used by dlvhex, see
 * \ref dlvhextypes "dlvhex Datatypes".
 */


/**
 * \defgroup dlvhextypes dlvhex Types
 *
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H


#include "dlvhex/ProgramCtx.h"
#include "dlvhex/PluginContainer.h"
#include "dlvhex/Program.h"
#include "dlvhex/AtomNode.h"
#include "dlvhex/NodeGraph.h"
#include "dlvhex/globals.h"
#include "dlvhex/Error.h"
#include "dlvhex/RuleMLOutputBuilder.h"
#include "dlvhex/PrintVisitor.h"
#include "dlvhex/DLVProcess.h"

#include <getopt.h>
#include <iostream>
#include <sstream>
#include <boost/tokenizer.hpp>


DLVHEX_NAMESPACE_USE


/// argv[0]
const char*  WhoAmI;



/**
 * @brief Print logo.
 */
void
printLogo()
{
	std::cout
		<< "DLVHEX "
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
printUsage(std::ostream &out, bool full)
{
	//      123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
	out << "Usage: " << WhoAmI 
		<< " [OPTION] FILENAME [FILENAME ...]" << std::endl
		<< std::endl;

	out << "   or: " << WhoAmI 
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
		<< " -s, --silent         Do not display anything than the actual result." << std::endl
		//        << "--strongsafety     Check rules also for strong safety." << std::endl
		<< " -p, --plugindir=DIR  Specify additional directory where to look for plugin" << std::endl
		<< "                      libraries (additionally to the installation plugin-dir" << std::endl
		<< "                      and $HOME/.dlvhex/plugins)." << std::endl
		<< " -f, --filter=foo[,bar[,...]]" << std::endl
		<< "                      Only display instances of the specified predicate(s)." << std::endl
		<< " -a, --allmodels      Display all models also under weak constraints." << std::endl
		<< " -r, --reverse        Reverse weak constraint ordering." << std::endl
		<< "     --firstorder     No higher-order reasoning." << std::endl
		<< "     --ruleml         Output in RuleML-format (v0.9)." << std::endl
		<< "     --noeval         Just parse the program, don't evaluate it (only useful" << std::endl
		<< "                      with --verbose)." << std::endl
		<< "     --keepnsprefix   Keep specified namespace-prefixes in the result." << std::endl
                << "     --solver=S       Use S as ASP engine, where S is one of (dlv,dlvdb)" << std::endl
		<< " -v, --verbose[=N]    Specify verbose category (default: 1):" << std::endl
		<< "                      1  - program analysis information (including dot-file)" << std::endl
		<< "                      2  - program modifications by plugins" << std::endl
		<< "                      4  - intermediate model generation info" << std::endl
		<< "                      8  - timing information (only if configured with" << std::endl
		<< "                                               --enable-debug)" << std::endl
		<< "                      add values for multiple categories." << std::endl;
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


void
insertNamespaces()
{
  ///@todo move this stuff to Term, this has nothing to do here!

  if (Term::namespaces.size() == 0)
    return;

  std::string prefix;

  for (NamesTable<std::string>::const_iterator nm = Term::names.begin();
       nm != Term::names.end();
       ++nm)
    {
      for (std::vector<std::pair<std::string, std::string> >::iterator ns = Term::namespaces.begin();
	   ns != Term::namespaces.end();
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
	      
		  Term::names.modify(nm, r);
		}
	    }
	}
    }
}



void
removeNamespaces()
{
  ///@todo move this stuff to Term, this has nothing to do here!

  if (Term::namespaces.size() == 0)
    return;

  std::string prefix;
  std::string fullns;

  for (NamesTable<std::string>::const_iterator nm = Term::names.begin();
       nm != Term::names.end();
       ++nm)
    {
      for (std::vector<std::pair<std::string, std::string> >::iterator ns = Term::namespaces.begin();
	   ns != Term::namespaces.end();
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

	      Term::names.modify(nm, r);
	    }
	}
    }
}



int
main (int argc, char *argv[])
{
  WhoAmI = argv[0];

  // The Program Context
  ProgramCtx pctx;


  /////////////////////////////////////////////////////////////////
  //
  // Option handling
  //
  /////////////////////////////////////////////////////////////////

  // global defaults:
  ///@todo clean up!!
  Globals::Instance()->setOption("NoPredicate", 1);
  Globals::Instance()->setOption("Silent", 0);
  Globals::Instance()->setOption("Verbose", 0);
  Globals::Instance()->setOption("NoPredicate", 1);
  Globals::Instance()->setOption("StrongSafety", 1);
  Globals::Instance()->setOption("AllModels", 0);
  Globals::Instance()->setOption("ReverseAllModels", 0);

  // options only used here in main():
  bool optionPipe = false;
  bool optionNoEval = false;
  bool optionKeepNSPrefix = false;

  // path to an optional plugin directory
  std::string optionPlugindir;

  //
  // dlt switch should be temporary until we have a proper rewriter for flogic!
  //
  bool optiondlt = false;

  bool helpRequested = false;

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
  
  static const char* shortopts = "f:hsvp:ar";
  static struct option longopts[] =
    {
      { "help", no_argument, 0, 'h' },
      { "silent", no_argument, 0, 's' },
      { "verbose", optional_argument, 0, 'v' },
      { "filter", required_argument, 0, 'f' },
      { "plugindir", required_argument, 0, 'p' },
      { "allmodels", no_argument, 0, 'a' },
      { "reverse", no_argument, 0, 'r' },
      { "firstorder", no_argument, &longid, 1 },
      { "weaksafety", no_argument, &longid, 2 },
      { "ruleml",     no_argument, &longid, 3 },
      { "dlt",        no_argument, &longid, 4 },
      { "noeval",     no_argument, &longid, 5 },
      { "keepnsprefix", no_argument, &longid, 6 },
      { "solver", required_argument, &longid, 7 },
      { NULL, 0, NULL, 0 }
    };

  while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1)
    {
      switch (ch)
	{
	case 'h':
	  helpRequested = 1;
	  break;

	case 's':
	  Globals::Instance()->setOption("Silent", 1);
	  break;

	case 'v':
	  if (optarg)
	    Globals::Instance()->setOption("Verbose", atoi(optarg));
	  else
	    Globals::Instance()->setOption("Verbose", 1);
	  break;

	case 'f':
	  {
	    boost::char_separator<char> sep(",");
	    std::string oa(optarg); // g++ 3.3 is unable to pass that at the ctor line below
	    boost::tokenizer<boost::char_separator<char> > tok(oa, sep);
	    
	    for (boost::tokenizer<boost::char_separator<char> >::const_iterator f = tok.begin();
		 f != tok.end(); ++f)
	      {
		Globals::Instance()->addFilter(*f);
	      }
	  }
	  break;
	  
	case 'p':
	  optionPlugindir = std::string(optarg);
	  break;

	case 'a':
	  Globals::Instance()->setOption("AllModels", 1);
	  break;

	case 'r':
	  Globals::Instance()->setOption("ReverseOrder", 1);
	  break;

	case 0:
	  switch (longid)
	    {
	    case 1:
	      Globals::Instance()->setOption("NoPredicate", 0);
	      break;

	    case 2:
	      Globals::Instance()->setOption("StrongSafety", 0);
	      break;

	    case 3:
	      pctx.setOutputBuilder(new RuleMLOutputBuilder);
	      // XML output makes only sense with silent:
	      Globals::Instance()->setOption("Silent", 1);
	      break;

	    case 4:
	      optiondlt = true;
	      break;

	    case 5:
	      optionNoEval = true;
	      break;

	    case 6:
	      optionKeepNSPrefix = true;
	      break;

	    case 7:
	      std::string solver(optarg);
	      if (solver == "dlvdb")
		{
		  pctx.setProcess(new DLVDBProcess);
		}
	      else // default is DLV
		{
		  pctx.setProcess(new DLVProcess);
		}
	      break;
	    }
	  break;

	case '?':
	  pctx.addOption(argv[optind - 1]);
	  break;
	}
    }

  //
  // before anything else we dump the logo
  //

  if (!Globals::Instance()->getOption("Silent"))
    {
      printLogo();
    }

  //
  // no arguments at all: shorthelp
  //
  if (argc == 1)
    {
      printUsage(std::cerr, false);
      exit(1);
    }

  bool inputIsWrong = false;

  //
  // check if we have any input (stdin, file, or URI)
  // if inout is not or badly specified, remember this and show shorthelp
  // later if everthing was ok with the options
  //

  //
  // stdin requested, append it first
  //
  if (!strcmp(argv[optind - 1], "--"))
    {
      optionPipe = true;
      pctx.addInputSource(argv[optind - 1]);
    }

  if (optind == argc && !optionPipe)
    {
      // no files and no stdin - error
      inputIsWrong = true;
    }
  else if (optind == argc && optionPipe)
    {
      // no files and stdin: set the lpfilename to a dummy value
      Globals::Instance()->lpfilename = "lpgraph.dot";
    }
  else
    {
      //
      // collect filenames/URIs
      //
      for (int i = optind; i < argc; ++i)
	{
	  pctx.addInputSource(argv[i]);
	}
    }

  // setup the plugin container
  pctx.setPluginContainer(PluginContainer::instance(optionPlugindir));


  /////////////////////////////////////////////////////////////////
  //
  // DLVHEX main execution
  //
  /////////////////////////////////////////////////////////////////
  try
    {

      /////////////////////////////////////////////////////////////////
      //
      // search for plugins
      //
      /////////////////////////////////////////////////////////////////
  
      ///@todo this could be better
      Globals::Instance()->setOption("HelpRequested", helpRequested);

      if (helpRequested)
	{
	  printUsage(std::cerr, true);
	}

      pctx.openPlugins();

      //
      // help was requested -> we are done now
      //
      if (helpRequested)
	{
	  exit(0);
	}
      
      //
      // any unknown options left?
      //
      if (!pctx.getOptions()->empty())
	{
	  std::cerr << "Unknown option(s):";
	  
	  std::vector<std::string>::const_iterator opb = pctx.getOptions()->begin();
	  
	  while (opb != pctx.getOptions()->end())
	    std::cerr << " " << *opb++;
	  
	  std::cerr << std::endl;
	  printUsage(std::cerr, false);
	  
	  exit(1);
	}
      
      //
      // options are all ok, but input is badly specified
      //
      if (inputIsWrong)
	{
	  printUsage(std::cerr, false);
	  exit(1);
	}
      
      /////////////////////////////////////////////////////////////////
      //
      // convert input
      //
      /////////////////////////////////////////////////////////////////
      
      pctx.convert();
      
      if (Globals::Instance()->doVerbose(Globals::DUMP_CONVERTED_PROGRAM))
	{
	  //
	  // we need to read the input-istream now - use a stringstream
	  // for output and initialize the input-istream to its
	  // content again
	  //
	  std::stringstream ss;
	  ss << pctx.getInput().rdbuf();
	  Globals::Instance()->getVerboseStream() << "Converted input:" << std::endl;
	  Globals::Instance()->getVerboseStream() << ss.str();
	  Globals::Instance()->getVerboseStream() << std::endl;
	  delete pctx.getInput().rdbuf(); 
	  pctx.getInput().rdbuf(new std::stringbuf(ss.str()));
	}
      
      /////////////////////////////////////////////////////////////////
      //
      // parse input
      //
      /////////////////////////////////////////////////////////////////
      
      pctx.parse();
      
      //
      // expand constant names
      ///@todo move to Term
      //
      insertNamespaces();
      
      if (Globals::Instance()->doVerbose(Globals::DUMP_PARSED_PROGRAM))
	{
	  Globals::Instance()->getVerboseStream() << "Parsed Rules: " << std::endl;
	  RawPrintVisitor rpv(Globals::Instance()->getVerboseStream());
	  pctx.getIDB()->accept(rpv);
	  Globals::Instance()->getVerboseStream() << std::endl << "Parsed EDB: " << std::endl;
	  pctx.getEDB()->accept(rpv);
	  Globals::Instance()->getVerboseStream() << std::endl << std::endl;
	}
      
      
      /////////////////////////////////////////////////////////////////
      //
      // rewrite program
      //
      /////////////////////////////////////////////////////////////////
      
      pctx.rewrite();
      
      if (Globals::Instance()->doVerbose(Globals::DUMP_REWRITTEN_PROGRAM))
	{
	  Globals::Instance()->getVerboseStream() << "Rewritten rules:" << std::endl;
	  RawPrintVisitor rpv(Globals::Instance()->getVerboseStream());
	  pctx.getIDB()->accept(rpv);
	  Globals::Instance()->getVerboseStream() << std::endl << "Rewritten EDB:" << std::endl;
	  pctx.getEDB()->accept(rpv);
	  Globals::Instance()->getVerboseStream() << std::endl << std::endl;
	}
      
      /////////////////////////////////////////////////////////////////
      //
      // generate node graph
      //
      /////////////////////////////////////////////////////////////////
      
      pctx.createNodeGraph();
      
      if (Globals::Instance()->doVerbose(Globals::DUMP_DEPENDENCY_GRAPH))
	{
	  const NodeGraph* nodegraph = pctx.getNodeGraph();

	  Globals::Instance()->getVerboseStream() << "Dependency graph - Program Nodes:" << std::endl;

	  for (std::vector<AtomNodePtr>::const_iterator node = nodegraph->getNodes().begin();
	       node != nodegraph->getNodes().end();
	       ++node)
	    {
	      Globals::Instance()->getVerboseStream() << **node << std::endl;
	    }
	  
	  Globals::Instance()->getVerboseStream() << std::endl;
	}
      
      /////////////////////////////////////////////////////////////////
      //
      // optimize program
      //
      /////////////////////////////////////////////////////////////////
      
      pctx.optimize();
      
      if (Globals::Instance()->doVerbose(Globals::DUMP_OPTIMIZED_PROGRAM))
	{
	  Globals::Instance()->getVerboseStream() << "Optimized graph:" << std::endl;

	  const NodeGraph* nodegraph = pctx.getNodeGraph();

	  Globals::Instance()->getVerboseStream() << "Dependency graph - Program Nodes:" << std::endl;

	  for (std::vector<AtomNodePtr>::const_iterator node = nodegraph->getNodes().begin();
	       node != nodegraph->getNodes().end();
	       ++node)
	    {
	      Globals::Instance()->getVerboseStream() << **node << std::endl;
	    }
	  
	  Globals::Instance()->getVerboseStream() << std::endl;

	  Globals::Instance()->getVerboseStream() << std::endl << "Optimized EDB:" << std::endl;
	  RawPrintVisitor rpv(Globals::Instance()->getVerboseStream());
	  pctx.getEDB()->accept(rpv);
	  Globals::Instance()->getVerboseStream() << std::endl << std::endl;
	}
      
      /////////////////////////////////////////////////////////////////
      //
      // generate dependency graph
      //
      /////////////////////////////////////////////////////////////////
      
      pctx.createDependencyGraph();
      
      /////////////////////////////////////////////////////////////////
      //
      // perform safety check
      //
      /////////////////////////////////////////////////////////////////
      
      pctx.safetyCheck();
      
      /////////////////////////////////////////////////////////////////
      //
      // perform strong safety check
      //
      /////////////////////////////////////////////////////////////////
      
      pctx.strongSafetyCheck();

      /////////////////////////////////////////////////////////////////
      //
      // last change to setup ProgramCtx before we evaluate it
      //
      /////////////////////////////////////////////////////////////////
      
      pctx.setupProgramCtx();
      
      /////////////////////////////////////////////////////////////////
      //
      // evaluate program
      //
      /////////////////////////////////////////////////////////////////
      
      if (optionNoEval)
	{
	  exit(0);
	}
      
      pctx.evaluate();
      
      /////////////////////////////////////////////////////////////////
      //
      // postprocess results
      //
      /////////////////////////////////////////////////////////////////
      
      pctx.postProcess();
      
      //
      // contract constant names again, if specified
      //
      if (optionKeepNSPrefix)
	{
	  removeNamespaces();
	}
      
      /////////////////////////////////////////////////////////////////
      //
      // output results
      //
      /////////////////////////////////////////////////////////////////
      
      pctx.output();
    }
  catch (GeneralError &e)
    {
      std::cerr << e.getErrorMsg() << std::endl << std::endl;
      exit(1);
    }

  /////////////////////////////////////////////////////////////////
  //
  // execution completed
  //
  /////////////////////////////////////////////////////////////////
 
 return 0;
}


/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
