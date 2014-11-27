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
 * @file   GenuineSolver.h
 * @author Christoph Redl
 * @date   Thu 02 16:00:00 CET 2012
 * 
 * @brief  Interface to genuine nonground disjunctive ASP Grounder and
 *         Solver (powered by gringo/clasp or internal grounder/solver)
 * 
 */

#if !defined(_DLVHEX_GENUINESOLVER_HPP)
#define _DLVHEX_GENUINESOLVER_HPP


#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ASPSolverManager.h"
#include "dlvhex2/Error.h"

#include "dlvhex2/OrdinaryASPProgram.h"
#include "dlvhex2/AnnotatedGroundProgram.h"
#include "dlvhex2/OrdinaryASPSolver.h"

#include "dlvhex2/Nogood.h"

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <vector>

DLVHEX_NAMESPACE_BEGIN

class DLVHEX_EXPORT PropagatorCallback{
public:
	virtual void propagate(InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed) = 0;
};

class DLVHEX_EXPORT GenuineGrounder{
public:
	virtual const OrdinaryASPProgram& getGroundProgram() = 0;

	typedef boost::shared_ptr<GenuineGrounder> Ptr;
	typedef boost::shared_ptr<const GenuineGrounder> ConstPtr;

	static Ptr getInstance(ProgramCtx& ctx, const OrdinaryASPProgram& program, InterpretationConstPtr frozen = InterpretationConstPtr());
};

typedef GenuineGrounder::Ptr GenuineGrounderPtr;
typedef GenuineGrounder::ConstPtr GenuineGrounderConstPtr;


class DLVHEX_EXPORT GenuineGroundSolver : virtual public NogoodContainer, public OrdinaryASPSolver{
public:
	virtual std::string getStatistics() = 0;
	virtual void setOptimum(std::vector<int>& optimum) = 0;
	virtual InterpretationPtr getNextModel() = 0;
	virtual int getModelCount() = 0;
	virtual void restartWithAssumptions(const std::vector<ID>& assumptions) = 0;
	virtual void addPropagator(PropagatorCallback* pb) = 0;
	virtual void removePropagator(PropagatorCallback* pb) = 0;
	virtual void addProgram(const AnnotatedGroundProgram& program, InterpretationConstPtr frozen = InterpretationConstPtr()) = 0;
	virtual void addNogoodSet(const NogoodSet& ns, InterpretationConstPtr frozen = InterpretationConstPtr()) = 0;

	typedef boost::shared_ptr<GenuineGroundSolver> Ptr;
	typedef boost::shared_ptr<const GenuineGroundSolver> ConstPtr;

	static Ptr getInstance(ProgramCtx& ctx, const OrdinaryASPProgram& program, InterpretationConstPtr frozen = InterpretationConstPtr(), bool minCheck = true);
	static Ptr getInstance(ProgramCtx& ctx, const AnnotatedGroundProgram& program, InterpretationConstPtr frozen = InterpretationConstPtr(), bool minCheck = true);
};

typedef GenuineGroundSolver::Ptr GenuineGroundSolverPtr;
typedef GenuineGroundSolver::ConstPtr GenuineGroundSolverConstPtr;


class DLVHEX_EXPORT GenuineSolver : public GenuineGrounder, public GenuineGroundSolver{
private:
	GenuineGrounderPtr grounder;
	GenuineGroundSolverPtr solver;
	OrdinaryASPProgram gprog;

	GenuineSolver(GenuineGrounderPtr grounder, GenuineGroundSolverPtr solver, OrdinaryASPProgram gprog) : grounder(grounder), solver(solver), gprog(gprog){}
public:
	const OrdinaryASPProgram& getGroundProgram();

	std::string getStatistics();
	void setOptimum(std::vector<int>& optimum);
	InterpretationPtr getNextModel();
	int getModelCount();
	void addNogood(Nogood ng);
	void restartWithAssumptions(const std::vector<ID>& assumptions);
	void addPropagator(PropagatorCallback* pb);
	void removePropagator(PropagatorCallback* pb);
	void addProgram(const AnnotatedGroundProgram& program, InterpretationConstPtr frozen = InterpretationConstPtr());
	void addNogoodSet(const NogoodSet& ns, InterpretationConstPtr frozen = InterpretationConstPtr());

	inline GenuineGrounderPtr getGenuineGrounder(){ return grounder; }
	inline GenuineGroundSolverPtr getGenuineGroundSolver(){ return solver; }

	typedef boost::shared_ptr<GenuineSolver> Ptr;
	typedef boost::shared_ptr<const GenuineSolver> ConstPtr;

	static Ptr getInstance(ProgramCtx& ctx, const OrdinaryASPProgram& p, InterpretationConstPtr frozen = InterpretationConstPtr(), bool minCheck = true);
};

typedef GenuineSolver::Ptr GenuineSolverPtr;
typedef GenuineSolver::ConstPtr GenuineSolverConstPtr;

DLVHEX_NAMESPACE_END

#endif // _DLVHEX_GENUINESOLVER_H

// Local Variables:
// mode: C++
// End:
