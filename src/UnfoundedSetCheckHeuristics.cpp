/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) Peter Schüller
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
 * @file UnfoundedSetCheckHeuristics.cpp
 * @author Christoph Redl
 *
 * @brief  Base class and concrete classes with heuristics for
 *         unfounded set checks in genuine G&C model generators.
 */

#include "dlvhex2/UnfoundedSetCheckHeuristics.h"
#include "dlvhex2/Interpretation.h"
#include "dlvhex2/Printer.h"

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

// ============================== Post ==============================

UnfoundedSetCheckHeuristicsPost::UnfoundedSetCheckHeuristicsPost(const AnnotatedGroundProgram& groundProgram, RegistryPtr reg) : UnfoundedSetCheckHeuristics(groundProgram, reg)
{
}


bool UnfoundedSetCheckHeuristicsPost::doUFSCheck(InterpretationConstPtr verifiedAuxes, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed)
{
    return false;
}


UnfoundedSetCheckHeuristicsPtr UnfoundedSetCheckHeuristicsPostFactory::createHeuristics(const AnnotatedGroundProgram& groundProgram, RegistryPtr reg)
{
    return UnfoundedSetCheckHeuristicsPtr(new UnfoundedSetCheckHeuristicsPost(groundProgram, reg));
}


// ============================== Max ==============================

UnfoundedSetCheckHeuristicsMax::UnfoundedSetCheckHeuristicsMax(const AnnotatedGroundProgram& groundProgram, RegistryPtr reg) : UnfoundedSetCheckHeuristics(groundProgram, reg)
{
}


bool UnfoundedSetCheckHeuristicsMax::doUFSCheck(InterpretationConstPtr verifiedAuxes, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed)
{
    return true;
}


UnfoundedSetCheckHeuristicsPtr UnfoundedSetCheckHeuristicsMaxFactory::createHeuristics(const AnnotatedGroundProgram& groundProgram, RegistryPtr reg)
{
    return UnfoundedSetCheckHeuristicsPtr(new UnfoundedSetCheckHeuristicsMax(groundProgram, reg));
}


// ============================== Periodic ==============================

UnfoundedSetCheckHeuristicsPeriodic::UnfoundedSetCheckHeuristicsPeriodic(const AnnotatedGroundProgram& groundProgram, RegistryPtr reg) : UnfoundedSetCheckHeuristicsMax(groundProgram, reg), counter(0)
{
}


bool UnfoundedSetCheckHeuristicsPeriodic::doUFSCheck(InterpretationConstPtr verifiedAuxes, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed)
{
    static std::set<ID> emptySkipProgram;
    counter++;
    if (counter >= 10) {
        counter = 0;
        return UnfoundedSetCheckHeuristicsMax::doUFSCheck(verifiedAuxes, partialAssignment, assigned, changed);
    }
    else {
        return false;
    }
}


UnfoundedSetCheckHeuristicsPtr UnfoundedSetCheckHeuristicsPeriodicFactory::createHeuristics(const AnnotatedGroundProgram& groundProgram, RegistryPtr reg)
{
    return UnfoundedSetCheckHeuristicsPtr(new UnfoundedSetCheckHeuristicsPeriodic(groundProgram, reg));
}


DLVHEX_NAMESPACE_END
