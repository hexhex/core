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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/BaseModelGenerator.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/ASPSolver.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/Atoms.h"
#include "dlvhex2/ExternalLearningHelper.h"

#include <boost/foreach.hpp>

#include <fstream>

dlvhex::ProgramCtx* globalpc;

DLVHEX_NAMESPACE_BEGIN

BaseModelGenerator::ExternalAnswerTupleCallback::
~ExternalAnswerTupleCallback()
{
}

BaseModelGenerator::
IntegrateExternalAnswerIntoInterpretationCB::
IntegrateExternalAnswerIntoInterpretationCB(
    InterpretationPtr outputi):
  outputi(outputi),
  reg(outputi->getRegistry()),
  replacement(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX | ID::PROPERTY_EXTERNALAUX)
{
}

bool
BaseModelGenerator::IntegrateExternalAnswerIntoInterpretationCB::
eatom(const ExternalAtom& eatom)
{
  replacement.tuple.resize(1);
  replacement.tuple[0] = 
    reg->getAuxiliaryConstantSymbol('r', eatom.predicate);

  // never abort
  return true;
}

// remembers input
bool
BaseModelGenerator::IntegrateExternalAnswerIntoInterpretationCB::
input(const Tuple& input)
{
  assert(replacement.tuple.size() >= 1);
  // shorten
  replacement.tuple.resize(1);
  // add
  replacement.tuple.insert(replacement.tuple.end(),
      input.begin(), input.end());

  // never abort
  return true;
}

// creates replacement ogatom and activates respective bit in output interpretation
bool
BaseModelGenerator::IntegrateExternalAnswerIntoInterpretationCB::
output(const Tuple& output)
{
  assert(replacement.tuple.size() >= 1);
  // add, but remember size to reset it later
  unsigned size = replacement.tuple.size();
  replacement.tuple.insert(replacement.tuple.end(),
      output.begin(), output.end());

  // this replacement might already exists
  DBGLOG(DBG,"integrating eatom tuple " << printrange(replacement.tuple));
  ID idreplacement = reg->storeOrdinaryGAtom(replacement);
  DBGLOG(DBG,"got replacement ID " << idreplacement);
  outputi->setFact(idreplacement.address);
  DBGLOG(DBG,"output interpretation is now " << *outputi);

  // shorten it, s.t. we can add the next one
  replacement.tuple.resize(size);

  // never abort
  return true;
}

// projects input interpretation
// calls eatom function
// reintegrates output tuples as auxiliary atoms into outputi
// (inputi and outputi may point to the same interpretation)

