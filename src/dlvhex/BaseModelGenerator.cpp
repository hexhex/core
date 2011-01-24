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
 * @file BaseModelGenerator.cpp
 * @author Peter Schüller
 *
 * @brief Implementation of common model generator functionalities.
 */

#include "dlvhex/BaseModelGenerator.hpp"
#include "dlvhex/Logger.hpp"
#include "dlvhex/Registry.hpp"
#include "dlvhex/Printer.hpp"
#include "dlvhex/ASPSolver.h"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/PluginInterface.h"
#include "dlvhex/Benchmarking.h"

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

// projects input interpretation
// calls eatom function
// reintegrates output tuples as auxiliary atoms into outputi
// (inputi and outputi may point to the same interpretation)
void BaseModelGenerator::evaluateExternalAtom(RegistryPtr reg,
  const ExternalAtom& eatom,
  InterpretationConstPtr inputi, InterpretationPtr outputi) const
{
  LOG_SCOPE(PLUGIN,"eEA",true);
  DBGLOG(DBG,"= evaluateExternalAtom for " << eatom <<
      " with input interpretation " << *inputi);
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sideea,"evaluate external atom");
	DLVHEX_BENCHMARK_REGISTER(sidier,"integrate external results");

  // build input interpretation
  // for each input tuple (multiple auxiliary inputs possible)
  //   build query
  //   call retrieve
  //   integrate answer into interpretation i as additional facts

  // lock weak pointer
  assert(!eatom.pluginAtom.expired());
  PluginAtomPtr pluginAtom(eatom.pluginAtom);

  // project interpretation for predicate inputs
  InterpretationConstPtr eatominp =
    projectEAtomInputInterpretation(reg, eatom, inputi);
  LOG(DBG,"projected eatom input interpretation = " << *eatominp);

  // build input tuples
  std::list<Tuple> inputs;
  buildEAtomInputTuples(reg, eatom, inputi, inputs);
  #ifndef NDEBUG
  {
    DBGLOG(DBG,"eatom input tuples:");
    DBGLOG_INDENT(DBG);
    BOOST_FOREACH(const Tuple& t, inputs)
    {
      std::stringstream s;
      RawPrinter printer(s, reg);
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
    LOG(PLUGIN,"got " << answer.get().size() << " answer tuples" <<
        " from querying " << eatom.predicate <<
        " with input tuple " << printrange(inputtuple));

    DLVHEX_BENCHMARK_START(sidier);
    // integrate result into interpretation
    BOOST_FOREACH(const Tuple& t, answer.get())
    {
      if( !verifyEAtomAnswerTuple(reg, eatom, t) )
      {
        LOG(WARNING,"external atom " << eatom << " returned tuple " <<
            printrange(t) << " which does not match output pattern (skipping)");
        continue;
      }

      // create replacement atom for each tuple
      OrdinaryAtom replacement(
          ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_ATOM_AUX);

      // tuple: (replacement_predicate, inputs_as_in_inputtuple*, outputs*)
      replacement.tuple.push_back(pluginAtom->getReplacementPredicateID());
      replacement.tuple.insert(replacement.tuple.end(),
          inputtuple.begin(), inputtuple.end());
      replacement.tuple.insert(replacement.tuple.end(),
          t.begin(), t.end());

      // this replacement might already exists
      LOG(DBG,"integrating external answer tuple " << printrange(t));
      ID idreplacement = reg->storeOrdinaryGAtom(replacement);
      DBGLOG(DBG,"got replacement ID " << idreplacement);
      outputi->setFact(idreplacement.address);
    }
    DLVHEX_BENCHMARK_STOP(sidier);

    DBGLOG(DBG,"output interpretation is now " << *outputi);
  } // go over all input tuples of this eatom
  DBGLOG(DBG,"interpretation after all input tuples is " << *outputi);
}

// calls evaluateExternalAtom for each atom in eatoms
void BaseModelGenerator::evaluateExternalAtoms(RegistryPtr reg,
  const std::vector<ID>& eatoms,
  InterpretationConstPtr inputi, InterpretationPtr outputi) const
{
  BOOST_FOREACH(ID eatomid, eatoms)
  {
    const ExternalAtom& eatom = reg->eatoms.getByID(eatomid);
    evaluateExternalAtom(reg, eatom, inputi, outputi);
  }
}

// returns false iff tuple does not unify with eatom output pattern
// (the caller must decide whether to throw an exception or ignore the tuple)
bool BaseModelGenerator::verifyEAtomAnswerTuple(RegistryPtr reg,
  const ExternalAtom& eatom, const Tuple& t) const
{
  return true;

    #warning TODO verify answer tuple! (as done in dlvhex trunk using std::mismatch)
    #if 0
    // check answer tuple, if it corresponds to pattern
    this is the respective code


    /**
     * @brief Check the answers returned from the external atom, and
     * remove ill-formed tuples.
     *
     * Check whether the answers in the output list are
     * (1) ground
     * (2) conform to the output pattern, i.e.,
     *     &rdf[uri](S,rdf:subClassOf,O) shall only return tuples of form
     *     <s, rdf:subClassOf, o>, and not for instance <s,
     *     rdf:subPropertyOf, o>, we have to filter them out (do we?)
     */
    struct CheckOutput
      : public std::binary_function<const Term, const Term, bool>
    {
      bool
      operator() (const Term& t1, const Term& t2) const
      {
        // answers must be ground, otw. programming error in the plugin
        assert(t1.isInt() || t1.isString() || t1.isSymbol());

        // pattern tuple values must coincide
        if (t2.isInt() || t2.isString() || t2.isSymbol())
          {
      return t1 == t2;
          }
        else // t2.isVariable() -> t1 is a variable binding for t2
          {
      return true;
          }
      }
    };


    for (std::vector<Tuple>::const_iterator s = answers->begin(); s != answers->end(); ++s)
    {
      if (s->size() != externalAtom->getArguments().size())
        {
          throw PluginError("External atom " + externalAtom->getFunctionName() + " returned tuple of incompatible size.");
        }

      // check if this answer from pluginatom conforms to the external atom's arguments
      std::pair<Tuple::const_iterator,Tuple::const_iterator> mismatched =
        std::mismatch(s->begin(),
          s->end(),
          externalAtom->getArguments().begin(),
          CheckOutput()
          );

      if (mismatched.first == s->end()) // no mismatch found -> add this tuple to the result
        {
          // the replacement atom contains both the input and the output list!
          // (*inputi must be ground here, since it comes from
          // groundInputList(i, inputArguments))
          Tuple resultTuple(*inputi);

          // add output list
          resultTuple.insert(resultTuple.end(), s->begin(), s->end());

          // setup new atom with appropriate replacement name
          AtomPtr ap(new Atom(externalAtom->getReplacementName(), resultTuple));

          result.insert(ap);
        }
      else
        {
          // found a mismatch, ignore this answer tuple
        }
    }
    #endif
}

InterpretationPtr BaseModelGenerator::projectEAtomInputInterpretation(RegistryPtr reg,
  const ExternalAtom& eatom, InterpretationConstPtr full) const
{
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"BaseModelGen::projectEAII");
  eatom.updatePredicateInputMask();
  InterpretationPtr ret;
  if( full == 0 )
    ret.reset(new Interpretation(reg));
  else
    ret.reset(new Interpretation(*full));
  ret->getStorage() &= eatom.getPredicateInputMask()->getStorage();
  return ret;
}

void BaseModelGenerator::buildEAtomInputTuples(RegistryPtr reg,
  const ExternalAtom& eatom,
  InterpretationConstPtr i,
  std::list<Tuple>& inputs) const
{
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"BaseModelGen::buildEAIT");
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
  DBGLOG(DBG,"matching aux input predicate " << eatom.auxInputPredicate <<
      ", original eatom.inputs = " << printrange(eatom.inputs));
  dlvhex::OrdinaryAtomTable::PredicateIterator it, it_end;
  assert(reg != 0);
  for(boost::tie(it, it_end) =
      reg->ogatoms.getRangeByPredicateID(eatom.auxInputPredicate);
      it != it_end; ++it)
  {
    const dlvhex::OrdinaryAtom& oatom = *it;
    #warning perhaps this could be made more efficient by storing back the id into oatom or by creating ogatoms.getIDRangeByPredicateID with some projecting adapter to PredicateIterator
    ID idoatom = reg->ogatoms.getIDByStorage(oatom);
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

// get rule
// rewrite all eatoms in body to auxiliary replacement atoms
// store and return id
ID BaseModelGeneratorFactory::convertRule(RegistryPtr reg, ID ruleid)
{
  if( !ruleid.doesRuleContainExtatoms() )
    return ruleid;

  // we need to rewrite
  const Rule& rule = reg->rules.getByID(ruleid);
  #ifndef NDEBUG
  {
    std::stringstream s;
    RawPrinter printer(s, reg);
    printer.print(ruleid);
    DBGLOG(DBG,"rewriting rule " << s.str() << " from " << rule <<
        " with id " << ruleid << " to auxiliary predicates");
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
    const ExternalAtom& eatom = reg->eatoms.getByID(
        ID::atomFromLiteral(*itlit));
    DBGLOG(DBG,"rewriting external atom " << eatom <<
        " literal with id " << *itlit);

    // lock weak pointer
    assert(!eatom.pluginAtom.expired());
    PluginAtomPtr pluginAtom(eatom.pluginAtom);

    // create replacement atom
    OrdinaryAtom replacement(ID::MAINKIND_ATOM | ID::PROPERTY_ATOM_AUX);
    replacement.tuple.push_back(pluginAtom->getReplacementPredicateID());
    replacement.tuple.insert(replacement.tuple.end(),
        eatom.inputs.begin(), eatom.inputs.end());
    replacement.tuple.insert(replacement.tuple.end(),
        eatom.tuple.begin(), eatom.tuple.end());

    // bit trick: replacement is ground so far, by setting one bit we make it nonground
    bool ground = true;
    BOOST_FOREACH(ID term, replacement.tuple)
    {
      if( term.isVariableTerm() )
        ground = false;
    }
    if( !ground )
      replacement.kind |= ID::SUBKIND_ATOM_ORDINARYN;

    ID idreplacement;
    if( ground )
      idreplacement = reg->storeOrdinaryGAtom(replacement);
    else
      idreplacement = reg->storeOrdinaryNAtom(replacement);
    DBGLOG(DBG,"storing replacement atom " << idreplacement << " as literal");
    *itlit = ID::literalFromAtom(idreplacement, naf);
  }

  ID newruleid = reg->rules.storeAndGetID(newrule);
  #ifndef NDEBUG
  {
    std::stringstream s;
    RawPrinter printer(s, reg);
    printer.print(newruleid);
    DBGLOG(DBG,"rewritten rule " << s.str() << " from " << newrule <<
        " got id " << newruleid);
  }
  #endif
  return newruleid;
}

DLVHEX_NAMESPACE_END
