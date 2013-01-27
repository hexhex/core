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
 * @brief  Base class for model generator factories and model generators.
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
#include "dlvhex2/ComponentGraph.h"

#include <list>
#include "dlvhex2/CDNLSolver.h"

DLVHEX_NAMESPACE_BEGIN

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
  // adds domain predicates to rule bodies in order to make external atoms groundable
  virtual ID addDomainPredicatesWhereNecessary(ProgramCtx& ctx, const ComponentGraph::ComponentInfo& ci, RegistryPtr reg, ID ruleid);
  // rewrite all eatoms in body to auxiliary replacement atoms
  // store into registry and return id
  virtual ID convertRule(ProgramCtx& ctx, ID ruleid);
  // rewrite all eatoms in body tuple to auxiliary replacement atoms
  // store new body into convbody
  // (works recursively for aggregate atoms,
  // will create additional "auxiliary" aggregate atoms in registry)
  virtual void convertRuleBody(ProgramCtx& ctx, const Tuple& body, Tuple& convbody);
};

//
// the base model generator
//
class BaseModelGenerator:
  public ModelGeneratorBase<Interpretation>
{
  friend class UnfoundedSetCheckerOld;
  friend class UnfoundedSetChecker;
  friend class EncodingBasedUnfoundedSetChecker;
  friend class AssumptionBasedUnfoundedSetChecker;
  // members
public:
  BaseModelGenerator(InterpretationConstPtr input):
    ModelGeneratorBase<Interpretation>(input) {}
  virtual ~BaseModelGenerator() {}

protected:
  //
  // ========== External Atom Evaluation Helpers ==========
  //

public:
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

  // projects input interpretation for predicate inputs
  // calculates constant input tuples from auxiliary input predicates and from given constants
  // calls eatom function with each input tuple
  // reintegrates output tuples as auxiliary atoms into outputi
  // (inputi and outputi may point to the same interpretation)
  //
  // returns false if process was aborted by callback, true otherwise
  virtual bool evaluateExternalAtom(ProgramCtx& ctx,
    const ExternalAtom& eatom,
    InterpretationConstPtr inputi,
    ExternalAnswerTupleCallback& cb,
    NogoodContainerPtr nogoods = NogoodContainerPtr()) const;
  virtual bool evaluateExternalAtomQuery(
    PluginAtom::Query& query,
    ExternalAnswerTupleCallback& cb,
    NogoodContainerPtr nogoods) const;

  // calls evaluateExternalAtom for each atom in eatoms
  //
  // returns false if process was aborted by callback, true otherwise
  virtual bool evaluateExternalAtoms(ProgramCtx& ctx,
    const std::vector<ID>& eatoms,
    InterpretationConstPtr inputi,
    ExternalAnswerTupleCallback& cb,
    NogoodContainerPtr nogoods = NogoodContainerPtr()) const;

  // helper methods used by evaluateExternalAtom

  // returns false iff tuple does not unify with eatom output pattern
  // (the caller must decide whether to throw an exception or ignore the tuple)
  virtual bool verifyEAtomAnswerTuple(RegistryPtr reg,
    const ExternalAtom& eatom, const Tuple& t) const;

  // project a given interpretation to all predicates that are predicate inputs in the given eatom
  // return this as a new interpretation
  virtual InterpretationPtr projectEAtomInputInterpretation(RegistryPtr reg,
    const ExternalAtom& eatom, InterpretationConstPtr full) const;

  // from auxiliary input predicates and the eatom,
  // calculate all tuples that are inputs to the eatom
  // and store them as true bits into "inputs", bits can be looked up in the EAInputTupleCache in registry
  virtual void buildEAtomInputTuples(RegistryPtr reg,
    const ExternalAtom& eatom, InterpretationConstPtr i, InterpretationPtr inputs) const;
};

DLVHEX_NAMESPACE_END

#endif // BASE_MODEL_GENERATOR_HPP_INCLUDED__09112010
