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

#define DLVHEX_BENCHMARK

#include "dlvhex/FinalModelGenerator.hpp"
#include "dlvhex/Logger.hpp"
#include "dlvhex/ASPSolver.h"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/PluginInterface.h"
#include "dlvhex/Benchmarking.h"

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

FinalModelGeneratorFactory::FinalModelGeneratorFactory(
    ProgramCtx& ctx,
    const ComponentInfo& ci,
    ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig):
  externalEvalConfig(externalEvalConfig),
  ctx(ctx),
  eatoms(ci.outerEatoms),
  idb(),
  xidb()
{
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

  // transform original innerRules and innerConstraints to xidb with only auxiliaries
  xidb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
  std::back_insert_iterator<std::vector<ID> > inserter(xidb);
  std::transform(ci.innerRules.begin(), ci.innerRules.end(),
      inserter, boost::bind(&FinalModelGeneratorFactory::convertRule, this, _1));
  std::transform(ci.innerConstraints.begin(), ci.innerConstraints.end(),
      inserter, boost::bind(&FinalModelGeneratorFactory::convertRule, this, _1));

  #ifndef NDEBUG
  {
    {
      std::ostringstream s;
      RawPrinter printer(s,ctx.registry);
      printer.printmany(idb," ");
      DBGLOG(DBG,"FinalModelGeneratorFactory got idb " << s.str());
    }
    {
      std::ostringstream s;
      RawPrinter printer(s,ctx.registry);
      printer.printmany(xidb," ");
      DBGLOG(DBG,"FinalModelGeneratorFactory got xidb " << s.str());
    }
  }
  #endif
}

// get rule
// rewrite all eatoms in body to auxiliary replacement atoms
// store and return id
ID FinalModelGeneratorFactory::convertRule(ID ruleid)
{
  if( !ruleid.doesRuleContainExtatoms() )
    return ruleid;

  // we need to rewrite
  const Rule& rule = ctx.registry->rules.getByID(ruleid);
  #ifndef NDEBUG
  {
    std::stringstream s;
    RawPrinter printer(s, ctx.registry);
    printer.print(ruleid);
    DBGLOG(DBG,"rewriting rule " << s.str() << " from " << rule << " with id " << ruleid << " to auxiliary predicates");
  }
  #endif

  // copy it
  Rule newrule(rule);
  for(Tuple::iterator itlit = newrule.body.begin();
      itlit != newrule.body.end(); ++itlit)
  {
    if( !itlit->isExternalAtom() )
      continue;

    bool naf = itlit->isNaf();
    const ExternalAtom& eatom = ctx.registry->eatoms.getByID(
        ID::atomFromLiteral(*itlit));
    DBGLOG(DBG,"rewriting external atom " << eatom << " literal with id " << *itlit);

    // lock weak pointer
    assert(!eatom.pluginAtom.expired());
    PluginAtomPtr pluginAtom(eatom.pluginAtom);

    // create replacement atom
    OrdinaryAtom replacement(ID::MAINKIND_ATOM | ID::PROPERTY_ATOM_AUX);
    replacement.tuple.push_back(pluginAtom->getReplacementPredicateID());
    replacement.tuple.insert(replacement.tuple.end(), eatom.inputs.begin(), eatom.inputs.end());
    replacement.tuple.insert(replacement.tuple.end(), eatom.tuple.begin(), eatom.tuple.end());

    // bit trick: replacement is ground so far, by setting one bit we make it nonground
    bool ground = true;
    BOOST_FOREACH(ID term, replacement.tuple)
    {
      if( term.isVariableTerm() )
        ground = false;
    }
    if( !ground )
      replacement.kind |= ID::SUBKIND_ATOM_ORDINARYN;

    OrdinaryAtomTable* oat;
    if( ground )
      oat = &ctx.registry->ogatoms;
    else
      oat = &ctx.registry->onatoms;

    // this replacement might already exists
    ID idreplacement = oat->getIDByTuple(replacement.tuple);
    if( idreplacement == ID_FAIL )
    {
      // text
      #warning cache this partially site 1?
      std::stringstream s;
      RawPrinter printer(s, ctx.registry);
      s << pluginAtom->getReplacementPredicate();
      s << "(";
      printer.printmany(eatom.inputs,",");
      if( !eatom.inputs.empty() && !eatom.tuple.empty() )
        s << ",";
      printer.printmany(eatom.tuple,",");
      s << ")";
      replacement.text = s.str();

      idreplacement = oat->storeAndGetID(replacement);
      LOG(PLUGIN,"created new replacement " << replacement << " which got " << idreplacement);
    }
    DBGLOG(DBG," => storing replacement " << idreplacement);
    *itlit = ID::literalFromAtom(idreplacement, naf);
  }

  ID newruleid = ctx.registry->rules.storeAndGetID(newrule);
  #ifndef NDEBUG
  {
    std::stringstream s;
    RawPrinter printer(s, ctx.registry);
    printer.print(newruleid);
    DBGLOG(DBG,"rewritten rule " << s.str() << " from " << newrule << " got id " << newruleid);
  }
  #endif
  return newruleid;
}

