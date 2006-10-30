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
 * To the developer of a plugin, the documentation of the following classes are
 * most relevant:
 * - Term
 * - Atom
 * - AtomSet
 * - PluginAtom
 * - PluginAtom::Query
 * - PluginAtom::Answer
 * - PluginInterface
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
#include "dlvhex/PrintVisitor.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H


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
    out << "     --               Parse from stdin." << std::endl
        << " -s, --silent         Do not display anything than the actual result." << std::endl
//        << "--strongsafety     Check rules also for strong safety." << std::endl
        << " -v, --verbose[=N]    Specify verbose level (default: 1). dump various" << std::endl
        << "                      intermediate information and write the program graph to" << std::endl
        << "                      FILENAME.dot." << std::endl
        << " -p, --plugindir=dir  Specify additional directory where to look for plugin" << std::endl
        << "                      libraries (additionally to the installation plugin-dir" << std::endl
        << "                      and $HOME/.dlvhex/plugins)." << std::endl
        << " -f, --filter=foo[,bar[,...]]" << std::endl
        << "                      Only display instances of the specified predicate(s)." << std::endl
        << " -a, --allmodels      Display all models, even under weak constraints." << std::endl
        << "     --firstorder     No higher-order reasoning." << std::endl
        << "     --ruleml         Output in RuleML-format (v0.9)." << std::endl
        << "     --noeval         Just parse the program, don't evaluate it (only useful" << std::endl
        << "                      with --verbose)." << std::endl
        << "     --keepnsprefix   Keep specified namespace-prefixes in the result." << std::endl
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
    /*
    Atom* a = new Atom("p");
    ExternalAtom* e = new ExternalAtom();

    AtomPtr apa(a);
    AtomPtr ape(e);

    bool b;

    if (*(apa.get()) == *(ape.get())) b = true;
    if (*(ape.get()) == *(apa.get())) b = false;
    exit(0);
    */

    WhoAmI = argv[0];

    //
    // Option handling
    //
    
    // global defaults:
    Globals::Instance()->setOption("NoPredicate", 1);
    Globals::Instance()->setOption("Silent", 0);
    Globals::Instance()->setOption("Verbose", 0);
    Globals::Instance()->setOption("NoPredicate", 1);
    Globals::Instance()->setOption("StrongSafety", 1);
    Globals::Instance()->setOption("AllModels", 0);
    
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
    
    while ((ch = getopt_long(argc, argv, "f:hsvp:a", longopts, NULL)) != -1)
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


    //
    // now search for plugins
    //
    std::set<std::string> libfilelist;

    std::stringstream allpluginhelp;

    //
    // first look into specified plugin dir
    //
    if (!optionPlugindir.empty())
    {
        if (Globals::Instance()->getOption("Verbose"))
            std::cout << "searching " << optionPlugindir << "..." << std::endl;

        searchPlugins(optionPlugindir, libfilelist);
    }

    //
    // now look into user's home
    //
    std::string userhome(::getpwuid(::geteuid())->pw_dir);

    userhome = userhome + "/" + (std::string)USER_PLUGIN_DIR;

    if (Globals::Instance()->getOption("Verbose"))
        std::cout << "searching " << userhome << "..." << std::endl;

    searchPlugins(userhome, libfilelist);

    //
    // eventually look into system plugin dir
    //
    if (Globals::Instance()->getOption("Verbose"))
        std::cout << "searching " << SYS_PLUGIN_DIR << "..." << std::endl;

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

                allpluginhelp << pluginhelp.str();
            }
        }
        catch (GeneralError &e)
        {
            std::cerr << e.getErrorMsg() << std::endl;
            
            exit(1);
        }
    }

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

    //if (!global::optionSilent)
    //    std::cout << std::endl;
    
    //
    // parse input
    //
    
    if (Globals::Instance()->getOption("Verbose"))
        std::cout << std::endl << "@@@ reading input @@@" << std::endl << std::endl;

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

        Globals::Instance()->lpfilename = "lpgraph.dot";
    }
    else
    {
        //
        // store filename of (first) logic program, we might use this somewhere
        // else (e.g., when writing the graphviz file in the boost-part
        //
        std::vector<std::string> filepath = helper::stringExplode(allFiles[0], "/");
        Globals::Instance()->lpfilename = filepath.back() + ".dot";

        std::ifstream ifs;

        for (std::vector<std::string>::const_iterator f = allFiles.begin();
            f != allFiles.end();
            f++)
        {
            try
            {
                std::ifstream ifs;

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

                //
                // make a new stream to store the file content
                //
                std::stringstream tmpin;
                tmpin << ifs.rdbuf();

                //
                // create a stringbuffer on the heap (will be deleted later) to
                // hold the file-content. put it into a stream "input"
                //
                std::istream input(new std::stringbuf(tmpin.str()));

                //
                // new output stream with stringbuffer on the heap
                //
                std::ostream rewriterResult(new std::stringbuf);

                bool wasRewritten(0);

                for (std::vector<PluginInterface*>::iterator pi = plugins.begin();
                     pi != plugins.end();
                     ++pi)
                {
                    PluginRewriter* pr = (*pi)->createRewriter(input, rewriterResult);

                    if (pr != NULL)
                    {
                        //
                        // rewrite input -> rewriterResult
                        //
                        pr->rewrite();

                        wasRewritten = 1;
                   
                        //
                        // old input buffer can be deleted now
                        //
                        delete input.rdbuf();

                        //
                        // store the current output buffer
                        //
                        std::streambuf* tmp = rewriterResult.rdbuf();

                        //
                        // make a new buffer for the output (=reset the output)
                        //
                        rewriterResult.rdbuf(new std::stringbuf);

                        //
                        // set the input buffer to be the output of the last
                        // rewriting. now, after each loop, the rewritten
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
                
                if (Globals::Instance()->doVerbose(Globals::DUMP_REWRITTEN_PROGRAM) && wasRewritten)
                {
                    //
                    // we need to read the input-istream now - use a stringstream
                    // for output and initialize the input-istream to its
                    // content again
                    //
                    std::stringstream ss;
                    ss << input.rdbuf();
                    Globals::Instance()->getVerboseStream() << "@@@ rewritten program: @@@" << std::endl;
                    Globals::Instance()->getVerboseStream() << ss.str();
                    Globals::Instance()->getVerboseStream() << std::endl << std::endl;
                    delete input.rdbuf(); 
                    input.rdbuf(new std::stringbuf(ss.str()));
                }

                FILE* fp;

                //
                // now call dlt if needed
                //
                if (optiondlt)
                {
                    if (Globals::Instance()->getOption("Verbose"))
                    {
                        std::cout << "@@@ calling dlt @@@" << std::endl;
                    }
                    
                    mkstemp(tempfile);

                    std::ofstream dlttemp(tempfile);

                    //
                    // write program into tempfile
                    //
                    dlttemp << input.rdbuf();

                    dlttemp.close();

                    std::string execPreParser("dlt -silent -preparsing " + std::string(tempfile));
                    //std::cout << "dlttemp: " << std::string(tempfile) << std::endl;
                        
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
                //e.setFile(*f);
                std::cerr << e.getErrorMsg() << std::endl;
                
                exit(1);
            }
            catch (GeneralError& e)
            {
                std::cerr << e.getErrorMsg() << std::endl;

                exit(1);
            }

        }
    }

    
    //
    // expand constant names
    //
    insertNamespaces();

    
    
    /*
    for (NamesTable<std::string>::const_iterator nm = Term::names.begin();
         nm != Term::names.end();
         ++nm)
    {
        std::cerr << "nametable entry: " << nm.getIndex();
        std::cerr << " " << *nm << std::endl;
    }
    */



    if (Globals::Instance()->getOption("Verbose"))
    {
        RawPrintVisitor rpv(std::cerr);
        std::cerr << "Parsed Rules: " << std::endl;
        IDB.dump(std::cerr);
        std::cerr << std::endl << "Parsed EDB: " << std::endl;
        EDB.accept(rpv);
        std::cerr << std::endl << std::endl;
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
        delete gb;

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
        //
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


    //
    // put GraphProcessor result into ResultContainer
    //
    AtomSet* res;

    while ((res = gp.getNextModel()) != NULL)
    {
        result.addSet(*res);
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
    if (Globals::Instance()->getOption("Verbose"))
        std::cerr << "@@@ Final Result @@@" << std::endl << std::endl;

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

    exit(0);
}
