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
 * @file   UnfoundedSetChecker.h
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Unfounded set checker for programs with disjunctions and external atoms.
 */

#ifndef UNFOUNDEDSETCHECKER_H__
#define UNFOUNDEDSETCHECKER_H__

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/BaseModelGenerator.h"
#include "dlvhex2/AnnotatedGroundProgram.h"

#include <boost/unordered_map.hpp>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

//
// Unfounded set checker for HEX programs (with external atoms)
//
class UnfoundedSetChecker{
protected:
	BaseModelGenerator* mg;

	enum Mode{
		// consider external atoms as ordinary ones
		Ordinary,
		// consider external atoms as such (requires ggncmg to be set)
		WithExt
	};
	Mode mode;

	ProgramCtx& ctx;
	RegistryPtr reg;

	// problem specification
	const OrdinaryASPProgram& groundProgram;
	AnnotatedGroundProgram agp;
	InterpretationConstPtr componentAtoms;
	SimpleNogoodContainerPtr ngc;
	InterpretationPtr domain; // domain of all problem variables

	// in AssumptionBasedUnfoundedSetChecker, solver is defined during the whole lifetime of the object
	// In EncodingBasedUnfoundedSetChecker, solver is only defined while getUnfoundedSet runs
	SATSolverPtr solver;

	/**
	* Checks if an UFS candidate is actually an unfounded set
	* @param compatibleSet The interpretation over which we compute UFSs
	* @param compatibleSetWithoutAux The interpretation over which we compute UFSs (without EA replacements)
	* @param ufsCandidate A candidate compatible set (solution to the nogood set created by getUFSDetectionProblem)
	* @param bool True iff ufsCandidate is an unfounded set
	*/
	bool isUnfoundedSet(InterpretationConstPtr compatibleSet, InterpretationConstPtr compatibleSetWithoutAux, InterpretationConstPtr ufsCandidate);

	/**
	* Transforms a nogood (valid input-output relationship of some external atom) learned in the main search for being used in the UFS search
	* @param ng The nogood from the main search
	* @param compatibleSet The current compatible set we do the unfounded set search wrt.
	*                      Note: For AssumptionBasedUnfoundedSetChecker it is essential that the nogood transformation is independent of the compatible set
	* @return std::vector<Nogood> A set of nogoods which can be used in the unfounded set search
	*/
	virtual std::vector<Nogood> nogoodTransformation(Nogood ng, InterpretationConstPtr compatibleSet) = 0;

public:
	/**
	* \brief Initialization for UFS search considering external atoms as ordinary ones
	* @param groundProgram Ground program over which the ufs check is done
	* @param componentAtoms The atoms in the strongly connected component in the atom dependency graph; if 0, then all atoms in groundProgram are considered to be in the SCC
	* @param ngc Set of valid input-output relationships learned in the main search (to be extended by this UFS checker)
	*/
	UnfoundedSetChecker(
		ProgramCtx& ctx,
		const OrdinaryASPProgram& groundProgram,
		InterpretationConstPtr componentAtoms = InterpretationConstPtr(),
		SimpleNogoodContainerPtr ngc = SimpleNogoodContainerPtr());

	/**
	* \brief Initialization for UFS search under consideration of the semantics of external atoms
	* @param ggncmg Reference to the G&C model generator for which this UnfoundedSetChecker runs
	* @param ctx ProgramCtx
	* @param groundProgram Ground program over which the ufs check is done
	* @param agp Annotated version of the ground program; may be a superset of groundProgram, but must contain meta information about all external atoms in groundProgram
	* @param componentAtoms The atoms in the strongly connected component in the atom dependency graph; if 0, then all atoms in groundProgram are considered to be in the SCC
	* @param ngc Set of valid input-output relationships learned in the main search (to be extended by this UFS checker)
	*/
	UnfoundedSetChecker(
		BaseModelGenerator* mg,
		ProgramCtx& ctx,
		const OrdinaryASPProgram& groundProgram,
		const AnnotatedGroundProgram& agp,
		InterpretationConstPtr componentAtoms = InterpretationConstPtr(),
		SimpleNogoodContainerPtr ngc = SimpleNogoodContainerPtr());

	virtual ~UnfoundedSetChecker() {}

