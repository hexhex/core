/* -*- C++ -*- */

/**
 * @file   dlvhex.cpp
 * @author Roman Schindlauer
 * @date   Tue Nov 15 17:29:45 CET 2005
 * 
 * @brief  Definition of ASP solver class.
 * 
 */


#include <sstream>
#include <iterator>

#include "dlvhex/ASPsolver.h"
#include "dlvhex/Error.h"
#include "dlvhex/helper.h"
#include "dlvhex/globals.h"
#include "dlvhex/DLVresultParserDriver.h"
#include "dlvhex/ProcessBuf.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H


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

void
ASPsolver::callSolver(const std::string& prg, bool noEDB)// throw (FatalError)
{
    answersets.clear();

    if (Globals::Instance()->getOption("Verbose") >= 3)
        Globals::Instance()->getVerboseStream() << "Sending program to dlv:"
            << std::endl << prg << std::endl;

    // setup command
    std::vector<std::string> argv;

    argv.push_back(lpcommand);
    argv.push_back("-silent");

    if (noEDB)
      argv.push_back("-nofacts");

    argv.push_back("--");

    // dlv result stuff:
    int retcode;
    std::string dlvError;

    try
    {
        // create a new dlv process
        ProcessBuf pb;
        pb.open(argv);

        std::iostream iopipe(&pb);
	// let ProcessBuf throw std::ios_base::failure
	iopipe.exceptions(std::ios_base::badbit);

        //
        // dirty hack: add stuff for each solver call from globals:
        //
        try
        {
            iopipe << prg << std::endl << Globals::Instance()->maxint << std::endl;
        }
        catch (std::ios_base::failure e)
        {
            throw FatalError("Error executing " + lpcommand + "!");
        }

        pb.endoffile(); // send EOF to dlv

        ///todo if dlv is not executable, we get a strange process error - make
        //this more clear to the user!

        DLVresultParserDriver driver;
    
        driver.parse(iopipe, answersets, dlvError);

        // get exit code of dlv process
        retcode = pb.close();
    }
    catch (FatalError& e)
    {
        throw;
    }
    catch (GeneralError& e)
    {
        throw FatalError(e.getErrorMsg());
    }
    catch (std::exception& e)
    {
        throw FatalError(e.what());
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

        errstr << "LP solver failure: returncode: " << retcode << std::endl;

        errstr << "error msg: " << dlvError;

        if (Globals::Instance()->getOption("Verbose"))
        {
            errstr << "executed: ";

	    std::copy(argv.begin(), argv.end(),
		      std::ostream_iterator<std::string>(errstr, " "));

	    errstr << std::endl
		   << "Try to call dlv manually with this program and see what happens:"
		   << std::endl << prg
		   << std::endl;
        }
        else
        {
            errstr << std::endl << "Run with --verbose for more info." << std::endl;
        }

        throw FatalError(errstr.str());
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

