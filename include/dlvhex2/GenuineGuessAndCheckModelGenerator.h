/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * Copyright (C) 2011, 2012, 2013, 2014 Christoph Redl
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
 * @file   GenuineGuessAndCheckModelGenerator.hpp
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Model generator for eval units that do not allow a fixpoint calculation.
 *
 * Those units may be of any form.
 */

#ifndef GENUINEGUESSANDCHECK_MODEL_GENERATOR_HPP_INCLUDED__09122011
#define GENUINEGUESSANDCHECK_MODEL_GENERATOR_HPP_INCLUDED__09122011

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/FLPModelGeneratorBase.h"
#include "dlvhex2/ComponentGraph.h"
#include "dlvhex2/PredicateMask.h"
#include "dlvhex2/GenuineSolver.h"
#include "dlvhex2/UnfoundedSetChecker.h"
#include "dlvhex2/NogoodGrounder.h"

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

// the factory
class GenuineGuessAndCheckModelGeneratorFactory:
  public FLPModelGeneratorFactoryBase,
  public ostream_printable<GenuineGuessAndCheckModelGeneratorFactory>
{
  // types
public:
  friend class GenuineGuessAndCheckModelGenerator;
  typedef ComponentGraph::ComponentInfo ComponentInfo;

  // storage
protected:
  // which solver shall be used for external evaluation?
  ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig;

  ProgramCtx& ctx;

  ComponentInfo ci;  // should be a reference, but there is currently a bug in the copy constructor of ComponentGraph: it seems that the component info is shared between different copies of a component graph, hence it is deallocated when one of the copies dies.
  WARNING("TODO see comment above about ComponentInfo copy construction bug")

  // outer external atoms
  std::vector<ID> outerEatoms;

public:
  GenuineGuessAndCheckModelGeneratorFactory(
      ProgramCtx& ctx, const ComponentInfo& ci,
      ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig);
  virtual ~GenuineGuessAndCheckModelGeneratorFactory() { }

  virtual ModelGeneratorPtr createModelGenerator(InterpretationConstPtr input);

  virtual std::ostream& print(std::ostream& o) const;
  virtual std::ostream& print(std::ostream& o, bool verbose) const;
};

// the model generator (accesses and uses the factory)
class GenuineGuessAndCheckModelGenerator:
  public FLPModelGeneratorBase,
  public ostream_printable<GenuineGuessAndCheckModelGenerator>,
  public PropagatorCallback
{
  // types
public:
  typedef GenuineGuessAndCheckModelGeneratorFactory Factory;

  // storage
protected:
  // we store the factory again, because the base class stores it with the base type only!
  Factory& factory;

  RegistryPtr reg;

  // information about verification/falsification of current external atom guesses
  boost::unordered_map<IDAddress, std::vector<int> > verifyWatchList, unverifyWatchList;
  std::vector<bool> eaEvaluated;	// is true iff the external atom guess was checked against the semantics (i.e., it is either verified or falsified)
  std::vector<bool> eaVerified;		// if eaEvaluated is true, then: eaVerified is true iff the check verified the guess
  InterpretationPtr verifiedAuxes;	// the set of currently verified external atom auxiliaries
  std::vector<InterpretationPtr> changedAtomsPerExternalAtom;	// stores for each inner external atom the cumulative atoms which potentially changes since last evaluation

  // incremental solving
  //	sub-component management for regrounding
  bool groundingIsComplete;
  ComponentGraphPtr subcompgraph;
  std::vector<PredicateMaskPtr> domainMaskPerComponent;
  std::vector<std::vector<ID> > gxidbPerComponent;
  //	management of resolving
  std::vector<ID> hookAssumptions;
  std::vector<ID> modelEliminationConstraints;
  InterpretationPtr inputWithDomainAtoms;
  InterpretationPtr previousInnerEatomOutputs;
  PredicateMaskPtr innerEatomOutputs;
  int previousRuleCount;
  InterpretationPtr currentRules;
  InterpretationPtr frozenHookAtoms;
  std::map<ID, ID> hookAtoms;

  // heuristics
  ExternalAtomEvaluationHeuristicsPtr defaultExternalAtomEvalHeuristics;
  std::vector<ExternalAtomEvaluationHeuristicsPtr> eaEvalHeuristics;	// stores for each external atom its evaluation heuristics
  UnfoundedSetCheckHeuristicsPtr ufsCheckHeuristics;

  // edb + original (input) interpretation plus auxiliary atoms for evaluated external atoms
  InterpretationConstPtr postprocessedInput;
  // non-external fact input, i.e., postprocessedInput before evaluating outer eatoms
  InterpretationPtr mask;

  // internal solver
  NogoodGrounderPtr nogoodGrounder;		// grounder for nonground nogoods
  SimpleNogoodContainerPtr learnedEANogoods;	// all nogoods learned from EA evaluations
  int learnedEANogoodsTransferredIndex;		// the highest index in learnedEANogoods which has already been transferred to the solver
  GenuineGrounderPtr grounder;
  GenuineGroundSolverPtr solver;
  UnfoundedSetCheckerManagerPtr ufscm;
  InterpretationPtr programMask;		// all atoms in the program

  // members

  /**
   * \brief Initializes heuristics for external atom evaluation and UFS checking over partial assignments.
   */
  void setHeuristics();

  /**
   * \brief Adds watches to all external auxilies for incremental verification and unverification of external atoms.
   */
  void createVerificationWatchLists();

  /**
   * \brief Learns related nonground nogoods.
   * @param ng The nogood to generalize.
   */
  void generalizeNogood(Nogood ng);

  /**
   * \brief Learns all support sets provided by external sources and adds them to supportSets
   */
  void learnSupportSets();

  /**
   * \brief Triggern nonground nogood learning and instantiation.
   * 
   * @param compatibleSet Current compatible set.
   * @param assigned Set of currently assigned atoms or NULL to represent a complete assignment (optimization).
   * @param changed Set of atoms which possibly changes since the previous call (optimization)
   *
   * Transferes new nogoods from learnedEANogoods to the solver and updates learnedEANogoodsTransferredIndex accordingly.
   */
  void updateEANogoods(InterpretationConstPtr compatibleSet, InterpretationConstPtr assigned = InterpretationConstPtr(), InterpretationConstPtr changed = InterpretationConstPtr());

  /**
   * \brief Checks after completion of an assignment if it is compatible.
   *
   * Depending on the eaVerificationMode, the compatibility is either directly checked in this function,
   * of previously recorded verfication results are used to compute the return value.
   *
   * @param modelCandidate The model candidate to check for compatibility
   * @return True if compatibel and false otherwise.
   */
  bool finalCompatibilityCheck(InterpretationConstPtr modelCandidate);

  /**
   * \brief Checks if a compatible set is a model, i.e., it does the FLP check.
   *
   * The details depend on the selected semantics (well-justified FLP or FLP) and the selected algorithm (explicit or ufs-based).
   * Depending on the eaVerificationMode, the compatibility is either directly checked in this function,
   * of previously recorded verfication results are used to compute the return value.
   *
   * @param compatibleSet The model candidate to check for minimality.
   * @return True if \p compatibleSet is an answer set and false otherwise.
   */
  bool isModel(InterpretationConstPtr compatibleSet);

  /**
   * \brief Adds hook rules for all atoms in the annotatedGroundProgram to allow for incremental extension.
   *
   * The method adds for all atoms "a" in the ground program rules of type "a :- a'" where "a'" is a new atom;
   * this allows for defining "a" in a later incremental step.
   */
  void addHookRules();

  /**
   * \brief Resets vector "hookAssumptions" and then adds assumptions such that all atoms in GenuineGuessAndCheckModelGenerator::frozenHookAtoms are false.
   */
  void buildFrozenHookAtomAssumptions();

  /**
   * \brief Checks if the domain of external atoms needs to be expanded wrt. a given compatible set.
   *
   * If this is the case, then the domain predicates are extended accordingly.
   *
   * @param expandedComponents The indices of all expanded components will be added to this vector.
   * @param model The compatible set used for domain expansion; if no model is specified, then the program is exhaustively grounded.
   * @return True if the domain needs to be expanded and false otherwise.
   */
  bool incrementalDomainExpansion(std::vector<int>& expandedComponents, InterpretationConstPtr model = InterpretationConstPtr());

  /**
   * \brief Updates the AnnotatedGroundProgram and the internal solver state wrt. extended domain predicates (if necessary).
   * @param expandedComponents The vector of all component indices which shall be respected in the expansion; other components will be ignored.
   */
  void incrementalProgramExpansion(const std::vector<int>& expandedComponents);

  /**
   * Constructs the rule headAtomID :- hookAtomID.
   */
  ID getIncrementalHookRule(ID headAtomID, ID hookAtomID);

  /**
   * Makes an unfounded set check over a (possibly) partial interpretation if useful.
   * @param partialInterpretation The current assignment.
   * @param assigned Currently assigned atoms (can be 0 if partial=false).
   * @param changed The set of atoms with modified truth value since the last call (can be 0 if partial=false).
   * @param partial True if the assignment is (potentially) partial; in this case the check is driven by a heuristic.
   * @return bool True if the check is passed, i.e., if there is *no* unfounded set. False if the check is failed, i.e., there *is* an unfounded set.
   */
  bool unfoundedSetCheck(InterpretationConstPtr partialInterpretation, InterpretationConstPtr assigned = InterpretationConstPtr(), InterpretationConstPtr changed = InterpretationConstPtr(), bool partial = false);

  /**
   * Finds a new atom in the scope of an external atom which shall be watched wrt. an interpretation.
   * @pre Some atom in the scope of the external atom is yet unassigned.
   * @param eaIndex The index of the inner external atom.
   * @param search Search interpretation; can be 0 to indicate that all atoms of the EA's mask are eligable.
   * @param truthValue Indicates whether to search for a true or a false atom in search.
   * @return Address of an atom to watch or ALL_ONES if none exists.
   */
  IDAddress getWatchedLiteral(int eaIndex, InterpretationConstPtr search, bool truthValue);

  /**
   * Removes verification results for external atoms if relevant parts of the input have changed.
   * @param changed The set of atoms with modified truth value since the last call.
   */
  void unverifyExternalAtoms(InterpretationConstPtr changed);

  /**
   * Heuristically decides if and which external atoms we evaluate.
   * @param partialInterpretation The current assignment.
   * @param assigned Currently assigned atoms.
   * @param changed The set of atoms with modified truth value since the last call.
   * @return bool True if evaluation yielded a conflict, otherwise false.
   */
  bool verifyExternalAtoms(InterpretationConstPtr partialInterpretation, InterpretationConstPtr assigned, InterpretationConstPtr changed);

  /**
   * Evaluates the inner external atom with index eaIndex (if possible, i.e., if the input is complete).
   * Learns nogoods if external learning is activated.
   * Sets eaVerified and eaEvaluated if eaVerificationMode == mixed.	
   * @param eaIndex The index of the external atom to verify.
   * @param partialInterpretation The current assignment.
   * @param assigned Currently assigned atoms (if 0, then the assignment is assumed to be complete).
   * @param changed The set of atoms with modified truth value since the last call (if 0, then all atoms are assumed to have changed).
   * @param answeredFromCacheOrSupportSets Optional pointer to a boolean where it is to be stored whether the query was answered from cache or support sets (true) or the external source was actually called (false).
   * @return bool True if the assignment is conflicting wrt. this external atom, otherwise false.
   */
  bool verifyExternalAtom(int eaIndex, InterpretationConstPtr partialInterpretation,
					           InterpretationConstPtr assigned = InterpretationConstPtr(),
					           InterpretationConstPtr changed = InterpretationConstPtr(),
					           bool* answeredFromCacheOrSupportSets = 0);
  /**
   * Evaluates the inner external atom with index eaIndex (if possible, i.e., if the input is complete) using explicit evaluation.
   * Learns nogoods if external learning is activated.
   * Sets eaVerified and eaEvaluated if eaVerificationMode == mixed.	
   * @param eaIndex The index of the external atom to verify.
   * @param partialInterpretation The current assignment.
   * @param assigned Currently assigned atoms (if 0, then the assignment is assumed to be complete).
   * @param changed The set of atoms with modified truth value since the last call (if 0, then all atoms are assumed to have changed).
   * @param answeredFromCache Optional pointer to a boolean where it is to be stored whether the query was answered from cache (true) or the external source was actually called (false).
   * @return bool True if the assignment is conflicting wrt. this external atom, otherwise false.
   */
  bool verifyExternalAtomByEvaluation(int eaIndex, InterpretationConstPtr partialInterpretation,
					           InterpretationConstPtr assigned = InterpretationConstPtr(),
					           InterpretationConstPtr changed = InterpretationConstPtr(),
					           bool* answeredFromCache = 0);

  /**
   * Evaluates the inner external atom with index eaIndex (if possible, i.e., if the input is complete) using complete support sets.
   * Learns nogoods if external learning is activated.
   * Sets eaVerified and eaEvaluated if eaVerificationMode == mixed.	
   * @param eaIndex The index of the external atom to verify.
   * @param partialInterpretation The current assignment.
   * @param assigned Currently assigned atoms (if 0, then the assignment is assumed to be complete).
   * @param changed The set of atoms with modified truth value since the last call (if 0, then all atoms are assumed to have changed).
   * @return bool True if the assignment is conflicting wrt. this external atom, otherwise false.
   */
  bool verifyExternalAtomBySupportSets(int eaIndex, InterpretationConstPtr partialInterpretation,
					            InterpretationConstPtr assigned = InterpretationConstPtr(),
					            InterpretationConstPtr changed = InterpretationConstPtr());

  /**
   * Returns the ground program in this component
   * @return const std::vector<ID>& Reference to the ground program in this component
   */
  const OrdinaryASPProgram& getGroundProgram();

  /**
   * \brief Is called by the ASP solver in its propagation method to trigger fruther learning methods.
   *
   * This function can add additional (learned) nogoods to the solver to force implications or tell the solver that the current assignment is conflicting.
   *
   * @param partialInterpretation The current assignment.
   * @param assigned Currently assigned atoms.
   * @param changed The set of atoms with modified truth value since the last call.
   */
  void propagate(InterpretationConstPtr partialInterpretation, InterpretationConstPtr assigned, InterpretationConstPtr changed);

public:
  /**
   * \brief Constructor.
   * @param factory Reference to the factory which created this model generator.
   * @param input Input interpretation to this model generator.
   */
  GenuineGuessAndCheckModelGenerator(Factory& factory, InterpretationConstPtr input);

  /**
   * \brief Destuctor.
   */
  virtual ~GenuineGuessAndCheckModelGenerator();

  // generate and return next model, return null after last model
  virtual InterpretationPtr generateNextModel();
};

DLVHEX_NAMESPACE_END

#endif // GUESSANDCHECK_MODEL_GENERATOR_HPP_INCLUDED__09112010
