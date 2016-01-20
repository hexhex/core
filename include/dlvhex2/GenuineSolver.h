/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2016 Peter Schüller
 * Copyright (C) 2011-2016 Christoph Redl
 * Copyright (C) 2015-2016 Tobias Kaminski
 * Copyright (C) 2015-2016 Antonius Weinzierl
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

/**
 * \brief Base class for propagator callbacks.
 */
class DLVHEX_EXPORT PropagatorCallback
{
    public:
        /**
         * \brief This method is called by the solver whenever
         * 1. it cannot propagate by other means, or 2. when a model is complete but before getNextModel returns it.
         * @param partialInterpretation Current partial assignment. Bits represent true atoms, cleared bits represent false or unassigned atoms.
         *
         * The method is expected to trigger propagations by adding appropriate nogoods to the reasoner using GenuineSolver::addNogood.
         * @param assigned %Set of currently assigned atoms.
         * @param changes %Set of atoms which have been reassigned since the last call and thus have possibly(!) changed.
         */
        virtual void propagate(InterpretationConstPtr partialInterpretation, InterpretationConstPtr assigned, InterpretationConstPtr changed) = 0;
};

/**
 * \brief Base class for grounder implementations.
 */
class DLVHEX_EXPORT GenuineGrounder
{
    public:
        virtual const OrdinaryASPProgram& getGroundProgram() = 0;

        typedef boost::shared_ptr<GenuineGrounder> Ptr;
        typedef boost::shared_ptr<const GenuineGrounder> ConstPtr;

        /**
         * \brief Interprets the settings in \p ctx and creates an instance of a concrete subclass of this class, which implements a specific grounder.
         *
         * @param ctx ProgramCtx.
         * @param program OrdinaryASPProgram to be grounded.
         * @param frozen A set of atoms which occur in \p ns and are saved from being optimized away (e.g. because their truth values are relevant);
         * if NULL, then all variables are frozen.
         * @return Pointer to the new grounder instance.
         */
        static Ptr getInstance(ProgramCtx& ctx, const OrdinaryASPProgram& program, InterpretationConstPtr frozen = InterpretationConstPtr());
};

typedef GenuineGrounder::Ptr GenuineGrounderPtr;
typedef GenuineGrounder::ConstPtr GenuineGrounderConstPtr;

/**
 * \brief Base class for ground ASP solvers.
 */
class DLVHEX_EXPORT GenuineGroundSolver : virtual public NogoodContainer, public OrdinaryASPSolver
{
    public:
        /**
         * \brief Create solver statistics in a no further specified format (for debug purposes).
         * @return Debug statistics.
         */
        virtual std::string getStatistics() = 0;

        /**
         * \brief Sets the current optimum for optimization problems.
         *
         * The solver may (or may not) use this value to prune the search space and not return less optimal models.
         * @param optimum The optimum in form of current costs for each level, migh levels with greater index are considered more important (see AnswerSet::costVector).
         */
        virtual void setOptimum(std::vector<int>& optimum) = 0;

        // inherited from OrdinaryASPSolver
        virtual InterpretationPtr getNextModel() = 0;

        /**
         * \brief Returns the number of models enumerated so far.
         *
         * @return Model count.
         */
        virtual int getModelCount() = 0;

        /**
         * \brief Resets the search and assumes truth values for selected atoms.
         *
         * @param assumptions Vector of positive or negated (using ID::NAF_MASK) atoms which are temporarily assumed to hold (until the next reset);
         * ID::NAF_MASK is used to represent that the according atom is assumed to be false.
         */
        virtual void restartWithAssumptions(const std::vector<ID>& assumptions) = 0;

        /**
         * \brief Adds a propagator callback which is to be called by the SAT solver whenever
         * it cannot propagate by other means or when a model is complete but before getNextModel returns it.
         *
         * @param pb The callback to be added.
         */
        virtual void addPropagator(PropagatorCallback* pb) = 0;

        /**
         * \brief Removes a propagator callback.
         * @param pb The callback to be removed.
         */
        virtual void removePropagator(PropagatorCallback* pb) = 0;

        /**
         * \brief Incrementally adds another program component to the solver.
         *
         * Note that modularity conditions do not allow for closing cycles over multiple incremental step (the conditions are as in gringo and clasp).
         * @param program The program component to be added.
         * @param frozen A set of atoms which occur in \p ns and are saved from being optimized away (e.g. because their truth values are relevant).
         */
        virtual void addProgram(const AnnotatedGroundProgram& program, InterpretationConstPtr frozen = InterpretationConstPtr()) = 0;

        /**
         * \brief Returns an explanation for an inconsistency in terms of litervals over the atoms given in \p explanationAtoms.
         *
         * Let A be the set of \p explanationAtoms. Let further P be the program with IDB I and EDB E.
         * A \emph proto-explanation X is a subset of A such that
         * for any program P' with IDB I and EDB E', where E' coincides with E on all atoms which are (i) not in A or (ii) in X, is inconsistent.
         * That is, X guarantees the inconsistency all programs with the same IDB I and the same EDB over atoms other than A,
         * while facts over atoms in A but not in X are not relevant for inconsistency.
         * (Note that set A itself is always a proto-explanation for an inconsistent program P because then the only
         * possible P' is P itself.)
         *
         * Example: A proto-explanation of the Program
         * \code
         * a. b.
         * :- a, not c.
         * \endcode
         * wrt. A={a,b,c} is X={a,c} because any program with the same IDB, fact a and c not being a fact is inconsistent
         * (while b might be removed as a fact and the program is still inconsistent).
         *
         * An explanation is a proto-explanation X if all atoms in X are turned into signed literals,
         * where the sign is positive of it is a fact in P and negative otherwise.
         *  
         * The method may only be called after getNextModel() has returned a NULL-pointer after first call.
         * @param explanationAtoms The atoms which serve as explanation.
         * @return An explanation for the inconsistency depending on the atoms in \p explanationAtoms.
         */
        virtual Nogood getInconsistencyCause(InterpretationConstPtr explanationAtoms) = 0;

