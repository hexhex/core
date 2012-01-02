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
 * @file   PluginInterface.cpp
 * @author Roman Schindlauer
 * @date   Mon Oct 17 14:37:07 CEST 2005
 * 
 * @brief Definition of Classes PluginAtom, PluginRewriter,
 * and PluginInterface.
 * 
 *      
 */     

#include "dlvhex/PluginInterface.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "dlvhex/Registry.hpp"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/Term.hpp"
#include "dlvhex/ID.hpp"
#include "dlvhex/Benchmarking.h"

DLVHEX_NAMESPACE_BEGIN

#if 0
bool PluginAtom::Query::operator<(const Query& other) const
{
	/*
  return
    ( interpretation < other.interpretation ) ||
    ( interpretation == other.interpretation &&
      input < other.input ) ||
    ( interpretation == other.interpretation &&
      input == other.input &&
      pattern < other.pattern );
			*/
}
#endif
        
bool PluginAtom::Query::operator==(const Query& other) const
{
  return
      (input == other.input) &&
      (pattern == other.pattern) &&
      (
        (interpretation == other.interpretation) ||
        (interpretation != 0 && other.interpretation != 0 &&
         *interpretation == *other.interpretation)
      );
}

// hash function for QueryAnswerCache
std::size_t hash_value(const PluginAtom::Query& q)
{
  std::size_t seed = 0;
  boost::hash_combine(seed, q.input);
  //LOG("hash_combine inp " << printrange(q.input) << " yields " << seed);
  boost::hash_combine(seed, q.pattern);
  //LOG("hash_combine pat " << printrange(q.pattern) << " yields " << seed);
  // TODO: can we take hash of pointer to interpretation here?
  if( q.interpretation == 0 )
  {
    boost::hash_combine(seed, 0);
  }
  else
  {
    // TODO: outsource this
    //boost::hash_combine(seed, q.interpretation->getStorage());
    const Interpretation::Storage& bits = q.interpretation->getStorage();
    for(Interpretation::Storage::enumerator en = bits.first();
        en != bits.end(); ++en)
    {
      boost::hash_combine(seed, *en);
      //LOG("hash_combine at " << *en << " yields " << seed);
    }
  }
  //LOG("hash_combine returning " << seed);
  return seed;
}
        
PluginAtom::Answer::Answer():
  output(new std::vector<Tuple>),
  used(false)
{
}

void
PluginAtom::addInputPredicate()
{
	// throw error if last input term was tuple
	if (inputType.size() > 0)
		if (inputType.back() == TUPLE)
			throw GeneralError("Tuple inputs must be specified last in input list");

    inputType.push_back(PREDICATE);
}


void
PluginAtom::addInputConstant()
{
	// throw error if last input term was tuple
	if (inputType.size() > 0)
		if (inputType.back() == TUPLE)
			throw GeneralError("Tuple inputs must be specified last in input list");

    inputType.push_back(CONSTANT);
}


void
PluginAtom::addInputTuple()
{
    inputType.push_back(TUPLE);
}


bool
PluginAtom::checkInputArity(const unsigned arity) const
{
  bool ret = (inputType.size() == arity);

  if (!inputType.empty())
    {
      return inputType.back() == TUPLE ? true : ret;
    }
  else
    {
      return ret;
    }
}


void
PluginAtom::setOutputArity(const unsigned arity)
{
    outputSize = arity;
}


bool
PluginAtom::checkOutputArity(const unsigned arity) const
{
    return arity == outputSize;
}


void PluginAtom::retrieveCached(const Query& query, Answer& answer)
{
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidrc,"PluginAtom::retrieveCached");
  // Cache answer for queries which were already done once:
  //
  // The most efficient way would be:
  // * use cache for same inputSet + same *inputi + more specific pattern
  // * store new cache for new inputSet/*inputi combination or unrelated (does not unify) pattern
  // * replace cache for existing inputSet/*inputi combination and less specific (unifies in one direction) pattern
  // 
  // The currently implemented "poor (wo)man's version" is:
  // * store answers in cache with queries as keys, disregard relations between patterns
  ///@todo: efficiency could be increased for certain programs by considering pattern relationships as indicated above

#if 0
  #include "dlvhex/PrintVisitor.h"
  #include <iostream>
  std::cerr << "cache:" << std::endl;
  for( QueryAnswerCache::const_iterator i = queryAnswerCache.begin(); i != queryAnswerCache.end(); ++i)
  {
	  std::cerr << "  query: <";
	  RawPrintVisitor visitor(std::cerr);
	  i->first.getInterpretation().accept(visitor);
	  std::cerr << "|" << i->first.getInputTuple() << "|" << i->first.getPatternTuple() << ">" << std::endl;
  }


	  std::cerr << "retrieving query: <";
	  RawPrintVisitor visitor(std::cerr);
	  query.getInterpretation().accept(visitor);
	  std::cerr << "|" << query.getInputTuple() << "|" << query.getPatternTuple() << ">";
