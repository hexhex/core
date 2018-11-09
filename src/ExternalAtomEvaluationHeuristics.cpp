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
 * @file ExternalAtomEvaluationHeuristics.cpp
 * @author Christoph Redl
 *
 * @brief  Concrete classes with heuristics for external
 *         atom evaluations in genuine G&C model generators.
 */

#include "dlvhex2/ExternalAtomEvaluationHeuristics.h"
#include "dlvhex2/Interpretation.h"

#include <bm/bmalgo.h>

DLVHEX_NAMESPACE_BEGIN

// ============================== Always ==============================

ExternalAtomEvaluationHeuristicsAlways::ExternalAtomEvaluationHeuristicsAlways(RegistryPtr reg) : ExternalAtomEvaluationHeuristics(reg)
{
}


bool ExternalAtomEvaluationHeuristicsAlways::doEvaluate(const ExternalAtom& eatom, InterpretationConstPtr eatomMask, InterpretationConstPtr programMask, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed)
{
    return true;
}


bool ExternalAtomEvaluationHeuristicsAlways::frequent()
{
    return true;
}


ExternalAtomEvaluationHeuristicsPtr ExternalAtomEvaluationHeuristicsAlwaysFactory::createHeuristics(RegistryPtr reg)
{
    return ExternalAtomEvaluationHeuristicsPtr(new ExternalAtomEvaluationHeuristicsAlways(reg));
}


// ============================== Periodic ==============================

ExternalAtomEvaluationHeuristicsPeriodic::ExternalAtomEvaluationHeuristicsPeriodic(RegistryPtr reg) : ExternalAtomEvaluationHeuristics(reg), counter(0)
{
}


bool ExternalAtomEvaluationHeuristicsPeriodic::doEvaluate(const ExternalAtom& eatom, InterpretationConstPtr eatomMask, InterpretationConstPtr programMask, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed)
{
    static int b = 10;

    counter++;
    if (counter > b){
        counter = 0;
//        b += 10;
        return true;
    }else{
        return false;
    }
}


bool ExternalAtomEvaluationHeuristicsPeriodic::frequent()
{
    return true;
}


ExternalAtomEvaluationHeuristicsPtr ExternalAtomEvaluationHeuristicsPeriodicFactory::createHeuristics(RegistryPtr reg)
{
    return ExternalAtomEvaluationHeuristicsPtr(new ExternalAtomEvaluationHeuristicsPeriodic(reg));
}



// ============================== Dynamic ==============================

ExternalAtomEvaluationHeuristicsDynamic::ExternalAtomEvaluationHeuristicsDynamic(RegistryPtr reg) : ExternalAtomEvaluationHeuristics(reg), counter(0), frequency(1)
{
}


bool ExternalAtomEvaluationHeuristicsDynamic::doEvaluate(const ExternalAtom& eatom, InterpretationConstPtr eatomMask, InterpretationConstPtr programMask, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed)
{
    counter++;
    if (counter > frequency){
        counter = 0;
        return true;
    }else{
        return false;
    }
}


bool ExternalAtomEvaluationHeuristicsDynamic::frequent()
{
    return true;
}


void ExternalAtomEvaluationHeuristicsDynamic::decreaseFrequency(float factor)
{
    frequency = frequency * (factor / 1000);
}

void ExternalAtomEvaluationHeuristicsDynamic::resetFrequency()
{
    frequency = 1;
}

ExternalAtomEvaluationHeuristicsPtr ExternalAtomEvaluationHeuristicsDynamicFactory::createHeuristics(RegistryPtr reg)
{
    return ExternalAtomEvaluationHeuristicsPtr(new ExternalAtomEvaluationHeuristicsDynamic(reg));
}


// ============================== InputComplete ==============================

ExternalAtomEvaluationHeuristicsInputComplete::ExternalAtomEvaluationHeuristicsInputComplete(RegistryPtr reg) : ExternalAtomEvaluationHeuristics(reg)
{
}


bool ExternalAtomEvaluationHeuristicsInputComplete::doEvaluate(const ExternalAtom& eatom, InterpretationConstPtr eatomMask, InterpretationConstPtr programMask, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed)
{

    eatom.updatePredicateInputMask();

    bool aux = true;
    if (eatom.auxInputPredicate != ID_FAIL) {
        aux = (eatom.getAuxInputMask()->getStorage() & programMask->getStorage() & assigned->getStorage()).count() == (eatom.getAuxInputMask()->getStorage() & programMask->getStorage()).count();
    }

    return !assigned ||
        (((eatom.getPredicateInputMask()->getStorage() & programMask->getStorage() & assigned->getStorage()).count() == (eatom.getPredicateInputMask()->getStorage() & programMask->getStorage()).count()) &&
        aux);
}


ExternalAtomEvaluationHeuristicsPtr ExternalAtomEvaluationHeuristicsInputCompleteFactory::createHeuristics(RegistryPtr reg)
{
    return ExternalAtomEvaluationHeuristicsPtr(new ExternalAtomEvaluationHeuristicsInputComplete(reg));
}


// ============================== EAComplete ==============================

ExternalAtomEvaluationHeuristicsEAComplete::ExternalAtomEvaluationHeuristicsEAComplete(RegistryPtr reg) : ExternalAtomEvaluationHeuristics(reg)
{
}


bool ExternalAtomEvaluationHeuristicsEAComplete::doEvaluate(const ExternalAtom& eatom, InterpretationConstPtr eatomMask, InterpretationConstPtr programMask, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed)
{

    return !assigned ||
        !bm::any_sub(eatomMask->getStorage() & programMask->getStorage(), assigned->getStorage() & programMask->getStorage());
}


ExternalAtomEvaluationHeuristicsPtr ExternalAtomEvaluationHeuristicsEACompleteFactory::createHeuristics(RegistryPtr reg)
{
    return ExternalAtomEvaluationHeuristicsPtr(new ExternalAtomEvaluationHeuristicsEAComplete(reg));
}


// ============================== Never ==============================

ExternalAtomEvaluationHeuristicsNever::ExternalAtomEvaluationHeuristicsNever(RegistryPtr reg) : ExternalAtomEvaluationHeuristics(reg)
{
}


bool ExternalAtomEvaluationHeuristicsNever::doEvaluate(const ExternalAtom& eatom, InterpretationConstPtr eatomMask, InterpretationConstPtr programMask, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed)
{
    return false;
}


ExternalAtomEvaluationHeuristicsPtr ExternalAtomEvaluationHeuristicsNeverFactory::createHeuristics(RegistryPtr reg)
{
    return ExternalAtomEvaluationHeuristicsPtr(new ExternalAtomEvaluationHeuristicsNever(reg));
}


DLVHEX_NAMESPACE_END

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