bool BaseModelGenerator::evaluateExternalAtom(ProgramCtx& ctx,
  const ExternalAtom& eatom,
  InterpretationConstPtr inputi,
  ExternalAnswerTupleCallback& cb,
  NogoodContainerPtr nogoods) const
{
  LOG_SCOPE(PLUGIN,"eEA",false);
  DBGLOG(DBG,"= evaluateExternalAtom for " << eatom <<
      " with input interpretation " << *inputi);

  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sideea,"evaluate external atom");
	DLVHEX_BENCHMARK_REGISTER(sidier,"integrate external results");

  // build input interpretation
  // for each input tuple (multiple auxiliary inputs possible)
  //   build query
  //   call retrieve
  //   integrate answer into interpretation i as additional facts

  assert(!!eatom.pluginAtom);
  PluginAtom* pluginAtom = eatom.pluginAtom;

  // if this is wrong, we might have mixed up registries between plugin and program
  assert(eatom.predicate == pluginAtom->getPredicateID());

  // project interpretation for predicate inputs
  InterpretationConstPtr eatominp =
    projectEAtomInputInterpretation(ctx.registry(), eatom, inputi);

  // build input tuples
  std::list<Tuple> inputs;
  buildEAtomInputTuples(ctx.registry(), eatom, inputi, inputs);

  if( Logger::Instance().shallPrint(Logger::PLUGIN) )
  {
    LOG(PLUGIN,"eatom input tuples = ");
    LOG_INDENT(PLUGIN);
    BOOST_FOREACH(const Tuple& t, inputs)
    {
      std::stringstream s;
      RawPrinter printer(s, ctx.registry());
      s << "[";
      printer.printmany(t,",");
      s << "] ";
      LOG(PLUGIN,s.str());
    }
    {
      std::stringstream s;
      RawPrinter printer(s, ctx.registry());
      s << "(";
      printer.printmany(eatom.tuple,",");
      s << ")";
      LOG(PLUGIN,"eatom output pattern = " << s.str());
    }
    LOG(PLUGIN,"projected eatom input interpretation = " << *eatominp);
  }

  // call callback and abort if requested
  if( !inputs.empty() )
  {
    if( !cb.eatom(eatom) )
    {
      LOG(DBG,"callback aborted for eatom " << eatom);
      return false;
    }
  }

  // go over all ground input tuples as grounded by auxiliary inputs rule
  BOOST_FOREACH(const Tuple& inputtuple, inputs)
  {
    #ifndef NDEBUG
    std::string sinput;
    {
      std::stringstream s;
      RawPrinter printer(s, ctx.registry());
      s << "[";
      printer.printmany(inputtuple,",");
      s << "]";
      sinput = s.str();
    }
    DBGLOG(DBG,"processing input tuple " << sinput << " = " << printrange(inputtuple));
    #endif

    // query
    PluginAtom::Query query(&ctx, eatominp, inputtuple, eatom.tuple, &eatom);
    query.extinterpretation = inputi;
    PluginAtom::Answer answer;
    assert(globalpc);
    if( globalpc->config.getOption("UseExtAtomCache") ){
      pluginAtom->retrieveCached(query, answer, nogoods);
    }
    else
    {
      DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidr,"PluginAtom retrieve");

      pluginAtom->retrieve(query, answer, nogoods);
    }
    LOG(PLUGIN,"got " << answer.get().size() << " answer tuples");

    if( !answer.get().empty() )
    {
      Tuple it;
      if (ctx.config.getOption("IncludeAuxInputInAuxiliaries") && eatom.auxInputPredicate != ID_FAIL){
        it.push_back(eatom.auxInputPredicate);
      }
      BOOST_FOREACH (ID i, inputtuple) it.push_back(i);
      if( !cb.input(it) )
      {
        LOG(DBG,"callback aborted for input tuple " << printrange(inputtuple));
        return false;
      }
    }

    DLVHEX_BENCHMARK_SCOPE(sidier);

    // integrate result into interpretation
    BOOST_FOREACH(const Tuple& t, answer.get())
    {
      if( Logger::Instance().shallPrint(Logger::PLUGIN) )
      {
        std::stringstream s;
        RawPrinter printer(s, ctx.registry());
        s << "(";
        printer.printmany(t,",");
        s << ")";
        LOG(PLUGIN,"got answer tuple " << s.str());
      }
      if( !verifyEAtomAnswerTuple(ctx.registry(), eatom, t) )
      {
        LOG(WARNING,"external atom " << eatom << " returned tuple " <<
            printrange(t) << " which does not match output pattern (skipping)");
        continue;
      }

      // call callback and abort if requested
      if( !cb.output(t) )
      {
        LOG(DBG,"callback aborted for output tuple " << printrange(t));
        return false;
      }
    }
  } // go over all input tuples of this eatom

  return true;
}

// calls evaluateExternalAtom for each atom in eatoms

bool BaseModelGenerator::evaluateExternalAtoms(ProgramCtx& ctx,
  const std::vector<ID>& eatoms,
  InterpretationConstPtr inputi,
  ExternalAnswerTupleCallback& cb,
  NogoodContainerPtr nogoods) const
{
  BOOST_FOREACH(ID eatomid, eatoms)
  {
    const ExternalAtom& eatom = ctx.registry()->eatoms.getByID(eatomid);
    if( !evaluateExternalAtom(ctx, eatom, inputi, cb, nogoods) )
    {
      LOG(DBG,"callbacks aborted evaluateExternalAtoms");
      return false;
    }
  }
  return true;
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

// rewrite all eatoms in body tuple to auxiliary replacement atoms
// store new body into convbody
// (works recursively for aggregate atoms,
// will create additional "auxiliary" aggregate atoms in registry)
void BaseModelGeneratorFactory::convertRuleBody(
    ProgramCtx& ctx, const Tuple& body, Tuple& convbody)
{
  assert(convbody.empty());
  RegistryPtr reg = ctx.registry();
  for(Tuple::const_iterator itlit = body.begin();
      itlit != body.end(); ++itlit)
  {
    if( itlit->isAggregateAtom() )
    {
      // recursively treat aggregates
      
      // findout if aggregate contains external atoms
      const AggregateAtom& aatom = reg->aatoms.getByID(*itlit);
      AggregateAtom convaatom(aatom);
      convaatom.literals.clear();
      convertRuleBody(ctx, aatom.literals, convaatom.literals);
      if( convaatom.literals != aatom.literals )
      {
        // really create new aggregate atom
        convaatom.kind |= ID::PROPERTY_AUX;
        ID newaatomid = reg->aatoms.storeAndGetID(convaatom);
        convbody.push_back(newaatomid);
      }
      else
      {
        // use original aggregate atom
        convbody.push_back(*itlit);
      }
    }
    else if( itlit->isExternalAtom() )
    {
      bool naf = itlit->isNaf();
      const ExternalAtom& eatom = reg->eatoms.getByID(
          ID::atomFromLiteral(*itlit));
      DBGLOG(DBG,"rewriting external atom " << eatom <<
          " literal with id " << *itlit);

      assert(!!eatom.pluginAtom);
      PluginAtom* pluginAtom = eatom.pluginAtom;

      // create replacement atom
      OrdinaryAtom replacement(ID::MAINKIND_ATOM | ID::PROPERTY_AUX | ID::PROPERTY_EXTERNALAUX);
      replacement.tuple.push_back(
          reg->getAuxiliaryConstantSymbol('r',
            pluginAtom->getPredicateID()));
      if (ctx.config.getOption("IncludeAuxInputInAuxiliaries") && eatom.auxInputPredicate != ID_FAIL){
        replacement.tuple.push_back(eatom.auxInputPredicate);
      }
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
      DBGLOG(DBG,"adding replacement atom " << idreplacement << " as literal");
      convbody.push_back(ID::literalFromAtom(idreplacement, naf));
    }
    else
    {
      DBGLOG(DBG,"adding original literal " << *itlit);
      convbody.push_back(*itlit);
    }
  }
}

// adds for all external atoms with output variables which fail the strong safety check
// a domain predicate to the rule body
ID BaseModelGeneratorFactory::addDomainPredicatesWhereNecessary(const ComponentGraph::ComponentInfo& ci, RegistryPtr reg, ID ruleid)
{
  if( !ruleid.doesRuleContainExtatoms() )
  {
    DBGLOG(DBG,"not processing rule " << ruleid << " (does not contain extatoms)");
    return ruleid;
  }

  const Rule& rule = reg->rules.getByID(ruleid);
  Rule ruledom = rule;
  BOOST_FOREACH (ID b, rule.body){
    if (!b.isNaf() && b.isExternalAtom()){
      const ExternalAtom& ea = reg->eatoms.getByID(b);
      BOOST_FOREACH (ID o, ea.tuple){
        if (o.isVariableTerm() &&
	      (ci.stronglySafeVariables.find(ruleid) == ci.stronglySafeVariables.end() ||
	       std::find(ci.stronglySafeVariables.at(ruleid).begin(), ci.stronglySafeVariables.at(ruleid).end(), o) == ci.stronglySafeVariables.at(ruleid).end())){
          OrdinaryAtom oatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN | ID::PROPERTY_AUX);
          oatom.tuple.push_back(reg->getAuxiliaryConstantSymbol('d', b));
          BOOST_FOREACH (ID o2, ea.tuple){
            oatom.tuple.push_back(o2);
          }
          ruledom.body.push_back(reg->storeOrdinaryNAtom(oatom));
          break;
        }
      }
    }
  }
  ID ruledomid = reg->storeRule(ruledom);
#ifndef NDEBUG
  {
  std::stringstream s;
  RawPrinter printer(s, reg);
  printer.print(ruledomid);
  DBGLOG(DBG,"rewriting rule " << s.str() << " from " << rule <<
  " with id " << ruleid << " to rule with domain predicates");
  }
#endif
  return ruledomid;
}

// get rule
// rewrite all eatoms in body to auxiliary replacement atoms
// store and return id
ID BaseModelGeneratorFactory::convertRule(ProgramCtx& ctx, ID ruleid)
{
  RegistryPtr reg = ctx.registry();
  if( !ruleid.doesRuleContainExtatoms() )
  {
    DBGLOG(DBG,"not converting rule " << ruleid << " (does not contain extatoms)");
    return ruleid;
  }

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
  newrule.kind |= ID::PROPERTY_AUX;
  newrule.body.clear();

  // convert (recursively in aggregates)
  convertRuleBody(ctx, rule.body, newrule.body);

  // store as rule
  ID newruleid = reg->storeRule(newrule);
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
