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
 * @file ASPSolver.cpp
 * @author Thomas Krennwallner
 * @date Tue Jun 16 14:34:00 CEST 2009
 *
 * @brief ASP Solvers
 *
 *
 */


#include "dlvhex/ASPSolver.h"
#include "dlvhex/DLVresultParserDriver.h"

#include <cassert>

DLVHEX_NAMESPACE_BEGIN


ASPSolverComposite::ASPSolverComposite()
  : solvers()
{ }


ASPSolverComposite::~ASPSolverComposite()
{
  for (std::vector<BaseASPSolver*>::iterator it = solvers.begin(); it != solvers.end(); ++it)
    {
      delete *it;
    }
}


void
ASPSolverComposite::addSolver(BaseASPSolver* s)
{
  solvers.push_back(s);
}

  
void
ASPSolverComposite::solve(const Program& p, const AtomSet& s, std::vector<AtomSet>& as) throw (FatalError)
{
  for (std::vector<BaseASPSolver*>::iterator it = solvers.begin(); it != solvers.end(); ++it)
    {
      (*it)->solve(p, s, as);
    }
}


ASPStringSolver::ASPStringSolver(Process& proc):
	proc(proc)
{
}

ASPStringSolver::~ASPStringSolver()
{
}

//! give a DLV program as a string to this function and it returns the answer sets (or throws)
void
ASPStringSolver::solve(const std::string& input, std::vector<AtomSet>& as) throw (FatalError)
{
  int retcode = -1;
  
  try
    {
      DEBUG_START_TIMER;

      proc.spawn();
      proc.getOutput() << input << std::endl;
      proc.endoffile();

      // parse result
      DLVresultParserDriver parser;
      parser.parse(proc.getInput(), as);
      
      // get exit code of process
      retcode = proc.close();

      //                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
      DEBUG_STOP_TIMER("Calling LP solver + result parsing:     ");
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

/* vim: set noet sw=4 ts=8 tw=80: */
// Local Variables:
// mode: C++
// End:
