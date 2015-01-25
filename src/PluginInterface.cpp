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
 * @file   PluginInterface.cpp
 * @author Roman Schindlauer
 * @date   Mon Oct 17 14:37:07 CEST 2005
 * 
 * @brief Definition of Classes PluginAtom, PluginRewriter,
 * and PluginInterface.
 * 
 *      
 */     

#include "dlvhex2/PluginInterface.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "dlvhex2/Registry.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/GenuineSolver.h"
#include "dlvhex2/Term.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/Term.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/HexParser.h"
#include "dlvhex2/ExternalLearningHelper.h"

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
PluginAtom::addInputPredicate(bool nameIsRelevant)
{
    // throw error if last input term was tuple
    if (inputType.size() > 0)
        if (inputType.back() == TUPLE)
            throw GeneralError("Tuple inputs must be specified last in input list");

    inputType.push_back(PREDICATE);

    if (!nameIsRelevant) prop.predicateParameterNameIndependence.insert(inputType.size() - 1);

    if (allmonotonic) prop.monotonicInputPredicates.insert(inputType.size() - 1);
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

int
PluginAtom::getInputArity() const{
	return inputType.size();
}

int
PluginAtom::getOutputArity() const{
	return outputSize;
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
PluginAtom::checkOutputArity(const ExtSourceProperties& prop, const unsigned arity) const
{
    return prop.hasVariableOutputArity() || (arity == outputSize);
}

/*
void PluginAtom::retrieveCached(const Query& query, Answer& answer)
{
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidrc,"PluginAtom retrieveCached");
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

  boost::mutex::scoped_lock lock(cacheMutex);

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
      retrieve(query, ans);
      // if there was no answer, perhaps it has never been used, so we use it manually
      ans.use();
    }
    answer = ans;
  }
}
*/

bool PluginAtom::retrieveFacade(const Query& query, Answer& answer, NogoodContainerPtr nogoods, bool useCache)
{
	bool fromCache = false;

	// split the query
	const ExtSourceProperties& prop = query.ctx->registry()->eatoms.getByID(query.eatomID).getExtSourceProperties();

	DBGLOG(DBG, "Splitting query");
	std::vector<Query> atomicQueries = splitQuery(query, prop);
	DBGLOG(DBG, "Got " << atomicQueries.size() << " atomic queries");
	BOOST_FOREACH (Query atomicQuery, atomicQueries){
		Answer atomicAnswer;
		bool subqueryFromCache;
		if (useCache){
			subqueryFromCache = retrieveCached(atomicQuery, atomicAnswer, query.ctx->config.getOption("ExternalLearningUser") ? nogoods : NogoodContainerPtr());
		}else{
			subqueryFromCache = false;

			DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidr,"PluginAtom retrieve");
			retrieve(atomicQuery, atomicAnswer, query.ctx->config.getOption("ExternalLearningUser") ? nogoods : NogoodContainerPtr());
		}

		// learn only if the query was not answered from cache (otherwise also the nogoods come from the cache)
		if (!subqueryFromCache){
			if (!!nogoods && query.ctx->config.getOption("ExternalLearningIOBehavior")) ExternalLearningHelper::learnFromInputOutputBehavior(atomicQuery, atomicAnswer, prop, nogoods);
			if (!!nogoods && query.ctx->config.getOption("ExternalLearningFunctionality") && prop.isFunctional()) ExternalLearningHelper::learnFromFunctionality(atomicQuery, atomicAnswer, prop, otuples, nogoods);
		}

		// overall answer is the union of the atomic answers
		DBGLOG(DBG, "Atomic query delivered " << atomicAnswer.get().size() << " tuples");
		answer.get().insert(answer.get().end(), atomicAnswer.get().begin(), atomicAnswer.get().end());

		// query counts as answered from cache if at least one subquery was answered from cache
		fromCache |= subqueryFromCache;
	}

	if (!!nogoods && query.ctx->config.getOption("ExternalLearningNeg")) ExternalLearningHelper::learnFromNegativeAtoms(query, answer, prop, nogoods);

	return fromCache;
}

bool PluginAtom::retrieveCached(const Query& query, Answer& answer, NogoodContainerPtr nogoods)
{
	DBGLOG(DBG, "Retrieve with learning, pointer to nogood container: " << (!nogoods ? "not " : "") << "available" );

	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidrc,"PluginAtom retrieveCached");
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

	boost::mutex::scoped_lock lock(cacheMutex);

	Answer& ans = queryAnswerCache[query];
	if( ans.hasBeenUsed())
	{
		// answer was not default -> use
		answer = ans;

		// check if there are cached nogoods
		if (nogoods){
			SimpleNogoodContainerPtr& cachedNogoods = queryNogoodCache[query];
			if (cachedNogoods){
				// return cached nogoods
				for (int i = 0; i < cachedNogoods->getNogoodCount(); ++i) nogoods->addNogood(cachedNogoods->getNogood(i));
			}else{
				// answer is cached but no nogoods: reevaluate and return nogoods
				queryAnswerCache[query] = Answer();
				Answer& ans2 = queryAnswerCache[query];
				cachedNogoods.reset(new SimpleNogoodContainer());
				retrieve(query, ans2, cachedNogoods);
				ans2.use();
				answer = ans2;	// ans2 should be the same as ans
				for (int i = 0; i < cachedNogoods->getNogoodCount(); ++i) nogoods->addNogood(cachedNogoods->getNogood(i));
			}
		}

		return true; // answered from cache
	}
	else
	{
		// answer was default constructed
		// -> retrieve and replace in cache
		{
			DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidr,"PluginAtom retrieve");

			if (nogoods){
				SimpleNogoodContainerPtr& cachedNogoods = queryNogoodCache[query];
				assert(!cachedNogoods);

				cachedNogoods.reset(new SimpleNogoodContainer());
				retrieve(query, ans, cachedNogoods);
				for (int i = 0; i < cachedNogoods->getNogoodCount(); ++i) nogoods->addNogood(cachedNogoods->getNogood(i));
			}else{
				retrieve(query, ans, NogoodContainerPtr());
			}

			// if there was no answer, perhaps it has never been used, so we use it manually
			ans.use();
		}
		answer = ans;

		return false; // not answered from cache
	}
}

void PluginAtom::retrieve(const Query& query, Answer& answer, NogoodContainerPtr nogoods){
	DBGLOG(DBG, "Default implementation of PluginAtom::retrieve(const Query& query, Answer& answer, NogoodContainerPtr nogoods): delegating the call to PluginAtom::retrieve(const Query& query, Answer& answer)");
	retrieve(query, answer);
}

void PluginAtom::retrieve(const Query& query, Answer& answer){
	DBGLOG(DBG, "Default implementation of PluginAtom::retrieve(const Query& query, Answer& answer): doing nothing");
}

void PluginAtom::learnSupportSets(const Query&, NogoodContainerPtr nogoods){
	assert(prop.providesSupportSets() && "This external source does not provide support sets");
}

std::vector<PluginAtom::Query> PluginAtom::splitQuery(const Query& query, const ExtSourceProperties& prop){

	const ExternalAtom& eatom = query.ctx->registry()->eatoms.getByID(query.eatomID);
	std::vector<Query> atomicQueries;
	if ((prop.isLinearOnAtomLevel() || prop.isLinearOnTupleLevel()) && query.ctx->config.getOption("ExternalLearningLinearity")){

		DBGLOG(DBG, "Splitting query by exploiting linearity");

		if (prop.isLinearOnAtomLevel()){
			bm::bvector<>::enumerator en = eatom.getPredicateInputMask()->getStorage().first();
			bm::bvector<>::enumerator en_end = eatom.getPredicateInputMask()->getStorage().end();
			while (en < en_end){
				DBGLOG(DBG, "Creating partial query for input atom " << *en);
				// create a partial query only for this input atom
				Query qa = query;
				qa.predicateInputMask = InterpretationPtr(new Interpretation(query.interpretation->getRegistry()));
				qa.predicateInputMask->setFact(*en);
				atomicQueries.push_back(qa);

				en++;
			}
		}
		if (prop.isLinearOnTupleLevel()){
			// collect all tuples
			bm::bvector<>::enumerator en = eatom.getPredicateInputMask()->getStorage().first();
			bm::bvector<>::enumerator en_end = eatom.getPredicateInputMask()->getStorage().end();

			// extract tuples from atoms in input interpretation
			std::set<Tuple> tuples;
			while (en < en_end){
				const OrdinaryAtom& atom = query.interpretation->getRegistry()->ogatoms.getByAddress(*en);

				Tuple t;
				for (uint32_t i = 1; i < atom.tuple.size(); i++){
					t.push_back(atom.tuple[i]);
				}
				tuples.insert(t);

				en++;
			}

			// check if all input atoms have the same arity
			int arity = -1;
			bool split = true;
			BOOST_FOREACH (Tuple t, tuples){
				if (arity != -1 && t.size() != arity){
					split = false;
					break;
				}
				arity = t.size();
			}

			if (split){
				// create for each tuple a subquery
				BOOST_FOREACH (Tuple t, tuples){
					// create a partial query only for this input atom
#ifndef NDEBUG
					std::stringstream ss;
					bool first = true;
					BOOST_FOREACH (ID id, t){
						if (!first) ss << ", ";
						ss << id;
						first = false;
					}
					DBGLOG(DBG, "Creating partial query for input tuple " << ss.str());
#endif
					Query qa = query;
					qa.predicateInputMask = InterpretationPtr(new Interpretation(query.interpretation->getRegistry()));
					for (uint32_t parIndex = 0; parIndex < qa.input.size(); parIndex++){
						if (getInputType(parIndex) == PREDICATE){
							// assemble input atom over this tuple and predicate parameter
							OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
							atom.tuple.push_back(qa.input[parIndex]);
							BOOST_FOREACH (ID p, t) atom.tuple.push_back(p);
							ID atomID = query.interpretation->getRegistry()->storeOrdinaryGAtom(atom);
							DBGLOG(DBG, "Input atom: " << atomID);

							if (eatom.getPredicateInputMask()->getFact(atomID.address)){
								qa.predicateInputMask->setFact(atomID.address);
							}
						}
					}
					atomicQueries.push_back(qa);
				}
			}else{
				DBGLOG(DBG, "Do not split query because not all input atoms have the same arity");
				atomicQueries.push_back(query);
			}
		}

		// for all atomic queries, remove input facts which are not in the predicate input
		for (uint32_t i = 0; i < atomicQueries.size(); ++i){
			Query& q = atomicQueries[i];
			InterpretationPtr intr = InterpretationPtr(new Interpretation(q.interpretation->getRegistry()));
			intr->add(*q.interpretation);
			intr->getStorage() &= q.predicateInputMask->getStorage();
			q.interpretation = intr;
			DBGLOG(DBG, "Constructed atomic query (mask: " << *q.predicateInputMask << ", interpretation: " << *q.interpretation << ")");
		}
	}else{
		DBGLOG(DBG, "Do not split query");
		atomicQueries.push_back(query);
	}

	return atomicQueries;
}

