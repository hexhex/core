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

DLVHEX_NAMESPACE_BEGIN

//
// a model generator factory
// that provides capability for true FLP reduct computation via rewriting, guessing, and checking
//
class FLPModelGeneratorFactoryBase:
  public BaseModelGeneratorFactory
{
public:
  FLPModelGeneratorFactoryBase(RegistryPtr reg);
  virtual ~FLPModelGeneratorFactoryBase() {}

protected:
  // data

  // for getting auxiliaries and registering FLP replacements
  RegistryPtr reg;

  // original idb (may contain eatoms where all inputs are known
  // -> auxiliary input rules of these eatoms must be in predecessor unit!)
  std::vector<ID> idb;

  // inner external atoms (those are guessed)
  std::vector<ID> innerEatoms;

  // one guessing rule for each inner eatom
  // (if one rule contains two inner eatoms, two guessing rules are created)
  std::vector<ID> gidb;

  // idb rewritten with eatom replacement atoms
  std::vector<ID> xidb;

  // xidb rewritten for FLP calculation
  std::vector<ID> xidbflphead; // rewriting to find out which body is satisfied -> creates heads
  std::vector<ID> xidbflpbody; // rewriting to compute the reduct's models -> has flp auxiliaries in bodies

  // incrementally updated bitmask for guessed eatom replacement predicates
  // (positive and negative, respectively)
  PredicateMask gpMask;
  PredicateMask gnMask;

  // incrementally updated bitmask for FLP auxiliary predicates
  PredicateMask fMask;

protected:
  // create guessing rules for external atom values
  void createEatomGuessingRules();

  // create rules from xidb
  // * for evaluating which bodies are satisfied -> xidbflphead
  //   -> this program creates flp auxiliary atoms
  // * for evaluating the reduct -> xidbflpbody
  //   -> this program has flp auxiliary atoms in the body
  void createFLPRules();

  friend class FLPModelGeneratorBase;
};

//
// the flp model generator base class (always refers to a factory which prepares the rewritten rules)
//
class FLPModelGeneratorBase:
  public BaseModelGenerator
{
  // members
public:
  FLPModelGeneratorBase(FLPModelGeneratorFactoryBase& factory, InterpretationConstPtr input);
  virtual ~FLPModelGeneratorBase() {}

protected:
  // the factory storing our flp rewriting and external atom guessing and shared bitmasks
  FLPModelGeneratorFactoryBase& factory;

protected:
  // callback for checking whether external computations
  // reflect guesses of external atom truth values
  struct VerifyExternalAnswerAgainstPosNegGuessInterpretationCB:
    public ExternalAnswerTupleCallback
  {
    VerifyExternalAnswerAgainstPosNegGuessInterpretationCB(
        InterpretationPtr guess_pos,
        InterpretationPtr guess_neg);
    virtual ~VerifyExternalAnswerAgainstPosNegGuessInterpretationCB() {}
    // remembers eatom and prepares replacement.tuple[0]
    virtual bool eatom(const ExternalAtom& eatom);
    // remembers input
    virtual bool input(const Tuple& input);
    // creates replacement ogatom and activates respective bit in output interpretation
    virtual bool output(const Tuple& output);
  protected:
    RegistryPtr reg;
    InterpretationPtr guess_pos, guess_neg;
    ID pospred, negpred;
    OrdinaryAtom replacement;
  };

  // checks whether guessed external atom truth values
  // and external atom computations coincide
  virtual bool isCompatibleSet(
		InterpretationConstPtr candidateCompatibleSet,
		InterpretationConstPtr postprocessedInput,
    ProgramCtx& ctx,
		NogoodContainerPtr nc);

  // checks whether a given model
  // is 
  virtual bool isSubsetMinimalFLPModel(
		InterpretationConstPtr compatibleSet,
		InterpretationConstPtr postprocessedInput,
		ProgramCtx& ctx);


  // computes for each predicate p in idb/edb
  // a shadow predicate sp which does not yet occur
  void computeShadowPredicates(
      RegistryPtr reg,
      InterpretationConstPtr edb,
      const std::vector<ID>& idb,
      std::map<ID, std::pair<int, ID> >& shadowPredicates,
      std::string& shadowPostfix
      );

  // adds the shadow facts for some edb input to output
  void addShadowInterpretation(
      RegistryPtr reg,
      std::map<ID, std::pair<int, ID> >& shadowPredicates,
      InterpretationConstPtr input,
      InterpretationPtr output);

  // computes for each pair of predicate p and shadow predicate sp
  // of arity n rules:
  //    :- p(X1, ..., Xn), not sp(X1, ..., Xn).
  //    smaller :- not p(X1, ..., Xn), sp(X1, ..., Xn).
  // and one rule
  //    :- not smaller
  void createMinimalityRules(
      RegistryPtr reg,
      std::map<ID, std::pair<int, ID> >& shadowPredicates,
      std::string& shadowPostfix,
      std::vector<ID>& idb);
};


DLVHEX_NAMESPACE_END

#endif // FLP_MODEL_GENERATOR_BASE_HPP_INCLUDED__09112010
