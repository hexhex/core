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
 * @file ExternalAtomEvaluationHeuristics.cpp
 * @author Christoph Redl
 *
 * @brief  Base class and concrete classes with heuristics for external
 *         atom evaluations in genuine G&C model generators.
 */

#include "dlvhex2/ExternalAtomEvaluationHeuristics.h"

#include "dlvhex2/GenuineGuessAndCheckModelGenerator.h"

DLVHEX_NAMESPACE_BEGIN

// ============================== Base ==============================

ExternalAtomEvaluationHeuristics::ExternalAtomEvaluationHeuristics(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg) : mg(mg), reg(reg){
}

// ============================== Always ==============================

ExternalAtomEvaluationHeuristicsAlways::ExternalAtomEvaluationHeuristicsAlways(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg) : ExternalAtomEvaluationHeuristics(mg, reg){
}

bool ExternalAtomEvaluationHeuristicsAlways::doEvaluate(const ExternalAtom& eatom, InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed){
	return true;
}

ExternalAtomEvaluationHeuristicsPtr ExternalAtomEvaluationHeuristicsAlwaysFactory::createHeuristics(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg){
	return ExternalAtomEvaluationHeuristicsPtr(new ExternalAtomEvaluationHeuristicsAlways(mg, reg));
}

// ============================== Never ==============================

ExternalAtomEvaluationHeuristicsNever::ExternalAtomEvaluationHeuristicsNever(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg) : ExternalAtomEvaluationHeuristics(mg, reg){
}

bool ExternalAtomEvaluationHeuristicsNever::doEvaluate(const ExternalAtom& eatom, InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed){
	return false;
}

ExternalAtomEvaluationHeuristicsPtr ExternalAtomEvaluationHeuristicsNeverFactory::createHeuristics(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg){
	return ExternalAtomEvaluationHeuristicsPtr(new ExternalAtomEvaluationHeuristicsNever(mg, reg));
}

DLVHEX_NAMESPACE_END