#endif

  //LOG("before queryAnswerCache");
  Answer& ans = queryAnswerCache[query];
  //LOG("after queryAnswerCache");
  if( ans.hasBeenUsed() )
  {
    // answer was not default constructed
    // -> use cache
    answer = ans;
  }
  else
  {
    // answer was default constructed
    // -> retrieve and replace in cache
    {
      DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidr,"PluginAtom retrieve");
      #if 0
      #ifndef NDEBUG
      std::stringstream o;
      RawPrinter printer(o, query.interpretation->getRegistry());
      o << "retrieving for ";
      printer.printmany(query.input, ",");
      o << "/";
      printer.printmany(query.pattern, ",");
      o << "/" << *query.interpretation;
      LOG(o.str());
      #endif
      #endif
      retrieve(query, ans);
      // if there was no answer, perhaps it has never been used, so we use it manually
      ans.use();
      //LOG("after retrieve: answ is used = " << ans.hasBeenUsed());
    }
    answer = ans;
  }
}

void PluginAtom::retrieveCached(const Query& query, Answer& answer, CDNLSolverPtr solver)
{
	DBGLOG(DBG, "Retrieve with learning, solver: " << (solver == CDNLSolverPtr() ? "not " : "") << "available" );

	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidrc,"PluginAtom::retrieveCached");
	// Cache answer for queries which were already done once:
	//
	// The most efficient way would be:
	// * use cache for same inputSet + same *inputi + more specific pattern
	// * store new cache for new inputSet/*inputi combination or unrelated (does not unify) pattern
	// * replace cache for existing inputSet/*inputi combination and less specific (unifies in one direction) pattern
	// 
	// The currently implemented "poor (wo)man's version" is:
	// * store answers in cache with queries as keys, disregard relations between patterns
	///@todo: efficiency could be increased for certain programs by considering pattern relationships as indicated above

	Answer& ans = queryAnswerCache[query];
	if( ans.hasBeenUsed() )
	{
		// answer was not default constructed
		// -> use cache
		answer = ans;
	}
	else
	{
		// answer was default constructed
		// -> retrieve and replace in cache
		{
			DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidr,"PluginAtom retrieve");

			retrieve(query, ans, solver);
			// if there was no answer, perhaps it has never been used, so we use it manually
			ans.use();
		}
		answer = ans;
	}

#if 0
	retrieveCached(query, answer);

	if (solver != CDNLSolverPtr()){

		DBGLOG(DBG, "Learning from external call");

		// find relevant input
		bm::bvector<>::enumerator en = query.eatom->getPredicateInputMask()->getStorage().first();
		bm::bvector<>::enumerator en_end = query.eatom->getPredicateInputMask()->getStorage().end();

		Nogood extNgInput;

		while (en < en_end){
			extNgInput.insert(solver->createLiteral(*en, query.interpretation->getFact(*en)));
			en++;
		}

		DBGLOG(DBG, "Input nogood: " << extNgInput);

		// construct replacement atom
		OrdinaryAtom replacement(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX);
		replacement.tuple.resize(1);
		replacement.tuple[0] = registry->getAuxiliaryConstantSymbol('n', query.eatom->predicate);
		replacement.tuple.insert(replacement.tuple.end(), query.input.begin(), query.input.end());
		int s = replacement.tuple.size();

		const std::vector<Tuple>& otuples = answer.get();
		BOOST_FOREACH (Tuple otuple, otuples){
			replacement.tuple.resize(s);
			// add current output
			replacement.tuple.insert(replacement.tuple.end(), otuple.begin(), otuple.end());
			// get ID of this replacement atom
			ID idreplacement = registry->storeOrdinaryGAtom(replacement);

			Nogood extNg = extNgInput;
			extNg.insert(solver->createLiteral(idreplacement.address));
			DBGLOG(DBG, "Overall nogood: " << extNg);
			solver->addNogood(extNg);
		}
	}
#endif
}

