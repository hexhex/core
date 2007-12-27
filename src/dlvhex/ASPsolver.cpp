/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


/**
 * @file   ASPsolver.cpp
 * @author Roman Schindlauer
 * @date   Tue Nov 15 17:29:45 CET 2005
 * 
 * @brief  Definition of ASP solver class.
 * 
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex/ASPsolver.h"
#include "dlvhex/Error.h"
#include "dlvhex/helper.h"
#include "dlvhex/globals.h"
#include "dlvhex/DLVresultParserDriver.h"
#include "dlvhex/ProcessBuf.h"

#include <sstream>
#include <iterator>

#ifdef DLVHEX_DEBUG
#include <boost/date_time/posix_time/posix_time.hpp>
#endif // DLVHEX_DEBUG

DLVHEX_NAMESPACE_BEGIN

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

    // std::cout << "prg" << std::endl;
    // std::cout        << std::endl << prg << std::endl;
    //if (Globals::Instance()->getOption("Verbose") >= 3)
    //    Globals::Instance()->getVerboseStream() << "Sending program to dlv:"
    //        << std::endl << prg << std::endl;

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
#ifdef DLVHEX_DEBUG
		DEBUG_START_TIMER
#endif // DLVHEX_DEBUG

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

#ifdef DLVHEX_DEBUG
		//                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
		DEBUG_STOP_TIMER("Calling dlv and parsing its result     ")
#endif // DLVHEX_DEBUG
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
    	RawPrintVisitor rpv(std::cout);
	    (*o).accept(rpv);
        std::cout << std::endl;
        
        //for (AtomSet::const_iterator foo = (*o).begin();foo != (*o).end();++foo)
        //    std::cout << "have predicate: " << foo->getPredicate() << std::endl;
    }
*/
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
