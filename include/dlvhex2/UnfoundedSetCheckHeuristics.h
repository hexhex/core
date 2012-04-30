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

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
 * Decides when to do an unfounded set check (over partial interpretations)
 */
class HeuristicsModelGeneratorInterface;

// ============================== Base ==============================

class UnfoundedSetCheckHeuristics{
protected:
	HeuristicsModelGeneratorInterface* mg;
	RegistryPtr reg;
public:
	UnfoundedSetCheckHeuristics(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg);
	/**
	* Decides if the reasoner shall do an unfounded set check at this point.
	* @param groundIDB The IDB of the ground program
	* @param partialInterpretation The current (partial) interpretation
	* @param factWasSet The current set of assigned atoms; if 0, then the interpretation is complete
	* @param changed The set of atoms with a (possibly) modified truth value since the last call; if 0, then all atoms have changed
	* @return std::pair<bool, std::vector<ID> >
	*         The first component is true if the heuristics suggests to do an UFS check, otherwise false.
	*         If true, then the second component is the set of rules which shall be ignored in the UFS check. The assignment must be complete for all non-ignored rules.
	*/
	virtual std::pair<bool, std::set<ID> > doUFSCheck(InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed) = 0;
};

typedef boost::shared_ptr<UnfoundedSetCheckHeuristics> UnfoundedSetCheckHeuristicsPtr;

class UnfoundedSetCheckHeuristicsFactory{
public:
	virtual UnfoundedSetCheckHeuristicsPtr createHeuristics(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg) = 0;
	virtual ~UnfoundedSetCheckHeuristicsFactory(){}
};

typedef boost::shared_ptr<UnfoundedSetCheckHeuristicsFactory> UnfoundedSetCheckHeuristicsFactoryPtr;

// ============================== Post ==============================

class UnfoundedSetCheckHeuristicsPost : public UnfoundedSetCheckHeuristics{
public:
	UnfoundedSetCheckHeuristicsPost(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg);
	virtual std::pair<bool, std::set<ID> > doUFSCheck(InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed);
};

class UnfoundedSetCheckHeuristicsPostFactory : public UnfoundedSetCheckHeuristicsFactory{
	virtual UnfoundedSetCheckHeuristicsPtr createHeuristics(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg);
};

// ============================== Max ==============================

class UnfoundedSetCheckHeuristicsMax : public UnfoundedSetCheckHeuristics{
public:
	UnfoundedSetCheckHeuristicsMax(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg);
	virtual std::pair<bool, std::set<ID> > doUFSCheck(InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed);
};

class UnfoundedSetCheckHeuristicsMaxFactory : public UnfoundedSetCheckHeuristicsFactory{
	virtual UnfoundedSetCheckHeuristicsPtr createHeuristics(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg);
};

// ============================== Periodic ==============================

class UnfoundedSetCheckHeuristicsPeriodic : public UnfoundedSetCheckHeuristicsMax{
private:
	int counter;
public:
	UnfoundedSetCheckHeuristicsPeriodic(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg);
	virtual std::pair<bool, std::set<ID> > doUFSCheck(InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed);
};

class UnfoundedSetCheckHeuristicsPeriodicFactory : public UnfoundedSetCheckHeuristicsFactory{
	virtual UnfoundedSetCheckHeuristicsPtr createHeuristics(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg);
};

DLVHEX_NAMESPACE_END

#endif

