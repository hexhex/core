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
 * @file   GuessAndCheckModelGenerator.hpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Model generator for eval units that do not allow a fixpoint calculation.
 *
 * Those units may be of any form.
 */

#ifndef GUESSANDCHECK_MODEL_GENERATOR_HPP_INCLUDED__09112010
#define GUESSANDCHECK_MODEL_GENERATOR_HPP_INCLUDED__09112010

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/fwd.hpp"
#include "dlvhex/ID.hpp"
#include "dlvhex/BaseModelGenerator.hpp"
#include "dlvhex/ComponentGraph.hpp"
#include "dlvhex/PredicateMask.hpp"

#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

class GuessAndCheckModelGeneratorFactory;

class GuessAndCheckModelGenerator:
  public BaseModelGenerator,
  public ostream_printable<GuessAndCheckModelGenerator>
{
  // types
public:
  typedef GuessAndCheckModelGeneratorFactory Factory;

  // storage
protected:
  Factory& factory;

  // edb + original (input) interpretation plus auxiliary atoms for evaluated external atoms
  InterpretationConstPtr postprocessedInput;
  // result handle for retrieving set of minimal models of this eval unit
  ASPSolverManager::ResultsPtr currentResults;

  // members
public:
  GuessAndCheckModelGenerator(Factory& factory, InterpretationConstPtr input);
  virtual ~GuessAndCheckModelGenerator() {}

  // generate and return next model, return null after last model
  virtual InterpretationPtr generateNextModel();

  // TODO debug output?
  //virtual std::ostream& print(std::ostream& o) const
  //  { return o << "ModelGeneratorBase::print() not overloaded"; }
};

class GuessAndCheckModelGeneratorFactory:
  public BaseModelGeneratorFactory,
  public ostream_printable<GuessAndCheckModelGeneratorFactory>
{
  // types
public:
  friend class GuessAndCheckModelGenerator;
  typedef ComponentGraph::ComponentInfo ComponentInfo;

  // storage
protected:
  // which solver shall be used for external evaluation?
  ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig;
  ProgramCtx& ctx;

  //
  // see also comments in GuessAndCheckModelGenerator.cpp
  //

  // outer external atoms
  std::vector<ID> outerEatoms;

  // inner external atoms
  std::vector<ID> innerEatoms;
  // one guessing rule for each inner eatom
  // (if one rule contains two inner eatoms, two guessing rules are created)
  std::vector<ID> gidb;

  // original idb (containing eatoms where all inputs are known
  // -> auxiliary input rules of these eatoms must be in predecessor unit!)
  std::vector<ID> idb;
  // idb rewritten with eatom replacement atoms
  std::vector<ID> xidb;
  // xidb rewritten for FLP calculation
  std::vector<ID> xidbflphead;
  std::vector<ID> xidbflpbody;

  // cache: xidb+gidb
  std::vector<ID> xgidb;

  // bitmask for filtering out (positive and negative) guessed eatom replacement predicates
  PredicateMask gpMask;
  PredicateMask gnMask;
  // bitmask for filtering out FLP predicates
  PredicateMask fMask;

  // methods
public:
  GuessAndCheckModelGeneratorFactory(
      ProgramCtx& ctx, const ComponentInfo& ci,
      ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig);
  virtual ~GuessAndCheckModelGeneratorFactory() {}

  virtual ModelGeneratorPtr createModelGenerator(
    InterpretationConstPtr input)
    { return ModelGeneratorPtr(new GuessAndCheckModelGenerator(*this, input)); }

  virtual std::ostream& print(std::ostream& o) const;
  virtual std::ostream& print(std::ostream& o, bool verbose) const;
};

DLVHEX_NAMESPACE_END

#endif // GUESSANDCHECK_MODEL_GENERATOR_HPP_INCLUDED__09112010
