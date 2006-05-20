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
#include <sstream>
#include <stdio.h>
// for waitpid:
#include <sys/wait.h>

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
#include <cerrno>
#include <cstdio>
#include <csignal>

void
ASPsolver::callSolver(std::string prg, bool noEDB)// throw (FatalError)
{
    //
    // dirty hack: add stuff for each solver call from globals:
    //
    prg = global::maxint + "\n" + prg;

    answersets.clear();
    
    std::cout << "ASP solver input:" << std::endl << prg << std::endl << std::endl;

    int retcode;

    int outpipes[2];
    int inpipes[2];
    if (pipe(outpipes) < 0)
        perror("pipes");
    if (pipe(inpipes) < 0)
        perror("pipes");


    pid_t ret = fork();

    switch(ret)
    {
    case -1: // error
        exit(ret);
        return;

    case 0: // child
        if (dup2(outpipes[1], STDOUT_FILENO) < 0) exit(1);
        if (dup2(inpipes[0], STDIN_FILENO) < 0) exit(1);

        close(outpipes[0]);
        close(inpipes[1]);

        if (noEDB)
        {
            char* argv[5] = { "dlv", "-silent", "-nofacts", "--", 0 };
            execv(lpcommand.c_str(), argv);
        }
        else
        {
            char* argv[4] = { "dlv", "-silent", "--", 0 };
            execv(lpcommand.c_str(), argv);
        }

        perror("exec");
        exit(125 + errno); // never here
        break;

    default: // parent
        close(outpipes[1]);
        close(inpipes[0]);

        __gnu_cxx::stdio_filebuf<char> in(outpipes[0], std::ios::in);
        __gnu_cxx::stdio_filebuf<char> out(inpipes[1], std::ios::out);
        std::istream inpipe(&in);
        std::ostream outpipe(&out);

        // 	std::streambuf* tmp = std::cout.rdbuf();
        // 	std::cout.rdbuf(&out);
        // 	std::cout << prg << std::endl;
        // 	std::cout.flush();
        // 	std::cout.rdbuf(tmp);

        outpipe << prg << std::endl;
        outpipe.flush();

        close(inpipes[1]); // send EOF to dlv

        DLVresultParserDriver driver;

        try
        {
            driver.parse(inpipe, answersets, retcode);
        }
        catch (GeneralError& e)
        {
            throw FatalError(e.getErrorMsg());
        }

        // get return value of dlv process
        waitpid(ret, &retcode, 0);

        // we're done reading
        close(outpipes[1]);

        break;
    }

    if (retcode == 127)
    {
        throw FatalError("LP solver command not found!");
    }
    
    //
    // other problem:
    //
    if (retcode != 0)
    {
        std::stringstream errstr;

        errstr << "LP solver failure: returncode: " << retcode;

        std::string dlverror(errstr.str());

        if (global::optionVerbose)
        {
            dlverror += "\nexecuted: " + lpcommand + " -silent ";
	    if (noEDB) dlverror += " -nofacts ";
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
    for (std::vector<AtomSet>::iterator o = answersets.begin();
            o != answersets.end();
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

