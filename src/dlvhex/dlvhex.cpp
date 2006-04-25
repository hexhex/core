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
    out << "Usage: " << WhoAmI 
        << " [OPTION] ... FILENAME ..." << std::endl
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
    out << "     --               parse from stdin" << std::endl
        << " -s, --silent         do not display anything than the actual result" << std::endl
//        << "--strongsafety     Check rules also for strong safety." << std::endl
        << " -v, --verbose        dump also various intermediate information" << std::endl
        << " -p, --plugindir=dir  specify additional directory where to look for plugin" << std::endl
        << "                      libraries (additionally to the installation plugin-dir and" << std::endl
        << "                      $HOME/.dlvhex/plugins)" << std::endl
        << " -f, --filter=foo[,bar[,...]]" << std::endl
        << "                      only display instances of the specified predicate(s)" << std::endl
        << "     --firstorder     no higher-order reasoning" << std::endl
        << "     --ruleml         output in RuleML (v0.9) format" << std::endl
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


#include <ext/stdio_filebuf.h> 


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
    //bool optiondlt = false;

    std::vector<std::string> optionFilter;
    
    std::vector<std::string> allFiles;
    
    extern char* optarg;
    int ch;
    int longid;

    static struct option longopts[] = {
        { "help", no_argument, 0, 'h' },
        { "silent", no_argument, 0, 's' },
        { "verbose", no_argument, 0, 'v' },
        { "firstorder", no_argument, &longid, 1 },
        { "weaksafety", no_argument, &longid, 2 },
        { "ruleml", no_argument, &longid, 3 },
        { "filter", required_argument, 0, 'f' },
        { "plugindir", required_argument, 0, 'p'},
        { NULL, 0, NULL, 0 }
    };

    if (argc == 1)
    {
        printUsage(std::cerr, false);

        exit(-1);
    }

    while ((ch = getopt_long(argc, argv, "f:hsvp:", longopts, NULL)) != -1)
    {
        switch (ch)
        {
        case 'h':
            printUsage(std::cerr, true);
            exit(0);
            break;
        case 's':
            global::optionSilent = 1;
            break;
        case 'v':
            global::optionVerbose = 1;
            break;
        case 'f':
            optionFilter = helper::stringExplode(std::string(optarg), ",");
            break;
        case 'p':
            optionPlugindir = std::string(optarg);
            break;
        case 0:
            if (longid == 1)
                global::optionNoPredicate = false;
            else if (longid == 2)
                global::optionStrongSafety = false;
            else if (longid == 3)
            {
                optionXML = true;

                //
                // XMl output makes only sense with silent:
                //
                global::optionSilent = true;
            }
            break;
        case '?':
            printUsage(std::cerr, false);
            exit(0);
            break;
        }
    }

    //
    // sdtin requested
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
        {
            printUsage(std::cerr, false);

            exit(-1);
        }
    }
    else
    {
        //
        // files and stdin - error
        //
        if (optionPipe)
        {
            printUsage(std::cerr, false);

            exit(-1);
        }

        //
        // collect filenames
        //
        for (int i = optind; i < argc; ++i)
        {
            allFiles.push_back(argv[i]);
        }
    }

    if (!global::optionSilent)
        printLogo();

    if (global::optionVerbose)
        std::cout << std::endl << "@@@ reading input @@@" << std::endl << std::endl;


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
                plugins.push_back(plugin);

                plugin->setOptions(argc, argv);
            }
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
    
    HexParserDriver driver;

    if (optionPipe)
    {
        try
        {
            driver.parse(std::cin, IDB, EDB);
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

        std::ifstream ifs;

        for (std::vector<std::string>::const_iterator f = allFiles.begin();
            f != allFiles.end();
            f++)
        {
            std::string parser_file = f->c_str();


            //FILE *inputfile;

            
            /*
            std::string execPreParser("dlt -silent -preparsing " + *f);
                
            FILE* fp = popen(execPreParser.c_str(), "r");

            __gnu_cxx::stdio_filebuf<char>* fb = new __gnu_cxx::stdio_filebuf<char>(fp, std::ios::in);

            std::istream inpipe(fb);
            */
            

            std::ifstream ifs;

            ifs.open((*f).c_str());

            

            /*
            if (optiondlt)
                inputfile = popen(execPreParser.c_str(), "r");
            else
                inputfile = fopen(parser_file, "r");
            */

//                ifs.open(parser_file.c_str());

//                if (!ifs.is_open())
//                if (inputfile == NULL)
//              {
//                  if (optiondlt)
//                    std::cerr << "unable to call preparser: " << execPreParser << std::endl;
    //              else
//                    std::cerr << "file " << parser_file << " not found" << std::endl;

//                exit(1);
//            }

            //parser_line = 1;
    
            //inputin = inputfile;


            //HexParserDriver driver(ifs);
            try
            {
                if (!ifs.is_open())
                {
                    throw GeneralError("File " + *f + " not found");
                }
           /*     std::stringstream input, output;

                input << ifs;

                for (std::vector<PluginInterface*>::iterator pi = plugins.begin();
                     pi != plugins.end();
                     ++pi)
                {
                    PluginRewriter* pr = (*pi)->createRewriter(input, output);

                    if (pr != NULL)
                        pr->rewrite();
                    else
                        output << input;
                }
              */

                driver.parse(ifs, IDB, EDB);

                ifs.close();
            }
            catch (SyntaxError& e)
            {
                e.file = *f;
                std::cerr << e.getErrorMsg() << std::endl;
                
                exit(1);
            }
            catch (GeneralError& e)
            {
                //
                // input file not found - just skip it, maybe there are more
                //
                std::cerr << e.getErrorMsg() << std::endl;
            }

        }
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