        /**
         * \brief Adds a set of nogoods to the solver.
         *
         * @param ns Encoding of the SAT instance as a set of nogoods.
         * @param frozen A set of atoms which occur in \p ns and are saved from being optimized away (e.g. because their truth values are relevant).
         * if NULL, then all variables are frozen.
         */
        virtual void addNogoodSet(const NogoodSet& ns, InterpretationConstPtr frozen = InterpretationConstPtr()) = 0;

        typedef boost::shared_ptr<GenuineGroundSolver> Ptr;
        typedef boost::shared_ptr<const GenuineGroundSolver> ConstPtr;

        /**
         * \brief Interprets the settings in \p ctx and creates an instance of a concrete subclass of this class, which implements a specific ground program solver.
         *
         * @param ctx ProgramCtx.
         * @param program Ground program to be solved; will automatically be wrapped in an AnnotatedGroundProgram.
         * @param frozen A set of atoms which occur in \p ns and are saved from being optimized away (e.g. because their trutz values are relevant).
         * if NULL, then all variables are frozen.
         * @param minCheck True to force a minimality check for detecting unfounded sets due to disjunctions before returning a model.
         * False leaves the decision open to the implementer of this class and might be more efficient. This might be useful to avoid redundancy if the caller performs an extended
         * unfounded set check anyway (e.g. for detecting unfounded sets due to external atoms).
         * @return Pointer to the new grounder instance.
         */
        static Ptr getInstance(ProgramCtx& ctx, const OrdinaryASPProgram& program, InterpretationConstPtr frozen = InterpretationConstPtr(), bool minCheck = true);

        /**
         * \brief Interprets the settings in \p ctx and creates an instance of a concrete subclass of this class, which implements a specific ground program solver.
         *
         * @param ctx ProgramCtx.
         * @param program Ground program to be solved.
         * @param frozen A set of atoms which occur in \p ns and are saved from being optimized away (e.g. because their trutz values are relevant);
         * if NULL, then all variables are frozen.
         * @param minCheck True to force a minimality check for detecting unfounded sets due to disjunctions before returning a model.
         * False leaves the decision open to the implementer of this class and might be more efficient. This might be useful to avoid redundancy if the caller performs an extended
         * unfounded set check anyway (e.g. for detecting unfounded sets due to external atoms).
         * @return Pointer to the new grounder instance.
         */
        static Ptr getInstance(ProgramCtx& ctx, const AnnotatedGroundProgram& program, InterpretationConstPtr frozen = InterpretationConstPtr(), bool minCheck = true);
};

typedef GenuineGroundSolver::Ptr GenuineGroundSolverPtr;
typedef GenuineGroundSolver::ConstPtr GenuineGroundSolverConstPtr;

/**
 * \brief This class combines a grounder and a solver to support direct solving of nonground programs.
 */
class DLVHEX_EXPORT GenuineSolver : public GenuineGrounder, public GenuineGroundSolver
{
    private:
        /** \brief Internal grounder. */
        GenuineGrounderPtr grounder;
        /** \brief Internal solver. */
        GenuineGroundSolverPtr solver;
        /** \brief Ground program produced by the grounder. */
        OrdinaryASPProgram gprog;

        /**
         * \brief Constructor.
         * @param grounder Grounder to be used.
         * @param solver Solver to be used.
         * @param gprog Ground program produced by \p grounder.
         */
        GenuineSolver(GenuineGrounderPtr grounder, GenuineGroundSolverPtr solver, OrdinaryASPProgram gprog) : grounder(grounder), solver(solver), gprog(gprog){}
    public:
        /**
         * \brief Getter for the internal ground program.
         * @return Ground program gprog.
         */
        const OrdinaryASPProgram& getGroundProgram();

        // inherited
        std::string getStatistics();
        void setOptimum(std::vector<int>& optimum);
        InterpretationPtr getNextModel();
        int getModelCount();
        void addNogood(Nogood ng);
        void restartWithAssumptions(const std::vector<ID>& assumptions);
        void addPropagator(PropagatorCallback* pb);
        void removePropagator(PropagatorCallback* pb);
        void addProgram(const AnnotatedGroundProgram& program, InterpretationConstPtr frozen = InterpretationConstPtr());
        Nogood getInconsistencyCause(InterpretationConstPtr explanationAtoms);
        void addNogoodSet(const NogoodSet& ns, InterpretationConstPtr frozen = InterpretationConstPtr());

        /**
         * \brief Returns a pointer to the internal grounder.
         * @return grounder.
         */
        inline GenuineGrounderPtr getGenuineGrounder(){ return grounder; }

        /**
         * \brief Returns a pointer to the internal solver.
         * @return solver.
         */
        inline GenuineGroundSolverPtr getGenuineGroundSolver(){ return solver; }

        typedef boost::shared_ptr<GenuineSolver> Ptr;
        typedef boost::shared_ptr<const GenuineSolver> ConstPtr;

        // inherited
        static Ptr getInstance(ProgramCtx& ctx, const OrdinaryASPProgram& p, InterpretationConstPtr frozen = InterpretationConstPtr(), bool minCheck = true);
};

typedef GenuineSolver::Ptr GenuineSolverPtr;
typedef GenuineSolver::ConstPtr GenuineSolverConstPtr;

DLVHEX_NAMESPACE_END
#endif                           // _DLVHEX_GENUINESOLVER_H


// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
