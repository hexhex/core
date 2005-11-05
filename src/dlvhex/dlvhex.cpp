/* -*- C++ -*- */

/**
 * @file   dlvhex.cpp
 * @author Roman Schindlauer
 * @date   Thu Apr 28 15:00:10 2005
 * 
 * @brief  main().
 * 
 */

/** @mainpage dlvhex Documentation
 *
 * @section intro_sec Introduction
 *
 * TODO
 *
 * @section install_sec Installation
 *
 * TODO
 *
 * @section Writing Plugins for dlvhex
 *  
 * see \ref plugininterface_page "The Plugin Interface"
 */


#include <iostream>
#include <sstream>
#include <vector>

#include "dlvhex/Atom.h"
#include "dlvhex/Interpretation.h"
#include "dlvhex/GraphProcessor.h"
#include "dlvhex/globals.h"
#include "dlvhex/helper.h"
#include "dlvhex/errorHandling.h"


unsigned parser_line;
const char *parser_file;
unsigned parser_errors = 0;


const char*  WhoAmI;


/**
 * @brief Stores the rules of the program in a structure.
 */
Rules IDB;


/**
 * @brief Stores the facts of the program in a structure.
 */
GAtomSet EDB;



//std::vector<EXTATOM> externalAtoms;


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
    out  << "usage: " << WhoAmI 
         << " [-silent]"
         << " [-filter=foo,bar,...]"
         << " [filename [filename [...]]]" << std::endl
         << std::endl;

    if (full)
    { 
    }
    
    return;
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

//
// definition for getwd ie MAXPATHLEN etc
//
#include <sys/param.h>
#include <stdio.h>
#include <dlfcn.h>

#include "dlvhex/ProgramBuilder.h"
#include "dlvhex/GraphProcessor.h"

void
insertNamespaces()
{
    if (Term::namespaces.size() == 0)
        return;

    std::string prefix, fullns;

    for (NamesTable<std::string>::const_iterator nm = Term::names.begin();
         nm != Term::names.end();
         ++nm)
    {
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

                std::cout << "modified: " << r << std::endl;
            }
        }
        
    }
}

void
removeNamespaces()
{
    if (Term::namespaces.size() == 0)
        return;

    std::string prefix, fullns;

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
    std::vector<std::string> optionFilter;
    
    std::vector<std::string> allFiles;
    
    for (int j = 1; j < argc; j++)
	{
        if (argv[j][0] == '-')
        {
            if (!strcmp(argv[j],"-fo"))
                global::optionNoPredicate = false;
            else if (!strcmp(argv[j], "-silent"))
                global::optionSilent = true;
            else if (!strncmp(argv[j], "-filter=", 8))
                optionFilter = helper::stringExplode(std::string(argv[j] + 8), ",");
            else if (!strcmp(argv[j], "--"))
//                optionPipe = true;
                { std::cout << "Piping not working yet, sorry!" << std::endl; exit(-1); }
            else if (!strcmp(argv[j], "-h") || !strcmp(argv[j], "-help"))
            {
                printLogo();
                printUsage(std::cout, false);
                exit(0);
            }
            else 
            {
                // TODO:
                // for now: don't consider unknown options!
                printLogo();
                printUsage(std::cout,true);
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


    //
    // look for all shared libraries we find
    //
    int count, i;
    struct dirent **files;
    std::string filename;

    count = scandir(PLUGIN_DIR, &files, 0, alphasort);

    for (i = 1; i < count + 1; ++i)
    {
        filename = files[i - 1]->d_name;

        if (filename.substr(0,9) == "libdlvhex")
        {
            if (filename.substr(filename.size() - 3, 3) == ".so")
            {
                // TODO: test if we really have to add the slash to the path!
                filename = (std::string)PLUGIN_DIR + '/' + filename;

                try
                {
                    PluginContainer::Instance()->importPlugin(filename);
                }
                catch (fatalError &e)
                {
                    std::cerr << e.getErrorMsg() << std::endl;
                    
                    exit(1);
                }
            }
        }
    }


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

            FILE* fp = fopen(parser_file, "r");

            if (fp == 0)
            {
                std::cerr << "file " << parser_file << " not found" << std::endl;
            }
            else
            {
                //
                // we start line numbering with 1
                //
                parser_line = 1;

                inputin = fp;

                try
                {
                    inputparse ();
                }
                catch (generalError& e)
                {
                    std::cerr << e.getErrorMsg() << std::endl;

                    exit(1);
                }
            }
        }
    }

    if( parser_errors )
    {
        std::cerr << "Aborting due to parser errors." << std::endl;

        exit(1);
    }

    
    // testing the parser:
    
    /*
    ProgramDLVBuilder dlvprogram(global::optionNoPredicate);
    for (Rules::const_iterator r = IDB.begin(); r != IDB.end(); r++)
        dlvprogram.buildRule(*r);
    std::cout << "parser test: " << std::endl;
    std::cout << "IDB: " << std::endl << dlvprogram.getString() << std::endl;
    std::cout << "EDB: " << std::endl;
    printGAtomSet(EDB, std::cout, 0);
    std::cout << std::endl;
    //exit(0);
    */
    
/*    for (vector<EXTATOM>::const_iterator r = externalAtoms.begin(); r != externalAtoms.end(); r++)
        std::cout << (*r) << std::endl;
    
    */

    insertNamespaces();

    GraphBuilder* sgb = new SimpleGraphBuilder;

    DependencyGraph dg(IDB, sgb);
    
    GraphProcessor gp(&dg);
    
    try
    {
        gp.run(EDB); 
    }
    catch (generalError &e)
    {
        std::cerr << e.getErrorMsg() << std::endl;
        
        exit(1);
    }


    //removeNamespaces();

    
    //
    // filtering result models
    //
    std::ostringstream finaloutput;
    GAtomSet* res;
    GAtomSet filtered;
    Interpretation iout;
    
    while ((res = gp.getNextModel()) != NULL)
    {
        iout.replaceBy(*res);
       
        if (optionFilter.size() > 0)
        {
            GAtomSet g;
            
            for (std::vector<std::string>::const_iterator f = optionFilter.begin(); f != optionFilter.end(); f++)
            {
                iout.matchPredicate(*f, g);
             
                filtered.insert(g.begin(), g.end());
            }
        }
        else
        {
            filtered = iout.getAtomSet();
        }

        printGAtomSet(filtered, finaloutput, 0);

        finaloutput << std::endl;
    }

    for (std::vector<std::string>::const_iterator l = global::Messages.begin();
         l != global::Messages.end();
         l++)
        std::cout << *l << std::endl;
    

    std::cout << finaloutput.str() << std::endl;

}