std::ostream& FinalModelGeneratorFactory::print(
    std::ostream& o) const
{
  RawPrinter printer(o, ctx.registry);
  if( !eatoms.empty() )
  {
    printer.printmany(eatoms,",");
  }
  if( !xidb.empty() )
  {
    printer.printmany(xidb,",");
  }
  return o;
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

      if( factory.xidb.empty() )
      {
        // we only have eatoms -> return singular result
        currentResults = ASPSolverManager::ResultsPtr(new EmptyResults());
        return newint;
      }
    }

    // store in model generator and store as const
    postprocessedInput = newint;

    DLVHEX_BENCHMARK_REGISTER_AND_START(sidaspsolve,"initiating external solver");
    ASPProgram program(factory.ctx.registry, factory.xidb, postprocessedInput, factory.ctx.maxint);
    ASPSolverManager mgr;
    currentResults = mgr.solve(*factory.externalEvalConfig, program);
    DLVHEX_BENCHMARK_STOP(sidaspsolve);
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
  DLVHEX_BENCHMARK_REGISTER(sidcountexternalanswersets,"external answersets");
  DLVHEX_BENCHMARK_COUNT(sidcountexternalanswersets,1);

  return ret->interpretation;
}

void FinalModelGenerator::evaluateExternalAtoms(InterpretationPtr i) const
{
  LOG_SCOPE(PLUGIN,"eEA",true);
  DBGLOG(DBG,"= evaluateExternalAtoms with interpretation " << *i);
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sideea,"evaluate external atoms");
	DLVHEX_BENCHMARK_REGISTER(sidier,"integrate external results");

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
    LOG(DBG,"projected eatom input interpretation = " << *eatominp);

    // build input tuples
    std::list<Tuple> inputs;
    buildEAtomInputTuples(eatom, i, inputs);
    #ifndef NDEBUG
    {
      DBGLOG(DBG,"eatom input tuples:");
      DBGLOG_INDENT(DBG);
      BOOST_FOREACH(const Tuple& t, inputs)
      {
        std::stringstream s;
        RawPrinter printer(s, factory.ctx.registry);
        s << "[";
        printer.printmany(t,",");
        s << "]";
        DBGLOG(DBG,s.str());
      }
    }
    #endif

    // go over all ground input tuples as grounded by auxiliary inputs rule
    BOOST_FOREACH(const Tuple& inputtuple, inputs)
    {
      // query
      PluginAtom::Query query(eatominp, inputtuple, eatom.tuple);
      PluginAtom::Answer answer;
      pluginAtom->retrieveCached(query, answer);
      LOG(PLUGIN,"got " << answer.get().size() << " answer tuples from querying " << eatom.predicate << " with input tuple " << printrange(inputtuple));

			DLVHEX_BENCHMARK_START(sidier);
      // integrate result into interpretation
      BOOST_FOREACH(const Tuple& t, answer.get())
      {
        // check answer tuple, if it corresponds to pattern
        #warning TODO verify answer tuple! (as done in dlvhex trunk using std::mismatch)

        // create replacement atom for each tuple
        OrdinaryAtom replacement(
            ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_ATOM_AUX);

        // tuple: (replacement_predicate, inputs_as_in_inputtuple*, outputs*)
        replacement.tuple.push_back(pluginAtom->getReplacementPredicateID());
        replacement.tuple.insert(replacement.tuple.end(), inputtuple.begin(), inputtuple.end());
        replacement.tuple.insert(replacement.tuple.end(), t.begin(), t.end());

        // this replacement might already exists
        LOG(DBG,"integrating external answer tuple " << printrange(t));
        ID idreplacement = factory.ctx.registry->ogatoms.getIDByTuple(replacement.tuple);
        if( idreplacement == ID_FAIL )
        {
          // text
          #warning cache this partially site 2 ?
          std::stringstream s;
          RawPrinter printer(s, factory.ctx.registry);
          s << pluginAtom->getReplacementPredicate();
          s << "(";
          printer.printmany(inputtuple,",");
          if( !inputtuple.empty() && !t.empty() )
            s << ",";
          printer.printmany(t,",");
          s << ")";
          replacement.text = s.str();

          DBGLOG(DBG,"integrating " << replacement);
          idreplacement = factory.ctx.registry->ogatoms.storeAndGetID(replacement);
          DBGLOG(DBG,"got ID " << idreplacement);
        }
        i->setFact(idreplacement.address);
      }
			DLVHEX_BENCHMARK_STOP(sidier);

      DBGLOG(DBG,"interpretation is now " << *i);
    } // go over all input tuples of this eatom
    DBGLOG(DBG,"interpretation after all input tuples is " << *i);
  } // go over all eatoms
  DBGLOG(DBG,"interpretation after all eatoms is " << *i);
}

InterpretationPtr FinalModelGenerator::projectEAtomInputInterpretation(
  const ExternalAtom& eatom, InterpretationConstPtr full) const
{
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"FinalModelGen::projectEAII");
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
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"FinalModelGen::buildEAIT");
  LOG_SCOPE(PLUGIN,"bEAIT",false);
  DBGLOG(DBG,"= buildEAtomInputTuples " << eatom);

  // if there are no variables, there is no aux input predicate and only one input tuple
  if( eatom.auxInputPredicate == ID_FAIL )
  {
    DBGLOG(DBG,"no auxiliary input predicate -> "
        " returning single unchanged eatom.inputs " <<
        printrange(eatom.inputs));
    inputs.push_back(eatom.inputs);
    return;
  }

  // otherwise we have to calculate a bit, using the aux input predicate
  DBGLOG(DBG,"matching aux input predicate " << eatom.auxInputPredicate << ", original eatom.inputs = " << printrange(eatom.inputs));
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
      DBGLOG(DBG,"after inserting auxiliary predicate inputs: input = " << printrange(inp));
    }
  }
}

DLVHEX_NAMESPACE_END
