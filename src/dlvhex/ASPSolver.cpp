/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2009 Thomas Krennwallner
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
 * @author Thomas Krennwallenr
 * @date Tue Jun 16 14:34:00 CEST 2009
 *
 * @brief ASP Solvers
 *
 *
 */


#include "dlvhex/ASPSolver.h"

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



DLVHEX_NAMESPACE_END

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
