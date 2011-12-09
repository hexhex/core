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
 * @file   CDNLSolver.hpp
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Conflict-driven Nogood Learning Solver.
 */

#ifndef INTERNALGROUNDASPSOLVER_HPP_INCLUDED__09122011
#define INTERNALGROUNDASPSOLVER_HPP_INCLUDED__09122011

#include "dlvhex/ID.hpp"
#include "dlvhex/Interpretation.hpp"
#include "dlvhex/ProgramCtx.h"
#include <vector>
#include <set>
#include <map>
#include <boost/foreach.hpp>
#include "dlvhex/Printhelpers.hpp"
#include "CDNLSolver.hpp"
#include "dlvhex/ASPSolverManager.h"
//#include <bm/bm.h>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

class LearningCallback{
public:
	virtual bool learn(Interpretation::Ptr partialInterpretation, bm::bvector<> factWasSet) = 0;
};

class InternalGroundASPSolver : public CDNLSolver{
private:
	std::string bodyAtomPrefix;
	int bodyAtomNumber;
	bool firstmodel;

protected:
	// structural program information
	ASPProgram program;
	RegistryPtr reg;

	Set<IDAddress> ordinaryFacts;
	Set<IDAddress> nonSingularFacts;

										// dependency graph
	typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, IDAddress> Graph;
	typedef Graph::vertex_descriptor Node;
	std::map<IDAddress, Node> depNodes;
	Graph depGraph;

	std::vector<Set<IDAddress> > depSCC;					// store for each component the contained atoms
	std::map<IDAddress, int> componentOfAtom;				// store for each atom its component number

	std::map<ID, IDAddress> bodyAtomOfRule;					// store for each rule the body atom


	// data structures for unfounded set computation
	Set<IDAddress> unfoundedAtoms;						// currently unfounded atoms
	std::map<ID, Set<ID> > rulesWithBodyLiteral;				// store for each literal the rules which contain it in their body
	std::map<ID, Set<ID> > rulesWithHeadLiteral;				// store for each literal the rules which contain it in their head
	std::map<IDAddress, Set<IDAddress> > foundedAtomsOfBodyAtom;		// store for each body atom the set of atoms which use the corresponding rule as source
	std::map<IDAddress, ID> sourceRule;					// store for each atom a source rule (if available); for facts, ID_FAIL will be stored

	// external learning
	Set<LearningCallback*> learner;

	// statistics
	long cntDetectedUnfoundedSets;

	// initialization members
	void createNogoodsForRule(ID ruleBodyAtomID, ID ruleID);
	void createNogoodsForRuleBody(ID ruleBodyAtomID, const Tuple& ruleBody);
	Set<std::pair<ID, ID> > createShiftedProgram();
	void createSingularLoopNogoods();
	void computeClarkCompletion();
	void setEDB();
	void computeDepGraph();
	void computeStronglyConnectedComponents();
	void initSourcePointers();
	void initializeLists();

	// unfounded set members
	virtual void setFact(ID fact, int dl, int cause);
	virtual void clearFact(IDAddress litadr);
	void removeSourceFromAtom(IDAddress litadr);
	void addSourceToAtom(IDAddress, ID rule);
	Set<IDAddress> getDependingAtoms(IDAddress litadr);
	void getInitialNewlyUnfoundedAtomsAfterSetFact(ID fact, Set<IDAddress>& newlyUnfoundedAtoms);
	void updateUnfoundedSetStructuresAfterSetFact(ID fact);
	void updateUnfoundedSetStructuresAfterClearFact(IDAddress litadr);
	ID getPossibleSourceRule(Set<ID> ufs);
	bool useAsNewSourceForHeadAtom(IDAddress headAtom, ID sourceRule);
	Set<ID> getUnfoundedSet();

	// helper members
	bool doesRuleExternallySupportLiteral(ID ruleID, ID lit, Set<ID> s);
	Set<ID> getExternalSupport(Set<ID> s);
	Set<ID> satisfiesIndependently(ID ruleID, Set<ID> y);
	Nogood getLoopNogood(Set<ID> ufs);
	ID createNewAtom(std::string predName);
	ID createNewBodyAtom();
	std::string toString(Set<ID> lits);
	std::string toString(Set<IDAddress> lits);
	std::string toString(std::vector<IDAddress> lits);

	template <typename T>
	inline std::vector<T> intersect(const Set<T>& a, const std::vector<T>& b){
		std::vector<T> i;
		BOOST_FOREACH (T el, a){
			if (contains(b, el)) i.push_back(el);
		}
		return i;
	}
	template <typename T>
	inline Set<T> intersect(const Set<T>& a, const Set<T>& b){
		Set<T> out;
		std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), insert_set_iterator<T>(out)); //inserter(out, out.begin()));
		return out;
	}

public:
	virtual std::string getStatistics();

	InternalGroundASPSolver(ProgramCtx& ctx, ASPProgram& p);

	void addExternalLearner(LearningCallback* lb);
	void removeExternalLearner(LearningCallback* lb);

	virtual InterpretationConstPtr getNextModel();
	InterpretationPtr projectToOrdinaryAtoms(InterpretationConstPtr intr);

	typedef boost::shared_ptr<InternalGroundASPSolver> Ptr;
	typedef boost::shared_ptr<const InternalGroundASPSolver> ConstPtr;
};

typedef InternalGroundASPSolver::Ptr InternalGroundASPSolverPtr;
typedef InternalGroundASPSolver::ConstPtr InternalGroundASPSolverConstPtr;

DLVHEX_NAMESPACE_END

#endif
