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
 * @file   FLPModelGeneratorBase.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * @author Chrisoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Base class for model generators using FLP reduct.
 */

#ifndef FLP_MODEL_GENERATOR_BASE_HPP_INCLUDED__09112010
#define FLP_MODEL_GENERATOR_BASE_HPP_INCLUDED__09112010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/BaseModelGenerator.h"
#include "dlvhex2/AnnotatedGroundProgram.h"

#include <boost/unordered_map.hpp>

DLVHEX_NAMESPACE_BEGIN

/** \brief Factory for model generators which
  * provides capability for true FLP reduct computation via rewriting, guessing, and checking. */
class DLVHEX_EXPORT FLPModelGeneratorFactoryBase:
  public BaseModelGeneratorFactory
{
public:
  FLPModelGeneratorFactoryBase(ProgramCtx& ctx);
  virtual ~FLPModelGeneratorFactoryBase() {}

protected:
  // data
  /** \brief ProgramCtx. */
  ProgramCtx& ctx;

  // for getting auxiliaries and registering FLP replacements
  /** \brief RegistryPtr. */
  RegistryPtr reg;

  /** \brief Original idb, possibly augmented with domain predicates
    * (may contain eatoms where all inputs are known).
    *
    * Auxiliary input rules of these eatoms must be in predecessor unit! */
  std::vector<ID> idb;

  /** \brief Inner external atoms (those are guessed). */
  std::vector<ID> innerEatoms;

  /** \brief One guessing rule for each inner eatom.
    *
    * If one rule contains two inner eatoms, two guessing rules are created. */
  std::vector<ID> gidb;

  /** \brief IDB rewritten with eatom replacement atoms. */
  std::vector<ID> xidb;

  /** \brief IDB for domain exploration (equivalent to xidb, except that it does not contain domain predicates). */
  std::vector<ID> deidb;
  /** \brief Inner external Atoms in deidb. */
  std::vector<ID> deidbInnerEatoms;

  // xidb rewritten for FLP calculation
  /** \brief Rewriting to find out which body is satisfied -> creates heads. */
  std::vector<ID> xidbflphead;
  /** \brief Rewriting to compute the reduct's models -> has flp auxiliaries in bodies */
  std::vector<ID> xidbflpbody;

  // incrementally updated bitmask for guessed eatom replacement predicates
  // (positive and negative, respectively)
  /** \brief Mask for positive external atom guesses. */
  PredicateMask gpMask;
  /** \brief Mask for negative external atom guesses. */
  PredicateMask gnMask;

  /** \brief Incrementally updated bitmask for FLP auxiliary predicates. */
  PredicateMask fMask;

protected:
  /** \brief Creates guessing rules for all external atoms in the component.
    * \param ctx ProgramCtx. */
  void createEatomGuessingRules(const ProgramCtx& ctx);
  /** \brief Creates guessing rules for one external atom in the component.
    * \param ctx ProgramCtx.
    * @param ruleID Rule which contains the external atom.
    * @param litID External atom ID.
    * @return ID of the guessing rule. */
  ID createEatomGuessingRule(const ProgramCtx& ctx, ID ruleID, ID litID);

  /** \brief Create rules from xidb.
    *
    * * For evaluating which bodies are satisfied -> xidbflphead
    *   -> this program creates flp auxiliary atoms
    * * For evaluating the reduct -> xidbflpbody
    *   -> this program has flp auxiliary atoms in the body
    */
  void createFLPRules();

  /** \brief Computes the set of predicates which occur in a cycle.
    * @param reg Registry.
    * @param ctx ProgramCtx.
    * @param idb Original IDB. */
  void computeCyclicInputPredicates(
			RegistryPtr reg,
			ProgramCtx& ctx,
			const std::vector<ID>& idb);


  friend class FLPModelGeneratorBase;
};

