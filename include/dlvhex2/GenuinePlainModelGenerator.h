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
 * @file   GenuinePlainModelGenerator.hpp
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Model generator for the "Plain" type of components using CDNL.
 */

#ifndef GENUINE_PLAIN_MODEL_GENERATOR_HPP_INCLUDED__09122011
#define GENUINE_PLAIN_MODEL_GENERATOR_HPP_INCLUDED__09122011

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/BaseModelGenerator.h"
#include "dlvhex2/ComponentGraph.h"
#include "dlvhex2/GenuineSolver.h"
#include "dlvhex2/InternalGroundDASPSolver.h"
#include "dlvhex2/InternalGrounder.h"

DLVHEX_NAMESPACE_BEGIN

class GenuinePlainModelGeneratorFactory;

//
// A model generator does the following:
// * it is constructed by a ModelGeneratorFactory which knows the program
//   (and can precompute information for evaluation,
//    and may also provide this to the model generator)
// * it is evaluated on a (probably empty) input interpretation
// * this evaluation can be performed online
// * evaluation yields a (probably empty) set of output interpretations
//
class GenuinePlainModelGenerator:
  public BaseModelGenerator,
  public ostream_printable<GenuinePlainModelGenerator>
{
  // types
public:
  typedef GenuinePlainModelGeneratorFactory Factory;

  // storage
protected:
  Factory& factory;

  // edb + original (input) interpretation plus auxiliary atoms for evaluated external atoms
  InterpretationConstPtr postprocessedInput;
  // result handle for asp solver evaluation, using externallyAugmentedInput
  ASPSolverManager::ResultsPtr currentResults;

  // genuine solver
  GenuineSolverPtr solver;
//  InternalGroundASPSolverPtr igas;
//  InternalGrounderPtr grounder;
//  Interpretation* currentanswer;

  // members

public:
  GenuinePlainModelGenerator(Factory& factory, InterpretationConstPtr input);
  virtual ~GenuinePlainModelGenerator();

  // generate and return next model, return null after last model
  virtual InterpretationPtr generateNextModel();

  // TODO debug output?
  //virtual std::ostream& print(std::ostream& o) const
  //  { return o << "ModelGeneratorBase::print() not overloaded"; }
};

//
// a model generator factory provides model generators
// for a certain types of interpretations
//
class GenuinePlainModelGeneratorFactory:
  public BaseModelGeneratorFactory,
  public ostream_printable<GenuinePlainModelGeneratorFactory>
{
  // types
public:
  friend class GenuinePlainModelGenerator;
  typedef ComponentGraph::ComponentInfo ComponentInfo;

  // storage
protected:
  // which solver shall be used for external evaluation?
  ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig;
  ProgramCtx& ctx;
  const ComponentInfo& ci;

  std::vector<ID> eatoms;
  // original idb (containing eatoms where all inputs are known
  // -> auxiliary input rules of these eatoms must be in predecessor unit!)
  std::vector<ID> idb;
  // rewritten idb (containing replacements for eatoms)
  // (x stands for transformed)
  std::vector<ID> xidb;

  // methods
public:
  GenuinePlainModelGeneratorFactory(
      ProgramCtx& ctx, const ComponentInfo& ci,
      ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig);
  virtual ~GenuinePlainModelGeneratorFactory() {}

  virtual ModelGeneratorPtr createModelGenerator(
    InterpretationConstPtr input)
    { return ModelGeneratorPtr(new GenuinePlainModelGenerator(*this, input)); }

  virtual std::ostream& print(std::ostream& o) const;
};

DLVHEX_NAMESPACE_END

#endif // PLAIN_MODEL_GENERATOR_HPP_INCLUDED__09112010
