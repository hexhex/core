/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with dlvhex; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */


/**
 * @file   ASPSolver.tcc
 * @author Thomas Krennwallner
 * @date   Thu Feb 21 09:20:32 CET 2008
 * 
 * @brief  Definition of ASP solver class.
 * 
 */

#if !defined(_DLVHEX_ASPSOLVER_TCC)
#define _DLVHEX_ASPSOLVER_TCC


// @todo: no header should use config.h, so i guess no .tcc should use it (at least no installed header/.tcc)
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#  ifdef DLVHEX_DEBUG
#    define DLVHEX_BENCHMARK
#  endif

#include "dlvhex/Error.h"
#include "dlvhex/Program.h"
#include "dlvhex/AtomSet.h"
#include "dlvhex/globals.h"
#include "dlvhex/Benchmarking.h"

#include <sstream>

DLVHEX_NAMESPACE_BEGIN


template<typename Builder, typename Parser>
ASPSolver<Builder,Parser>::ASPSolver(Process& p)
  : proc(p)
{ }


template<typename Builder, typename Parser>
void
ASPSolver<Builder,Parser>::solve(const Program& prg,
				 const AtomSet& facts,
				 std::vector<AtomSet>& as)
  throw (FatalError)
{
  int retcode = -1;
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"ASPSolver<B,P>::solve (+parse)");
  
  try
    {
      proc.spawn();

      // send program and facts
      try
        {
	  Builder builder(proc.getOutput());
		///@todo: this is marked as "temporary hack" in globals.h
	  if( !Globals::Instance()->maxint.empty() )
	    proc.getOutput() << Globals::Instance()->maxint << std::endl;
	  const_cast<Program&>(prg).accept(builder);
	  const_cast<AtomSet&>(facts).accept(builder);
	}
      catch (std::ios_base::failure& e)
        {
	  std::stringstream errstr1;
	  std::stringstream errstr2;

	  errstr2 << proc.path() << " error message:" << std::endl;

	  std::istream& inp = proc.getInput();
	  std::string s;

	  while (inp.good() && std::getline(inp, s))
	    {
	      errstr2 << s << std::endl;
	    }

	  // now get the exit code of the process
	  retcode = proc.close();
	     
	  errstr1 << "Received an error while sending a program to " << proc.path() 
		 << " (exitcode = " << retcode << "): "
		 << e.what();

	  throw FatalError(errstr1.str() + errstr2.str());
        }
      
      proc.endoffile(); // send EOF to process

      // parse result
      Parser parser;
      parser.parse(proc.getInput(), as);
      
      // get exit code of process
      retcode = proc.close();

    }
  catch (GeneralError& e)
    {
      std::stringstream errstr;

      // get exit code of process
      if (retcode == -1)
	{
	  retcode = proc.close();
	}

      errstr << proc.path() << " (exitcode = " << retcode << "): " + e.getErrorMsg();

      throw FatalError(errstr.str());
    }
  catch (std::exception& e)
    {
      throw FatalError(proc.path() + ": " + e.what());
    }

  // check for errors
  if (retcode == 127)
    {
      throw FatalError("LP solver command `" + proc.path() + "´ not found!");
    }
  else if (retcode != 0) // other problem
    {
      std::stringstream errstr;

      errstr << "LP solver `" << proc.path() << "´ bailed out with exitcode " << retcode << ": "
	     << "re-run dlvhex with `strace -f´.";

      throw FatalError(errstr.str());
    }
}



template<typename Parser>
ASPFileSolver<Parser>::ASPFileSolver(Process& p, const std::vector<std::string>& o)
  : proc(p),
    options(o)
{ }


template<typename Parser>
void
ASPFileSolver<Parser>::solve(const Program&, const AtomSet&, std::vector<AtomSet>& as)
  throw (FatalError)
{
  int retcode = -1;
  
  try
    {
      DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(aspCallParse,"ASPFilesSolver<P>::solve (+parse)");

      proc.spawn(options);

      proc.endoffile(); // send EOF to process

      // parse result
      Parser parser;
      parser.parse(proc.getInput(), as);
      
      // get exit code of process
      retcode = proc.close();
    }
  catch (GeneralError& e)
    {
      std::stringstream errstr;

      // get exit code of process
      if (retcode == -1)
	{
	  retcode = proc.close();
	}

      errstr << proc.path() << " (exitcode = " << retcode << "): " + e.getErrorMsg();

      throw FatalError(errstr.str());
    }
  catch (std::exception& e)
    {
      throw FatalError(proc.path() + ": " + e.what());
    }

  // check for errors
  if (retcode == 127)
    {
      throw FatalError("LP solver command `" + proc.path() + "´ not found!");
    }
  else if (retcode != 0) // other problem
    {
      std::stringstream errstr;

      errstr << "LP solver `" << proc.path() << "´ bailed out with exitcode " << retcode << ": "
	     << "re-run dlvhex with `strace -f´.";

      throw FatalError(errstr.str());
    }
}



DLVHEX_NAMESPACE_END

#endif // _DLVHEX_ASPSOLVER_TCC

// vim:se ts=8:
// Local Variables:
// mode: C++
// End:
