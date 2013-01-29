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
#include "dlvhex2/AttributeGraph.h"

#include <boost/foreach.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <fstream>

DLVHEX_NAMESPACE_BEGIN

// we define this class here, as we use it only in the implementation
// it is stored in registry as an opaque shared pointer
class EAInputTupleCache
{
protected:
	// a vector of pointers to tuple, with null values allowed
	// we use this similar to a bitset:
	//
        // the idaddress of an ordinary ground atom
        // with predicate externalatom::auxinputpredicate
	// is the address in this vector
	//
	// (null values are for all ordinary ground atoms which are not auxiliary input predicates)
	// we trade this null space for addressing speed, as these tuples need to be looked up very often
	//
	// the stored tuples are input tuples to external atoms, with all replacements of variables
        // due to externalatom::auxinputmapping already done
	boost::ptr_vector< boost::nullable< Tuple > > cache;

public:
	// nothing virtual, this is for implementation
	// and virtual function calls could slow us down
	EAInputTupleCache(): cache() {}
	// free the cache and delete all non-NULL pointers (automatically)
	~EAInputTupleCache() {}

	// just looks up and asserts everything is ok
	inline const Tuple& lookup(IDAddress auxInputOgAtomAddress) const
	{
		assert(auxInputOgAtomAddress < cache.size());
		assert( !cache.is_null(auxInputOgAtomAddress) );
		return cache[auxInputOgAtomAddress];
	}

	// looks up tuple in vector and returns it
	// creates empty tuple in vector and returns it if nothing was stored in vector
	//
	// resizes vector if necessary
	inline Tuple& lookupOrCreate(IDAddress auxInputOgAtomAddress)
	{
		if( auxInputOgAtomAddress >= cache.size() )
			cache.resize(auxInputOgAtomAddress+1, NULL);
		if( cache.is_null(auxInputOgAtomAddress) )
		{
			cache.replace(auxInputOgAtomAddress, new Tuple);
		}
		return cache[auxInputOgAtomAddress];
	}
};
typedef boost::shared_ptr<EAInputTupleCache> EAInputTupleCachePtr;

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

  RegistryPtr reg = ctx.registry();

  // build input interpretation
  // for each input tuple (multiple auxiliary inputs possible)
  //   build query
  //   call retrieve
  //   integrate answer into interpretation i as additional facts

  // if this is wrong, we might have mixed up registries between plugin and program
  assert(!!eatom.pluginAtom && eatom.predicate == eatom.pluginAtom->getPredicateID());

  // update masks (inputMask and auxInputMask)
  eatom.updatePredicateInputMask();

  // project interpretation for predicate inputs
  InterpretationConstPtr eatominp =
    projectEAtomInputInterpretation(ctx.registry(), eatom, inputi);

  if( eatom.auxInputPredicate == ID_FAIL )
  {
	// only one input tuple, and that is the one stored in eatom.inputs

	// prepare callback for evaluation of this eatom
	if( !cb.eatom(eatom) )
	{
		LOG(DBG,"callback aborted for eatom " << eatom);
		return false;
	}

	// XXX here we copy it, we should just reference it
	PluginAtom::Query query(&ctx, eatominp, eatom.inputs, eatom.tuple, &eatom);
	// XXX make this part of constructor
	query.extinterpretation = inputi;
	return evaluateExternalAtomQuery(query, cb, nogoods);
  }
  else
  {
	// auxiliary input predicate -> get input tuples (with cache)

	// ensure we have a cache for external atom input tuples
	if( !reg->eaInputTupleCache )
		reg->eaInputTupleCache.reset(new EAInputTupleCache);
	EAInputTupleCache& eaitc = *reg->eaInputTupleCache;

	// build input tuples
	// (we associate input tuples in the cache with the auxiliary external
	// atom input tuples they have been created from)
	// (for eatoms where no auxiliary input is required, we directly use ExternalAtom::inputs)
	InterpretationPtr inputs(new Interpretation(reg));
	// allocates inputs if necessary
	buildEAtomInputTuples(ctx.registry(), eatom, inputi, inputs);

	Interpretation::TrueBitIterator bit, bit_end;
	boost::tie(bit, bit_end) = inputs->trueBits();

	if( bit != bit_end )
	{
		// we have an input atom, so we tell the callback that we will process it
		if( !cb.eatom(eatom) )
		{
			LOG(DBG,"callback aborted for eatom " << eatom);
			return false;
		}

		for(;bit != bit_end; ++bit) {
			const Tuple& inputtuple = eaitc.lookup(*bit);
			// build query as reference to the storage in cache
			// XXX here we copy, we could make it const ref in Query
			PluginAtom::Query query(&ctx, eatominp, inputtuple, eatom.tuple, &eatom);
			query.extinterpretation = inputi;
			if( ! evaluateExternalAtomQuery(query, cb, nogoods) )
				return false;
		}
	}
  }
  return true;
}

bool BaseModelGenerator::evaluateExternalAtomQuery(
		PluginAtom::Query& query,
	       	ExternalAnswerTupleCallback& cb,
	       	NogoodContainerPtr nogoods) const {
	const ProgramCtx& ctx = *query.ctx;
	const RegistryPtr reg = ctx.registry();
	const ExternalAtom& eatom = *query.eatom;
	const Tuple& inputtuple = query.input;

	if( Logger::Instance().shallPrint(Logger::PLUGIN) ) {
	        LOG(PLUGIN,"eatom projected interpretation = " << *query.interpretation);
	        LOG(PLUGIN,"eatom input pattern = " << printManyToString<RawPrinter>(eatom.inputs, ",", reg));
		LOG(PLUGIN,"eatom output pattern = " << printManyToString<RawPrinter>(eatom.tuple, ",", reg));
	        LOG(PLUGIN,"eatom input tuple = " << printManyToString<RawPrinter>(inputtuple, ",", reg));
	}

    PluginAtom::Answer answer;
    assert(!!eatom.pluginAtom);
    if( query.ctx->config.getOption("UseExtAtomCache") ){
      eatom.pluginAtom->retrieveCached(query, answer, nogoods);
    }
    else
    {
      DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidr,"PluginAtom retrieve");
      eatom.pluginAtom->retrieve(query, answer, nogoods);
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

    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidier,"integrate external results");

    // integrate result into interpretation
    BOOST_FOREACH(const Tuple& t, answer.get())
    {
      LOG(PLUGIN,"got answer tuple " << printManyToString<RawPrinter>(t, ",", reg));
      if( !verifyEAtomAnswerTuple(reg, eatom, t) )
      {
        LOG(WARNING,"external atom " << eatom << " returned tuple " <<
            printrange(t) << " which does not match output pattern (skipping)");
        continue;
      }

      // call callback and abort if requested
      if( !cb.output(t) )
      {
        LOG(DBG,"callback aborted for output tuple <" << printManyToString<RawPrinter>(t, ",", reg) << ">");
        return false;
      }
    }

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
  LOG_SCOPE(DBG, "vEAAT", false);
  LOG(DBG,"= verifyEAtomAnswerTuple for " << eatom << " and tuple <" << printManyToString<RawPrinter>(t, ", ", reg) << ">");
  // check answer tuple, if it corresponds to pattern

  if( t.size() != eatom.tuple.size() )
    throw PluginError("External atom " + eatom.pluginAtom->getPredicate() +
       " returned tuple <" + printManyToString<RawPrinter>(t, ", ", reg) + "> of incompatible size.");

  // pattern may contain variables and constants
  Tuple pattern(eatom.tuple);

  // consecutively compare tuple term vs pattern term of same index:
  // * if variable appears throw exception (programming error, plugins may only return constants)
  // * if constant meets variable -> set all variables of same ID in pattern to that constant and continue verifying
  // * if constant meets other constant -> return false (mismatch)
  // * if constant meets same constant -> continue verifying
  // return true

  const unsigned arity = t.size();
  for(unsigned at = 0; at < arity; ++at)
  {
    if( t[at].isVariableTerm() )
      throw PluginError("External atom " + eatom.pluginAtom->getPredicate() +
         " returned variable in result tuple <" + printManyToString<RawPrinter>(t, ", ", reg) + "> which is forbidden");

    if( pattern[at].isVariableTerm() )
    {
      // set all variables to this constant and continue
      ID variable = pattern[at];
      for(unsigned i = at; i < arity; ++i)
      {
        if( pattern[i] == variable )
          pattern[i] = t[at];
      }
    }
    else if( pattern[at] != t[at] )
    {
      // mismatch
      return false;
    }
    else
    {
      // ok, continue
      assert(t[at] == pattern[at]);
    }
  }

  return true;
}

InterpretationPtr BaseModelGenerator::projectEAtomInputInterpretation(RegistryPtr reg,
  const ExternalAtom& eatom, InterpretationConstPtr full) const
{
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"BaseModelGen::projectEAII");
  // we do this in general for the eatom
  //eatom.updatePredicateInputMask();

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
		InterpretationConstPtr interpretation,
		InterpretationPtr inputs) const
{
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"BaseModelGen::buildEAIT");
	LOG_SCOPE(PLUGIN,"bEAIT",false);
	DBGLOG(DBG,"= buildEAtomInputTuples " << eatom);

	// it must be true here
	assert(!!reg->eaInputTupleCache);
	EAInputTupleCache& eaitc = *reg->eaInputTupleCache;

	// if there are no variables, there is no eatom.auxInputPredicate and this function should not be called
	assert(eatom.auxInputPredicate != ID_FAIL);

	// otherwise find all aux input predicates that are true and extract their tuples
	Interpretation relevant(reg);
	relevant.getStorage() |= interpretation->getStorage() & eatom.getAuxInputMask()->getStorage();
	Interpretation::TrueBitIterator it, it_end;
	boost::tie(it, it_end) = relevant.trueBits();
	{
		for(;it != it_end; ++it)
		{
			IDAddress inputAtomBit = *it;

			// lookup or create in cache
			Tuple& t = eaitc.lookupOrCreate(inputAtomBit);

			if( t.empty() )
			{
				// create it

				const dlvhex::OrdinaryAtom& oatom = reg->ogatoms.getByAddress(inputAtomBit);

				// add copy of original input tuple
				t = eatom.inputs;

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
						// (this also verifies that we do not overwrite a variable twice with different values)
						assert(t[*it].isTerm() && t[*it].isVariableTerm());
						t[*it] = replaceBy;
					}
				}
				DBGLOG(DBG,"after inserting auxiliary predicate inputs: input = " << printManyToString<RawPrinter>(t, ",", reg));
			}

			// signal to caller, that it should use the bit/tuple
			inputs->setFact(inputAtomBit);
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

      // create replacement atom
      OrdinaryAtom replacement(ID::MAINKIND_ATOM | ID::PROPERTY_AUX | ID::PROPERTY_EXTERNALAUX);
      assert(!!eatom.pluginAtom);
      replacement.tuple.push_back(
          reg->getAuxiliaryConstantSymbol('r',
            eatom.pluginAtom->getPredicateID()));
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
ID BaseModelGeneratorFactory::addDomainPredicatesWhereNecessary(ProgramCtx& ctx, const ComponentGraph::ComponentInfo& ci, RegistryPtr reg, ID ruleid)
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
        if (ctx.attrgraph->isExternalAtomNecessaryForDomainExpansionSafety(b)){
        //if (o.isVariableTerm() &&
	//      (ci.stronglySafeVariables.find(ruleid) == ci.stronglySafeVariables.end() ||
	//       std::find(ci.stronglySafeVariables.at(ruleid).begin(), ci.stronglySafeVariables.at(ruleid).end(), o) == ci.stronglySafeVariables.at(ruleid).end())){
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
