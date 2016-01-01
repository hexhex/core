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
 * @file   UnfoundedSetCheckHeuristicsInterface.h
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 *
 * @brief  Base class for
 *         unfounded set checks in genuine G&C model generators.
 */

#ifndef UNFOUNDEDSETCHECKHEURISTICSINTERFACE_H
#define UNFOUNDEDSETCHECKHEURISTICSINTERFACE_H

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/AnnotatedGroundProgram.h"

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
 * \brief Decides when to do an unfounded set check (over partial interpretations)
 *
 * The implementers of this interface decide for a given (partial) assignment
 * if a minimality check shall be performed at this point. Note that this is only for optimization purposes
 * as the reasoner will automatically do such a check whenever it is necessary.
 * However, heuristics may initiate additional checks to possibly detect unfounded atoms earlier.
 */

// ============================== Base ==============================

/**
 * \brief Base class for all unfounded set check heuristics.
 */
class DLVHEX_EXPORT UnfoundedSetCheckHeuristics
{
    protected:
        /**
         * \brief Pointer to the registry.
         */
        RegistryPtr reg;

        /** \brief Reference to the ground program for which the initation of unfounded set checks shall be decided. */
        const AnnotatedGroundProgram& groundProgram;

        /** \brief stores the atoms which were assigned and verified when the skipProgram was updated last time. */
        InterpretationPtr previouslyAssignedAndVerifiedAtoms;

        /** \brief Remembers external atom replacement atoms which have already been assigned but could not be verified yet. */
        InterpretationPtr notYetVerifiedExternalAtoms;

        /** \brief Stores for each atom in which rule (identified by its index in the ground program) it occurs (positively or negatively). */
        std::map<IDAddress, std::set<int> > rulesOfAtom;

        /** \brief Stores for each rule (address) the number of total and of currently assigned and verified atoms. */
        std::vector<int> atomsInRule, assignedAndVerifiedAtomsInRule;

        /** \brief See UnfoundedSetCheckHeuristics::updateSkipProgram and UnfoundedSetCheckHeuristics::getSkipProgram. */
        std::set<ID> skipProgram;
    public:
        UnfoundedSetCheckHeuristics(const AnnotatedGroundProgram& groundProgram, RegistryPtr reg);

        /**
         * \brief Decides if the reasoner shall do an unfounded set check at this point.
         *
         * @param verifiedAuxes The set of verified external atom auxiliaries wrt. the current partial interpretation
         * @param partialAssignment The current (partial) interpretation
         * @param assigned The current set of assigned atoms; if NULL, then the interpretation is complete
         * @param changed The set of atoms with a (possibly) modified truth value since the last call of this method; if NULL, then all atoms have changed
         * @return True if the heuristics decides to do an unfounded set check now, and false otherwise.
         */
        virtual bool doUFSCheck(InterpretationConstPtr verifiedAuxes, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed) = 0;

        /**
         * \brief Notifies the heuristic about changes in the assignment, although the caller is not going to perform an UFS check at this point.
         * This allows the heuristic to update internal data structures.
         *
         * @param partialAssignment The current (partial) interpretation
         * @param assigned The current set of assigned atoms; if NULL, then the interpretation is complete
         * @param changed The set of atoms with a (possibly) modified truth value since the last call of this method; if NULL, then all atoms have changed
         */
        virtual void notify(InterpretationConstPtr verifiedAuxes, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed);

        /**
         * \brief Updates the skip program according to a new partial assignment.
         *
         * The skip program is the set of rules which are currently not (fully) assignment and thus have to be excluded from UFS checks.
         *
         * @param partialAssignment The current (partial) interpretation
         * @param assigned The current set of assigned atoms; if NULL, then the interpretation is complete
         * @param changed The set of atoms with a (possibly) modified truth value since the last call of this method; if NULL, then all atoms have changed
         */
        void updateSkipProgram(InterpretationConstPtr verifiedAuxes, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed);

        /**
         * \brief Returns a reference to the current skip program.
         *
         * The method UnfoundedSetCheckHeuristics::updateSkipProgram should be called before this method is used.
         * @return The current set of rules which are not fully assigned and thus have to be excluded from UFS checks.
         */
        inline const std::set<ID>& getSkipProgram() const { return skipProgram; }
};

typedef boost::shared_ptr<UnfoundedSetCheckHeuristics> UnfoundedSetCheckHeuristicsPtr;

/**
 * \brief Factory for UnfoundedSetCheckHeuristics.
 */
class DLVHEX_EXPORT UnfoundedSetCheckHeuristicsFactory
{
    public:
        /**
         * \brief Creates a heuristic instance for a certain ground program
         * @param groundProgram The ground program
         * @param reg RegistryPtr
         */
        virtual UnfoundedSetCheckHeuristicsPtr createHeuristics(const AnnotatedGroundProgram& groundProgram, RegistryPtr reg) = 0;

        /**
         * \brief Destructor.
         */
        virtual ~UnfoundedSetCheckHeuristicsFactory(){}
};

typedef boost::shared_ptr<UnfoundedSetCheckHeuristicsFactory> UnfoundedSetCheckHeuristicsFactoryPtr;

DLVHEX_NAMESPACE_END
#endif

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
