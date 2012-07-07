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

#include "dlvhex2/PluginInterface.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "dlvhex2/Registry.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/GenuineSolver.h"
#include "dlvhex2/InternalGrounder.h"
#include "dlvhex2/Term.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/Term.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/HexParser.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

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

void PluginAtom::Query::builtInputPredicateTable(){

	int index = 0;
	BOOST_FOREACH (ID inp, input){
		inputPredicateTable[inp] = index++;
	}
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

    if (!nameIsRelevant) prop.predicateParameterNameIndependence.push_back(inputType.size() - 1);
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

#if 0
  #include "dlvhex2/PrintVisitor.h"
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

void PluginAtom::retrieveCached(const Query& query, Answer& answer, ProgramCtx* ctx, NogoodContainerPtr nogoods)
{
	DBGLOG(DBG, "Retrieve with learning, pointer nogood container: " << (!nogoods ? "not " : "") << "available" );

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

			retrieve(query, ans, ctx, nogoods);
			// if there was no answer, perhaps it has never been used, so we use it manually
			ans.use();
		}
		answer = ans;
	}
}

void PluginAtom::retrieve(const Query& query, Answer& answer, ProgramCtx* ctx, NogoodContainerPtr nogoods){

	// decide which properties to use
	ExtSourceProperties prop = this->prop;			// by default the settings of the plugin count
  assert(query.eatom != 0);
	if (query.eatom->useProp) prop = query.eatom->prop;	// this may be overridden by single external atoms

	std::vector<Query> atomicQueries = splitQuery(ctx, query, prop);
	DBGLOG(DBG, "Got " << atomicQueries.size() << " atomic queries");
	BOOST_FOREACH (Query atomicQuery, atomicQueries){
		Answer atomicAnswer;
		retrieve(atomicQuery, atomicAnswer);

		learnFromInputOutputBehavior(ctx, nogoods, atomicQuery, prop, atomicAnswer);
		learnFromFunctionality(ctx, nogoods, atomicQuery, prop, answer);

		// overall answer is the union of the atomic answers
		answer.get().insert(answer.get().end(), atomicAnswer.get().begin(), atomicAnswer.get().end());
	}
}

