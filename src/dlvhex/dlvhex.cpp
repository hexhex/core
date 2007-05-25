/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* -*- C++ -*- */

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
 * plugin. In this case, please continue with the \ref pluginframework "Plugin Interface Module", which
 * contains all necessary information.
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef DLVHEX_DEBUG
#include <boost/date_time/posix_time/posix_time.hpp>
#endif // DLVHEX_DEBUG

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <getopt.h>

#include "dlvhex/GraphProcessor.h"
#include "dlvhex/GraphBuilder.h"
#include "dlvhex/ComponentFinder.h"
#include "dlvhex/BoostComponentFinder.h"
#include "dlvhex/globals.h"
#include "dlvhex/helper.h"
#include "dlvhex/Error.h"
#include "dlvhex/ResultContainer.h"
#include "dlvhex/OutputBuilder.h"
#include "dlvhex/SafetyChecker.h"
#include "dlvhex/HexParserDriver.h"
#include "dlvhex/PrintVisitor.h"


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
		<< " -p, --plugindir=dir  Specify additional directory where to look for plugin" << std::endl
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
		<< " -v, --verbose[=N]    Specify verbose category (default: 1):" << std::endl
		<< "                      1  - program analysis information (including dot-file)" << std::endl
		<< "                      2  - program modifications by plugins" << std::endl
		<< "                      4  - intermediate model generation info" << std::endl
		<< "                      8  - timing information (only if configured with" << std::endl
		<< "                                               --enable-debug)" << std::endl
		<< "                      add values for multiple categories." << std::endl;
//		<< std::endl;
}


/**
 * @brief Print a fatal error message and terminate.
 */
void
InternalError (const char *msg)
{
	std::cerr << std::endl
		<< "An internal error occurred (" << msg << ")."
		<< std::endl << "Please contact <roman@kr.tuwien.ac.at>!" << std::endl;
	exit (99);
}



#include "dlvhex/PluginContainer.h"
#include "dlvhex/DependencyGraph.h"


#include <sys/types.h>
//#include <sys/dir.h>
#include <dirent.h>

#include "pwd.h"

//
// definition for getwd ie MAXPATHLEN etc
//
#include <sys/param.h>
#include <stdio.h>
#include <dlfcn.h>

#include "dlvhex/ProgramBuilder.h"
#include "dlvhex/GraphProcessor.h"
#include "dlvhex/Component.h"


void
insertNamespaces()
{
	if (Term::namespaces.size() == 0)
		return;

	std::string prefix, fullns;

	//NamesTable<std::string>* names = Term::getNamesTable();

	for (NamesTable<std::string>::const_iterator nm = Term::names.begin();
			nm != Term::names.end();
			++nm)
	{
		//std::cout << "orig: nametable entry: " << *nm << std::endl;

		for (std::vector<std::pair<std::string, std::string> >::iterator ns = Term::namespaces.begin();
				ns != Term::namespaces.end();
				ns++)
		{
			prefix = ns->second + ":";

			//
			// prefix must occur either at beginning or right after quote
			//
			unsigned start = 0;
			if ((*nm)[0] == '"')
				start = 1;

			if ((*nm).find(prefix, start) == start)
			{
				std::string r(*nm);

				r.replace(start, prefix.length(), ns->first);

				Term::names.modify(nm, r);

				//std::cout << "modified: " << r << std::endl;
			}
		}

		//std::cout << "nametable entry: " << nm.getIndex() << " " << *nm << std::endl;
	}

	/*
	   NamesTable<std::string> names2 = Term::getNamesTable();

	   std::cout << "addr1: " << &names << std::endl;
	   std::cout << "addr2: " << &names2 << std::endl;

	   for (NamesTable<std::string>::const_iterator nm = names2.begin();
	   nm != names2.end();
	   ++nm)
	   {
	   std::cout << "nametable entry: " << nm.getIndex() << " " << *nm << std::endl;
	   }
	   */
}

