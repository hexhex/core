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
 * @file   FinalModelGenerator.hpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Concrete model generator used for prototype. (TODO refactor, this should be AcycliModelGenerator, perhaps even a template parameterized by solver software)
 */

#ifndef FINAL_MODEL_GENERATOR_HPP_INCLUDED__09112010
#define FINAL_MODEL_GENERATOR_HPP_INCLUDED__09112010

#include "dlvhex/ModelGenerator.hpp"
#include "dlvhex/Interpretation.hpp"
#include "dlvhex/Logger.hpp"
#include "dlvhex/ComponentGraph.hpp"
#include "dlvhex/ASPSolverManager.h"

#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

class ProgramCtx;
class FinalModelGeneratorFactory;
struct Registry;
typedef boost::shared_ptr<Registry> RegistryPtr;

//
// A model generator does the following:
// * it is constructed by a ModelGeneratorFactory which knows the program
//   (and can precompute information for evaluation,
//    and may also provide this to the model generator)
// * it is evaluated on a (probably empty) input interpretation
// * this evaluation can be performed online
// * evaluation yields a (probably empty) set of output interpretations
//
class FinalModelGenerator:
  public ModelGeneratorBase<Interpretation>,
  public ostream_printable<FinalModelGenerator>
{
  // types
public:
  typedef FinalModelGeneratorFactory Factory;

protected:
  struct EmptyResults:
    public ASPSolverManager::Results
  {
    EmptyResults() {}
    virtual ~EmptyResults() {}
    virtual AnswerSet::Ptr getNextAnswerSet() { return AnswerSet::Ptr(); }
  };

  struct SingularResults:
    public ASPSolverManager::Results
  {
    SingularResults(AnswerSet::Ptr as): ASPSolverManager::Results(), ret(as) {}
    virtual ~SingularResults() {}
    virtual AnswerSet::Ptr getNextAnswerSet()
      { AnswerSet::Ptr p = ret; ret.reset(); return p; };
    AnswerSet::Ptr ret;
  };

  // storage
protected:
  Factory& factory;

  // edb + original (input) interpretation plus auxiliary atoms for evaluated external atoms
  InterpretationConstPtr postprocessedInput;
  // result handle for asp solver evaluation, using externallyAugmentedInput
  ASPSolverManager::ResultsPtr currentResults;

  // members
public:
  FinalModelGenerator(Factory& factory, InterpretationConstPtr input);
  virtual ~FinalModelGenerator() {}

  // generate and return next model, return null after last model
  virtual InterpretationPtr generateNextModel();

  // debug output
  //virtual std::ostream& print(std::ostream& o) const
  //  { return o << "ModelGeneratorBase::print() not overloaded"; }
protected:
  virtual void evaluateExternalAtoms(InterpretationPtr i) const;
  virtual InterpretationPtr projectEAtomInputInterpretation(
    const ExternalAtom& eatom, InterpretationConstPtr full) const;
  virtual void buildEAtomInputTuples(
    const ExternalAtom& eatom, InterpretationConstPtr i, std::list<Tuple>& inputs) const;
};

//
// a model generator factory provides model generators
// for a certain types of interpretations
//
class FinalModelGeneratorFactory:
  public ModelGeneratorFactoryBase<Interpretation>,
  public ostream_printable<FinalModelGeneratorFactory>
{
  // types
public:
  friend class FinalModelGenerator;
  typedef ComponentGraph::ComponentInfo ComponentInfo;

  // storage
protected:
  // which solver shall be used for external evaluation?
  ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig;
  ProgramCtx& ctx;
  std::vector<ID> eatoms;
  // original idb (containing eatoms, but already including auxiliary input rules)
  std::vector<ID> idb;
  // rewritten idb (containing replacements for eatoms) TODO we could remove the auxiliary input rules for xidb
  std::vector<ID> xidb; // x stands for transformed

  // methods
public:
  FinalModelGeneratorFactory(
      ProgramCtx& ctx, const ComponentInfo& ci,
      ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig);
  virtual ~FinalModelGeneratorFactory() {}

  virtual ModelGeneratorPtr createModelGenerator(
    InterpretationConstPtr input)
    { return ModelGeneratorPtr(new FinalModelGenerator(*this, input)); }

  // get rule
  // rewrite all eatoms in body to auxiliary replacement atoms
  // store and return id
  ID convertRule(ID ruleid);

  virtual std::ostream& print(std::ostream& o) const;
};

DLVHEX_NAMESPACE_END

#endif // FINAL_MODEL_GENERATOR_HPP_INCLUDED__09112010
