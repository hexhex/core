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
 * TODO
 *
 */


#include <iostream>
#include <sstream>
#include <vector>

#include "dlvhex/GraphProcessor.h"
#include "dlvhex/GraphBuilder.h"
#include "dlvhex/ComponentFinder.h"
#include "dlvhex/BoostComponentFinder.h"
#include "dlvhex/globals.h"
#include "dlvhex/helper.h"
#include "dlvhex/GeneralError.h"
#include "dlvhex/ResultContainer.h"
#include "dlvhex/OutputBuilder.h"
#include "dlvhex/SafetyChecker.h"


unsigned parser_line;
const char *parser_file;
unsigned parser_errors = 0;


const char*  WhoAmI;


/**
 * @brief Stores the rules of the program.
 */
Program IDB;


/**
 * @brief Stores the facts of the program.
 */
AtomSet EDB;



/**
 * @brief Print logo.
 */
void
printLogo()
{
    std::cout
         << "DLVHEX [build " 
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
    out << "usage: " << WhoAmI 
        << " [--option]"
        << " [filename [filename [...]]]" << std::endl
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
    out << "--                 parse from stdin." << std::endl
        << "--silent           Do not display anything than the actual result." << std::endl
        << "--firstorder       No higher-order reasoning." << std::endl
//        << "--strongsafety     Check rules also for strong safety." << std::endl
        << "--verbose          dump also various intermediate information." << std::endl
        << "--plugindir=dir    Specify additional directory where to look for plugin" << std::endl
        << "                   libraries." << std::endl
        << "                   (Additionally to the installation plugin-dir and" << std::endl
        << "                   $HOME/.dlvhex/plugins)" << std::endl
        << "--filter=foo[,bar[,...]]" << std::endl
        << "                   Only display instances of the specified predicate(s)." << std::endl
        << "--ruleml           output in RuleML (v0.9) format." << std::endl
        << std::endl;
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

extern "C" FILE* inputin;         // Where LEX reads its input from

extern int inputparse();


#include <sys/types.h>
#include <sys/dir.h>
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

            if ((*nm).find(prefix, 0) == 0)
            {
                std::string r(*nm);

                r.replace(0, prefix.length(), ns->first);

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

            if ((*nm).find(fullns, 0) == 0)
            {
                std::string r(*nm);

                r.replace(0, fullns.length(), prefix);

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

    for (i = 0; i < count; ++i)
    {
        filename = files[i]->d_name;

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


int
main (int argc, char *argv[])
{
    WhoAmI = argv[0];

    //
    // Option handling
    //
    
    bool optionPipe = false;
    bool optionXML = false;

    std::string optionPlugindir("");

    //
    // dlt switch should be temporary until we have a proper rewriter for flogic!
    //
    bool optiondlt = false;

    std::vector<std::string> optionFilter;
    
    std::vector<std::string> allFiles;
    
    for (int j = 1; j < argc; j++)
    {
        if (argv[j][0] == '-')
        {
            if (!strcmp(argv[j],"--dlt"))
                optiondlt = true;
            else if (!strncmp(argv[j],"--plugindir=", 12))
                optionPlugindir = std::string(argv[j] + 12);
            else if (!strcmp(argv[j],"--firstorder"))
                global::optionNoPredicate = false;
            else if (!strcmp(argv[j], "--silent"))
                global::optionSilent = true;
            else if (!strcmp(argv[j], "--ruleml"))
            {
                optionXML = true;

                //
                // XMl output makes only sense with silent:
                //
                global::optionSilent = true;
            }
            else if (!strcmp(argv[j], "--verbose"))
                global::optionVerbose = true;
            else if (!strncmp(argv[j], "--filter=", 9))
                optionFilter = helper::stringExplode(std::string(argv[j] + 9), ",");
            else if (!strcmp(argv[j], "--"))
                optionPipe = true;
//                { std::cout << "Piping not working yet, sorry!" << std::endl; exit(-1); }
            else if (!strcmp(argv[j], "-h") || !strcmp(argv[j], "--help"))
            {
                printLogo();
                printUsage(std::cout, true);
                exit(0);
            }
            else 
            {
                // TODO:
                // for now: don't consider unknown options!
                printLogo();
                printUsage(std::cout, false);
                exit(-1);
            }
        }
        else
        {
            // TODO: test for file existence!
            allFiles.push_back(argv[j]);
        }
    }

    if (!global::optionSilent)
        printLogo();


    if (global::optionVerbose)
        std::cout << std::endl << "@@@ reading input @@@" << std::endl << std::endl;

    //
    // no file and no stdin?
    //
    if ((allFiles.size() == 0) && !optionPipe)
    {
        printUsage(std::cerr,false);
        
        exit(-1);
    }


    std::set<std::string> libfilelist;
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


    //
    // import found plugin-libs
    //
    for (std::set<std::string>::const_iterator si = libfilelist.begin();
         si != libfilelist.end();
         si++)
    {
        try
        {
            PluginContainer::Instance()->importPlugin(*si);
        }
        catch (GeneralError &e)
        {
            std::cerr << e.getErrorMsg() << std::endl;
            
            exit(1);
        }
    }

    if (!global::optionSilent)
        std::cout << std::endl;

    
    //
    // parse input
    //
    
    try
    {
        if (optionPipe)
        {
            parser_file = "";
            parser_line = 1;

            inputin = stdin;

            try
            {
                inputparse ();
            }
            catch (GeneralError& e)
            {
                std::cerr << e.getErrorMsg() << std::endl;
                
                exit(1);
            }

            global::lpfilename = "lpgraph.dot";
        }
        else
        {
            //
            // store filename of (first) logic program, we might use this somewhere
            // else (e.g., when writing the graphviz file in the boost-part
            //
            std::vector<std::string> filepath = helper::stringExplode(allFiles[0], "/");
            global::lpfilename = filepath.back() + ".dot";

            for (std::vector<std::string>::const_iterator f = allFiles.begin();
                f != allFiles.end();
                f++)
            {
                parser_file = f->c_str();

                FILE *inputfile;

                std::string execPreParser("dlt -silent -preparsing " + *f);
                    
                if (optiondlt)
                    inputfile = popen(execPreParser.c_str(), "r");
                else
                    inputfile = fopen(parser_file, "r");

                if (inputfile == NULL)
                {
                    if (optiondlt)
                        std::cerr << "unable to call preparser: " << execPreParser << std::endl;
                    else
                        std::cerr << "file " << parser_file << " not found" << std::endl;

                    exit(1);
                }

                parser_line = 1;
        
                inputin = inputfile;

                try
                {
                    inputparse ();
                }
                catch (GeneralError& e)
                {
                    std::cerr << e.getErrorMsg() << std::endl;
                    
                    exit(1);
                }

                fclose(inputin);
            }
        }
    }
    catch (GeneralError &e)
    {
        std::cerr << e.getErrorMsg() << std::endl << std::endl;

        exit(1);
    }

    if (parser_errors)
    {
        std::cerr << "Aborting due to parser errors." << std::endl << std::endl;

        exit(1);
    }

    
    //
    // expand constant names
    //
    insertNamespaces();

    /*
    NamesTable<std::string> names = Term::getNamesTable();

    for (NamesTable<std::string>::const_iterator nm = names.begin();
         nm != names.end();
         ++nm)
    {
        std::cout << "nametable entry: " << nm.getIndex() << " " << *nm << std::endl;
    }
*/


    if (global::optionVerbose)
    {
        std::cout << "Parsed Rules: " << std::endl;
        IDB.dump(std::cout);
        std::cout << std::endl;
        std::cout << "Parsed EDB: " << std::endl;
        //printGAtomSet(EDB, std::cout, 0);
        EDB.print(std::cout, 0);
        std::cout << std::endl;
        std::cout << std::endl;
    }

    /// @todo: when exiting after an exception, we have to cleanup things!
    // maybe using boost-pointers!

    //
    // The GraphBuilder creates nodes and dependency edges from the raw program.
    //
    GraphBuilder* gb;

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
        gb = new GraphBuilder;

        cf = new BoostComponentFinder;

        //
        // Initializing the DependencyGraph. Its constructor uses the GraphBuilder
        // to construct the graph and the ComponentFinder to find relevant graph
        // properties for the subsequent processing stage.
        //
        dg = new DependencyGraph(IDB, gb, cf);

        if (global::optionStrongSafety)
            StrongSafetyChecker sc(IDB, dg);
        else
            SafetyChecker sc(IDB);
    }
    catch (GeneralError &e)
    {
        std::cerr << e.getErrorMsg() << std::endl << std::endl;

        exit(1);
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
        //
        gp.run(EDB);
    }
    catch (GeneralError &e)
    {
        std::cerr << e.getErrorMsg() << std::endl << std::endl;

        exit(1);
    }
    
    //
    // contract constant names again
    //
    removeNamespaces();


    //
    // pack GraphProcessor result into ResultContainer
    //
    ResultContainer result;

    AtomSet* res;

    while ((res = gp.getNextModel()) != NULL)
    {
        result.addSet(*res);
    }

    //
    // remove auxiliary atoms
    //
    result.filterOut(Term::auxnames);

    
    //
    // apply filter
    //
    std::vector<Term> filter;

    for (std::vector<std::string>::const_iterator f = optionFilter.begin();
         f != optionFilter.end();
         f++)
    {
        filter.push_back(Term(*f));
    }

    if (optionFilter.size() > 0)
        result.filterIn(optionFilter);

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
    if (global::optionVerbose)
        std::cout << "@@@ Final Result @@@" << std::endl << std::endl;

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

    delete gb;

    delete dg;
}
