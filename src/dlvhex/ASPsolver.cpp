/* -*- C++ -*- */

/**
 * @file   dlvhex.cpp
 * @author Roman Schindlauer
 * @date   Tue Nov 15 17:29:45 CET 2005
 * 
 * @brief  Definition of ASP solver class.
 * 
 */


#include <fstream>
#include <stdio.h>

#include "dlvhex/ASPsolver.h"
#include "dlvhex/Error.h"
#include "dlvhex/helper.h"
#include "dlvhex/globals.h"
#include "dlvhex/DLVresultParserDriver.h"

#include "../config.h"



ASPsolver::ASPsolver()
    : lpcommand(DLVPATH)
{
    answersets.clear();
    
    answerSetIndex = answersets.end();
}


AtomSet*
ASPsolver::getNextAnswerSet()
{
    if (answerSetIndex != answersets.end())
        return &(*(answerSetIndex++));

    return NULL;
}


unsigned
ASPsolver::numAnswerSets()
{
    return answersets.size();
}



#include <ext/stdio_filebuf.h> 

void
ASPsolver::callSolver(std::string prg, bool noEDB)// throw (FatalError)
{

    answersets.clear();
    
    std::string dlvOptions("");

    if (noEDB)
        dlvOptions = "-nofacts";
    
    //std::cout << "ASP solver input:" << std::endl << prg << std::endl << std::endl;

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

    FILE* fp = popen(execdlv.c_str(), "r");

    //
    // creating the filebuf on the heap, is deleted somewhere else and causes a
    // segfault on the stack
    //
    __gnu_cxx::stdio_filebuf<char>* fb = new __gnu_cxx::stdio_filebuf<char>(fp, std::ios::in);

    std::istream inpipe(fb);
    
    
    DLVresultParserDriver driver;

    int retcode;

    try
    {
        driver.parse(inpipe, answersets, retcode);
    }
    catch (GeneralError& e)
    {
        throw FatalError(e.getErrorMsg());
    }

    pclose(fp);

    unlink(tempfile);

    if (retcode == 127)
    {
        throw FatalError("LP solver command not found!");
    }
    
    //
    // other problem:
    //
    if (retcode != 0)
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
    
    //
    // reset retrieval pointer:
    //
    answerSetIndex = answersets.begin();
    

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

