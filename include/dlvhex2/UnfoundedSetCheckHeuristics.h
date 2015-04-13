/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schller
 * Copyright (C) 2011-2015 Christoph Redl
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
 * @file   UnfoundedSetCheckHeuristics.h
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 *
 * @brief  Base class and concrete classes with heuristics for
 *         unfounded set checks in genuine G&C model generators.
 */

#ifndef UNFOUNDEDSETCHECKHEURISTICS_H
#define UNFOUNDEDSETCHECKHEURISTICS_H

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/AnnotatedGroundProgram.h"
#include "dlvhex2/UnfoundedSetCheckHeuristicsInterface.h"

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
 * \brief Contains implementers of UnfoundedSetCheckHeuristics to decide for a given (partial) assignment
 * if a minimality check shall be performed at this point. Note that this is only for optimization purposes
 * as the reasoner will automatically do such a check whenever it is necessary.
 * However, heuristics may initiate additional checks to possibly detect unfounded atoms earlier.
 */

// ============================== Post ==============================

/**
 * \brief Performs UFS checks only over complete interpretations.
 */
class DLVHEX_EXPORT UnfoundedSetCheckHeuristicsPost : public UnfoundedSetCheckHeuristics
{
    public:
        UnfoundedSetCheckHeuristicsPost(const AnnotatedGroundProgram& groundProgram, RegistryPtr reg);
        virtual bool doUFSCheck(InterpretationConstPtr verifiedAuxes, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed);
};

/**
 * \brief Factory for UnfoundedSetCheckHeuristicsPost.
 */
class DLVHEX_EXPORT UnfoundedSetCheckHeuristicsPostFactory : public UnfoundedSetCheckHeuristicsFactory
{
    virtual UnfoundedSetCheckHeuristicsPtr createHeuristics(const AnnotatedGroundProgram& groundProgram, RegistryPtr reg);
};

// ============================== Max ==============================

/**
 * \brief Performs UFS checks whenever deterministic reasoning cannot derive further atoms.
 */
class DLVHEX_EXPORT UnfoundedSetCheckHeuristicsMax : public UnfoundedSetCheckHeuristics
{
    public:
        UnfoundedSetCheckHeuristicsMax(const AnnotatedGroundProgram& groundProgram, RegistryPtr reg);
        virtual bool doUFSCheck(InterpretationConstPtr verifiedAuxes, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed);
};

/**
 * \brief Factory for UnfoundedSetCheckHeuristicsMax.
 */
class DLVHEX_EXPORT UnfoundedSetCheckHeuristicsMaxFactory : public UnfoundedSetCheckHeuristicsFactory
{
    virtual UnfoundedSetCheckHeuristicsPtr createHeuristics(const AnnotatedGroundProgram& groundProgram, RegistryPtr reg);
};

// ============================== Periodic ==============================

/**
 * \brief Performs UFS checks periodically.
 */
class DLVHEX_EXPORT UnfoundedSetCheckHeuristicsPeriodic : public UnfoundedSetCheckHeuristicsMax
{
    private:
        /** Counts the number of calls in order to periodically perform the UFS check. */
        int counter;

    public:
        UnfoundedSetCheckHeuristicsPeriodic(const AnnotatedGroundProgram& groundProgram, RegistryPtr reg);
        virtual bool doUFSCheck(InterpretationConstPtr verifiedAuxes, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed);
};

/**
 * \brief Factory for UnfoundedSetCheckHeuristicsPeriodic.
 */
class DLVHEX_EXPORT UnfoundedSetCheckHeuristicsPeriodicFactory : public UnfoundedSetCheckHeuristicsFactory
{
    virtual UnfoundedSetCheckHeuristicsPtr createHeuristics(const AnnotatedGroundProgram& groundProgram, RegistryPtr reg);
};

DLVHEX_NAMESPACE_END
#endif
