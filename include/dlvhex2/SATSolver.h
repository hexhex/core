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

/**
 * \brief Base class for satisfiability solvers.
 */
class SATSolver : virtual public NogoodContainer
{
    public:
        typedef boost::shared_ptr<SATSolver> Ptr;
        typedef boost::shared_ptr<const SATSolver> ConstPtr;

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
         * The propagator can add additional nogoods by calling NogoodContainer::addNogood inherited from the base class.
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
         * \brief Returns the next model.
         *
         * This will also trigger callbacks to the propagators, see addPropagator.
         * @return The next model or a NULL-pointer of no more models exist.
         */
        virtual InterpretationPtr getNextModel() = 0;

        /**
         * \brief Returns an explanation for an inconsistency in terms of litervals over the atoms given in \p explanationAtoms.
         *
         * Details (definition of explanations) are specified in subclasses.
         *  
         * The method may only be called after getNextModel() has returned a NULL-pointer after first call.
         * @param explanationAtoms The atoms which serve as explanation.
         * @return An explanation for the inconsistency depending on the atoms in \p explanationAtoms.
         */
        virtual Nogood getInconsistencyCause(InterpretationConstPtr explanationAtoms) = 0;

        /**
         * \brief Adds a set of additional nogoods to the solver instance.
         *
         * @param ns The set of nogoods to add.
         * @param frozen A set of atoms which occur in \p ns and are saved from being optimized away (e.g. because their truth values are relevant).
         */
        virtual void addNogoodSet(const NogoodSet& ns, InterpretationConstPtr frozen = InterpretationConstPtr()) = 0;

        /**
         * \brief Interprets the settings in \p ctx and creates an instance of a concrete subclass of this class, which implements a specific reasoner.
         *
         * @param ctx ProgramCtx.
         * @param ns Encoding of the SAT instance as a set of nogoods.
         * @param frozen A set of atoms which occur in \p ns and are saved from being optimized away (e.g. because their truth values are relevant).
         * if NULL, then all variables are frozen.
         * @return Pointer to the new solver instance.
         */
        static Ptr getInstance(ProgramCtx& ctx, NogoodSet& ns, InterpretationConstPtr frozen = InterpretationConstPtr());
};

typedef SATSolver::Ptr SATSolverPtr;
typedef SATSolver::ConstPtr SATSolverConstPtr;

DLVHEX_NAMESPACE_END
#endif

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
