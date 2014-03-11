/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * Copyright (C) 2011, 2012, 2013 Christoph Redl
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
 * @file   SATSolver.cpp
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Interface to (genuine) SAT solvers.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "dlvhex2/SATSolver.h"
#include "dlvhex2/CDNLSolver.h"
#include "dlvhex2/ClaspSolver.h"
#include "dlvhex2/ProgramCtx.h"

DLVHEX_NAMESPACE_BEGIN

SATSolverPtr SATSolver::getInstance(ProgramCtx& ctx, NogoodSet& ns, InterpretationConstPtr frozen){

	switch (ctx.config.getOption("GenuineSolver")){
	case 3: case 4:	// internal grounder or Gringo + clasp
#ifdef HAVE_LIBCLASP
		{
		DBGLOG(DBG, "Instantiating genuine sat solver with clasp");
		SATSolverPtr ptr = SATSolverPtr(new ClaspSolver(ctx, ns, frozen));
		return ptr;
		}
#else
		throw GeneralError("No support for clasp compiled into this binary");
#endif // HAVE_LIBCLINGO
		break;
	case 1: case 2:	// internal grounder or Gringo + internal solver
	default:	// translation solver
		{
		DBGLOG(DBG, "Instantiating genuine sat solver with internal solver");
		SATSolverPtr ptr = SATSolverPtr(new CDNLSolver(ctx, ns));	// this solver does not implement optimizations, thus all variables are always frozen
		return ptr;
		}
		break;
	}
}

SATSolverPtr SATSolver::getInstance(ProgramCtx& ctx, const OrdinaryASPProgram& p, InterpretationConstPtr frozen){

	switch (ctx.config.getOption("GenuineSolver")){
	case 3: case 4:	// internal grounder or Gringo + clasp
#ifdef HAVE_LIBCLASP
		{
		DBGLOG(DBG, "Instantiating genuine sat solver with clasp");
		SATSolverPtr ptr = SATSolverPtr(new ClaspSolver(ctx, AnnotatedGroundProgram(ctx, p), frozen, ClaspSolver::SAT));
		return ptr;
		}
#else
		throw GeneralError("No support for clasp compiled into this binary");
#endif // HAVE_LIBCLINGO
		break;
	case 1: case 2:	// internal grounder or Gringo + internal solver
	default:	// translation solver
		throw GeneralError("No support for internal solver");
	}
}

DLVHEX_NAMESPACE_END

