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
 * @file   BaseModelGenerator.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Base class for model generators, implementing common functionality.
 */

#ifndef BASE_MODEL_GENERATOR_HPP_INCLUDED__09112010
#define BASE_MODEL_GENERATOR_HPP_INCLUDED__09112010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/ModelGenerator.h"
#include "dlvhex2/Interpretation.h"
#include "dlvhex2/ASPSolverManager.h"
#include "dlvhex2/Atoms.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Nogood.h"
#include "dlvhex2/GenuineSolver.h"

#include <list>
#include "dlvhex2/CDNLSolver.h"

DLVHEX_NAMESPACE_BEGIN

class BaseModelGenerator:
  public ModelGeneratorBase<Interpretation>
{
  // members
public:
  BaseModelGenerator(InterpretationConstPtr input):
    ModelGeneratorBase<Interpretation>(input) {}
  virtual ~BaseModelGenerator() {}

  // callback function object for handling external atom answer tuples
  struct ExternalAnswerTupleCallback
  {
    virtual ~ExternalAnswerTupleCallback();
    // boolean return values specifies whether to continue the process
    // (true = continue, false = abort)

    // encountering next eatom
    virtual bool eatom(const ExternalAtom& eatom) = 0;
    // encountering next input tuple
    // (preceded by eatom(...))
    virtual bool input(const Tuple& input) = 0;
    // encountering next output tuple
    // (preceded by input(...) even for empty input tuples)
    virtual bool output(const Tuple& output) = 0;
  };

protected:
  // for usual model building where we want to collect all true answers
  // as replacement atoms in an interpretation
  struct IntegrateExternalAnswerIntoInterpretationCB:
    public ExternalAnswerTupleCallback
  {
    IntegrateExternalAnswerIntoInterpretationCB(
        InterpretationPtr outputi);
    virtual ~IntegrateExternalAnswerIntoInterpretationCB() {}
    // remembers eatom and prepares replacement.tuple[0]
    virtual bool eatom(const ExternalAtom& eatom);
    // remembers input
    virtual bool input(const Tuple& input);
    // creates replacement ogatom and activates respective bit in output interpretation
    virtual bool output(const Tuple& output);
  protected:
    RegistryPtr reg;
    InterpretationPtr outputi;
    OrdinaryAtom replacement;
  };

  // for usual model building where we want to collect all true answers
  // as replacement atoms in an interpretation
  struct VerifyExternalAnswerAgainstPosNegGuessInterpretationCB:
    public BaseModelGenerator::ExternalAnswerTupleCallback
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

  // ========== Global Learning ==========

  // computes the set of predicate IDs which are relevant
  // to a certain edb+idb
  std::set<ID> getPredicates(const RegistryPtr reg, InterpretationConstPtr edb, const std::vector<ID>& idb);

  // restricts an interpretation to the atoms over specified predicates
  InterpretationPtr restrictInterpretationToPredicates(const RegistryPtr reg, InterpretationConstPtr intr, const std::set<ID>& predicates);

  // converts an interpretation into a nogood
  Nogood interpretationToNogood(InterpretationConstPtr intr, NogoodContainer& ngContainer);

  void globalConflictAnalysis(ProgramCtx& ctx, const std::vector<ID>& idb, GenuineSolverPtr solver, bool componentIsMonotonic);

  // ========== ==========

  // projects input interpretation for predicate inputs
  // calculates constant input tuples from auxiliary input predicates and from given constants
  // calls eatom function with each input tuple
  // reintegrates output tuples as auxiliary atoms into outputi
  // (inputi and outputi may point to the same interpretation)
  //
  // returns false if process was aborted by callback, true otherwise
  virtual bool evaluateExternalAtom(RegistryPtr reg,
    const ExternalAtom& eatom,
    InterpretationConstPtr inputi,
    ExternalAnswerTupleCallback& cb,
    ProgramCtx* ctx = 0,
    NogoodContainerPtr nogoods = NogoodContainerPtr()) const;

  // calls evaluateExternalAtom for each atom in eatoms
  //
  // returns false if process was aborted by callback, true otherwise
  virtual bool evaluateExternalAtoms(RegistryPtr reg,
    const std::vector<ID>& eatoms,
    InterpretationConstPtr inputi,
    ExternalAnswerTupleCallback& cb,
    ProgramCtx* ctx = 0,
    NogoodContainerPtr nogoods = NogoodContainerPtr()) const;

  //
  // helper methods used by evaluateExternalAtom
  //

  // returns false iff tuple does not unify with eatom output pattern
  // (the caller must decide whether to throw an exception or ignore the tuple)
  virtual bool verifyEAtomAnswerTuple(RegistryPtr reg,
    const ExternalAtom& eatom, const Tuple& t) const;

  // project a given interpretation to all predicates that are predicate inputs in the given eatom
  // return this as a new interpretation
  virtual InterpretationPtr projectEAtomInputInterpretation(RegistryPtr reg,
    const ExternalAtom& eatom, InterpretationConstPtr full) const;

  // from auxiliary input predicates and the eatom,
  // calculate all tuples that are inputs to the eatom and store them into "inputs"
  virtual void buildEAtomInputTuples(RegistryPtr reg,
    const ExternalAtom& eatom, InterpretationConstPtr i, std::list<Tuple>& inputs) const;

  virtual bool isCompatibleSet(
		std::vector<ID>& innerEatoms,
		InterpretationConstPtr candidateCompatibleSet,
		InterpretationConstPtr postprocessedInput,
		PredicateMask& gpMask,
		PredicateMask& gnMask,
		ProgramCtx& ctx,
		NogoodContainerPtr nc);

  virtual bool isSubsetMinimalFLPModel(
		std::vector<ID>& innerEatoms,
		InterpretationConstPtr compatibleSet,
		InterpretationConstPtr postprocessedInput,
		PredicateMask& gpMask,
		PredicateMask& gnMask,
		PredicateMask& fMask,
		std::vector<ID>& xidbflphead,
		std::vector<ID>& xidbflpbody,
		std::vector<ID>& gidb,
		ProgramCtx& ctx);


  // predicate postfix for shadow predicates
  std::string shadowpostfix;

  // computes for each predicate p in idb/edb
  // a shadow predicate sp which does not yet occur
  void computeShadowPredicates(
      RegistryPtr reg,
      InterpretationConstPtr edb,
      const std::vector<ID>& idb,
      std::map<ID, std::pair<int, ID> >& shadowPredicates
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
      std::vector<ID>& idb);
};