void PluginAtom::generalizeNogood(Nogood ng, ProgramCtx* ctx, NogoodContainerPtr nogoods){

	if (!ng.isGround()) return;

	DBGLOG(DBG, "PluginAtom::generalizeNogood");

	// find the auxiliary in the nogood
	ID patternID = ID_FAIL;
	BOOST_FOREACH (ID l, ng){
		const OrdinaryAtom& lAtom = ctx->registry()->ogatoms.getByAddress(l.address);

		if (ctx->registry()->ogatoms.getIDByAddress(l.address).isExternalAuxiliary() && ctx->registry()->getIDByAuxiliaryConstantSymbol(lAtom.tuple[0]) == getPredicateID()){
			patternID = l;
			break;
		}
	}
	assert(patternID != ID_FAIL);
	DBGLOG(DBG, "patternID=" << patternID);
	const OrdinaryAtom& pattern = ctx->registry()->ogatoms.getByAddress(patternID.address);

	// rewrite atoms of form aux(p1,p2,p3,...) to aux(X1,X2,X3,...) and remember the variables used for the predicates
	std::map<ID, ID> translation;
	for (uint32_t par = 0; par < inputType.size(); ++par){
		if (getInputType(par) == PREDICATE && prop.isIndependentOfPredicateParameterName(par)){
			if (translation.find(pattern.tuple[par + 1]) == translation.end()){
				std::stringstream var;
				var << "X" << (par + 1);
				translation[pattern.tuple[par + 1]] = ctx->registry()->storeVariableTerm(var.str());
				DBGLOG(DBG, "Mapping " << pattern.tuple[par + 1] << " to " << translation[pattern.tuple[par + 1]]);
			}
		}
	}

	// translate the nogood
	Nogood translatedNG;
	BOOST_FOREACH (ID lID, ng){
		const OrdinaryAtom& l = ctx->registry()->ogatoms.getByAddress(lID.address);
		if (lID != patternID){
			OrdinaryAtom t = l;
			if (translation.find(l.tuple[0]) != translation.end()){
				t.tuple[0] = translation[l.tuple[0]];
			}
			if (t.tuple[0].isVariableTerm()){
				t.kind &= (ID::ALL_ONES ^ ID::SUBKIND_MASK);
				t.kind |= ID::SUBKIND_ATOM_ORDINARYN;
			}
			ID id = NogoodContainer::createLiteral(ctx->registry()->storeOrdinaryAtom(t).address, !lID.isNaf(), !t.tuple[0].isVariableTerm());
			DBGLOG(DBG, "Adding translated literal " << id << " to nogood");
			translatedNG.insert(id);
		}else{
			bool ground = true;
			OrdinaryAtom t = l;
			for (uint32_t i = 1; i <= inputType.size(); ++i){
				if (getInputType(i - 1) == PREDICATE){
					if (translation.find(pattern.tuple[i]) != translation.end()){
						t.tuple[i] = translation[pattern.tuple[i]];
					}
					if (t.tuple[i].isVariableTerm()){
						ground = false;
					}
				}else{
					t.tuple[i] = pattern.tuple[i];
				}
			}
			if (!ground){
				t.kind &= (ID::ALL_ONES ^ ID::SUBKIND_MASK);
				t.kind |= ID::SUBKIND_ATOM_ORDINARYN;
			}
			ID id = NogoodContainer::createLiteral(ctx->registry()->storeOrdinaryAtom(t).address, !lID.isNaf(), ground);
			DBGLOG(DBG, "Adding translated literal " << id << " to nogood");
			translatedNG.insert(id);
		}
	}

	// store the translated nogood
	DBGLOG(DBG, "Adding generalized nogood " << translatedNG.getStringRepresentation(ctx->registry()) << " (from " << ng.getStringRepresentation(ctx->registry()) << ")");
	nogoods->addNogood(translatedNG);
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

std::vector<PluginAtomPtr>
PluginInterface::createAtoms(ProgramCtx& ctx) const
{
	return std::vector<PluginAtomPtr>();
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

PluginConverterPtr
PluginInterface::createConverter(ProgramCtx&)
{
	return PluginConverterPtr();
}

std::vector<PluginConverterPtr>
PluginInterface::createConverters(ProgramCtx& ctx)
{
	// per default return the single converter created by createConverter
	std::vector<PluginConverterPtr> ret;
	PluginConverterPtr pc = this->createConverter(ctx);
	if( pc )
		ret.push_back(pc);
	return ret;
}

std::vector<HexParserModulePtr>
PluginInterface::createParserModules(ProgramCtx&)
{
	return std::vector<HexParserModulePtr>();
}

HexParserPtr PluginInterface::createParser(ProgramCtx&)
{
	return HexParserPtr();
}

PluginRewriterPtr PluginInterface::createRewriter(ProgramCtx&)
{
	return PluginRewriterPtr();
}

PluginOptimizerPtr PluginInterface::createOptimizer(ProgramCtx&)
{
	return PluginOptimizerPtr();
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
