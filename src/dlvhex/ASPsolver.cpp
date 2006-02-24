/* -*- C++ -*- */

/**
 * @file   dlvhex.cpp
 * @author Roman Schindlauer
 * @date   Tue Nov 15 17:29:45 CET 2005
 * 
 * @brief  Definition of ASP solver class.
 * 
 */

#include "dlvhex/ASPsolver.h"
#include "dlvhex/errorHandling.h"
#include "dlvhex/helper.h"
#include "dlvhex/globals.h"

#include "../config.h"


namespace solverResult
{
    std::vector<AtomSet> answersets;

    unsigned returncode;

    std::string message;

    AtomSet*
    createNewAnswerset()
    {
        solverResult::answersets.push_back(AtomSet());

        return &(solverResult::answersets.back());
    }

    void
    addMessage(std::string msg)
    {
        message += msg + '\n';
    }
}



ASPsolver::ASPsolver()
    : lpcommand(DLVPATH)
{
    solverResult::answersets.clear();
    
    answerSetIndex = solverResult::answersets.end();
}


AtomSet*
ASPsolver::getNextAnswerSet()
{
    if (answerSetIndex != solverResult::answersets.end())
        return &(*(answerSetIndex++));

    return NULL;
}


unsigned
ASPsolver::numAnswerSets()
{
    return solverResult::answersets.size();
}



//
// Where LEX reads its input from:
//
extern "C" FILE * dlvresultin;

extern int dlvresultparse();

extern bool dlvresultdebug;


void
ASPsolver::callSolver(std::string prg, bool noEDB)
{
    solverResult::answersets.clear();
    solverResult::message = "";
    
    std::string dlvOptions("");

    if (noEDB)
        dlvOptions = "-nofacts";
    
    std::cout << "ASP solver input:" << std::endl << prg << std::endl << std::endl;
    

    //char tempfile[] = "/tmp/dlvXXXXXX";
    char tempfile[L_tmpnam];

    std::string execdlv;

    //
    // if-branch: using a temporary file for the asp-program
    // else-branch: passing the program to dlv via echo and pipe
    //
    if (1)
    {
        tmpnam(tempfile);

        FILE* dlvinput = fopen(tempfile, "w");
//        FILE* dlvinput = mkstemp(tempfile);

        if (dlvinput == NULL)
            throw FatalError("LP solver temp-file " + (std::string)tempfile + " could not be created!");

        fputs(prg.c_str(), dlvinput);
        fflush(dlvinput);
        fclose(dlvinput);

        execdlv = lpcommand + " " +
                  dlvOptions + " " +
                  (std::string)tempfile + " 2>&1; echo $?";
    }
    else
    {
        //
        // escape quotes for shell command execution with echo!
        //
        helper::escapeQuotes(prg);
        
        execdlv = "echo \"" + prg + "\" | " +
                  lpcommand + " " + 
                  dlvOptions + " -- 2>&1; echo $?";
    }

    //std::cout << execdlv << std::endl;

    dlvresultin = popen(execdlv.c_str(), "r");

    dlvresultparse ();

    fclose(dlvresultin);

    unlink(tempfile);

    //std::cout << solverResult::returncode << std::endl;
    
    if (solverResult::returncode == 127)
    {
        throw FatalError("LP solver command not found!");
    }
    
    //
    // other problem:
    //
    if (solverResult::returncode != 0)
    {
        std::string dlverror("LP solver failure!");

        if (global::optionVerbose)
        {
            dlverror += "\nTry to call dlv manually with this program and see what happens:\n";
            dlverror += prg + "\n";
        }
        else
        {
            dlverror += " Run with --verbose for more info.\n";
        }

        throw FatalError(dlverror);
    }
    
    // TODO: what to do with solverResult::message?

    //
    // reset retrieval pointer:
    //

    answerSetIndex = solverResult::answersets.begin();

    
    /*
    for (std::vector<AtomSet>::iterator o = solverResult::answersets.begin();
            o != solverResult::answersets.end();
            o++)
    {
        std::cout << "as: ";
        (*o).print(std::cout, 0);
        std::cout << std::endl;
        
        //for (AtomSet::const_iterator foo = (*o).begin();foo != (*o).end();++foo)
        //    std::cout << "have predicate: " << foo->getPredicate() << std::endl;
    }
    */
}