void
removeNamespaces()
{
	if (Term::namespaces.size() == 0)
		return;

	std::string prefix, fullns;

	//    NamesTable<std::string> names = Term::getNamesTable();

	for (NamesTable<std::string>::const_iterator nm = Term::names.begin();
			nm != Term::names.end();
			++nm)
	{
		for (std::vector<std::pair<std::string, std::string> >::iterator ns = Term::namespaces.begin();
				ns != Term::namespaces.end();
				ns++)
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


/**
 * Search a specific directory for dlvhex-plugins and store their names.
 */
void
searchPlugins(std::string dir, std::set<std::string>& pluginlist)
{
	int count, i;
	struct dirent **files;
	std::string filename;

	count = scandir(dir.c_str(), &files, 0, alphasort);

//		std::cerr << "  d: " << dir << std::endl;
	for (i = 0; i < count; ++i)
	{
		filename = files[i]->d_name;
//		std::cerr << "  f: " << filename << std::endl;

		//        if (filename.substr(0,9) == "libdlvhex")
		if  (filename.size() > 3)
			if (filename.substr(filename.size() - 3, 3) == ".so")
				pluginlist.insert(dir + '/' + filename);

	}

	//
	// clean up scandir mess
	//
	if (count != -1)
	{
		while (count--)
			free(files[count]);

		free(files); 
	}

}


#include <ext/stdio_filebuf.h> 

int
main (int argc, char *argv[])
{
	//
	// Stores the rules of the program.
	//
	Program IDB;

	//
	// Stores the facts of the program.
	//
	AtomSet EDB;


	WhoAmI = argv[0];

	/////////////////////////////////////////////////////////////////
	//
	// Option handling
	//
	/////////////////////////////////////////////////////////////////

	// global defaults:
	Globals::Instance()->setOption("NoPredicate", 1);
	Globals::Instance()->setOption("Silent", 0);
	Globals::Instance()->setOption("Verbose", 0);
	Globals::Instance()->setOption("NoPredicate", 1);
	Globals::Instance()->setOption("StrongSafety", 1);
	Globals::Instance()->setOption("AllModels", 0);
	Globals::Instance()->setOption("ReverseAllModels", 0);

	// options only used here in main():
	bool optionPipe = false;
	bool optionXML = false;
	bool optionNoEval = false;
	bool optionKeepNSPrefix = false;

	std::string optionPlugindir("");

	//
	// dlt switch should be temporary until we have a proper rewriter for flogic!
	//
	bool optiondlt = false;

	std::vector<std::string> allFiles;

	std::vector<std::string> remainingOptions;


	extern char* optarg;
	extern int optind, opterr;

	bool helpRequested = 0;

	//
	// prevent error message for unknown options - they might be known to
	// plugins later!
	//
	opterr = 0;

	int ch;
	int longid;

	static struct option longopts[] = {
		{ "help", no_argument, 0, 'h' },
		{ "silent", no_argument, 0, 's' },
		{ "verbose", optional_argument, 0, 'v' },
		{ "filter", required_argument, 0, 'f' },
		{ "plugindir", required_argument, 0, 'p'},
		{ "allmodels", no_argument, 0, 'a'},
		{ "reverse", no_argument, 0, 'r'},
		{ "firstorder", no_argument, &longid, 1 },
		{ "weaksafety", no_argument, &longid, 2 },
		{ "ruleml",     no_argument, &longid, 3 },
		{ "dlt",        no_argument, &longid, 4 },
		{ "noeval",     no_argument, &longid, 5 },
		{ "keepnsprefix", no_argument, &longid, 6 },
		{ NULL, 0, NULL, 0 }
	};

	std::vector<std::string> fil;
	std::vector<std::string>::iterator i;

	while ((ch = getopt_long(argc, argv, "f:hsvp:ar", longopts, NULL)) != -1)
	{
		switch (ch)
		{
			case 'h':
				//printUsage(std::cerr, true);
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
				fil = helper::stringExplode(std::string(optarg), ",");
				i = fil.begin();
				while (i != fil.end())
					Globals::Instance()->addFilter(*i++);

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
				if (longid == 1)
					Globals::Instance()->setOption("NoPredicate", 0);
				else if (longid == 2)
					Globals::Instance()->setOption("StrongSafety", 0);
				else if (longid == 3)
				{
					optionXML = true;

					//
					// XMl output makes only sense with silent:
					//
					Globals::Instance()->setOption("Silent", 1);
				}
				else if (longid == 4)
					optiondlt = true;
				else if (longid == 5)
					optionNoEval = true;
				else if (longid == 6)
					optionKeepNSPrefix = true;
				break;
			case '?':
				remainingOptions.push_back(argv[optind - 1]);
				break;
		}
	}

	//
	// before anything else we dump the logo
	//

	if (!Globals::Instance()->getOption("Silent"))
		printLogo();

	//
	// no arguments at all: shorthelp
	//
	if (argc == 1)
	{
		printUsage(std::cerr, false);

		exit(1);
	}

	bool inputIsWrong = 0;

	//
	// check if we have any input (stdin or file)
	// if inout is not or badly specified, remember this and show shorthelp
	// later if everthing was ok with the options
	//

	//
	// stdin requested
	//
	if (!strcmp(argv[optind - 1], "--"))
	{
		optionPipe = true;
	}

	if (optind == argc)
	{
		//
		// no files and no stdin - error
		//
		if (!optionPipe)
			inputIsWrong = 1;
	}
	else
	{
		//
		// files and stdin - error
		//
		if (optionPipe)
			inputIsWrong = 1;

		//
		// collect filenames
		//
		for (int i = optind; i < argc; ++i)
		{
			allFiles.push_back(argv[i]);
		}
	}



	/////////////////////////////////////////////////////////////////
	//
	// now search for plugins
	//
	/////////////////////////////////////////////////////////////////
	
#ifdef DLVHEX_DEBUG
	DEBUG_START_TIMER
#endif // DLVHEX_DEBUG

	std::set<std::string> libfilelist;

	std::stringstream allpluginhelp;

	//
	// first look into specified plugin dir
	//
	if (!optionPlugindir.empty())
		searchPlugins(optionPlugindir, libfilelist);

	//
	// now look into user's home
	//
	std::string userhome(::getpwuid(::geteuid())->pw_dir);

	userhome = userhome + "/" + (std::string)USER_PLUGIN_DIR;

	searchPlugins(userhome, libfilelist);

	//
	// eventually look into system plugin dir
	//
	searchPlugins(SYS_PLUGIN_DIR, libfilelist);

	std::vector<PluginInterface*> plugins;

	//
	// import found plugin-libs
	//
	for (std::set<std::string>::const_iterator si = libfilelist.begin();
			si != libfilelist.end();
			si++)
	{
		try
		{
			PluginInterface* plugin;

			plugin = PluginContainer::Instance()->importPlugin(*si);

			if (plugin != NULL)
			{
				std::stringstream pluginhelp;

				plugins.push_back(plugin);

				plugin->setOptions(helpRequested, remainingOptions, pluginhelp);

				if (!pluginhelp.str().empty())
					allpluginhelp << std::endl << pluginhelp.str();
			}
		}
		catch (GeneralError &e)
		{
			std::cerr << e.getErrorMsg() << std::endl;

			exit(1);
		}
	}

#ifdef DLVHEX_DEBUG
	//                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
	DEBUG_STOP_TIMER("Importing plugins                      ")
#endif // DLVHEX_DEBUG

	if (!Globals::Instance()->getOption("Silent"))
		std::cout << std::endl;


	//
	// help was requested?
	//
	if (helpRequested)
	{
		printUsage(std::cerr, true);
		std::cerr << allpluginhelp.str() << std::endl;
		exit(0);
	}

	//
	// any unknown options left?
	//
	if (!remainingOptions.empty())
	{
		std::cerr << "Unknown option(s):";

		std::vector<std::string>::const_iterator opb = remainingOptions.begin();

		while (opb != remainingOptions.end())
			std::cerr << " " << *opb++;

		std::cerr << std::endl;
		printUsage(std::cerr, false);

		exit(1);
	}

	//
	// options are all ok, but input is missing
	//
	if (inputIsWrong)
	{
		printUsage(std::cerr, false);

		exit(1);
	}


	/////////////////////////////////////////////////////////////////
	//
	// parse input
	//
	/////////////////////////////////////////////////////////////////

#ifdef DLVHEX_DEBUG
	DEBUG_RESTART_TIMER
#endif // DLVHEX_DEBUG

	HexParserDriver driver;

	if (optionPipe)
	{
		//
		// if we are piping, use a dummy file-name in order to enter the
		// file-loop below
		//
		allFiles.push_back(std::string("dummy"));

		Globals::Instance()->lpfilename = "lpgraph.dot";
	}

		//
		// store filename of (first) logic program, we might use this somewhere
		// else (e.g., when writing the graphviz file in the boost-part
		//
		std::vector<std::string> filepath = helper::stringExplode(allFiles[0], "/");
		Globals::Instance()->lpfilename = filepath.back() + ".dot";

		for (std::vector<std::string>::const_iterator f = allFiles.begin();
			 f != allFiles.end();
			 f++)
		{
			try
			{
				//
				// stream to store the file/stdin content
				//
				std::stringstream tmpin;

				std::ifstream ifs;

				if (!optionPipe)
				{
					//
					// file
					//
					ifs.open((*f).c_str());

					if (!ifs.is_open())
					{
						throw GeneralError("File " + *f + " not found");
					}

					//
					// tell the parser driver where the rules are actually coming
					// from (needed for error-messages)
					//
					driver.setOrigin(*f);

					tmpin << ifs.rdbuf();
				}
				else
				{
					//
					// stdin
					//
					tmpin << std::cin.rdbuf();
				}

				//
				// create a stringbuffer on the heap (will be deleted later) to
				// hold the file-content. put it into a stream "input"
				//	
				std::istream input(new std::stringbuf(tmpin.str()));

				//
				// new output stream with stringbuffer on the heap
				//
				std::ostream converterResult(new std::stringbuf);

				bool wasConverted(0);

				for (std::vector<PluginInterface*>::iterator pi = plugins.begin();
						pi != plugins.end();
						++pi)
				{
					PluginConverter* pc = (*pi)->createConverter();

					if (pc != NULL)
					{
						//
						// rewrite input -> converterResult
						//
						pc->convert(input, converterResult);

						wasConverted = 1;

						//
						// old input buffer can be deleted now
						// (but not if we are piping from stdin and this is the
						// first conversion, because in this case input is set to
						// std::cin.rdbuf() (see above) and cin takes care of
						// its buffer deletion itself, so better don't
						// interfere!)
						//
						if (optionPipe && !wasConverted)
							delete input.rdbuf();

						//
						// store the current output buffer
						//
						std::streambuf* tmp = converterResult.rdbuf();

						//
						// make a new buffer for the output (=reset the output)
						//
						converterResult.rdbuf(new std::stringbuf);

						//
						// set the input buffer to be the output of the last
						// rewriting. now, after each loop, the converted
						// program is in input.
						//
						input.rdbuf(tmp);

					}
				}
				char tempfile[L_tmpnam];


				//
				// at this point, the program is in the stream "input" - wither
				// directly read from the file or as a result of some previous
				// rewriting!
				//

				if (Globals::Instance()->doVerbose(Globals::DUMP_CONVERTED_PROGRAM) && wasConverted)
				{
					//
					// we need to read the input-istream now - use a stringstream
					// for output and initialize the input-istream to its
					// content again
					//
					std::stringstream ss;
					ss << input.rdbuf();
					Globals::Instance()->getVerboseStream() << "Converted input:" << std::endl;
					Globals::Instance()->getVerboseStream() << ss.str();
					Globals::Instance()->getVerboseStream() << std::endl;
					delete input.rdbuf(); 
					input.rdbuf(new std::stringbuf(ss.str()));
				}

				FILE* fp;

				//
				// now call dlt if needed
				//
				if (optiondlt)
				{
					mkstemp(tempfile);

					std::ofstream dlttemp(tempfile);

					//
					// write program into tempfile
					//
					dlttemp << input.rdbuf();

					dlttemp.close();

					std::string execPreParser("dlt -silent -preparsing " + std::string(tempfile));

					fp = popen(execPreParser.c_str(), "r");

					if (fp == NULL)
					{
						throw GeneralError("Unable to call Preparser dlt");
					}

					__gnu_cxx::stdio_filebuf<char>* fb;
					fb = new __gnu_cxx::stdio_filebuf<char>(fp, std::ios::in);

					std::istream inpipe(fb);

					//
					// now we have a program rewriten by dlt - since it should
					// be in the stream "input", we have to delete the old
					// input-buffer and set input to the buffer from the
					// dlt-call
					//
					delete input.rdbuf(); 
					input.rdbuf(fb);
				}

				driver.parse(input, IDB, EDB);

				if (optiondlt)
				{
					int dltret = pclose(fp);

					if (dltret != 0)
					{
						throw GeneralError("Preparser dlt returned error");
					}
				}

				//
				// wherever the input-buffer was created before - now we don't
				// need it anymore
				//
				delete input.rdbuf();

				ifs.close();

				if (optiondlt)
				{
					unlink(tempfile);
				}
			}
			catch (SyntaxError& e)
			{
				std::cerr << e.getErrorMsg() << std::endl;

				exit(1);
			}
			catch (GeneralError& e)
			{
				std::cerr << e.getErrorMsg() << std::endl;

				exit(1);
			}

		}
//	}

#ifdef DLVHEX_DEBUG
	//                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
	DEBUG_STOP_TIMER("Parsing and converting input           ")
#endif // DLVHEX_DEBUG

	//
	// expand constant names
	//
	insertNamespaces();

	if (Globals::Instance()->doVerbose(Globals::DUMP_PARSED_PROGRAM))
	{
		Globals::Instance()->getVerboseStream() << "Parsed Rules: " << std::endl;
		RawPrintVisitor rpv(Globals::Instance()->getVerboseStream());
		IDB.dump(rpv);
		Globals::Instance()->getVerboseStream() << std::endl << "Parsed EDB: " << std::endl;
		EDB.accept(rpv);
		Globals::Instance()->getVerboseStream() << std::endl << std::endl;
	}


#ifdef DLVHEX_DEBUG
	DEBUG_RESTART_TIMER
#endif // DLVHEX_DEBUG
			
	//
	// now call rewriters
	//
	bool wasRewritten(0);

	for (std::vector<PluginInterface*>::iterator pi = plugins.begin();
			pi != plugins.end();
			++pi)
	{
		PluginRewriter* pr = (*pi)->createRewriter();

		if (pr != NULL)
		{
			pr->rewrite(IDB, EDB);

			wasRewritten = 1;
		}
	}

#ifdef DLVHEX_DEBUG
	//                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
	DEBUG_STOP_TIMER("Calling plugin rewriters               ")
#endif // DLVHEX_DEBUG

	if (Globals::Instance()->doVerbose(Globals::DUMP_REWRITTEN_PROGRAM) && wasRewritten)
	{
		Globals::Instance()->getVerboseStream() << "Rewritten rules:" << std::endl;
		RawPrintVisitor rpv(Globals::Instance()->getVerboseStream());
		IDB.dump(rpv);
		Globals::Instance()->getVerboseStream() << std::endl << "Rewritten EDB:" << std::endl;
		EDB.accept(rpv);
		Globals::Instance()->getVerboseStream() << std::endl << std::endl;
	}


	/// @todo: when exiting after an exception, we have to cleanup things!
	// maybe using boost-pointers!

#ifdef DLVHEX_DEBUG
	DEBUG_RESTART_TIMER
#endif // DLVHEX_DEBUG

	//
	// The GraphBuilder creates nodes and dependency edges from the raw program.
	//
	GraphBuilder gb;

	NodeGraph nodegraph;

    try
    {
        gb.run(IDB, nodegraph);
    }
    catch (GeneralError& e)
    {
		std::cerr << e.getErrorMsg() << std::endl << std::endl;

		exit(1);
    }

#ifdef DLVHEX_DEBUG
	//                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
	DEBUG_STOP_TIMER("Building dependency graph              ")
#endif // DLVHEX_DEBUG

    if (Globals::Instance()->doVerbose(Globals::DUMP_DEPENDENCY_GRAPH))
    {
        gb.dumpGraph(nodegraph, Globals::Instance()->getVerboseStream());
    }
    

#ifdef DLVHEX_DEBUG
	DEBUG_RESTART_TIMER
#endif // DLVHEX_DEBUG

	//
	// now call optimizers
	//
	bool wasOptimized(10);

	for (std::vector<PluginInterface*>::iterator pi = plugins.begin();
			pi != plugins.end();
			++pi)
	{
		PluginOptimizer* po = (*pi)->createOptimizer();

		if (po != NULL)
		{
			po->optimize(nodegraph, EDB);

			wasOptimized = 1;
		}
	}

#ifdef DLVHEX_DEBUG
	//                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
	DEBUG_STOP_TIMER("Calling plugins optimizers             ")
#endif // DLVHEX_DEBUG

	if (Globals::Instance()->doVerbose(Globals::DUMP_OPTIMIZED_PROGRAM) && wasOptimized)
	{
		Globals::Instance()->getVerboseStream() << "Optimized graph:" << std::endl;
        gb.dumpGraph(nodegraph, Globals::Instance()->getVerboseStream());
		Globals::Instance()->getVerboseStream() << std::endl << "Optimized EDB:" << std::endl;
		RawPrintVisitor rpv(Globals::Instance()->getVerboseStream());
		EDB.accept(rpv);
		Globals::Instance()->getVerboseStream() << std::endl << std::endl;
	}



	//
	// The ComponentFinder provides functions for finding SCCs and WCCs from a
	// set of nodes.
	//
	ComponentFinder* cf;

	//
	// The DependencyGraph identifies and creates the graph components that will
	// be processed by the GraphProcessor.
	//
	DependencyGraph* dg;

	try
	{
		cf = new BoostComponentFinder;

		//
		// Initializing the DependencyGraph. Its constructor uses the
		// ComponentFinder to find relevant graph
		// properties for the subsequent processing stage.
		//
		dg = new DependencyGraph(nodegraph, cf);

		if (Globals::Instance()->getOption("StrongSafety"))
			StrongSafetyChecker sc(IDB, dg);
		else
			SafetyChecker sc(IDB);
	}
	catch (GeneralError &e)
	{
		std::cerr << e.getErrorMsg() << std::endl << std::endl;

		exit(1);
	}


	if (optionNoEval)
	{
		delete dg;
		delete cf;

		exit(0);
	}

	//
	// The GraphProcessor provides the actual strategy of how to compute the
	// hex-models of a given dependency graph.
	//
	GraphProcessor gp(dg);


	try
	{
		//
		// The GraphProcessor starts its computation with the program's ground
		// facts as input.
		// But only if the original EDB is consistent, otherwise, we can skip it
		// anyway.
		//
		if (EDB.isConsistent())
			gp.run(EDB);
	}
	catch (GeneralError &e)
	{
		std::cerr << e.getErrorMsg() << std::endl << std::endl;

		exit(1);
	}

	//
	// contract constant names again, if specified
	//
	if (optionKeepNSPrefix)
		removeNamespaces();


	//
	// prepare result container
	//
	// if we had any weak constraints, we have to tell the result container the
	// prefix in order to be able to compute each asnwer set's costs!
	//
	std::string wcprefix;

	if (IDB.getWeakConstraints().size() > 0)
		wcprefix = "wch__";

	ResultContainer result(wcprefix);

#ifdef DLVHEX_DEBUG
	DEBUG_RESTART_TIMER
#endif // DLVHEX_DEBUG

	//
	// put GraphProcessor result into ResultContainer
	//
	AtomSet* res;

	while ((res = gp.getNextModel()) != NULL)
	{
		try
		{
			result.addSet(*res);
		}
		catch (GeneralError &e)
		{
			std::cerr << e.getErrorMsg() << std::endl << std::endl;

			exit(1);
		}
	}

	//
	// remove auxiliary atoms
	//
	result.filterOut(Term::getAuxiliaryNames());

	//
	// quick hack
	//
	if (optiondlt)
		result.filterOutDLT();


	//
	// apply filter
	//
	//if (optionFilter.size() > 0)
	result.filterIn(Globals::Instance()->getFilters());


#ifdef DEBUG
	//                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
	DEBUG_STOP_TIMER("Postprocessing GraphProcessor result             ")
#endif // DEBUG

	//
	// output format
	//
	OutputBuilder* outputbuilder;

	if (optionXML)
		outputbuilder = new OutputXMLBuilder;
	else
		outputbuilder = new OutputTextBuilder;

	//
	// dump it!
	//

	result.print(std::cout, outputbuilder);


	//
	// was there anything non-error the user should know? dump it directly
	/*
	   for (std::vector<std::string>::const_iterator l = global::Messages.begin();
	   l != global::Messages.end();
	   l++)
	   {
	   std::cout << *l << std::endl;
	   }
	   */

	//
	// cleaning up:
	//
	delete outputbuilder;

	delete cf;

	delete dg;

	exit(0);
}


/* vim: set noet sw=4 ts=4 tw=80: */
