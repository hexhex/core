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
 * @file   PlainModelGenerator.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Model generator for the "Plain" type of components.
 */

#ifndef PLAIN_MODEL_GENERATOR_HPP_INCLUDED__09112010
#define PLAIN_MODEL_GENERATOR_HPP_INCLUDED__09112010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/BaseModelGenerator.h"
#include "dlvhex2/ComponentGraph.h"

DLVHEX_NAMESPACE_BEGIN

class PlainModelGeneratorFactory;

//
// A model generator does the following:
// * it is constructed by a ModelGeneratorFactory which knows the program
//   (and can precompute information for evaluation,
//    and may also provide this to the model generator)
// * it is evaluated on a (probably empty) input interpretation
// * this evaluation can be performed online
// * evaluation yields a (probably empty) set of output interpretations
//
class PlainModelGenerator:
  public BaseModelGenerator,
  public ostream_printable<PlainModelGenerator>
{
  // types
public:
  typedef PlainModelGeneratorFactory Factory;

  // storage
protected:
  Factory& factory;

  // edb + original (input) interpretation plus auxiliary atoms for evaluated external atoms
  InterpretationConstPtr postprocessedInput;
  // result handle for asp solver evaluation, using externallyAugmentedInput
  ASPSolverManager::ResultsPtr currentResults;

  // members
public:
  PlainModelGenerator(Factory& factory, InterpretationConstPtr input);
  virtual ~PlainModelGenerator() {}

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
class PlainModelGeneratorFactory:
  public BaseModelGeneratorFactory,
  public ostream_printable<PlainModelGeneratorFactory>
{
  // types
public:
  friend class PlainModelGenerator;
  typedef ComponentGraph::ComponentInfo ComponentInfo;

  // storage
protected:
  // which solver shall be used for external evaluation?
  ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig;
  ProgramCtx& ctx;
  std::vector<ID> eatoms;
  // original idb (containing eatoms where all inputs are known
  // -> auxiliary input rules of these eatoms must be in predecessor unit!)
  std::vector<ID> idb;
  // rewritten idb (containing replacements for eatoms)
  // (x stands for transformed)
  std::vector<ID> xidb;

  // methods
public:
  PlainModelGeneratorFactory(
      ProgramCtx& ctx, const ComponentInfo& ci,
      ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig);
  virtual ~PlainModelGeneratorFactory() {}

  virtual ModelGeneratorPtr createModelGenerator(
    InterpretationConstPtr input)
    { return ModelGeneratorPtr(new PlainModelGenerator(*this, input)); }

  virtual std::ostream& print(std::ostream& o) const;
};

DLVHEX_NAMESPACE_END

#endif // PLAIN_MODEL_GENERATOR_HPP_INCLUDED__09112010
