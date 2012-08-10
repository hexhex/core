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
 * @file UnfoundedSetCheckHeuristics.cpp
 * @author Christoph Redl
 *
 * @brief  Base class and concrete classes with heuristics for
 *         unfounded set checks in genuine G&C model generators.
 */

#include "dlvhex2/ExternalAtomEvaluationHeuristics.h"

#include "dlvhex2/GenuineGuessAndCheckModelGenerator.h"

DLVHEX_NAMESPACE_BEGIN

// ============================== Base ==============================

UnfoundedSetCheckHeuristics::UnfoundedSetCheckHeuristics(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg) : mg(mg), reg(reg){
}

// ============================== Post ==============================

UnfoundedSetCheckHeuristicsPost::UnfoundedSetCheckHeuristicsPost(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg) : UnfoundedSetCheckHeuristics(mg, reg){
}

std::pair<bool, std::set<ID> > UnfoundedSetCheckHeuristicsPost::doUFSCheck(InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed){
	return std::pair<bool, std::set<ID> >(false, std::set<ID>() );
}

UnfoundedSetCheckHeuristicsPtr UnfoundedSetCheckHeuristicsPostFactory::createHeuristics(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg){
	return UnfoundedSetCheckHeuristicsPtr(new UnfoundedSetCheckHeuristicsPost(mg, reg));
}

// ============================== Max ==============================

UnfoundedSetCheckHeuristicsMax::UnfoundedSetCheckHeuristicsMax(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg) : UnfoundedSetCheckHeuristics(mg, reg){
}

std::pair<bool, std::set<ID> > UnfoundedSetCheckHeuristicsMax::doUFSCheck(InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed){

	// partial UFS check
	const std::vector<ID>& idb = mg->getGroundProgram().idb;
	std::set<ID> skipProgram;
	BOOST_FOREACH (ID ruleID, idb){
		// check if all atoms in the rule have been assigned
		const Rule& rule = reg->rules.getByID(ruleID);
		if (rule.isEAGuessingRule()) continue;
		bool assigned = true;
		BOOST_FOREACH (ID h, rule.head){
			if (!factWasSet->getFact(h.address)){
				assigned = false;
				break;
			}
		}
		BOOST_FOREACH (ID b, rule.body){
			if (!factWasSet->getFact(b.address)){
				assigned = false;
				break;
			}
			if (b.isExternalAuxiliary()){	
				assigned = mg->isVerified(b, factWasSet);
				if (!assigned) break;
			}
		}
		if (!assigned){
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

UnfoundedSetCheckHeuristicsPtr UnfoundedSetCheckHeuristicsMaxFactory::createHeuristics(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg){
	return UnfoundedSetCheckHeuristicsPtr(new UnfoundedSetCheckHeuristicsMax(mg, reg));
}

// ============================== Periodic ==============================

UnfoundedSetCheckHeuristicsPeriodic::UnfoundedSetCheckHeuristicsPeriodic(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg) : UnfoundedSetCheckHeuristicsMax(mg, reg), counter(0){
}

std::pair<bool, std::set<ID> > UnfoundedSetCheckHeuristicsPeriodic::doUFSCheck(InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed){
	counter++;
	if (counter >= 10){
		counter = 0;
		return UnfoundedSetCheckHeuristicsMax::doUFSCheck(partialInterpretation, factWasSet, changed);
	}else{
		return std::pair<bool, std::set<ID> >(false, std::set<ID>() );
	}
}

UnfoundedSetCheckHeuristicsPtr UnfoundedSetCheckHeuristicsPeriodicFactory::createHeuristics(HeuristicsModelGeneratorInterface* mg, RegistryPtr reg){
	return UnfoundedSetCheckHeuristicsPtr(new UnfoundedSetCheckHeuristicsPeriodic(mg, reg));
}

DLVHEX_NAMESPACE_END

