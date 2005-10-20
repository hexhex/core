/** @file ASPsolver.cpp
 * Definition of ASP solver class
 *  
 * @date 2005.07.05
 * @author Renato Umeton
 */

#include "dlvhex/ASPsolver.h"
#include "dlvhex/errorHandling.h"
#include "dlvhex/helper.h"

#include "../config.h"

namespace solverResult
{
    std::vector<GAtomSet> answersets;

    unsigned returncode;

    std::string message;

    GAtomSet* createNewAnswerset()
    {
        solverResult::answersets.push_back(GAtomSet());

        return &(solverResult::answersets.back());
    }

    void addMessage(std::string msg)
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


GAtomSet* ASPsolver::getNextAnswerSet()
{
    if (answerSetIndex != solverResult::answersets.end())
        return &(*(answerSetIndex++));
    else
        return NULL;
}


extern "C" FILE * dlvresultin;         // Where LEX reads its input from
extern int dlvresultparse();
extern bool dlvresultdebug;

void ASPsolver::callSolver(std::string prg)
{
    solverResult::answersets.clear();
    solverResult::message = "";
    
    FILE *fp;
    
    //std::cout << prg << std::endl;
    
    //
    // escape quotes for shell command execution with echo!
    //
    helper::escapeQuotes(prg);
    
    std::string execdlv("echo \"" + prg + "\" | " + lpcommand + " -nofacts -- foo 2>&1; echo $?");

    //std::cout << execdlv << std::endl;
    //std::cout << "program: " << prg << std::endl;
    
    fp = popen(execdlv.c_str(), "r");


    // dlvresultdebug = 1;

    dlvresultin = fp;
    dlvresultparse ();
    fclose(dlvresultin);

    //std::cout << solverResult::returncode << std::endl;
    
    if (solverResult::returncode == 127)
    {
        throw fatalError("LP solver command not found!");
    }
    
    //
    // other problem:
    //
    if (solverResult::returncode != 0)
    {
        throw fatalError("LP solver aborted due to program errors!");
    }
    
    // TODO: what to do with solverResult::message?

    //
    // reset retrieval pointer:
    //

    answerSetIndex = solverResult::answersets.begin();

    /*
    for (std::vector<GAtomSet>::iterator o = solverResult::answersets.begin();
            o != solverResult::answersets.end();
            o++)
        cout << "as: " << *o << endl;
        */
}



/******************************************************************************
 *
 * PRIVATE MEMBER FUNCTIONS
 *
 */
