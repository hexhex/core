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
#include "dlvhex/PluginInterface.h"

#include <boost/foreach.hpp>

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
      // empty construction
      newint.reset(new Interpretation(factory.ctx.registry));
    }
    else
    {
      // copy construction
      newint.reset(new Interpretation(*input));
    }

    // augment input with edb
    newint->add(*factory.ctx.edb);

    // manage outer external atoms
    if( !factory.eatoms.empty() )
    {
      // augment input with result of external atom evaluation
      // use newint as input and as output interpretation
      evaluateExternalAtoms(newint);
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

void FinalModelGenerator::evaluateExternalAtoms(InterpretationPtr i) const
{
  LOG_SCOPE("eEA",false);
  LOG("= evaluateExternalAtoms with interpretation " << *i);

  // for each external atom in factory.eatoms:
  //   build input interpretation
  //   for each input tuple (multiple auxiliary inputs possible)
  //     build query
  //     call retrieve
  //     integrate answer into interpretation i as additional facts

  for(std::vector<ID>::const_iterator ite = factory.eatoms.begin();
      ite != factory.eatoms.end(); ++ite)
  {
    const ExternalAtom& eatom = factory.ctx.registry->eatoms.getByID(*ite);

    // lock weak pointer
    assert(!eatom.pluginAtom.expired());
    PluginAtomPtr pluginAtom(eatom.pluginAtom);

    // project interpretation for predicate inputs
    InterpretationConstPtr eatominp =
      projectEAtomInputInterpretation(eatom, i);
    LOG("projected eatom input interpretation = " << *eatominp);

    // build input tuples
    std::list<Tuple> inputs;
    buildEAtomInputTuples(eatom, i, inputs);
    #ifndef NDEBUG
    LOG("eatom input tuples:");
    LOG_INDENT();
    BOOST_FOREACH(const Tuple& t, inputs)
    {
      std::stringstream s;
      RawPrinter printer(s, factory.ctx.registry);
      s << "[";
      printer.printmany(t,",");
      s << "]";
      LOG(s.str());
    }
    #endif

    BOOST_FOREACH(const Tuple& inputtuple, inputs)
    {
      // query
      PluginAtom::Query query(eatominp, inputtuple, eatom.tuple);
      PluginAtom::Answer answer;
      LOG("querying external atom &" << eatom.predicate << "!");
      pluginAtom->retrieveCached(query, answer);

      // integrate result into interpretation
      BOOST_FOREACH(const Tuple& t, answer.get())
      {
        std::stringstream s;
        RawPrinter printer(s, factory.ctx.registry);
        s << "[";
        printer.printmany(t,",");
        s << "]";
        LOG("TODO integrate answer tuple " << s.str() << " <=> " << printrange(t));
      }
    }
  }
}

InterpretationPtr FinalModelGenerator::projectEAtomInputInterpretation(
  const ExternalAtom& eatom, InterpretationConstPtr full) const
{
  eatom.updatePredicateInputMask();
  InterpretationPtr ret;
  if( full == 0 )
    ret.reset(new Interpretation(factory.ctx.registry));
  else
    ret.reset(new Interpretation(*full));
  ret->getStorage() &= eatom.getPredicateInputMask()->getStorage();
  return ret;
}

void FinalModelGenerator::buildEAtomInputTuples(
  const ExternalAtom& eatom,
  InterpretationConstPtr i,
  std::list<Tuple>& inputs) const
{
  LOG_SCOPE("bEAIT", false);
  LOG("= buildEAtomInputTuples " << eatom);

  // if there are no variables, there is no aux input predicate and only one input tuple
  if( eatom.auxInputPredicate == ID_FAIL )
  {
    LOG("no auxiliary input predicate -> "
        " returning single unchanged eatom.inputs " <<
        printrange(eatom.inputs));
    inputs.push_back(eatom.inputs);
    return;
  }

  // otherwise we have to calculate a bit, using the aux input predicate
  LOG("matching aux input predicate, original eatom.inputs = " << printrange(eatom.inputs));
  dlvhex::OrdinaryAtomTable::PredicateIterator it, it_end;
  assert(factory.ctx.registry != 0);
  for(boost::tie(it, it_end) =
      factory.ctx.registry->ogatoms.getRangeByPredicateID(eatom.auxInputPredicate);
      it != it_end; ++it)
  {
    const dlvhex::OrdinaryAtom& oatom = *it;
    #warning perhaps this could be made more efficient by storing back the id into oatom or by creating ogatoms.getIDRangeByPredicateID with some projecting adapter to PredicateIterator
    ID idoatom = factory.ctx.registry->ogatoms.getIDByStorage(oatom);
    if( i->getFact(idoatom.address) )
    {
      // add copy of original input tuple
      inputs.push_back(eatom.inputs);

      // modify this copy
      Tuple& inp = inputs.back();

      // replace all occurances of variables with the corresponding predicates in auxinput
      for(unsigned idx = 0; idx < eatom.auxInputMapping.size(); ++idx)
      {
        // idx is the index of the argument to the auxiliary predicate
        // at 0 there is the auxiliary predicate
        ID replaceBy = oatom.tuple[idx+1];
        // replaceBy is the ground term we will use instead of the input constant variable
        for(std::list<unsigned>::const_iterator it = eatom.auxInputMapping[idx].begin();
            it != eatom.auxInputMapping[idx].end(); ++it)
        {
          // *it is the index of the input term that is a variable
          assert(inp[*it].isTerm() && inp[*it].isVariableTerm());
          inp[*it] = replaceBy;
        }
      }
      LOG("after inserting auxiliary predicate inputs: input = " << printrange(inp));
    }
  }
}

DLVHEX_NAMESPACE_END
