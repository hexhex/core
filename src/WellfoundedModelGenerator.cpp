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
 * @file WellfoundedModelGenerator.cpp
 * @author Peter Schüller
 *
 * @brief Implementation of the model generator for "Wellfounded" components.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#define DLVHEX_BENCHMARK

#include "dlvhex2/WellfoundedModelGenerator.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/ASPSolver.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/Benchmarking.h"

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

WellfoundedModelGeneratorFactory::WellfoundedModelGeneratorFactory(
    ProgramCtx& ctx,
    const ComponentInfo& ci,
    ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig):
  BaseModelGeneratorFactory(),
  externalEvalConfig(externalEvalConfig),
  ctx(ctx),
  outerEatoms(ci.outerEatoms),
  innerEatoms(ci.innerEatoms),
  idb(),
  xidb()
{
  RegistryPtr reg = ctx.registry();

  // this model generator can handle:
  // components with outer eatoms
  // components with inner eatoms
  // components with inner rules
  // components with inner constraints
  // iff all inner eatoms are monotonic and there are no negative dependencies within idb

  // copy rules and constraints to idb
  // TODO we do not really need this except for debugging (tiny optimization possibility)
  idb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
  idb.insert(idb.end(), ci.innerRules.begin(), ci.innerRules.end());
  idb.insert(idb.end(), ci.innerConstraints.begin(), ci.innerConstraints.end());

  // transform original innerRules and innerConstraints to xidb with only auxiliaries
  xidb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
  std::back_insert_iterator<std::vector<ID> > inserter(xidb);
  std::transform(ci.innerRules.begin(), ci.innerRules.end(),
      inserter, boost::bind(
        &WellfoundedModelGeneratorFactory::convertRule, this, ctx, _1));
  std::transform(ci.innerConstraints.begin(), ci.innerConstraints.end(),
      inserter, boost::bind(
        &WellfoundedModelGeneratorFactory::convertRule, this, ctx, _1));

  // this calls print()
  DBGLOG(DBG,"WellfoundedModelGeneratorFactory(): " << *this);
}

std::ostream& WellfoundedModelGeneratorFactory::print(
    std::ostream& o) const
{
  RawPrinter printer(o, ctx.registry());
  if( !outerEatoms.empty() )
  {
    o << " outer Eatoms={";
    printer.printmany(outerEatoms,"\n");
    o << "}";
  }
  if( !innerEatoms.empty() )
  {
    o << " inner Eatoms={";
    printer.printmany(innerEatoms,"\n");
    o << "}";
  }
  if( !idb.empty() )
  {
    o << " idb={";
    printer.printmany(idb,"\n");
    o << "}";
  }
  if( !xidb.empty() )
  {
    o << " xidb={";
    printer.printmany(xidb,"\n");
    o << "}";
  }
  return o;
}

WellfoundedModelGenerator::WellfoundedModelGenerator(
    Factory& factory,
    InterpretationConstPtr input):
  BaseModelGenerator(input),
  factory(factory)
{
}