std::vector<PluginAtom::Query> PluginAtom::splitQuery(ProgramCtx* ctx, const Query& query, const ExtSourceProperties& prop){

	std::vector<Query> atomicQueries;
	if (ctx != 0 && (isLinearOnAtomLevel(prop) || isLinearOnTupleLevel(prop)) && ctx->config.getOption("ExternalLearningLinearity")){

		DBGLOG(DBG, "Splitting query by exploiting linearity");

		if (isLinearOnAtomLevel(prop)){
			bm::bvector<>::enumerator en = query.eatom->getPredicateInputMask()->getStorage().first();
			bm::bvector<>::enumerator en_end = query.eatom->getPredicateInputMask()->getStorage().end();
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
		if (isLinearOnTupleLevel(prop)){
			// collect all tuples
			bm::bvector<>::enumerator en = query.eatom->getPredicateInputMask()->getStorage().first();
			bm::bvector<>::enumerator en_end = query.eatom->getPredicateInputMask()->getStorage().end();

			// extract tuples from atoms in input interpretation
			std::set<Tuple> tuples;
			while (en < en_end){
				const OrdinaryAtom& atom = query.interpretation->getRegistry()->ogatoms.getByAddress(*en);

				Tuple t;
				for (int i = 1; i < atom.tuple.size(); i++){
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
#endif
					DBGLOG(DBG, "Creating partial query for input tuple " << ss.str());
					Query qa = query;
					qa.predicateInputMask = InterpretationPtr(new Interpretation(query.interpretation->getRegistry()));
					for (int parIndex = 0; parIndex < qa.input.size(); parIndex++){
						if (getInputType(parIndex) == PREDICATE){
							// assemble input atom over this tuple and predicate parameter
							OrdinaryAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
							atom.tuple.push_back(qa.input[parIndex]);
							BOOST_FOREACH (ID p, t) atom.tuple.push_back(p);
							ID atomID = query.interpretation->getRegistry()->storeOrdinaryGAtom(atom);
							DBGLOG(DBG, "Input atom: " << atomID);
// && query.interpretation->getFact(atomID.address)
							if (query.eatom->getPredicateInputMask()->getFact(atomID.address)){
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
		for (int i = 0; i < atomicQueries.size(); ++i){
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
	for (int par = 0; par < inputType.size(); ++par){
		if (getInputType(par) == PREDICATE && isIndependentOfPredicateParameterName(prop, par)){
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
			for (int i = 1; i <= inputType.size(); ++i){
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

/*
return;

	// for all related auxiliaries
	BOOST_FOREACH (ID auxID, auxes){
		DBGLOG(DBG, "Matching auxID=" << auxID << " (#parameters=" << inputType.size() << ")");
		const OrdinaryAtom& aux = ctx->registry()->ogatoms.getByAddress(auxID.address);

		// find a translation of the tuple of pattern to the tuple of aux
		std::map<ID, std::vector<ID> > translation;
		for (int p = 0; p < inputType.size(); ++p){
			if (getInputType(p) == PREDICATE){
				DBGLOG(DBG, pattern.tuple[p + 1] << " --> " << aux.tuple[p + 1]);
				translation[pattern.tuple[p + 1]].push_back(aux.tuple[p + 1]);
			}
		}

		// translate the nogood
		Nogood translatedNG;
		BOOST_FOREACH (ID lID, ng){
			const OrdinaryAtom& l = ctx->registry()->ogatoms.getByAddress(lID.address);
			if (lID != patternID){
				assert(translation.find(l.tuple[0]) != translation.end());
				for (int i = 0; i < translation[l.tuple[0]].size(); ++i){
					OrdinaryAtom t = l;
					t.tuple[0] = translation[l.tuple[0]][i];
					translatedNG.insert(NogoodContainer::createLiteral(ctx->registry()->storeOrdinaryGAtom(t).address, !lID.isNaf()));
				}
			}else{
				OrdinaryAtom t = l;
				for (int i = 1; i <= inputType.size(); ++i){
					if (getInputType(i - 1) == PREDICATE){
						t.tuple[i] = aux.tuple[i];
					}else{
						t.tuple[i] = pattern.tuple[i];
					}
				}
				translatedNG.insert(NogoodContainer::createLiteral(ctx->registry()->storeOrdinaryGAtom(t).address, !lID.isNaf()));
			}
		}

		// store the translated nogood
		DBGLOG(DBG, "Adding generalized nogood " << translatedNG.getStringRepresentation(ctx->registry()) << " (from " << ng.getStringRepresentation(ctx->registry()) << ")");
		nogoods->addNogood(translatedNG);
	}
*/
}

void PluginAtom::learnFromInputOutputBehavior(ProgramCtx* ctx, NogoodContainerPtr nogoods, const Query& query, const ExtSourceProperties& prop, const Answer& answer){

	if (ctx != 0 && nogoods){

		if (ctx->config.getOption("ExternalLearningIOBehavior")){
			DBGLOG(DBG, "External Learning: IOBehavior" << (ctx->config.getOption("ExternalLearningMonotonicity") ? " by exploiting monotonicity" : ""));

			Nogood extNgInput = getInputNogood(ctx, nogoods, query, prop);

			Set<ID> out = getOutputAtoms(ctx, nogoods, query, answer, false);
			BOOST_FOREACH (ID oid, out){
				Nogood extNg = extNgInput;
				extNg.insert(oid);
				DBGLOG(DBG, "Learned nogood " << extNg.getStringRepresentation(ctx->registry()) << " from input-output behavior");
				nogoods->addNogood(extNg);
			}
		}
	}
}

void PluginAtom::learnFromFunctionality(ProgramCtx* ctx, NogoodContainerPtr nogoods, const Query& query, const ExtSourceProperties& prop, const Answer& answer){

	if (ctx != 0 && nogoods){

		if (ctx->config.getOption("ExternalLearningFunctionality") && isFunctional(prop)){
			DBGLOG(DBG, "External Learning: Functionality");

			// there is a unique output
			const std::vector<Tuple>& otuples = answer.get();

			if (otuples.size() > 0){
				ID uniqueOut = getOutputAtom(ctx, nogoods, query, otuples[0], true);

				// go through all output tuples which have been generated so far
				BOOST_FOREACH (Tuple t, this->otuples){
					ID id = getOutputAtom(ctx, nogoods, query, t, true);
					if (id != uniqueOut){
						Nogood excludeOthers;
						excludeOthers.insert(uniqueOut);
						excludeOthers.insert(id);
						DBGLOG(DBG, "Learned nogood " << excludeOthers.getStringRepresentation(ctx->registry()) << " from functionality");
						nogoods->addNogood(excludeOthers);
					}
				}
			}
		}
	}
}

void PluginAtom::learnFromGroundRule(ProgramCtx* ctx, NogoodContainerPtr nogoods, const Query& query, ID groundRule){

	RegistryPtr reg = ctx->registry();

	if (ctx != 0 && nogoods){
		DBGLOG(DBG, "External Learning: Ground Rule");

		const Rule& rule = ctx->registry()->rules.getByID(groundRule);

		Nogood ng;
		BOOST_FOREACH (ID hId, rule.head){
			const OrdinaryAtom& oat = ctx->registry()->ogatoms.getByID(hId);
			Tuple t;
			t.insert(t.end(), oat.tuple.begin() + 1, oat.tuple.end());
			if (reg->terms.getByID(oat.tuple[0]).getUnquotedString() == "out"){
				// output atom is positive, i.e. it must not be false
				ng.insert(getOutputAtom(ctx, nogoods, query, t, false));
			}else{
				// output atom is negative, i.e. it must not be true
				ng.insert(getOutputAtom(ctx, nogoods, query, t, true));
			}
		}
		BOOST_FOREACH (ID bId, rule.body){
			ng.insert(bId);
		}
		DBGLOG(DBG, "Learned nogood " << ng.getStringRepresentation(ctx->registry()) << " from rule");
		nogoods->addNogood(ng);
	}
}

void PluginAtom::learnFromRule(ProgramCtx* ctx, NogoodContainerPtr nogoods, const Query& query, ID rid){

	if (ctx != 0 && nogoods){
		DBGLOG(DBG, "External Learning: Rule");

		// prepare map for replacing body predicates:
		// "in[i+1]" is replaced by the predicate passed as parameter number "i"
		std::map<ID, ID> predReplacementMap;
		for (int p = 0; p < query.input.size(); p++){
			std::stringstream inPredStr;
			inPredStr << "in" << (p + 1);
			Term inPredTerm(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, inPredStr.str());
			ID inPredID = ctx->registry()->storeTerm(inPredTerm);
			predReplacementMap[inPredID] = query.input[p];
		}

		DBGLOG(DBG, "Rewriting rule");
		const Rule& rule = ctx->registry()->rules.getByID(rid);

		Rule rrule(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);
		rrule.head = rule.head;
		BOOST_FOREACH (ID batom, rule.body){

			const OrdinaryAtom& oatom = batom.isOrdinaryGroundAtom() ? ctx->registry()->ogatoms.getByID(batom) : ctx->registry()->onatoms.getByID(batom);

			// replace predicate name by parameter from query.input
			OrdinaryAtom roatom = oatom;
			bool found = false;
			for (int inp = 0; inp < query.input.size(); inp++){
				std::stringstream inPredStr;
				inPredStr << "in" << (inp + 1);
				Term inPredTerm(ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT, inPredStr.str());
				ID inPredID = ctx->registry()->storeTerm(inPredTerm);

				if (roatom.tuple[0] == inPredID){
					roatom.tuple[0] = query.input[inp];
					found = true;
					break;
				}
			}
			assert(found);

			ID batomId = batom.isOrdinaryGroundAtom() ? ctx->registry()->storeOrdinaryGAtom(roatom) : ctx->registry()->storeOrdinaryNAtom(roatom);
			ID mask(ID::MAINKIND_LITERAL, 0);
			if (batom.isNaf()) mask = mask | ID(ID::NAF_MASK, 0);
			if (batom.isOrdinaryGroundAtom()) mask = mask | ID(ID::SUBKIND_ATOM_ORDINARYG, 0);
			if (batom.isOrdinaryNongroundAtom()) mask = mask | ID(ID::SUBKIND_ATOM_ORDINARYN, 0);
			rrule.body.push_back(ID(mask.kind, batomId.address));
		}
		ID rruleId = ctx->registry()->storeRule(rrule);

		DBGLOG(DBG, "Building ASP Program");
		InterpretationConstPtr edb = query.interpretation;
		std::vector<ID> idb;
		idb.push_back(rruleId);
		OrdinaryASPProgram program(ctx->registry(), idb, edb);

		DBGLOG(DBG, "Grounding learning rule");
		GenuineGrounderPtr grounder = GenuineGrounderPtr(new InternalGrounder(*ctx, program, InternalGrounder::builtin));
		const OrdinaryASPProgram& gprogram = grounder->getGroundProgram();
//		InternalGrounderPtr ig = InternalGrounderPtr(new InternalGrounder(*ctx, program, InternalGrounder::builtin));
//		OrdinaryASPProgram gprogram = ig->getGroundProgram();

		DBGLOG(DBG, "Generating nogoods for all ground rules");
		BOOST_FOREACH (ID rid, gprogram.idb){
			learnFromGroundRule(ctx, nogoods, query, rid);
		}
	}
}

Nogood PluginAtom::getInputNogood(ProgramCtx* ctx, NogoodContainerPtr nogoods, const Query& query, const ExtSourceProperties& prop, bool negateMonotonicity){

	// find relevant input: by default, the predicate mask of the external source counts; this can however be overridden for queries
	bm::bvector<>::enumerator en = query.predicateInputMask == InterpretationPtr() ? query.eatom->getPredicateInputMask()->getStorage().first() : query.predicateInputMask->getStorage().first();
	bm::bvector<>::enumerator en_end = query.predicateInputMask == InterpretationPtr() ? query.eatom->getPredicateInputMask()->getStorage().end() : query.predicateInputMask->getStorage().end();

	Nogood extNgInput;

	while (en < en_end){
		// get the predicate of the current input atom
		ID pred = query.interpretation->getRegistry()->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en)).tuple[0];

		// find the parameter index of this atom
		int index = query.inputPredicateTable.find(pred)->second;

		// positive atoms are only required for non-antimonotonic input parameters
		// negative atoms are only required for non-monotonic input parameters
		if (query.interpretation->getFact(*en) != negateMonotonicity){
			// positive
			if (!isAntimonotonic(prop, index) || !ctx->config.getOption("ExternalLearningMonotonicity")){
				extNgInput.insert(nogoods->createLiteral(*en, query.interpretation->getFact(*en)));
			}
		}else{
			// negative
			if (!isMonotonic(prop, index) || !ctx->config.getOption("ExternalLearningMonotonicity")){
				extNgInput.insert(nogoods->createLiteral(*en, query.interpretation->getFact(*en)));
			}
		}

		en++;
	}

	return extNgInput;
}

Set<ID> PluginAtom::getOutputAtoms(ProgramCtx* ctx, NogoodContainerPtr nogoods, const Query& query, const Answer& answer, bool sign){

	Set<ID> out;

	// construct replacement atom
	OrdinaryAtom replacement(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX | ID::PROPERTY_EXTERNALAUX);
	replacement.tuple.resize(1);
	replacement.tuple[0] = registry->getAuxiliaryConstantSymbol(sign ? 'r' : 'n', query.eatom->predicate);
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
		ID idreplacement = nogoods->createLiteral(registry->storeOrdinaryGAtom(replacement));
		out.insert(idreplacement);
	}

	return out;
}

ID PluginAtom::getOutputAtom(ProgramCtx* ctx, NogoodContainerPtr nogoods, const Query& query, Tuple otuple, bool sign){

	// construct replacement atom with input from query
	OrdinaryAtom replacement(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX | ID::PROPERTY_EXTERNALAUX);
	replacement.tuple.resize(1);
	replacement.tuple[0] = registry->getAuxiliaryConstantSymbol(sign ? 'r' : 'n', query.eatom->predicate);
	replacement.tuple.insert(replacement.tuple.end(), query.input.begin(), query.input.end());
	int s = replacement.tuple.size();

	// add output tuple
	replacement.tuple.insert(replacement.tuple.end(), otuple.begin(), otuple.end());

	ID idreplacement = nogoods->createLiteral(registry->storeOrdinaryGAtom(replacement));
	return idreplacement;
}

ID PluginAtom::getIDOfLearningRule(ProgramCtx* ctx, std::string learningrule){

	RegistryPtr reg = ctx->registry();

	// parse rule
	DBGLOG(DBG, "Parsing learning rule " << learningrule);
	InputProviderPtr ip(new InputProvider());
	ip->addStringInput(learningrule, "rule");
//	Logger::Levels l = Logger::Instance().getPrintLevels();	// workaround: verbose causes the parse call below to fail (registry pointer is 0)
//	Logger::Instance().setPrintLevels(0);
	ProgramCtx pc = *ctx;
//	pc.changeRegistry(ctx->registry());
	pc.edb->getStorage().clear();
	pc.idb.clear();
	ModuleHexParser hp;
	hp.parse(ip, pc);
//	Logger::Instance().setPrintLevels(l);

	if(pc.edb->getStorage().count() > 0){
		DBGLOG(DBG, "Learning Rule Error: Learning rule must not be a fact");
		return ID_FAIL;
	}else if (pc.idb.size() != 1){
		DBGLOG(DBG, "Error: Got " << pc.idb.size() << " rules; must be 1");
		return ID_FAIL;
	}else{
		DBGLOG(DBG, "Got 1 learning rule");
		ID rid = pc.idb[0];
		const Rule& r = reg->rules.getByID(rid);

		// learning rules must not be constraints or disjunctive
		if (r.head.size() != 1){
			DBGLOG(DBG, "Learning Rule Error: Learning rule is not ordinary (head size must be 1)");
			return ID_FAIL;
		}

		// learning rules must use only predicates "out" or "nout" (in head) and in[i] (in body)
		BOOST_FOREACH (ID hLit, r.head){
			const OrdinaryAtom& oatom = hLit.isOrdinaryGroundAtom() ? reg->ogatoms.getByID(hLit) : reg->onatoms.getByID(hLit);
			std::string hPred = reg->terms.getByID(oatom.tuple[0]).getUnquotedString();
			if (hPred != "out" && hPred != "nout"){
				DBGLOG(DBG, "Learning Rule Error: Head predicate of learning rule must be \"out\" or \"nout\"");
				return ID_FAIL;
			}
		}
		BOOST_FOREACH (ID bLit, r.body){
			const OrdinaryAtom& oatom = bLit.isOrdinaryGroundAtom() ? reg->ogatoms.getByID(bLit) : reg->onatoms.getByID(bLit);
			std::string bPred = reg->terms.getByID(oatom.tuple[0]).getUnquotedString();

			try {    
				if (boost::starts_with(bPred, "in")){
					boost::lexical_cast<int>(bPred.c_str() + 2);
				}else{
					throw std::bad_cast();
				}
			} catch (std::bad_cast){
				DBGLOG(DBG, "Learning Rule Error: Body predicates must be of kind \"in[integer]\"");
				return ID_FAIL;
			}
		}

		return rid;
	}
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
