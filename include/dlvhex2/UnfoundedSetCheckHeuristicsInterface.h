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
class DLVHEX_EXPORT UnfoundedSetCheckHeuristics{
protected:
	/**
	 * \brief Pointer to the registry.
	 */
	RegistryPtr reg;

	/**
	 * \brief Reference to the ground program for which the initation of unfounded set checks shall be decided.
	 */
	const AnnotatedGroundProgram& groundProgram;

public:
	/**
	 * \brief Wrapper for the result of heuristics.
	 */
	class UnfoundedSetCheckHeuristicsResult : public std::pair<bool, const std::set<ID>&>{
	public:
		/**
		 * \brief Constructor.
		 * @param doUFSCheck The outcome whether to do an unfounded set check or not.
		 * @param skipProgram The set of rules (identified by their IDs) which are to be ignored in the UFS check.
		 */
		UnfoundedSetCheckHeuristicsResult(bool doUFSCheck, const std::set<ID>& skipProgram);

		/**
		 * \brief See UnfoundedSetCheckHeuristicsResult::UnfoundedSetCheckHeuristicsResult.
		 */
		inline bool doUFSCheck() const { return first; }

		/**
		 * \brief See UnfoundedSetCheckHeuristicsResult::UnfoundedSetCheckHeuristicsResult.
		 */
		inline const std::set<ID>& skipProgram() const { return second; }
	};

	UnfoundedSetCheckHeuristics(const AnnotatedGroundProgram& groundProgram, RegistryPtr reg);

	/**
	 * \brief Decides if the reasoner shall do an unfounded set check at this point.
	 *
	 * @param verifiedAuxes The set of verified external atom auxiliaries wrt. the current partial interpretation
	 * @param partialAssignment The current (partial) interpretation
	 * @param assigned The current set of assigned atoms; if NULL, then the interpretation is complete
	 * @param changed The set of atoms with a (possibly) modified truth value since the last call of this method; if NULL, then all atoms have changed
	 * @return The first component is true if the heuristics suggests to do an UFS check, otherwise false.
	 *         If true, then the second component is the set of rules which shall be ignored in the UFS check. The assignment must be complete for all non-ignored rules.
	 */
	virtual UnfoundedSetCheckHeuristicsResult doUFSCheck(InterpretationConstPtr verifiedAuxes, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed) = 0;

	/**
	  * \brief Notifies the heuristic about changes in the assignment, although the caller is not going to perform an UFS check at this point.
	  * This allows the heuristic to update internal data structures.
	  * 
	  * @param partialAssignment The current (partial) interpretation
	  * @param assigned The current set of assigned atoms; if NULL, then the interpretation is complete
	  * @param changed The set of atoms with a (possibly) modified truth value since the last call of this method; if NULL, then all atoms have changed
	  */
	virtual void notify(InterpretationConstPtr verifiedAuxes, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed);
};

typedef boost::shared_ptr<UnfoundedSetCheckHeuristics> UnfoundedSetCheckHeuristicsPtr;

/**
 * \brief Factory for UnfoundedSetCheckHeuristics.
 */
class DLVHEX_EXPORT UnfoundedSetCheckHeuristicsFactory{
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

