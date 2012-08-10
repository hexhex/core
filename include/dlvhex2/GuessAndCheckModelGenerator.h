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
 * @file   GuessAndCheckModelGenerator.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Model generator for eval units that do not allow a fixpoint calculation.
 *
 * Those units may be of any form.
 */

#ifndef GUESSANDCHECK_MODEL_GENERATOR_HPP_INCLUDED__09112010
#define GUESSANDCHECK_MODEL_GENERATOR_HPP_INCLUDED__09112010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/FLPModelGeneratorBase.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/ComponentGraph.h"
#include "dlvhex2/PredicateMask.h"

#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

class GuessAndCheckModelGeneratorFactory:
  public FLPModelGeneratorFactoryBase,
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

  ComponentInfo ci;  // should be a reference, but there is currently a bug in the copy constructor of ComponentGraph: it seems that the component info is shared between different copies of a component graph, hence it is deallocated when one of the copies dies.
  #warning TODO see comment above about ComponentInfo copy construction bug

  // outer external atoms
  std::vector<ID> outerEatoms;

  // methods
public:
  GuessAndCheckModelGeneratorFactory(
      ProgramCtx& ctx, const ComponentInfo& ci,
      ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig);
  virtual ~GuessAndCheckModelGeneratorFactory() {}

  virtual ModelGeneratorPtr createModelGenerator(InterpretationConstPtr input);

  virtual std::ostream& print(std::ostream& o) const;
  virtual std::ostream& print(std::ostream& o, bool verbose) const;
};

class GuessAndCheckModelGenerator:
  public FLPModelGeneratorBase,
  public ostream_printable<GuessAndCheckModelGenerator>
{
  // types
public:
  typedef GuessAndCheckModelGeneratorFactory Factory;

  // storage
protected:
  // we store the factory again, because the base class stores it with the base type only!
  Factory& factory;

  // edb + original (input) interpretation plus auxiliary atoms for evaluated external atoms
  InterpretationConstPtr postprocessedInput;
  // non-external fact input, i.e., postprocessedInput before evaluating outer eatoms
  InterpretationPtr mask;

  // result handle for retrieving edb+xidb+gidb guesses of this eval unit
  ASPSolverManager::ResultsPtr guessres;

  // members
public:
  GuessAndCheckModelGenerator(Factory& factory, InterpretationConstPtr input);
  virtual ~GuessAndCheckModelGenerator() {}

  // generate and return next model, return null after last model
  virtual InterpretationPtr generateNextModel();
};

DLVHEX_NAMESPACE_END

#endif // GUESSANDCHECK_MODEL_GENERATOR_HPP_INCLUDED__09112010