//
// a model generator factory provides model generators
// for a certain types of interpretations
//
class BaseModelGeneratorFactory:
  public ModelGeneratorFactoryBase<Interpretation>
{
  // methods
public:
  BaseModelGeneratorFactory() {}
  virtual ~BaseModelGeneratorFactory() {}

protected:
  // rewrite all eatoms in body to auxiliary replacement atoms
  // store into registry and return id
  virtual ID convertRule(RegistryPtr reg, ID ruleid);
  // rewrite all eatoms in body tuple to auxiliary replacement atoms
  // store new body into convbody
  // (works recursively for aggregate atoms,
  // will create additional "auxiliary" aggregate atoms in registry)
  virtual void convertRuleBody(RegistryPtr reg, const Tuple& body, Tuple& convbody);


  // methods
  void createEatomGuessingRules(
      RegistryPtr reg,
      const std::vector<ID>& idb,
      const std::vector<ID>& innerEatoms,
      std::vector<ID>& gidb,
      PredicateMask& gpmask,
      PredicateMask& gnmask);

  void createFLPRules(
      RegistryPtr reg,
      const std::vector<ID>& xidb,
      std::vector<ID>& xidbflphead,
      std::vector<ID>& xidbflpbody,
      PredicateMask& fmask);
};

DLVHEX_NAMESPACE_END

#endif // BASE_MODEL_GENERATOR_HPP_INCLUDED__09112010
