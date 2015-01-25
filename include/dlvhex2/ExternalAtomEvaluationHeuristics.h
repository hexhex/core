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
 * @file   ExternalAtomEvaluationHeuristics.h
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Concrete classes with heuristics for external
 *         atom evaluations in genuine G&C model generators.
 */

#ifndef EXTERNALATOMEVALUATIONHEURISTICS_H
#define EXTERNALATOMEVALUATIONHEURISTICS_H

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ExternalAtomEvaluationHeuristicsInterface.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/Registry.h"

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

// ============================== Always ==============================

/**
 * \brief Evaluates always when the heuristics is asked.
 */
class ExternalAtomEvaluationHeuristicsAlways : public ExternalAtomEvaluationHeuristics{
public:
	ExternalAtomEvaluationHeuristicsAlways(RegistryPtr reg);
	virtual bool doEvaluate(const ExternalAtom& eatom, InterpretationConstPtr eatomMask, InterpretationConstPtr programMask, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed);
};

/**
 * \brief Factory for ExternalAtomEvaluationHeuristicsAlways.
 */
class ExternalAtomEvaluationHeuristicsAlwaysFactory : public ExternalAtomEvaluationHeuristicsFactory{
	virtual ExternalAtomEvaluationHeuristicsPtr createHeuristics(RegistryPtr reg);
};

// ============================== InputComplete ==============================

/**
 * \brief Evaluates always when the heuristics is asked and the input to the external atom is complete.
 */
class ExternalAtomEvaluationHeuristicsInputComplete : public ExternalAtomEvaluationHeuristics{
public:
	ExternalAtomEvaluationHeuristicsInputComplete(RegistryPtr reg);
	virtual bool doEvaluate(const ExternalAtom& eatom, InterpretationConstPtr eatomMask, InterpretationConstPtr programMask, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed);
};

/**
 * \brief Factory for ExternalAtomEvaluationHeuristicsInputComplete.
 */
class ExternalAtomEvaluationHeuristicsInputCompleteFactory : public ExternalAtomEvaluationHeuristicsFactory{
	virtual ExternalAtomEvaluationHeuristicsPtr createHeuristics(RegistryPtr reg);
};

// ============================== EAComplete ==============================

/**
 * \brief Evaluates always when the heuristics is asked and all atoms relevant for this external atom are assigned.
 */
class ExternalAtomEvaluationHeuristicsEAComplete : public ExternalAtomEvaluationHeuristics{
public:
	ExternalAtomEvaluationHeuristicsEAComplete(RegistryPtr reg);
	virtual bool doEvaluate(const ExternalAtom& eatom, InterpretationConstPtr eatomMask, InterpretationConstPtr programMask, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed);
};

/**
 * \brief Factory for ExternalAtomEvaluationHeuristicsEAComplete.
 */
class ExternalAtomEvaluationHeuristicsEACompleteFactory : public ExternalAtomEvaluationHeuristicsFactory{
	virtual ExternalAtomEvaluationHeuristicsPtr createHeuristics(RegistryPtr reg);
};

// ============================== Never ==============================

/**
 * Never evaluates when the heuristics is asked (but the reasoner will still do this whenever absolutely necessary).
 */
class ExternalAtomEvaluationHeuristicsNever : public ExternalAtomEvaluationHeuristics{
public:
	ExternalAtomEvaluationHeuristicsNever(RegistryPtr reg);
	virtual bool doEvaluate(const ExternalAtom& eatom, InterpretationConstPtr eatomMask, InterpretationConstPtr programMask, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed);
};

/**
 * \brief Factory for ExternalAtomEvaluationHeuristicsNever.
 */
class ExternalAtomEvaluationHeuristicsNeverFactory : public ExternalAtomEvaluationHeuristicsFactory{
	virtual ExternalAtomEvaluationHeuristicsPtr createHeuristics(RegistryPtr reg);
};

DLVHEX_NAMESPACE_END

#endif