	/**
	* Returns an unfounded set of groundProgram wrt. compatibleSet;
	* If the empty set is returned,
	* then there does not exist a greater (nonempty) unfounded set.
	* 
	* The method supports also unfounded set detection over partial interpretations.
	* For this purpose, skipProgram specifies all rules which shall be ignored
	* in the search. The interpretation must be complete and compatible over the non-ignored part.
	* Each detected unfounded set will remain an unfounded set for all possible
	* completions of the interpretation.
	*
	* @param compatibleSet The interpretation for which we want to compute an unfounded set
	* @param skipProgram The set of rules which shall be ignored in the UFS check (because the assignment might be incomplete wrt. these rules)
	* @return std::vector<IDAddress> An unfounded set (might be of size 0)
	*/
	virtual std::vector<IDAddress> getUnfoundedSet(InterpretationConstPtr compatibleSet, std::set<ID> skipProgram = std::set<ID>()) = 0;

	/**
	 * Forces the unfounded set checker to learn nogoods from main search now
	 */
	virtual void learnNogoodsFromMainSearch() = 0;

	/**
	* constructs a nogood which encodes the essence of an unfounded set
	* @param ufs The unfounded set to construct the nogood for
	* @param interpretation The interpretation which was used to compute the unfounded set for
	* @return Nogood The UFS-nogood
	*/
	Nogood getUFSNogood(
			const std::vector<IDAddress>& ufs,
			InterpretationConstPtr interpretation);

	typedef boost::shared_ptr<UnfoundedSetChecker> Ptr;
	typedef boost::shared_ptr<const UnfoundedSetChecker> ConstPtr;
};

typedef UnfoundedSetChecker::Ptr UnfoundedSetCheckerPtr;
typedef UnfoundedSetChecker::ConstPtr UnfoundedSetCheckerConstPtr;

class EncodingBasedUnfoundedSetChecker : public UnfoundedSetChecker{
private:
  /**
   * Constructs the nogood set used for unfounded set detection
   * The construction depends on the interpretation (encoding-based UFS detection) and requires:
   * - the compatible set (compatibleSet)
   * - the compatible set without external atom auxiliaries (compatibleSetWithoutAuxiliaries)
   * - the set of rules considered in the UFS search (ufsProgram)
   * - the set of rules in the program but ignored in the UFS search (skipProgram)
   * The constructed UFS detection problem is written to ufsDetectionProblem
   */
  void constructUFSDetectionProblem(
	NogoodSet& ufsDetectionProblem,
	InterpretationConstPtr compatibleSet,
	InterpretationConstPtr compatibleSetWithoutAux,
	std::set<ID>& skipProgram,
	std::vector<ID>& ufsProgram);
  void constructUFSDetectionProblemNecessaryPart(
	NogoodSet& ufsDetectionProblem,
	int& auxatomcnt,
	InterpretationConstPtr compatibleSet,
	InterpretationConstPtr compatibleSetWithoutAux,
	std::set<ID>& skipProgram,
	std::vector<ID>& ufsProgram);
  void constructUFSDetectionProblemOptimizationPart(
	NogoodSet& ufsDetectionProblem,
	int& auxatomcnt,
	InterpretationConstPtr compatibleSet,
	InterpretationConstPtr compatibleSetWithoutAux,
	std::set<ID>& skipProgram,
	std::vector<ID>& ufsProgram);
  void constructUFSDetectionProblemOptimizationPartRestrictToCompatibleSet(
	NogoodSet& ufsDetectionProblem,
	int& auxatomcnt,
	InterpretationConstPtr compatibleSet,
	InterpretationConstPtr compatibleSetWithoutAux,
	std::set<ID>& skipProgram,
	std::vector<ID>& ufsProgram);
  void constructUFSDetectionProblemOptimizationPartBasicEAKnowledge(
	NogoodSet& ufsDetectionProblem,
	int& auxatomcnt,
	InterpretationConstPtr compatibleSet,
	InterpretationConstPtr compatibleSetWithoutAux,
	std::set<ID>& skipProgram,
	std::vector<ID>& ufsProgram);
  void constructUFSDetectionProblemOptimizationPartLearnedFromMainSearch(
	NogoodSet& ufsDetectionProblem,
	int& auxatomcnt,
	InterpretationConstPtr compatibleSet,
	InterpretationConstPtr compatibleSetWithoutAux,
	std::set<ID>& skipProgram,
	std::vector<ID>& ufsProgram);
  void constructUFSDetectionProblemOptimizationPartEAEnforement(
	NogoodSet& ufsDetectionProblem,
	int& auxatomcnt,
	InterpretationConstPtr compatibleSet,
	InterpretationConstPtr compatibleSetWithoutAux,
	std::set<ID>& skipProgram,
	std::vector<ID>& ufsProgram);