WellfoundedModelGenerator::InterprPtr
WellfoundedModelGenerator::generateNextModel()
{
  RegistryPtr reg = factory.ctx.registry();

  if( currentResults == 0 )
  {
    // we need to create currentResults

    // create new interpretation as copy
    Interpretation::Ptr postprocessedInput;
    if( input == 0 )
    {
      // empty construction
      postprocessedInput.reset(new Interpretation(reg));
    }
    else
    {
      // copy construction
      postprocessedInput.reset(new Interpretation(*input));
    }

    // augment input with edb
    postprocessedInput->add(*factory.ctx.edb);

    // remember which facts we have to remove from each output interpretation
    InterpretationConstPtr mask(new Interpretation(*postprocessedInput));

    // manage outer external atoms
    if( !factory.outerEatoms.empty() )
    {
      // augment input with result of external atom evaluation
      // use newint as input and as output interpretation
      IntegrateExternalAnswerIntoInterpretationCB cb(postprocessedInput);
      evaluateExternalAtoms(factory.ctx, factory.outerEatoms, postprocessedInput, cb);
      DLVHEX_BENCHMARK_REGISTER(sidcountexternalatomcomps,
          "outer eatom computations");
      DLVHEX_BENCHMARK_COUNT(sidcountexternalatomcomps,1);

      assert(!factory.xidb.empty() && "the wellfounded model generator is not required for non-idb components! (use plain)");
    }

    // now we have postprocessed input in postprocessedInput
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidwfsolve, "wellfounded solver loop");

    #warning make wellfounded iteration limit configurable
    unsigned limit = 1000;
    bool inconsistent = false;

    // we store two interpretations "ints" and
    // one "src" integer for the current source interpretation
    //
    // the loop below uses ints[current] as source and stores into ints[1-current]
    // then current = 1 - current
    std::vector<InterpretationPtr> ints(2);
    unsigned current = 0;
    // the following creates a copy! (we need the postprocessedInput later)
    ints[0] = InterpretationPtr(new Interpretation(*postprocessedInput));
    // the following creates a copy!
    ints[1] = InterpretationPtr(new Interpretation(*postprocessedInput));
    do
    {
      InterpretationPtr src = ints[current];
      InterpretationPtr dst = ints[1-current];
      DBGLOG(DBG,"starting loop with source" << *src);
      DBGLOG(DBG,"starting loop with dst" << *dst);

      // evaluate inner external atoms
      IntegrateExternalAnswerIntoInterpretationCB cb(dst);
      evaluateExternalAtoms(factory.ctx, factory.innerEatoms, src, cb);
      DBGLOG(DBG,"after evaluateExternalAtoms: dst is " << *dst);

      // solve program
      {
        // we don't use a mask here!
        // -> we receive all facts
        OrdinaryASPProgram program(reg,
            factory.xidb, dst, factory.ctx.maxint);
        ASPSolverManager mgr;
        ASPSolverManager::ResultsPtr thisres =
          mgr.solve(*factory.externalEvalConfig, program);

        // get answer sets
        AnswerSet::Ptr thisret1 = thisres->getNextAnswerSet();
        if( !thisret1 )
        {
          LOG(DBG,"got no answer set -> inconsistent");
          inconsistent = true;
          break;
        }
        AnswerSet::Ptr thisret2 = thisres->getNextAnswerSet();
        if( thisret2 )
          throw FatalError("got more than one model in Wellfounded model generator -> use other model generator!");

        // cheap exchange -> thisret1 will then be free'd
        dst->getStorage().swap(thisret1->interpretation->getStorage());
        DBGLOG(DBG,"after evaluating ASP: dst is " << *dst);
      }

      // check whether new interpretation is superset of old one
      // break if they are equal (i.e., if the fixpoint is reached)
      // error if new one is smaller (i.e., fixpoint is not allowed)
      // (TODO do this error check, and do it only in debug mode)
      int cmpresult = dst->getStorage().compare(src->getStorage());
      if( cmpresult == 0 )
      {
        DBGLOG(DBG,"reached fixpoint");
        break;
      }

      // switch interpretations
      current = 1 - current;
      limit--;
      // loop if limit is not reached
    }
    while( limit != 0 && !inconsistent );

    if( limit == 0 )
      throw FatalError("reached wellfounded limit!");

    if( inconsistent )
    {
      DBGLOG(DBG,"leaving loop with result 'inconsistent'");
      currentResults.reset(new PreparedResults);
    }
    else
    {
      // does not matter which one we take, they are equal
      InterpretationPtr result = ints[0];
      DBGLOG(DBG,"leaving loop with result " << *result);

      // remove mask from result!
      result->getStorage() -= mask->getStorage();
      DBGLOG(DBG,"after removing input facts: result is " << *result);

      // store as single answer set (there can only be one)
      PreparedResults* pr = new PreparedResults;
      currentResults.reset(pr);
      pr->add(AnswerSetPtr(new AnswerSet(result)));
    }
  }

  assert(currentResults != 0);
  AnswerSet::Ptr ret = currentResults->getNextAnswerSet();
  if( ret == 0 )
  {
    currentResults.reset();
    return InterpretationPtr();
  }
  DLVHEX_BENCHMARK_REGISTER(sidcountwfanswersets,
      "WellFoundedMG answer sets");
  DLVHEX_BENCHMARK_COUNT(sidcountwfanswersets,1);

  return ret->interpretation;
}

DLVHEX_NAMESPACE_END
