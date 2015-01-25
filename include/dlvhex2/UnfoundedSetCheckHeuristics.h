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
class DLVHEX_EXPORT UnfoundedSetCheckHeuristicsPost : public UnfoundedSetCheckHeuristics{
public:
	UnfoundedSetCheckHeuristicsPost(const AnnotatedGroundProgram& groundProgram, RegistryPtr reg);
	virtual UnfoundedSetCheckHeuristicsResult doUFSCheck(InterpretationConstPtr verifiedAuxes, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed);
};

/**
 * \brief Factory for UnfoundedSetCheckHeuristicsPost.
 */
class DLVHEX_EXPORT UnfoundedSetCheckHeuristicsPostFactory : public UnfoundedSetCheckHeuristicsFactory{
	virtual UnfoundedSetCheckHeuristicsPtr createHeuristics(const AnnotatedGroundProgram& groundProgram, RegistryPtr reg);
};

// ============================== Max ==============================

/**
 * \brief Performs UFS checks whenever deterministic reasoning cannot derive further atoms.
 */
class DLVHEX_EXPORT UnfoundedSetCheckHeuristicsMax : public UnfoundedSetCheckHeuristics{
private:
	/** \brief stores the atoms which were assigned and verified when the skipProgram was updated last time. */
	InterpretationPtr previouslyAssignedAndVerifiedAtoms;

	/** \brief Remembers external atom replacement atoms which have already been assigned but could not be verified yet. */
	InterpretationPtr notYetVerifiedExternalAtoms;

	/** \brief Stores for each atom in which rule (identified by its index in the ground program) it occurs (positively or negatively). */
	std::map<IDAddress, std::set<int> > rulesOfAtom;

	/** \brief Stores for each rule (address) the number of total and of currently assigned and verified atoms. */
	std::vector<int> atomsInRule, assignedAndVerifiedAtomsInRule;

	/** \brief Skip program according to previouslyAssignedAtoms. */
	std::set<ID> skipProgram;
public:
	UnfoundedSetCheckHeuristicsMax(const AnnotatedGroundProgram& groundProgram, RegistryPtr reg);
	virtual UnfoundedSetCheckHeuristicsResult doUFSCheck(InterpretationConstPtr verifiedAuxes, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed);
	virtual void notify(InterpretationConstPtr verifiedAuxes, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed);
};

/**
 * \brief Factory for UnfoundedSetCheckHeuristicsMax.
 */
class DLVHEX_EXPORT UnfoundedSetCheckHeuristicsMaxFactory : public UnfoundedSetCheckHeuristicsFactory{
	virtual UnfoundedSetCheckHeuristicsPtr createHeuristics(const AnnotatedGroundProgram& groundProgram, RegistryPtr reg);
};

// ============================== Periodic ==============================

/**
 * \brief Performs UFS checks periodically.
 */
class DLVHEX_EXPORT UnfoundedSetCheckHeuristicsPeriodic : public UnfoundedSetCheckHeuristicsMax{
private:
	/** Counts the number of calls in order to periodically perform the UFS check. */
	int counter;

	/** \brief This interpretation is only resetted if an actual UFS check is performed.
	  *        This allows for reusing UnfoundedSetCheckHeuristicsMax for the implementation since it ensures that the notification of UnfoundedSetCheckHeuristicsMax
	  *        about changed atoms is correct.
	  */
	InterpretationPtr accumulatedChangedAtoms;
public:
	UnfoundedSetCheckHeuristicsPeriodic(const AnnotatedGroundProgram& groundProgram, RegistryPtr reg);
	virtual UnfoundedSetCheckHeuristicsResult doUFSCheck(InterpretationConstPtr verifiedAuxes, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed);
	virtual void notify(InterpretationConstPtr verifiedAuxes, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed);
};

/**
 * \brief Factory for UnfoundedSetCheckHeuristicsPeriodic.
 */
class DLVHEX_EXPORT UnfoundedSetCheckHeuristicsPeriodicFactory : public UnfoundedSetCheckHeuristicsFactory{
	virtual UnfoundedSetCheckHeuristicsPtr createHeuristics(const AnnotatedGroundProgram& groundProgram, RegistryPtr reg);
};

DLVHEX_NAMESPACE_END

#endif