  std::vector<Nogood> nogoodTransformation(Nogood ng, InterpretationConstPtr compatibleSet);

public:
	EncodingBasedUnfoundedSetChecker(
			ProgramCtx& ctx,
			const OrdinaryASPProgram& groundProgram,
			InterpretationConstPtr componentAtoms = InterpretationConstPtr(),
			SimpleNogoodContainerPtr ngc = SimpleNogoodContainerPtr());

	EncodingBasedUnfoundedSetChecker(	
			BaseModelGenerator& mg,
			ProgramCtx& ctx,
			const OrdinaryASPProgram& groundProgram,
			const AnnotatedGroundProgram& agp,
			InterpretationConstPtr componentAtoms = InterpretationConstPtr(),
			SimpleNogoodContainerPtr ngc = SimpleNogoodContainerPtr());

	void learnNogoodsFromMainSearch();

	std::vector<IDAddress> getUnfoundedSet(
			InterpretationConstPtr compatibleSet,
			std::set<ID> skipProgram = std::set<ID>());
};

class AssumptionBasedUnfoundedSetChecker : public UnfoundedSetChecker{
private:
	// a special atom "a_i" for each atom "a" in the program, representing the truth value of "a" in the compatible set
	boost::unordered_map<IDAddress, IDAddress> interpretationShadow;
	// a special atom "a_j" for each atom "a" in the program, representing the truth value of "a" in I u -X
	boost::unordered_map<IDAddress, IDAddress> residualShadow;
	// a special atom "a_f" for each atom "a" in the program, representing a change from of the truth value of a from true in I to false in I u -X
	boost::unordered_map<IDAddress, IDAddress> becomeFalse;

	// counter for auxiliary atoms
	int atomcnt;

	// stores how many nogoods in ngc we have already transformed and learned in the UFS search
	int learnedNogoodsFromMainSearch;

	void constructDomain();							// Goes through EDB and IDB and sets all facts in domain
	void constructUFSDetectionProblemFacts(NogoodSet& ns);			// Encodes that facts cannot be in the unfounded set
	void constructUFSDetectionProblemCreateAuxAtoms();			// sets up interpretationShadow and residualShadow
	void constructUFSDetectionProblemDefineResidualShadow(NogoodSet& ns);	// Defines: a_r = a_i \and -a,
										// where a_r = residualShadow[a] and a_i = interpretationShadow[a]
	void constructUFSDetectionProblemDefineBecomeFalse(NogoodSet& ns);	// Defines: a_f = a_i \and a,
										// where a_f = becomeFalse[a] and a_i = interpretationShadow[a]
	void constructUFSDetectionProblemRule(NogoodSet& ns, ID ruleID);	// Encodes a given program rule
	void constructUFSDetectionProblemNonempty(NogoodSet& ns);		// Encodes that we are looking for a nonempty unfounded set
	void constructUFSDetectionProblemRestrictToSCC(NogoodSet& ns);		// Restricts the search to the current strongly connected component
	void constructUFSDetectionProblemBasicEABehavior(NogoodSet& ns);	// Optimization: Basic behavior of external atoms

	/**
	* Constructs the nogood set used for unfounded set detection and instantiates the solver
	*/
	void constructUFSDetectionProblemAndInstantiateSolver();

	/**
	* Prepares the list of assumptions for an unfounded set search over a given compatible set
	* @param compatibleSet The compatible set over which we do the UFS search
	* @param skipProgram The set of rules ignored in the UFS check
	*/
	void setAssumptions(InterpretationConstPtr compatibleSet, const std::set<ID>& skipProgram);

	std::vector<Nogood> nogoodTransformation(Nogood ng, InterpretationConstPtr compatibleSet);

public:
	AssumptionBasedUnfoundedSetChecker(
			ProgramCtx& ctx,
			const OrdinaryASPProgram& groundProgram,
			InterpretationConstPtr componentAtoms = InterpretationConstPtr(),
			SimpleNogoodContainerPtr ngc = SimpleNogoodContainerPtr());