//
// the flp model generator base class (always refers to a factory which prepares the rewritten rules)
//
class DLVHEX_EXPORT FLPModelGeneratorBase:
  public BaseModelGenerator
{
  // members
public:
  /**
   * \brief Constructor.
   * @param factory Reference to the factory which created this model generator.
   * @param input Input interpretation to this model generator.
   */
  FLPModelGeneratorBase(FLPModelGeneratorFactoryBase& factory, InterpretationConstPtr input);
  /** \brief Destructor. */
  virtual ~FLPModelGeneratorBase() {}

protected:
  // the factory storing our flp rewriting and external atom guessing and shared bitmasks
  /** \brief Reference to the factory which created this model generator. */
  FLPModelGeneratorFactoryBase& factory;

  /** \brief Meta information about the ground program of this model generator. */
  AnnotatedGroundProgram annotatedGroundProgram;
protected:
  /** \brief Checks whether guessed external atom truth values
    * and external atom computations coincide.
    * @param candidateCompatibleSet Model of the ordinary ASP program to be checked for compatibility.
    * @param postprocessedInput Facts and auxiliaries for outer external atoms.
    * @param ctx ProgramCtx.
    * @param nc NogoodContainer to add learned nogoods to; can be NULL.
    * @return True if \p candidateCompatibleSet is compatible with the external atom semantics and false otherwise. */
  virtual bool isCompatibleSet(
		InterpretationConstPtr candidateCompatibleSet,
		InterpretationConstPtr postprocessedInput,
		ProgramCtx& ctx,
		SimpleNogoodContainerPtr nc);

  /** \brief Checks whether a given model is subset-minimal
    * OrdinaryASPSolverT must implement the OrdinaryASPSolver interface
    * (e.g., GenuineSolver).
    * @param compatibleSet A model of the ordinary ASP program which is compatible with the external atom semantics, i.e., it passed isCompatibleSet.
    * @param postprocessedInput Facts and auxiliaries for outer external atoms.
    * @param ctx ProgramCtx.
    * @param snc NogoodContainer to add learned nogoods to; can be NULL.
    * @return True if \p compatibleSet is an answer set and false otherwise. */
  template<typename OrdinaryASPSolverT>
  bool isSubsetMinimalFLPModel(
		InterpretationConstPtr compatibleSet,
		InterpretationConstPtr postprocessedInput,
		ProgramCtx& ctx,
		SimpleNogoodContainerPtr ngc = SimpleNogoodContainerPtr()
		);

  /** \brief Constructs a nogood which describes the essence of a
    * failed FLP check.
    * @param ctx ProgramCtx.
    * @param groundProgram Original ground program.
    * @param compatibleSet A model of the ordinary ASP program which is compatible with the external atom semantics, i.e., it passed isCompatibleSet.
    * @param projectedCompatibleSet Interpretation \p compatibleSet without replacement atoms.
    * @param smallerFLPModel A proper subset of \p projectedCompatibleSet.
    * @return A nogood which describes the essence of a failed FLP check and can be used for search space pruning.
    */
  Nogood getFLPNogood(
		ProgramCtx& ctx,
		const OrdinaryASPProgram& groundProgram,
		InterpretationConstPtr compatibleSet,
		InterpretationConstPtr projectedCompatibleSet,
		InterpretationConstPtr smallerFLPModel
		);

  /** \brief Computes for each predicate p in IDB/EDB a "shadow predicate" and an "unfounded predicate" (two new predicates) which does not yet occur.
    * @param reg RegistryPtr.
    * @param edb Program EDB.
    * @param idb Program IDB.
    * @param shadowPredicates Map where for each predicate a shadow predicate and its arity will be added.
    * @param unfoundedPredicates Map where for each predicate an unfoundedPredicates predicate and its arity will be added.
    * @param shadowPostfix String to append to shadow predicates.
    * @param unfoundedPostfix String to append to unfounded predicates.
    */
  void computeShadowAndUnfoundedPredicates(
      RegistryPtr reg,
      InterpretationConstPtr edb,
      const std::vector<ID>& idb,
      std::map<ID, std::pair<int, ID> >& shadowPredicates,
      std::map<ID, std::pair<int, ID> >& unfoundedPredicates,
      std::string& shadowPostfix,
      std::string& unfoundedPostfix
      );

  /**
    * \brief Transforms an EDB into its shadowed version, i.e., each predicate is replaced by its shadow predicate.
    * @param reg RegistryPtr.
    * @param shadowPredicates Map containing for each predicate a shadow predicate and its arity.
    * @param input EDB.
    * @param output Interpretation to receive the output.
    */
  void addShadowInterpretation(
      RegistryPtr reg,
      std::map<ID, std::pair<int, ID> >& shadowPredicates,
      InterpretationConstPtr input,
      InterpretationPtr output);

  /** Computes for each pair of predicate p and shadow predicate sp
    * of arity n rules:
    *    :- p(X1, ..., Xn), not sp(X1, ..., Xn).
    *    smaller :- not p(X1, ..., Xn), sp(X1, ..., Xn).
    * and one rule
    *    :- not smaller
    * @param reg RegistryPtr.
    * @param shadowPredicates Map containing for each predicate a shadow predicate and its arity.
    * @param shadowPostfix String to append to shadow predicates.
    * @param idb Original program IDB.
    */
  void createMinimalityRules(
      RegistryPtr reg,
      std::map<ID, std::pair<int, ID> >& shadowPredicates,
      std::string& shadowPostfix,
      std::vector<ID>& idb);

  /** \brief Creates rules which provide additional support for atoms in the program.
    *
    * We want to compute a _model_ of the reduct rather than an _answer set_,
    * i.e., atoms are allowed to be _not_ founded.
    * For this we introduce for each n-ary shadow predicate
    *	ps(X1, ..., Xn)
    * a rule
    *	p(X1, ..., Xn) v p_unfounded(X1, ..., Xn) :- ps(X1, ..., Xn)
    * which can be used to found an atom.
    * (p_unfounded(X1, ..., Xn) encodes that the atom is not artificially founded)
    * @param reg RegistryPtr.
    * @param shadowPredicates Map where for each predicate a shadow predicate and its arity will be added.
    * @param unfoundedPredicates Map where for each predicate an unfoundedPredicates predicate and its arity will be added.
    * @param idb Program IDB.
    */
  void createFoundingRules(
      RegistryPtr reg,
      std::map<ID, std::pair<int, ID> >& shadowPredicates,
      std::map<ID, std::pair<int, ID> >& unfoundedPredicates,
      std::vector<ID>& idb);
};

DLVHEX_NAMESPACE_END

#include "dlvhex2/FLPModelGeneratorBase.tcc"

#endif // FLP_MODEL_GENERATOR_BASE_HPP_INCLUDED__09112010
