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
#include "dlvhex/globals.h"
#include "dlvhex/helper.h"
#include "dlvhex/errorHandling.h"


unsigned parser_line;
const char *parser_file;
unsigned parser_errors = 0;


const char*  WhoAmI;


/**
 * @brief Stores the entire program.
 */
Program IDB;


/**
 * @brief Stores the facts of the program.
 */
GAtomSet EDB;



//std::vector<ExternalAtom> externalAtoms;


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

    NamesTable<std::string> names = Term::getNamesTable();

    for (NamesTable<std::string>::const_iterator nm = names.begin();
         nm != names.end();
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

                names.modify(nm, r);

                //std::cout << "modified: " << r << std::endl;
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

    NamesTable<std::string> names = Term::getNamesTable();

    for (NamesTable<std::string>::const_iterator nm = names.begin();
         nm != names.end();
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

                names.modify(nm, r);
            }
        }
        
    }
}

#include "pwd.h"

#include "dlvhex/Component.h"

int
main (int argc, char *argv[])
{

    WhoAmI = argv[0];

    
    //
    // Option handling
    //
    
    bool optionPipe = false;

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
            if (!strcmp(argv[j],"-dlt"))
                optiondlt = true;
            else if (!strcmp(argv[j],"-fo"))
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

    std::vector<std::string> libfilelist;

    count = scandir(PLUGIN_DIR, &files, 0, alphasort);

    for (i = 0; i < count; ++i)
    {
        filename = files[i]->d_name;

        if (filename.substr(0,9) == "libdlvhex")
            if (filename.substr(filename.size() - 3, 3) == ".so")
                libfilelist.push_back((std::string)PLUGIN_DIR + '/' + filename);
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
    // now look into user's home
    //
    std::string userhome(::getpwuid(::geteuid())->pw_dir);

    userhome = userhome + "/" + (std::string)USER_PLUGIN_DIR;

    count = scandir(userhome.c_str(), &files, 0, alphasort);

    for (i = 0; i < count; ++i)
    {
        filename = files[i]->d_name;

        if (filename.substr(0,9) == "libdlvhex")
            if (filename.substr(filename.size() - 3, 3) == ".so")
                libfilelist.push_back(userhome + '/' + filename);
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
    for (std::vector<std::string>::const_iterator si = libfilelist.begin();
         si != libfilelist.end();
         si++)
    {
        try
        {
            PluginContainer::Instance()->importPlugin(*si);
        }
        catch (fatalError &e)
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
            catch (generalError& e)
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

    

    /*    
    // testing the parser:
    ProgramDLVBuilder dlvprogram(global::optionNoPredicate);
    dlvprogram.buildProgram(IDB);
    std::cout << "parser test: " << std::endl;
    std::cout << "IDB: " << std::endl << dlvprogram.getString() << std::endl;
    std::cout << "EDB: " << std::endl;
    printGAtomSet(EDB, std::cout, 0);
    std::cout << std::endl;
    std::cout << "External Atoms: " << std::endl;
    for (std::vector<ExternalAtom>::const_iterator exi = IDB.getExternalAtoms().begin();
         exi != IDB.getExternalAtoms().end();
         ++exi)
        std::cout << *exi << std::endl;
    */

    insertNamespaces();

    GraphBuilder* sgb = new SimpleGraphBuilder;

    DependencyGraph dg(IDB, sgb);
    
    GraphProcessor<Subgraph, Component> gp(&dg);

    
    try
    {
        gp.run(EDB); 
    }
    catch (generalError &e)
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

    //
    // cleaning up:
    //
    delete sgb;
}
