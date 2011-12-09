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
 * @file   InternalGrounder.hpp
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Grounder for disjunctive logic programs.
 */

#ifndef INTERNALGROUNDER_HPP_INCLUDED__09122011
#define INTERNALGROUNDER_HPP_INCLUDED__09122011

#include "dlvhex/ID.hpp"
#include "dlvhex/Interpretation.hpp"
#include "dlvhex/ProgramCtx.h"
#include <vector>
#include <map>
#include <boost/foreach.hpp>
#include "dlvhex/Printhelpers.hpp"
#include "dlvhex/ASPSolverManager.h"
#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

class InternalGrounder{
protected:
	typedef std::map<ID, ID> Substitution;
	typedef std::map<ID, int> Binder;

	ASPProgram inputprogram;
	RegistryPtr reg;

	// dependency graph
	typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, ID> DepGraph;
	typedef DepGraph::vertex_descriptor Node;
	std::map<ID, Node> depNodes;
	DepGraph depGraph;
	std::vector<std::set<ID> > depSCC;					// store for each component the contained predicates

	// strata dependencies
	typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, int> SCCDepGraph;
	SCCDepGraph compDependencies;
	std::vector<std::set<ID> > predicatesOfStratum;				// stores for each stratum the contained predicates
	std::vector<std::set<ID> > rulesOfStratum;				// stores for each stratum the contained rules
	std::map<ID, int> stratumOfPredicate;					// stores for each predicate its stratum index


	std::map<ID, std::vector<ID> > derivableAtomsOfPredicate;		// stores for each predicate (=term) the set of ground atoms over this
										// predicate which are currently derivable

	std::map<ID, std::set<std::pair<int, int> > > positionsOfPredicate;	// stores for each predicate the set of non-ground rules and
										// body positions where the predicate occurs

	InterpretationPtr trueAtoms;
	InterpretationPtr falseAtoms;

	std::vector<ID> groundRules;
	std::vector<ID> nonGroundRules;

	std::set<ID> resolvedPredicates;


	// initialization members
	void computeDepGraph();
	void computeStrata();

	void buildPredicateIndex();
	void loadStratum(int index);


	// grounding members
	void groundStratum(int index);

	void groundRule(ID ruleID, Substitution& s, std::vector<ID>& groundedRules, std::set<ID>& newDerivableAtoms);
	void buildGroundInstance(ID ruleID, Substitution s, std::vector<ID>& groundedRules, std::set<ID>& newDerivableAtoms);

	bool match(ID atom, ID patternAtom, Substitution& s);
	int matchNextFromExtension(ID atom, Substitution& s, int startSearchIndex);
	int backtrack(ID ruleID, Binder& binders, int currentIndex);

	void setToTrue(ID atom);
	void addDerivableAtom(ID atom, std::vector<ID>& groundRules, std::set<ID>& newDerivableAtoms);


	// helper members
	ID applySubstitutionToAtom(Substitution s, ID atomID);
	std::string ruleToString(ID ruleID);
	ID getPredicateOfAtom(ID atomID);
	bool isGroundRule(ID ruleID);
	bool isPredicateResolved(ID pred);
	bool isAtomDerivable(ID atom);
	int getStratumOfRule(ID ruleID);
	Binder getBinderOfRule(ID ruleID);
public:
	InternalGrounder(ProgramCtx& ctx, ASPProgram& p);

	ASPProgram getGroundProgram();

	typedef boost::shared_ptr<InternalGrounder> Ptr;
	typedef boost::shared_ptr<const InternalGrounder> ConstPtr;
};

typedef InternalGrounder::Ptr InternalGrounderPtr;
typedef InternalGrounder::ConstPtr InternalGrounderConstPtr;

DLVHEX_NAMESPACE_END

#endif