void PluginAtom::retrieve(const Query& query, Answer& answer, CDNLSolverPtr solver){

	retrieve(query, answer);

	if (solver != CDNLSolverPtr()){

		DBGLOG(DBG, "Learning from external call");

		Nogood extNgInput = getInputNogood(solver, query);
		DBGLOG(DBG, "Input nogood: " << extNgInput);

		Set<ID> out = getOutputAtoms(solver, query, answer);
		BOOST_FOREACH (ID oid, out){
			Nogood extNg = extNgInput;
			extNg.insert(solver->createLiteral(oid));
			DBGLOG(DBG, "Overall nogood: " << extNg);
			solver->addNogood(extNg);
		}

		// functionality
		if (solver->getProgramContext().config.getOption("ExternalLearningFunctionality") && isFunctional()){
			// there is a unique output
			const std::vector<Tuple>& otuples = answer.get();
			ID uniqueOut = getOutputAtom(true, query, otuples[0]);

			// go through all output tuples which have been generated so far
			BOOST_FOREACH (Tuple t, this->otuples){
				ID id = getOutputAtom(true, query, t);
				if (id != uniqueOut){
					Nogood excludeOthers;
					excludeOthers.insert(solver->createLiteral(uniqueOut));
					excludeOthers.insert(solver->createLiteral(id));
					DBGLOG(DBG, "Nogood for functional external source: " << excludeOthers);
					solver->addNogood(excludeOthers);
				}
			}
		}


#if 0
		// construct replacement atom
		OrdinaryAtom replacement(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX);
		replacement.tuple.resize(1);
		replacement.tuple[0] = registry->getAuxiliaryConstantSymbol('n', query.eatom->predicate);
		replacement.tuple.insert(replacement.tuple.end(), query.input.begin(), query.input.end());
		int s = replacement.tuple.size();

		const std::vector<Tuple>& otuples = answer.get();
		BOOST_FOREACH (Tuple otuple, otuples){
			replacement.tuple.resize(s);
			// add current output
			replacement.tuple.insert(replacement.tuple.end(), otuple.begin(), otuple.end());
			// get ID of this replacement atom
			ID idreplacement = registry->storeOrdinaryGAtom(replacement);

			Nogood extNg = extNgInput;
			extNg.insert(solver->createLiteral(idreplacement.address));
			DBGLOG(DBG, "Overall nogood: " << extNg);
			solver->addNogood(extNg);
		}
#endif
	}
}

Nogood PluginAtom::getInputNogood(CDNLSolverPtr solver, const Query& query){

	// find relevant input
	bm::bvector<>::enumerator en = query.eatom->getPredicateInputMask()->getStorage().first();
	bm::bvector<>::enumerator en_end = query.eatom->getPredicateInputMask()->getStorage().end();

	Nogood extNgInput;

	while (en < en_end){
		// for nonmonotonic parameters we need the positive and negative input, for monotonic ones the positive input suffices
		if (query.interpretation->getFact(*en) || !isMonotonic() || !solver->getProgramContext().config.getOption("ExternalLearningMonotonicity")){
			extNgInput.insert(solver->createLiteral(*en, query.interpretation->getFact(*en)));
		}
		en++;
	}

	return extNgInput;
}

Set<ID> PluginAtom::getOutputAtoms(CDNLSolverPtr solver, const Query& query, const Answer& answer){

	Set<ID> out;

	// construct replacement atom
	OrdinaryAtom replacement(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX);
	replacement.tuple.resize(1);
	replacement.tuple[0] = registry->getAuxiliaryConstantSymbol('n', query.eatom->predicate);
	replacement.tuple.insert(replacement.tuple.end(), query.input.begin(), query.input.end());
	int s = replacement.tuple.size();

	const std::vector<Tuple>& otuples = answer.get();
	BOOST_FOREACH (Tuple otuple, otuples){
		// remember that otuple was generated
		this->otuples.push_back(otuple);

		replacement.tuple.resize(s);
		// add current output
		replacement.tuple.insert(replacement.tuple.end(), otuple.begin(), otuple.end());
		// get ID of this replacement atom
		ID idreplacement = registry->storeOrdinaryGAtom(replacement);
		out.insert(idreplacement);
	}

	return out;
}

ID PluginAtom::getOutputAtom(bool sign, const Query& query, Tuple otuple){

	// construct replacement atom with input from query
	OrdinaryAtom replacement(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX);
	replacement.tuple.resize(1);
	replacement.tuple[0] = registry->getAuxiliaryConstantSymbol(sign ? 'r' : 'n', query.eatom->predicate);
	replacement.tuple.insert(replacement.tuple.end(), query.input.begin(), query.input.end());
	int s = replacement.tuple.size();

	// add output tuple
	replacement.tuple.insert(replacement.tuple.end(), otuple.begin(), otuple.end());

	ID idreplacement = registry->storeOrdinaryGAtom(replacement);
	return idreplacement;
}

PluginAtom::InputType
PluginAtom::getInputType(const unsigned index) const
{
	if (index >= inputType.size())
	{
		assert(inputType.back() == TUPLE);
		return inputType.back();
	}

    return inputType[index];
}

void PluginAtom::setRegistry(RegistryPtr reg)
{
  // i think we really don't want to change registry during the lifetime,
  // it would invalidate the cache and more bad things would happen
  assert(registry == 0);
  assert(reg != 0);
  registry = reg;
  predicateID = registry->terms.getIDByString(predicate);
  if( predicateID == ID_FAIL )
  {
    Term t(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, predicate);
    predicateID = registry->terms.storeAndGetID(t);
  }
  assert(predicateID != ID_FAIL);
}

// call printUsage for each loaded plugin
void PluginInterface::printUsage(std::ostream& o) const
{
  // don't fail if no usage message has been defined, simply do not give one
}

// call processOptions for each loaded plugin
// (this is supposed to remove "recognized" options from pluginOptions)
void PluginInterface::processOptions(std::list<const char*>& pluginOptions, ProgramCtx& ctx)
{
  // don't fail if no option processing has been defined, simply do not process
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
