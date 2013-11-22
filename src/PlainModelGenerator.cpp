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
 * @file PlainModelGenerator.cpp
 * @author Peter Schüller
 *
 * @brief Implementation of the model generator for "Plain" components.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#define DLVHEX_BENCHMARK

#include "dlvhex2/PlainModelGenerator.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/ASPSolver.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/Benchmarking.h"

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

PlainModelGeneratorFactory::PlainModelGeneratorFactory(
    ProgramCtx& ctx,
    const ComponentInfo& ci,
    ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig):
  BaseModelGeneratorFactory(),
  externalEvalConfig(externalEvalConfig),
  ctx(ctx),
  eatoms(ci.outerEatoms),
  idb(),
  xidb()
{
  RegistryPtr reg = ctx.registry();

  // this model generator can handle:
  // components with outer eatoms
  // components with inner rules
  // components with inner constraints
  // this model generator CANNOT handle:
  // components with inner eatoms

  assert(ci.innerEatoms.empty());

  // copy rules and constraints to idb
  // TODO we do not need this except for debugging
  idb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
  idb.insert(idb.end(), ci.innerRules.begin(), ci.innerRules.end());
  idb.insert(idb.end(), ci.innerConstraints.begin(), ci.innerConstraints.end());

  // transform original innerRules and innerConstraints
  // to xidb with only auxiliaries
  xidb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
  std::back_insert_iterator<std::vector<ID> > inserter(xidb);
  std::transform(ci.innerRules.begin(), ci.innerRules.end(),
      inserter, boost::bind(
        &PlainModelGeneratorFactory::convertRule, this, ctx, _1));
  std::transform(ci.innerConstraints.begin(), ci.innerConstraints.end(),
      inserter, boost::bind(
        &PlainModelGeneratorFactory::convertRule, this, ctx, _1));

  #ifndef NDEBUG
  {
    {
      std::ostringstream s;
      RawPrinter printer(s,ctx.registry());
      printer.printmany(idb," ");
      DBGLOG(DBG,"PlainModelGeneratorFactory got idb " << s.str());
    }
    {
      std::ostringstream s;
      RawPrinter printer(s,ctx.registry());
      printer.printmany(xidb," ");
      DBGLOG(DBG,"PlainModelGeneratorFactory got xidb " << s.str());
    }
  }
  #endif
}

std::ostream& PlainModelGeneratorFactory::print(
    std::ostream& o) const
{
  RawPrinter printer(o, ctx.registry());
	o << "outer eatoms:" << std::endl;
  if( !eatoms.empty() )
  {
    printer.printmany(eatoms,"\n");
  }
	o << "xidb:" << std::endl;
  if( !xidb.empty() )
  {
    printer.printmany(xidb,"\n");
  }
  return o;
}

PlainModelGenerator::PlainModelGenerator(
    Factory& factory,
    InterpretationConstPtr input):
  BaseModelGenerator(input),
  factory(factory)
{
}

PlainModelGenerator::InterpretationPtr
PlainModelGenerator::generateNextModel()
{
  RegistryPtr reg = factory.ctx.registry();
  if( currentResults == 0 )
  {
    do // breakout possibility
    {
      // we need to create currentResults

      // create new interpretation as copy
      Interpretation::Ptr newint;
      if( input == 0 )
      {
        // empty construction
        newint.reset(new Interpretation(reg));
      }
      else
      {
        // copy construction
        newint.reset(new Interpretation(*input));
      }

      // augment input with edb
      newint->add(*factory.ctx.edb);

      // remember facts so far (we have to remove these from any output)
      InterpretationConstPtr mask(new Interpretation(*newint));

      // manage outer external atoms
      if( !factory.eatoms.empty() )
      {
        // augment input with result of external atom evaluation
        // use newint as input and as output interpretation
        IntegrateExternalAnswerIntoInterpretationCB cb(newint);
        evaluateExternalAtoms(factory.ctx, factory.eatoms, newint, cb);
        DLVHEX_BENCHMARK_REGISTER(sidcountexternalanswersets,
            "outer eatom computations");
        DLVHEX_BENCHMARK_COUNT(sidcountexternalanswersets,1);

        if( factory.xidb.empty() )
        {
          // we only have eatoms -> return singular result

          // remove EDB and direct input from newint
          // (keep local models as small as possible)
          newint->getStorage() -= mask->getStorage();

          PreparedResults* pr = new PreparedResults;
          currentResults.reset(pr);
          pr->add(AnswerSetPtr(new AnswerSet(newint)));
          break;
        }
      }

      // store in model generator and store as const
      postprocessedInput = newint;

      DLVHEX_BENCHMARK_REGISTER_AND_START(sidaspsolve,
          "initiating external solver");
      OrdinaryASPProgram program(reg,
          factory.xidb, postprocessedInput, factory.ctx.maxint, mask);
      ASPSolverManager mgr;
      currentResults = mgr.solve(*factory.externalEvalConfig, program);
      DLVHEX_BENCHMARK_STOP(sidaspsolve);
    }
    while(false); // end of breakout possibility
  }

  assert(currentResults != 0);
  AnswerSet::Ptr ret = currentResults->getNextAnswerSet();
  if( ret == 0 )
  {
    currentResults.reset();
    // the following is just for freeing memory early
    postprocessedInput.reset();
    return InterpretationPtr();
  }
  DLVHEX_BENCHMARK_REGISTER(sidcountplainanswersets, "PlainMG answer sets");
  DLVHEX_BENCHMARK_COUNT(sidcountplainanswersets,1);

  return ret->interpretation;
}

DLVHEX_NAMESPACE_END
