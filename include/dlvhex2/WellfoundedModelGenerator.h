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
 * @file   WellfoundedModelGenerator.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Model generator for eval units that allow a fixpoint calculation.
 *
 * Those units may contain external atoms at the input,
 * only monotonic eatoms and no negative cycles within the SCC.
 */

#ifndef WELLFOUNDED_MODEL_GENERATOR_HPP_INCLUDED__09112010
#define WELLFOUNDED_MODEL_GENERATOR_HPP_INCLUDED__09112010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/BaseModelGenerator.h"
#include "dlvhex2/ComponentGraph.h"

DLVHEX_NAMESPACE_BEGIN

class WellfoundedModelGeneratorFactory;

class WellfoundedModelGenerator:
  public BaseModelGenerator,
  public ostream_printable<WellfoundedModelGenerator>
{
  // types
public:
  typedef WellfoundedModelGeneratorFactory Factory;

  // storage
protected:
  Factory& factory;

  // result handle for asp solver evaluation, using externallyAugmentedInput
  ASPSolverManager::ResultsPtr currentResults;

  // members
public:
  WellfoundedModelGenerator(Factory& factory, InterpretationConstPtr input);
  virtual ~WellfoundedModelGenerator() {}

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
class WellfoundedModelGeneratorFactory:
  public BaseModelGeneratorFactory,
  public ostream_printable<WellfoundedModelGeneratorFactory>
{
  // types
public:
  friend class WellfoundedModelGenerator;
  typedef ComponentGraph::ComponentInfo ComponentInfo;

  // storage
protected:
  // which solver shall be used for external evaluation?
  ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig;
  ProgramCtx& ctx;
  // outer eatoms
  std::vector<ID> outerEatoms;
  // inner eatoms
  std::vector<ID> innerEatoms;
  // original idb (containing eatoms where all inputs are known
  // -> auxiliary input rules of these eatoms must be in predecessor unit!)
  std::vector<ID> idb;
  // rewritten idb (containing replacements for eatoms)
  // (x stands for transformed)
  std::vector<ID> xidb;

  // methods
public:
  WellfoundedModelGeneratorFactory(
      ProgramCtx& ctx, const ComponentInfo& ci,
      ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig);
  virtual ~WellfoundedModelGeneratorFactory() {}

  virtual ModelGeneratorPtr createModelGenerator(
    InterpretationConstPtr input)
    { return ModelGeneratorPtr(new WellfoundedModelGenerator(*this, input)); }

  virtual std::ostream& print(std::ostream& o) const;
};

DLVHEX_NAMESPACE_END

#endif // WELLFOUNDED_MODEL_GENERATOR_HPP_INCLUDED__09112010
