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
 * @file   InconsistencyAnalyzer.hpp
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Analyzes the inconsistency in a program wrt. selected input facts.
 */

#ifndef INCONSISTENCYANALYZER_H_INCLUDED__02032012
#define INCONSISTENCYANALYZER_H_INCLUDED__02032012

#include "dlvhex2/ID.h"
#include "dlvhex2/Interpretation.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Printhelpers.h"
#include "dlvhex2/OrdinaryASPProgram.h"
#include "dlvhex2/Set.h"

#include <vector>
#include <map>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include "dlvhex2/GenuineSolver.h"
#include "dlvhex2/Nogood.h"

DLVHEX_NAMESPACE_BEGIN

class InconsistencyAnalyzer{
private:
	ProgramCtx& ctx;
	RegistryPtr reg;

	InterpretationPtr allAtoms;

	ID true_id, false_id, undef_id;
	ID atom_id, rule_id, head_id, bodyP_id, bodyN_id;
	ID ruleBlocked_id, ruleApplicable_id, rulePossiblyApplicable_id, ruleHasHead_id, ruleSomeHInI_id, ruleSomeHUndef_id, ruleViolated_id;
	ID litOtherHInI_id, litSupported_id, litPossiblySupported_id, litUnsupported_id;
	ID noAnswerSet_id;

	void registerTerms();

	void registerAtom(InterpretationPtr explEDB, std::vector<ID>& explIDB, ID atmId);
	void registerRule(InterpretationPtr explEDB, std::vector<ID>& explIDB, ID ruleId);

	void registerExplanationAtoms(InterpretationPtr explEDB, std::vector<ID>& explIDB, InterpretationConstPtr progEDB, InterpretationConstPtr exAt);

	void writeStaticProgramParts(InterpretationPtr explEDB, std::vector<ID>& explIDB);

	Nogood extractExplanationFromInterpretation(InterpretationConstPtr intr);
public:
	InconsistencyAnalyzer(ProgramCtx& ctx);

	Nogood explainInconsistency(const OrdinaryASPProgram& groundProgram, InterpretationConstPtr explainationAtoms);
};

DLVHEX_NAMESPACE_END

#endif


