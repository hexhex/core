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
 * @file   ExternalAtomEvaluationHeuristicsInterface.h
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Base class for external
 *         atom evaluations in genuine G&C model generators.
 */

#ifndef EXTERNALATOMEVALUATIONHEURISTICSINTERFACE_H
#define EXTERNALATOMEVALUATIONHEURISTICSINTERFACE_H

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/Registry.h"

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
 * \brief Decides when to evaluate an external atom.
 *
 * The implementers of this interface decide for a given (partial) assignment and an external atom
 * if it should be evaluated at this point. Note that this is only for optimization purposes
 * as the reasoner will automatically evaluate external atoms whenever this is necessary.
 * However, heuristics may initiate additional calls which might trigger learning methods
 * to add nogoods in order to guide the search.
 */
class DLVHEX_EXPORT ExternalAtomEvaluationHeuristics{
protected:
	/** \brief Pointer to the registry. */
	RegistryPtr reg;

public:
	ExternalAtomEvaluationHeuristics(RegistryPtr reg) : reg(reg) {}
	virtual ~ExternalAtomEvaluationHeuristics() {}
	/**
	* Decides if the reasoner shall evaluate a given external atom at this point.
	* @param eatom The external atom in question.
	* @param eatomMask Mask with all atoms relevant for this external atom.
	* @param programMask All atoms in the program.
	* @param partialAssignment The current (partial) interpretation.
	* @param assigned The current set of assigned atoms; if 0, then the interpretation is complete.
	* @param changed The set of atoms with a (possibly) modified truth value since the last call; if NULL then all atoms have (possibly) changed.
	* @return True if the heuristics suggests to evaluate the external atom, otherwise false.
	*/
	virtual bool doEvaluate(const ExternalAtom& eatom, InterpretationConstPtr eatomMask, InterpretationConstPtr programMask, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed) = 0;
};

typedef boost::shared_ptr<ExternalAtomEvaluationHeuristics> ExternalAtomEvaluationHeuristicsPtr;

/**
 * \brief Factory for ExternalAtomEvaluationHeuristics.
 */
class DLVHEX_EXPORT ExternalAtomEvaluationHeuristicsFactory{
public:
	/**
	 * \brief Creates a new instance of the heuristics.
	 * @param reg Pointer to the registry.
	 */
	virtual ExternalAtomEvaluationHeuristicsPtr createHeuristics(RegistryPtr reg) = 0;

	/**
	 * \brief Destructor.
	 */
	virtual ~ExternalAtomEvaluationHeuristicsFactory(){}
};

typedef boost::shared_ptr<ExternalAtomEvaluationHeuristicsFactory> ExternalAtomEvaluationHeuristicsFactoryPtr;

DLVHEX_NAMESPACE_END

#endif

