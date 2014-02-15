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
 * @file UnfoundedSetCheckHeuristics.cpp
 * @author Christoph Redl
 *
 * @brief  Base class and concrete classes with heuristics for
 *         unfounded set checks in genuine G&C model generators.
 */

#include "dlvhex2/UnfoundedSetCheckHeuristics.h"
#include "dlvhex2/Interpretation.h"
#include "dlvhex2/OrdinaryASPProgram.h"
#include "dlvhex2/Printer.h"

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

// ============================== Base ==============================

UnfoundedSetCheckHeuristics::UnfoundedSetCheckHeuristics(RegistryPtr reg) : reg(reg){
}

// ============================== Post ==============================

UnfoundedSetCheckHeuristicsPost::UnfoundedSetCheckHeuristicsPost(RegistryPtr reg) : UnfoundedSetCheckHeuristics(reg){
}

std::pair<bool, std::set<ID> > UnfoundedSetCheckHeuristicsPost::doUFSCheck(const OrdinaryASPProgram& groundProgram, InterpretationConstPtr verifiedAuxes, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed){
	return std::pair<bool, std::set<ID> >(false, std::set<ID>() );
}

UnfoundedSetCheckHeuristicsPtr UnfoundedSetCheckHeuristicsPostFactory::createHeuristics(RegistryPtr reg){
	return UnfoundedSetCheckHeuristicsPtr(new UnfoundedSetCheckHeuristicsPost(reg));
}

// ============================== Max ==============================

UnfoundedSetCheckHeuristicsMax::UnfoundedSetCheckHeuristicsMax(RegistryPtr reg) : UnfoundedSetCheckHeuristics(reg){
}

std::pair<bool, std::set<ID> > UnfoundedSetCheckHeuristicsMax::doUFSCheck(const OrdinaryASPProgram& groundProgram, InterpretationConstPtr verifiedAuxes, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed){

	// partial UFS check
	const std::vector<ID>& idb = groundProgram.idb;
	std::set<ID> skipProgram;
	BOOST_FOREACH (ID ruleID, idb){
		// check if all atoms in the rule have been assigned
		const Rule& rule = reg->rules.getByID(ruleID);
		if (rule.isEAGuessingRule()) continue;
		bool allassigned = true;
		BOOST_FOREACH (ID h, rule.head){
			if (!assigned->getFact(h.address)){
				allassigned = false;
				break;
			}
		}
		BOOST_FOREACH (ID b, rule.body){
			if (!assigned->getFact(b.address)){
				allassigned = false;
				break;
			}
			if (b.isExternalAuxiliary()){
				allassigned = verifiedAuxes->getFact(b.address);
				if (!allassigned) break;
			}
		}
		if (!allassigned){
			skipProgram.insert(ruleID);
		}
	}
#ifndef NDEBUG
	{
		std::stringstream programstring;
		RawPrinter printer(programstring, reg);
		programstring << "Skipped program:" << std::endl;
		BOOST_FOREACH (ID ruleId, skipProgram){
			printer.print(ruleId);
			programstring << std::endl;
		}
		DBGLOG(DBG, programstring.str());
	}
#endif

	return std::pair<bool, std::set<ID> >(true, skipProgram);
}

UnfoundedSetCheckHeuristicsPtr UnfoundedSetCheckHeuristicsMaxFactory::createHeuristics(RegistryPtr reg){
	return UnfoundedSetCheckHeuristicsPtr(new UnfoundedSetCheckHeuristicsMax(reg));
}

// ============================== Periodic ==============================

UnfoundedSetCheckHeuristicsPeriodic::UnfoundedSetCheckHeuristicsPeriodic(RegistryPtr reg) : UnfoundedSetCheckHeuristicsMax(reg), counter(0){
}

std::pair<bool, std::set<ID> > UnfoundedSetCheckHeuristicsPeriodic::doUFSCheck(const OrdinaryASPProgram& groundProgram, InterpretationConstPtr verifiedAuxes, InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed){
	counter++;
	if (counter >= 10){
		counter = 0;
		return UnfoundedSetCheckHeuristicsMax::doUFSCheck(groundProgram, verifiedAuxes, partialAssignment, assigned, changed);
	}else{
		return std::pair<bool, std::set<ID> >(false, std::set<ID>() );
	}
}

UnfoundedSetCheckHeuristicsPtr UnfoundedSetCheckHeuristicsPeriodicFactory::createHeuristics(RegistryPtr reg){
	return UnfoundedSetCheckHeuristicsPtr(new UnfoundedSetCheckHeuristicsPeriodic(reg));
}

DLVHEX_NAMESPACE_END

