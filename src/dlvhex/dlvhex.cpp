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

#include "dlvhex/Atom.h"
#include "dlvhex/Interpretation.h"
#include "dlvhex/GraphProcessor.h"
#include "dlvhex/GraphBuilder.h"
#include "dlvhex/ComponentFinder.h"
#include "dlvhex/BoostComponentFinder.h"
#include "dlvhex/globals.h"
#include "dlvhex/helper.h"
#include "dlvhex/errorHandling.h"


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
GAtomSet EDB;



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
        << " [--silent]"
        << " [--firstorder]"
        << " [--verbose]"
        << " [--plugindir=dir]"
        << " [--filter=foo,bar,...]"
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
    out << "--silent           Do not display anything than the actual result." << std::endl
        << "--firstorder       No higher-order reasoning." << std::endl
        << "--verbose          dump also various intermediate information." << std::endl
        << "--plugindir=dir    Specify additional directory where to look for plugin" << std::endl
        << "                   libraries." << std::endl
        << "                   (Additionally to the installation plugin-dir and" << std::endl
        << "                   $HOME/.dlvhex/plugins)" << std::endl
        << "--filter=foo[,bar[,...]]" << std::endl
        << "                   Only display instances of the specified predicate(s)." << std::endl
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
        std::cout << "orig: nametable entry: " << *nm << std::endl;

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
        
        std::cout << "nametable entry: " << nm.getIndex() << " " << *nm << std::endl;
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


int
main (int argc, char *argv[])
{
    WhoAmI = argv[0];

    //
    // Option handling
    //
    
    bool optionPipe = false;
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
            else if (!strcmp(argv[j], "--verbose"))
                global::optionVerbose = true;
            else if (!strncmp(argv[j], "--filter=", 9))
                optionFilter = helper::stringExplode(std::string(argv[j] + 9), ",");
            else if (!strcmp(argv[j], "--"))
//                optionPipe = true;
                { std::cout << "Piping not working yet, sorry!" << std::endl; exit(-1); }
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



    //
    // no file and no stdin?
    //
    if ((allFiles.size() == 0) && !optionPipe)
    {
        printUsage(std::cerr,false);
        
        exit(-1);
    }


    int count, i;
    struct dirent **files;
    std::string filename;
    std::set<std::string> libfilelist;
    //
    // first look into specified plugin dir
    //
    if (!optionPlugindir.empty())
    {
        count = scandir(optionPlugindir.c_str(), &files, 0, alphasort);

        for (i = 0; i < count; ++i)
        {
            filename = files[i]->d_name;

    //        if (filename.substr(0,9) == "libdlvhex")
            if  (filename.size() > 3)
                if (filename.substr(filename.size() - 3, 3) == ".so")
                    libfilelist.insert(optionPlugindir + '/' + filename);
        }

        //
        // clean up scandir mess
        //
        if (count != -1)
        {
            while (count--)
                delete files[count];

            delete files; 
        }
    }


    //
    // now look into user's home
    //
    std::string userhome(::getpwuid(::geteuid())->pw_dir);

    userhome = userhome + "/" + (std::string)USER_PLUGIN_DIR;

    count = scandir(userhome.c_str(), &files, 0, alphasort);

    for (i = 0; i < count; ++i)
    {
        filename = files[i]->d_name;

//        if (filename.substr(0,9) == "libdlvhex")
        if  (filename.size() > 3)
            if (filename.substr(filename.size() - 3, 3) == ".so")
                libfilelist.insert(userhome + '/' + filename);
    }

    //
    // clean up scandir mess
    //
    if (count != -1)
    {
        while (count--)
            delete files[count];

        delete files; 
    }


    //
    // eventually look into system plugin dir
    //
    count = scandir(SYS_PLUGIN_DIR, &files, 0, alphasort);

    for (i = 0; i < count; ++i)
    {
        filename = files[i]->d_name;

//        if (filename.substr(0,9) == "libdlvhex")
        if  (filename.size() > 3)
            if (filename.substr(filename.size() - 3, 3) == ".so")
                libfilelist.insert((std::string)SYS_PLUGIN_DIR + '/' + filename);
        
    }

    //
    // clean up scandir mess
    //
    if (count != -1)
    {
        while (count--)
            delete files[count];

        delete files; 
    }


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
    
    if (optionPipe)
    {
        parser_file = "";
        parser_line = 1;

        inputin = stdin;

        inputparse();
    }
    else
    {
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

    if( parser_errors )
    {
        std::cerr << "Aborting due to parser errors." << std::endl;

        exit(1);
    }

    
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
        printGAtomSet(EDB, std::cout, 0);
        std::cout << std::endl;
        std::cout << std::endl;
    }


//    Atom* a = new Atom("p(q)");
//    std::stringstream out;
//    a->print(out, 0);
//    std::cout << "---" << out.str() << "---";

    //
    // The GraphBuilder creates nodes and dependency edges from the raw program.
    //
    GraphBuilder* gb = new GraphBuilder;

    //
    // The ComponentFinder provides functions for finding SCCs and WCCs from a
    // set of nodes.
    //
    ComponentFinder* cf = new BoostComponentFinder;

    /// @todo: when exiting after an exception, we have to cleanup things!

    //
    // Initializing the DependencyGraph. Its constructor uses the GraphBuilder
    // to construct the graph and the ComponentFinder to find relevant graph
    // properties for the subsequent processing stage.
    //
    DependencyGraph* dg;

    try
    {
        dg = new DependencyGraph(IDB, gb, cf);
    }
    catch (GeneralError &e)
    {
        std::cerr << e.getErrorMsg() << std::endl;

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
        std::cerr << e.getErrorMsg() << std::endl;
        
        exit(1);
    }
    

    removeNamespaces();

    
    //
    // filtering result models
    //
    std::ostringstream finaloutput;
    GAtomSet* res;
    Interpretation iout;
   
    //
    // go through all result models of the GraphProcessor
    //
    while ((res = gp.getNextModel()) != NULL)
    {
        GAtomSet filtered;

        //
        // initialize iout to the result model
        //
        iout.replaceBy(*res);
       
        //
        // do we have to filter the result?
        //
        if (optionFilter.size() > 0)
        {
            GAtomSet g;
            
            //
            // take each filter predicate
            //
            for (std::vector<std::string>::const_iterator f = optionFilter.begin();
                 f != optionFilter.end();
                 f++)
            {
                //
                // extract matching facts from the current result set
                //
                iout.matchPredicate(*f, g);
             
                filtered.insert(g.begin(), g.end());
            }
        }
        else
        {
            //
            // no filters - the take the entire model
            filtered = iout.getAtomSet();
        }

        //
        // stringify the result set
        //
        printGAtomSet(filtered, finaloutput, 0);

        //
        // build output stream
        //
        finaloutput << std::endl;
    }
    
    //
    // was there anything non-error the user should know? dump it directly
    //
    for (std::vector<std::string>::const_iterator l = global::Messages.begin();
         l != global::Messages.end();
         l++)
    {
        std::cout << *l << std::endl;
    }
    

    //
    // eventually, display the result
    //
    std::cout << finaloutput.str() << std::endl;


    //
    // cleaning up:
    //
    delete cf;

    delete gb;

    delete dg;
}
