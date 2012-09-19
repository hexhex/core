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
 * @file   SATSolver.h
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Interface to (genuine) SAT solvers.
 */

#ifndef SATSOLVER_H
#define SATSOLVER_H

#include "dlvhex2/Nogood.h"
#include "dlvhex2/Interpretation.h"

DLVHEX_NAMESPACE_BEGIN

// forward declaration
class PropagatorCallback;

class ProgramCtx;
class SATSolver : virtual public NogoodContainer{
public:
	typedef boost::shared_ptr<SATSolver> Ptr;
	typedef boost::shared_ptr<const SATSolver> ConstPtr;

	virtual void restartWithAssumptions(const std::vector<ID>& assumptions) = 0;
	virtual void addPropagator(PropagatorCallback* pb) = 0;
	virtual void removePropagator(PropagatorCallback* pb) = 0;
	virtual InterpretationPtr getNextModel() = 0;

	static Ptr getInstance(ProgramCtx& ctx, NogoodSet& ns);
};

typedef SATSolver::Ptr SATSolverPtr;
typedef SATSolver::ConstPtr SATSolverConstPtr;

DLVHEX_NAMESPACE_END

#endif

