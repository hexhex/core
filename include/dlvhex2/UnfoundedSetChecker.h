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

private:

	// defines data structures used for verification of UFS candidates
	struct UnfoundedSetVerificationStatus{
		// Input used for external atom evaluation
		InterpretationPtr eaInput;

		// The auxiliaries which's new truth value needs to be checked
		// For each auxiliary A with address adr there is a unique index i s.t. auxiliariesToVerify[i]=adr
		std::vector<IDAddress> auxiliariesToVerify;

		// Stores for each auxiliary A with index i (see above) the external atoms auxiliaryDependsOnEA[i] which are remain to be evaluated before the truth/falsity of A is certain
		// (Note: since it needs to store the external atoms which *remain to be verified*, we cannot use the features of AnnotatedGroundProgram)
		std::vector<std::set<ID> > auxIndexToRemainingExternalAtoms;

		// Stores for each external atom with address adr the indices eaToAuxIndex[adr] in the vector auxiliaryDependsOnEA which depend on this external atom
		// (Note: since we need only certain auxiliaries, we cannot use the features of AnnotatedGroundProgram)
		std::vector<std::vector<int> > externalAtomAddressToAuxIndices;

		/**
		 * Prepares data structures used for verification of an unfounded set candidate wrt. a compatible set.
		 * @param agp The program over which the UFS check is done
		 * @param domain Domain of this unfounded set check
		 * @param ufsCandidate Representation of the UFS candidate
		 * @param compatibleSet Compatible Set
		 */
		UnfoundedSetVerificationStatus(
			const AnnotatedGroundProgram& agp,
			InterpretationConstPtr domain, InterpretationConstPtr ufsCandidate, InterpretationConstPtr compatibleSet, InterpretationConstPtr compatibleSetWithoutAux);
	};

	/**
	 * Explicitly evaluates an external atom and verifies or falsifies the auxiliaries which depend on it.
	 * @param ufsCandidate Representation of the UFS candidate
	 * @param compatibleSet Compatible Set
	 * @param ufsVerStatus Represents the current verification status (to be prepared by indexEatomsForUfsVerification)
	 * @param bool True iff verification succeeded
	 */
	bool verifyExternalAtomByEvaluation(
		ID eaID,
		InterpretationConstPtr ufsCandidate, InterpretationConstPtr compatibleSet,
		UnfoundedSetVerificationStatus& ufsVerStatus);

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
	virtual std::vector<IDAddress> getUnfoundedSet(InterpretationConstPtr compatibleSet, const std::set<ID>& skipProgram) = 0;

	/**
	 * Forces the unfounded set checker to learn nogoods from main search now
	 */
	virtual void learnNogoodsFromMainSearch(bool reset) = 0;

	/**
	* constructs a nogood which encodes the essence of an unfounded set
	* @param ufs The unfounded set to construct the nogood for
	* @param interpretation The interpretation which was used to compute the unfounded set for
	* @return Nogood The UFS-nogood
	*/
	Nogood getUFSNogood(
			const std::vector<IDAddress>& ufs,
			InterpretationConstPtr interpretation);
	Nogood getUFSNogoodReductBased(
			const std::vector<IDAddress>& ufs,
			InterpretationConstPtr interpretation);
	Nogood getUFSNogoodUFSBased(
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
	const std::set<ID>& skipProgram,
	std::vector<ID>& ufsProgram);
  void constructUFSDetectionProblemNecessaryPart(
	NogoodSet& ufsDetectionProblem,
	int& auxatomcnt,
	InterpretationConstPtr compatibleSet,
	InterpretationConstPtr compatibleSetWithoutAux,
	const std::set<ID>& skipProgram,
	std::vector<ID>& ufsProgram);
  void constructUFSDetectionProblemOptimizationPart(
	NogoodSet& ufsDetectionProblem,
	int& auxatomcnt,
	InterpretationConstPtr compatibleSet,
	InterpretationConstPtr compatibleSetWithoutAux,
	const std::set<ID>& skipProgram,
	std::vector<ID>& ufsProgram);
  void constructUFSDetectionProblemOptimizationPartRestrictToCompatibleSet(
	NogoodSet& ufsDetectionProblem,
	int& auxatomcnt,
	InterpretationConstPtr compatibleSet,
	InterpretationConstPtr compatibleSetWithoutAux,
	const std::set<ID>& skipProgram,
	std::vector<ID>& ufsProgram);
  void constructUFSDetectionProblemOptimizationPartBasicEAKnowledge(
	NogoodSet& ufsDetectionProblem,
	int& auxatomcnt,
	InterpretationConstPtr compatibleSet,
	InterpretationConstPtr compatibleSetWithoutAux,
	const std::set<ID>& skipProgram,
	std::vector<ID>& ufsProgram);
  void constructUFSDetectionProblemOptimizationPartLearnedFromMainSearch(
	NogoodSet& ufsDetectionProblem,
	int& auxatomcnt,
	InterpretationConstPtr compatibleSet,
	InterpretationConstPtr compatibleSetWithoutAux,
	const std::set<ID>& skipProgram,
	std::vector<ID>& ufsProgram);
  void constructUFSDetectionProblemOptimizationPartEAEnforement(
	NogoodSet& ufsDetectionProblem,
	int& auxatomcnt,
	InterpretationConstPtr compatibleSet,
	InterpretationConstPtr compatibleSetWithoutAux,
	const std::set<ID>& skipProgram,
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

	void learnNogoodsFromMainSearch(bool reset);

	std::vector<IDAddress> getUnfoundedSet(
			InterpretationConstPtr compatibleSet,
			const std::set<ID>& skipProgram);
};

