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
 * @file   ExternalAtomEvaluationHeuristics.h
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Base class and concrete classes with heuristics for external
 *         atom evaluations in genuine G&C model generators.
 */

#ifndef EXTERNALATOMEVALUATIONHEURISTICS_H
#define EXTERNALATOMEVALUATIONHEURISTICS_H

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/Registry.h"

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
 * Decides when to evaluate an external atom
 */
class HeuristicsModelGeneratorInterface;

// ============================== Base ==============================

class ExternalAtomEvaluationHeuristics{
protected:
	HeuristicsModelGeneratorInterface* mg;
	RegistryPtr reg;
public:
	ExternalAtomEvaluationHeuristics(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg);
	virtual ~ExternalAtomEvaluationHeuristics(){}
	/**
	* Decides if the reasoner shall evaluate a given external atom at this point.
	* @param eatom The external atom in question
	* @param eatomMask Mask with all atoms relevant for this external atom
	* @param programMask All atoms in the program
	* @param partialInterpretation The current (partial) interpretation
	* @param factWasSet The current set of assigned atoms; if 0, then the interpretation is complete
	* @param changed The set of atoms with a (possibly) modified truth value since the last call; if 0, then all atoms have changed
	* @return bool True if the heuristics suggests to evaluate the external atom, otherwise false
	*/
	virtual bool doEvaluate(const ExternalAtom& eatom, InterpretationConstPtr eatomMask, InterpretationConstPtr programMask, InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed) = 0;
};

typedef boost::shared_ptr<ExternalAtomEvaluationHeuristics> ExternalAtomEvaluationHeuristicsPtr;

class ExternalAtomEvaluationHeuristicsFactory{
public:
	virtual ExternalAtomEvaluationHeuristicsPtr createHeuristics(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg) = 0;
	virtual ~ExternalAtomEvaluationHeuristicsFactory(){}
};

typedef boost::shared_ptr<ExternalAtomEvaluationHeuristicsFactory> ExternalAtomEvaluationHeuristicsFactoryPtr;

// ============================== Always ==============================

// Evaluates always when the heuristics is asked

class ExternalAtomEvaluationHeuristicsAlways : public ExternalAtomEvaluationHeuristics{
public:
	ExternalAtomEvaluationHeuristicsAlways(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg);
	virtual bool doEvaluate(const ExternalAtom& eatom, InterpretationConstPtr eatomMask, InterpretationConstPtr programMask, InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed);
};

class ExternalAtomEvaluationHeuristicsAlwaysFactory : public ExternalAtomEvaluationHeuristicsFactory{
	virtual ExternalAtomEvaluationHeuristicsPtr createHeuristics(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg);
};

// ============================== InputComplete ==============================

// Evaluates always when the heuristics is asked and the input to the external atom is complete

class ExternalAtomEvaluationHeuristicsInputComplete : public ExternalAtomEvaluationHeuristics{
public:
	ExternalAtomEvaluationHeuristicsInputComplete(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg);
	virtual bool doEvaluate(const ExternalAtom& eatom, InterpretationConstPtr eatomMask, InterpretationConstPtr programMask, InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed);
};

class ExternalAtomEvaluationHeuristicsInputCompleteFactory : public ExternalAtomEvaluationHeuristicsFactory{
	virtual ExternalAtomEvaluationHeuristicsPtr createHeuristics(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg);
};

// ============================== EAComplete ==============================

// Evaluates always when the heuristics is asked and all atoms relevant for this external atom are assigned

class ExternalAtomEvaluationHeuristicsEAComplete : public ExternalAtomEvaluationHeuristics{
public:
	ExternalAtomEvaluationHeuristicsEAComplete(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg);
	virtual bool doEvaluate(const ExternalAtom& eatom, InterpretationConstPtr eatomMask, InterpretationConstPtr programMask, InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed);
};

class ExternalAtomEvaluationHeuristicsEACompleteFactory : public ExternalAtomEvaluationHeuristicsFactory{
	virtual ExternalAtomEvaluationHeuristicsPtr createHeuristics(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg);
};

// ============================== Never ==============================

// Evaluates never when the heuristics is asked
class ExternalAtomEvaluationHeuristicsNever : public ExternalAtomEvaluationHeuristics{
public:
	ExternalAtomEvaluationHeuristicsNever(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg);
	virtual bool doEvaluate(const ExternalAtom& eatom, InterpretationConstPtr eatomMask, InterpretationConstPtr programMask, InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed);
};

class ExternalAtomEvaluationHeuristicsNeverFactory : public ExternalAtomEvaluationHeuristicsFactory{
	virtual ExternalAtomEvaluationHeuristicsPtr createHeuristics(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg);
};

DLVHEX_NAMESPACE_END

#endif