	AssumptionBasedUnfoundedSetChecker(	
			BaseModelGenerator& mg,
			ProgramCtx& ctx,
			const OrdinaryASPProgram& groundProgram,
			const AnnotatedGroundProgram& agp,
			InterpretationConstPtr componentAtoms = InterpretationConstPtr(),
			SimpleNogoodContainerPtr ngc = SimpleNogoodContainerPtr());

	void learnNogoodsFromMainSearch();

	std::vector<IDAddress> getUnfoundedSet(InterpretationConstPtr compatibleSet, std::set<ID> skipProgram = std::set<ID>());
};

class UnfoundedSetCheckerManager{
private:
	ProgramCtx& ctx;

	BaseModelGenerator* mg;
	AnnotatedGroundProgram agp;
	Nogood ufsnogood;
	std::map<int, UnfoundedSetCheckerPtr> preparedUnfoundedSetCheckers;

	std::vector<bool> intersectsWithNonHCFDisjunctiveRules;

	void computeChoiceRuleCompatibility(bool choiceRuleCompatible);

	UnfoundedSetCheckerPtr instantiateUnfoundedSetChecker(
			ProgramCtx& ctx,
			const OrdinaryASPProgram& groundProgram,
			InterpretationConstPtr componentAtoms = InterpretationConstPtr(),
			SimpleNogoodContainerPtr ngc = SimpleNogoodContainerPtr());

	UnfoundedSetCheckerPtr instantiateUnfoundedSetChecker(
			BaseModelGenerator& mg,
			ProgramCtx& ctx,
			const OrdinaryASPProgram& groundProgram,
			const AnnotatedGroundProgram& agp,
			InterpretationConstPtr componentAtoms = InterpretationConstPtr(),
			SimpleNogoodContainerPtr ngc = SimpleNogoodContainerPtr());

public:
	/**
	 * Initializes the UFS checker with support for external atoms.
	 * @param mg Reference to a model generator (used to evaluate the external atoms)
	 * @param ctx ProgramCtx
	 * @param agp Ground program with meta information used for optimized UFS checking
	 * @param choiceRuleCompatible This parameter is necessary for the clasp backend, which implements non-head cycle free disjunctive rules using choice rules.
	 *                             However, this transformation must be regarded in the optimization of UFS checking. More specifically, the UFS check MUST NOT BE SKIPPED
	 *                             for HFC-free components if they contain such choice rules. For more information, see examples/trickyufs.hex.
	 */
	UnfoundedSetCheckerManager(
			BaseModelGenerator& mg,
			ProgramCtx& ctx,
			const AnnotatedGroundProgram& agp,
			bool choiceRuleCompatible = false);

	/**
	 * Initializes the UFS checker without support for external atoms (they are considered as ordinary ones).
	 * @param ctx ProgramCtx
	 * @param agp Ground program with meta information used for optimized UFS checking
	 * @param choiceRuleCompatible This parameter is necessary for the clasp backend, which implements non-head cycle free disjunctive rules using choice rules.
	 *                             However, this transformation must be regarded in the optimization of UFS checking. More specifically, the UFS check MUST NOT BE SKIPPED
	 *                             for HFC-free components if they contain such choice rules. For more information, see examples/trickyufs.hex.
	 */
	UnfoundedSetCheckerManager(
			ProgramCtx& ctx,
			const AnnotatedGroundProgram& agp,
			bool choiceRuleCompatible = false);

	std::vector<IDAddress> getUnfoundedSet(
			InterpretationConstPtr interpretation,
			std::set<ID> skipProgram = std::set<ID>(),
			SimpleNogoodContainerPtr ngc = SimpleNogoodContainerPtr());

	/**
	 * Forces all unfounded set checker in this manager to learn nogoods from main search now
	 */
	void learnNogoodsFromMainSearch();

	Nogood getLastUFSNogood() const;

	typedef boost::shared_ptr<UnfoundedSetCheckerManager> Ptr;
};

typedef UnfoundedSetCheckerManager::Ptr UnfoundedSetCheckerManagerPtr;

DLVHEX_NAMESPACE_END

#endif
