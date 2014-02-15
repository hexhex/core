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

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
 * Decides when to do an unfounded set check (over partial interpretations)
 */
// ============================== Base ==============================

class UnfoundedSetCheckHeuristics{
protected:
	RegistryPtr reg;
public:
	UnfoundedSetCheckHeuristics(RegistryPtr reg);
	/**
	* Decides if the reasoner shall do an unfounded set check at this point.
	* @param groundProgram The ground program
	* @param verifiedAuxes The set of verified external atom auxiliaries wrt. the current partial interpretation
	* @param partialAssignment The current (partial) interpretation
	* @param assigned The current set of assigned atoms; if 0, then the interpretation is complete
	* @param changed The set of atoms with a (possibly) modified truth value since the last call; if 0, then all atoms have changed
	* @return std::pair<bool, std::vector<ID> >
	*         The first component is true if the heuristics suggests to do an UFS check, otherwise false.
	*         If true, then the second component is the set of rules which shall be ignored in the UFS check. The assignment must be complete for all non-ignored rules.
	*/
	virtual std::pair<bool, std::set<ID> > doUFSCheck(const OrdinaryASPProgram& groundProgram, InterpretationConstPtr verifiedAuxes, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed) = 0;
};

typedef boost::shared_ptr<UnfoundedSetCheckHeuristics> UnfoundedSetCheckHeuristicsPtr;

class UnfoundedSetCheckHeuristicsFactory{
public:
	virtual UnfoundedSetCheckHeuristicsPtr createHeuristics(RegistryPtr reg) = 0;
	virtual ~UnfoundedSetCheckHeuristicsFactory(){}
};

typedef boost::shared_ptr<UnfoundedSetCheckHeuristicsFactory> UnfoundedSetCheckHeuristicsFactoryPtr;

// ============================== Post ==============================

class UnfoundedSetCheckHeuristicsPost : public UnfoundedSetCheckHeuristics{
public:
	UnfoundedSetCheckHeuristicsPost(RegistryPtr reg);
	virtual std::pair<bool, std::set<ID> > doUFSCheck(const OrdinaryASPProgram& groundProgram, InterpretationConstPtr verifiedAuxes, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed);
};

class UnfoundedSetCheckHeuristicsPostFactory : public UnfoundedSetCheckHeuristicsFactory{
	virtual UnfoundedSetCheckHeuristicsPtr createHeuristics(RegistryPtr reg);
};

// ============================== Max ==============================

class UnfoundedSetCheckHeuristicsMax : public UnfoundedSetCheckHeuristics{
public:
	UnfoundedSetCheckHeuristicsMax(RegistryPtr reg);
	virtual std::pair<bool, std::set<ID> > doUFSCheck(const OrdinaryASPProgram& groundProgram, InterpretationConstPtr verifiedAuxes, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed);
};

class UnfoundedSetCheckHeuristicsMaxFactory : public UnfoundedSetCheckHeuristicsFactory{
	virtual UnfoundedSetCheckHeuristicsPtr createHeuristics(RegistryPtr reg);
};

// ============================== Periodic ==============================

class UnfoundedSetCheckHeuristicsPeriodic : public UnfoundedSetCheckHeuristicsMax{
private:
	int counter;
public:
	UnfoundedSetCheckHeuristicsPeriodic(RegistryPtr reg);
	virtual std::pair<bool, std::set<ID> > doUFSCheck(const OrdinaryASPProgram& groundProgram, InterpretationConstPtr verifiedAuxes, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed);
};

class UnfoundedSetCheckHeuristicsPeriodicFactory : public UnfoundedSetCheckHeuristicsFactory{
	virtual UnfoundedSetCheckHeuristicsPtr createHeuristics(RegistryPtr reg);
};

DLVHEX_NAMESPACE_END

#endif

