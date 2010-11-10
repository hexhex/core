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
 * @file FinalModelGenerator.cpp
 * @author Peter Schüller
 *
 * @brief Implementation of the final model generator (preliminary).
 */

#include "dlvhex/FinalModelGenerator.hpp"
#include "dlvhex/Logger.hpp"
#include "dlvhex/ASPSolver.h"
#include "dlvhex/ProgramCtx.h"

DLVHEX_NAMESPACE_BEGIN

FinalModelGeneratorFactory::FinalModelGeneratorFactory(
    ProgramCtx& ctx,
    const ComponentInfo& ci):
  ctx(ctx),
  eatoms(ci.outerEatoms),
  idb()
{
  // this model generator can handle:
  // components with outer eatoms
  // components with inner rules
  // components with inner constraints
  // this model generator CANNOT handle:
  // components with inner eatoms

  assert(ci.innerEatoms.empty());
  // TODO: manage auxiliaries for eatoms in idb
  idb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
  // copy rules and constraints to idb
  idb.insert(idb.end(), ci.innerRules.begin(), ci.innerRules.end());
  idb.insert(idb.end(), ci.innerConstraints.begin(), ci.innerConstraints.end());
}

FinalModelGenerator::FinalModelGenerator(
    Factory& factory,
    InterpretationConstPtr input):
  ModelGeneratorBase<Interpretation>(input),
  factory(factory)
{
}

FinalModelGenerator::InterpretationPtr
FinalModelGenerator::generateNextModel()
{
  if( currentResults == 0 )
  {
    // we need to create currentResults

    // create new interpretation as copy
    Interpretation::Ptr newint;
    if( input == 0 )
    {
      newint.reset(new Interpretation(factory.ctx.registry));
    }
    else
    {
      newint.reset(new Interpretation(*input));
    }

    // augment input with edb
    newint->add(*factory.ctx.edb);

    // manage outer external atoms
    if( !factory.eatoms.empty() )
    {
      // augment input to get externallyAugmentedInput
      throw std::runtime_error("TODO: augment input");
    }

    // store in model generator and store as const
    postprocessedInput = newint;

    ASPSolver::DLVSoftware::Configuration dlvConfiguration;
    ASPProgram program(factory.ctx.registry, factory.idb, postprocessedInput, factory.ctx.maxint);
    ASPSolverManager mgr;
    currentResults = mgr.solve(dlvConfiguration, program);
  }

  assert(currentResults != 0);
  AnswerSet::Ptr ret = currentResults->getNextAnswerSet();
  if( ret == 0 )
  {
    currentResults.reset();
    // the following is just for freeing memory
    postprocessedInput.reset();
  }

  return ret->interpretation;
}

DLVHEX_NAMESPACE_END
