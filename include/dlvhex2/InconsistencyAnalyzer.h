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

// The hitting set problem:
//
// Given a domain D and a set S = {S[0], ..., S[d]} of subsets S[i] of D,
// find another subset H of D, such that H intersects with each S[i] in
// at least one element, and |H| is minimal.
// The problem is a generalization of the vertex cover problem to hypergraphs.
// It is NP-complete.
//
// We use the following heuristics:
// 1. S' = S
// 2. H = {}
// 3. Store for each d in D a counter c[d] = | { S[i] in S' : d in S[i] } |
// 4. While H is no hitting set of S
//    4a. Add d with highest c[d] to H
//    4b. Remove all S[i] with d in S[i] from S'
//    4c. Update the counters
template<typename T>
class HittingSetDetector{
public:
	typedef std::vector<T> Hyperedge;
	typedef std::vector<Hyperedge> Hypergraph;

	static std::vector<T> getHittingSet(Hypergraph graph){

		std::map<T, int> counter;

		// 1.
		Hypergraph activeGraph = graph;

		// 2.
		std::vector<T> hittingSet;

		do{
			// 3./4c. compute counter
			counter.clear();
			BOOST_FOREACH (Hyperedge edge, activeGraph){
				BOOST_FOREACH (T element, edge){
					counter[element]++;
				}
			}

			// 4. check if we have found a hitting set
			if (activeGraph.size() == 0) break;

			// 4a. find d with largest counter
			typedef std::pair<T, int> Pair;
			Pair largest;
			largest.second = -1;
			BOOST_FOREACH (Pair p, counter){
				if (largest.second == -1 || p.second > largest.second) largest = p;
			}
			hittingSet.push_back(largest.first);

			// 4b.
			bool updated = true;
			while (updated){
				updated = false;
				int i = 0;
				BOOST_FOREACH (Hyperedge edge, activeGraph){
					if (std::find(edge.begin(), edge.end(), largest.first) != edge.end()){
						activeGraph.erase(activeGraph.begin() + i);
						updated = true;
						break;
					}
					i++;
				}
			}
		}while (true);

		return hittingSet;
	}
};

class InconsistencyAnalyzer{
private:
	ProgramCtx& ctx;
	RegistryPtr reg;

	ID neg_id, inc_id;

	void registerTerms();

/*
	InterpretationPtr allAtoms;

	ID true_id, false_id, undef_id;
	ID atom_id, rule_id, head_id, bodyP_id, bodyN_id;
	ID ruleBlocked_id, ruleApplicable_id, rulePossiblyApplicable_id, ruleHasHead_id, ruleSomeHInI_id, ruleSomeHUndef_id, ruleViolated_id;
	ID litOtherHInI_id, litSupported_id, litPossiblySupported_id, litUnsupported_id;
	ID noAnswerSet_id;


	void registerAtom(InterpretationPtr explEDB, std::vector<ID>& explIDB, ID atmId);
	void registerRule(InterpretationPtr explEDB, std::vector<ID>& explIDB, ID ruleId);

	void registerExplanationAtoms(InterpretationPtr explEDB, std::vector<ID>& explIDB, InterpretationConstPtr progEDB, InterpretationConstPtr exAt);

	void writeStaticProgramParts(InterpretationPtr explEDB, std::vector<ID>& explIDB);
*/

	
	Nogood getIncNogood(const OrdinaryASPProgram& groundProgram, InterpretationConstPtr answerset);

	void extractExplanationFromInterpretation(Nogood& ng, const OrdinaryASPProgram& groundProgram, InterpretationConstPtr answerset);
	HittingSetDetector<ID>::Hyperedge extractHyperedgeFromInterpretation(const OrdinaryASPProgram& groundProgram, InterpretationConstPtr answerset);
	Nogood getExplanationFromHypergraph(HittingSetDetector<ID>::Hypergraph graph);
	int getNumberOfIncAtoms(InterpretationConstPtr intr);
public:
	InconsistencyAnalyzer(ProgramCtx& ctx);

	Nogood explainInconsistency(const OrdinaryASPProgram& groundProgram, InterpretationConstPtr explanationAtoms);
};

DLVHEX_NAMESPACE_END

#endif


