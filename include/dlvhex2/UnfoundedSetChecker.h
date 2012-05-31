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
 * @author Chrisoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Unfounded set checker for programs with disjunctions and external atoms.
 */

#ifndef UNFOUNDEDSETCHECKER_H__
#define UNFOUNDEDSETCHECKER_H__

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/BaseModelGenerator.h"
#include "dlvhex2/GenuineGuessAndCheckModelGenerator.h"
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
private:
  GenuineGuessAndCheckModelGenerator* ggncmg;

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
  InterpretationConstPtr compatibleSet;
  InterpretationConstPtr compatibleSetWithoutAux;
  const std::set<ID>& skipProgram;
  InterpretationConstPtr componentAtoms;
  NogoodContainerPtr ngc;

  // ufs detection problem
  NogoodSet ufsDetectionProblem;
  InterpretationPtr domain; // domain of all problem variables
  std::vector<ID> ufsProgram;

  SATSolverPtr solver; // defined while getUnfoundedSet() runs, otherwise 0

  /**
   * Constructs the nogood set used for unfounded set detection
   */
  void constructUFSDetectionProblem();
  void constructUFSDetectionProblemNecessaryPart();
  void constructUFSDetectionProblemOptimizationPart();
  void constructUFSDetectionProblemOptimizationPartRestrictToCompatibleSet();
  void constructUFSDetectionProblemOptimizationPartBasicEAKnowledge();
  void constructUFSDetectionProblemOptimizationPartLearnedFromMainSearch();
  void constructUFSDetectionProblemOptimizationPartEAEnforement();

  /**
   * Checks if an UFS candidate is actually an unfounded set
   * @param ufsCandidate A candidate compatible set (solution to the nogood set created by getUFSDetectionProblem)
   * @param bool True iff ufsCandidate is an unfounded set
   */
  bool isUnfoundedSet(InterpretationConstPtr ufsCandidate);

  /**
   * Transforms a nogood (valid input-output relationship of some external atom) learned in the main search for being used in the UFS search
   */
  std::vector<Nogood> nogoodTransformation(Nogood ng, InterpretationConstPtr assignment);

public:
  /**
   * \brief Initialization for UFS search considering external atoms as ordinary ones
   * @param groundProgram Ground program over which the ufs check is done
   * @param compatibleSet A compatible set with external atom auxiliaries
   * @param skipProgram Part of groundProgram which is ignored in the unfounded set check
   * @param componentAtoms The atoms in the strongly connected component in the atom dependency graph; if 0, then all atoms in groundProgram are considered to be in the SCC
   */
  UnfoundedSetChecker(	ProgramCtx& ctx,
			const OrdinaryASPProgram& groundProgram,
			InterpretationConstPtr interpretation,
			std::set<ID> skipProgram = std::set<ID>(),
			InterpretationConstPtr componentAtoms = InterpretationConstPtr(),
			NogoodContainerPtr ngc = NogoodContainerPtr());

  /**
   * \brief Initialization for UFS search under consideration of the semantics of external atoms
   * @param ggncmg Reference to the G&C model generator for which this UnfoundedSetChecker runs
   * @param ctx ProgramCtx
   * @param groundProgram Ground program over which the ufs check is done
   * @param agp Annotated version of the ground program; may be a superset of groundProgram, but must contain meta information about all external atoms in groundProgram
   * @param compatibleSet A compatible set with external atom auxiliaries
   * @param skipProgram Part of groundProgram which is ignored in the unfounded set check
   * @param componentAtoms The atoms in the strongly connected component in the atom dependency graph; if 0, then all atoms in groundProgram are considered to be in the SCC
   * @param ngc Set of valid input-output relationships learned in the main search (to be extended by this UFS checker)
   */
  UnfoundedSetChecker(	
			GenuineGuessAndCheckModelGenerator& ggncmg,
			ProgramCtx& ctx,
			const OrdinaryASPProgram& groundProgram,
			const AnnotatedGroundProgram& agp,
//			const std::vector<ID>& innerEatoms,
			InterpretationConstPtr compatibleSet,
			std::set<ID> skipProgram = std::set<ID>(),
			InterpretationConstPtr componentAtoms = InterpretationConstPtr(),
			NogoodContainerPtr ngc = NogoodContainerPtr());

  // Returns an unfounded set of groundProgram wrt. compatibleSet;
  // If the empty set is returned,
  // then there does not exist a greater (nonempty) unfounded set.
  // 
  // The method supports also unfounded set detection over partial interpretations.
  // For this purpose, skipProgram specifies all rules which shall be ignored
  // in the search. The interpretation must be complete and compatible over the non-ignored part.
  // Each detected unfounded set will remain an unfounded set for all possible
  // completions of the interpretation.
  std::vector<IDAddress> getUnfoundedSet();

  // constructs a nogood which encodes the essence of an unfounded set
  Nogood getUFSNogood(
			std::vector<IDAddress> ufs,
			InterpretationConstPtr interpretation);
};

class UnfoundedSetCheckerManager{
private:
	GenuineGuessAndCheckModelGenerator* ggncmg;
	ProgramCtx& ctx;

	std::vector<ID> innerEatoms;

	AnnotatedGroundProgram agp;

	Nogood ufsnogood;
public:
	UnfoundedSetCheckerManager(
			GenuineGuessAndCheckModelGenerator& ggncmg,
			ProgramCtx& ctx,
			std::vector<ID>& innerEatoms,
			const AnnotatedGroundProgram& agp);

	UnfoundedSetCheckerManager(
			ProgramCtx& ctx,
			const AnnotatedGroundProgram& agp);

	std::vector<IDAddress> getUnfoundedSet(
			InterpretationConstPtr interpretation,
			std::set<ID> skipProgram = std::set<ID>(),
			NogoodContainerPtr ngc = NogoodContainerPtr());

	Nogood getLastUFSNogood() const;

	typedef boost::shared_ptr<UnfoundedSetCheckerManager> Ptr;
};

typedef UnfoundedSetCheckerManager::Ptr UnfoundedSetCheckerManagerPtr;

DLVHEX_NAMESPACE_END

#endif