class AssumptionBasedUnfoundedSetChecker : public UnfoundedSetChecker{
private:
	// a special atom "a_i" for each atom "a" in the program, representing the truth value of "a" in the compatible set
	boost::unordered_map<IDAddress, IDAddress> interpretationShadow;
	// a special atom "a_j" for each atom "a" in the program, representing the truth value of "a" in I u -X
	boost::unordered_map<IDAddress, IDAddress> residualShadow;
	// a special atom "a_f" for each atom "a" in the program, representing a change from of the truth value of a from true in I to false in I u -X
	boost::unordered_map<IDAddress, IDAddress> becomeFalse;
	// a special atom "a_{IandU}" for each atom "a" in the program, representing that a is true in I and member of U
	boost::unordered_map<IDAddress, IDAddress> IandU;
	// a special atom "a_{\overline{I}orU}" for each atom "a" in the program, representing that a is false in I or member of U
	boost::unordered_map<IDAddress, IDAddress> nIorU;

	// counter for auxiliary atoms
	int atomcnt;

	// stores how many nogoods in ngc we have already transformed and learned in the UFS search
	int learnedNogoodsFromMainSearch;

	void constructDomain();							// Goes through EDB and IDB and sets all facts in domain
	void constructUFSDetectionProblemFacts(NogoodSet& ns);			// Encodes that facts cannot be in the unfounded set
	void constructUFSDetectionProblemCreateAuxAtoms();			// sets up interpretationShadow and residualShadow
	void constructUFSDetectionProblemDefineAuxiliaries(NogoodSet& ns);	// Defines the auxiliary variables
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

	void learnNogoodsFromMainSearch(bool reset);

	std::vector<IDAddress> getUnfoundedSet(InterpretationConstPtr compatibleSet, const std::set<ID>& skipProgram);
};

class DLVHEX_EXPORT UnfoundedSetCheckerManager{
private:
	ProgramCtx& ctx;

	BaseModelGenerator* mg;
	AnnotatedGroundProgram agp;
	int lastAGPComponentCount;	// used in order to detect extensions of the agp
	Nogood ufsnogood;
	SimpleNogoodContainerPtr ngc;
	
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
	 * @param ngc Pointer to a container with valid input-output relationships (EANogoods)
	 */
	UnfoundedSetCheckerManager(
			BaseModelGenerator& mg,
			ProgramCtx& ctx,
			const AnnotatedGroundProgram& agp,
			bool choiceRuleCompatible = false,
			SimpleNogoodContainerPtr ngc = SimpleNogoodContainerPtr());

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
			const std::set<ID>& skipProgram,
			SimpleNogoodContainerPtr ngc = SimpleNogoodContainerPtr());

	std::vector<IDAddress> getUnfoundedSet(
			InterpretationConstPtr interpretation);

	/**
	 * Initializes the unfounded set checkers for all program components.
	 */
	void initializeUnfoundedSetCheckers();

	/**
	 * Forces all unfounded set checker in this manager to learn nogoods from main search now
	 */
	void learnNogoodsFromMainSearch(bool reset);

	Nogood getLastUFSNogood() const;

	typedef boost::shared_ptr<UnfoundedSetCheckerManager> Ptr;
};

typedef UnfoundedSetCheckerManager::Ptr UnfoundedSetCheckerManagerPtr;

DLVHEX_NAMESPACE_END

#endif
