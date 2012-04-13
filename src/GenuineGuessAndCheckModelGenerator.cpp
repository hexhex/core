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
 * @file GenuineGuessAndCheckModelGenerator.cpp
 * @author Christoph Redl
 *
 * @brief Implementation of the model generator for "GuessAndCheck" components.
 */

#define DLVHEX_BENCHMARK

#include "dlvhex2/GenuineGuessAndCheckModelGenerator.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/ASPSolver.h"
#include "dlvhex2/ASPSolverManager.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/InternalGroundDASPSolver.h"

#include <bm/bmalgo.h>

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
 * for one eval unit, we transform the rules (idb) independent of input
 * interpretations as follows:
 * * replace all external atoms with eatom replacements
 *   -> "xidb" (like in other model generators)
 * * create for each inner eatom a guessing rule for grounding and guessing
 *   eatoms
 *   -> "gidb"
 * * create for each rule in xidb a rule with same body and individual
 *   flp auxiliary head containing all variables in the rule
 *   (constraints can stay untouched)
 *   -> "xidbflphead"
 * * create for each rule in xidb a rule with body extended by respective
 *   flp auxiliary predicate containing all variables
 *   -> "xidbflpbody"
 *
 * evaluation works as follows:
 * * evaluate outer eatoms -> yields eedb replacements in interpretation
 * * evaluate edb + eedb + xidb + gidb -> yields guesses M_1,...,M_n
 * * check for each guess M
 *   * whether eatoms have been guessed correctly (remove others)
 *   * whether M is model of FLP reduct of xidb wrt edb, eedb and M
 *     this check is achieved by doing the following
 *     * evaluate edb + eedb + xidbflphead + M
 *       -> yields singleton answer set containing flp heads F for non-blocked rules
 *       (if there is no result answer set, some constraint fired and M can be discarded)
 *     * evaluate edb + eedb + xidbflpbody + (M \cap guess_auxiliaries) + F
 *       -> yields singleton answer set M'
 *       (there must be an answer set, or something went wrong)
 *     * if (M' \setminus F) == M then M is a model of the FLP reduct
 *       -> store as candidate
 * * drop non-subset-minimal candidates
 * * return remaining candidates as minimal models
 *   (this means, that for one input, all models have to be calculated
 *    before the first one can be returned due to the minimality check)
 */
/*
		// add minimality rules to flpbody program
		std::vector<ID> minimalityProgram = createMinimalityProgram();
		BOOST_FOREACH (ID rid, factory.gidb){
			simulatedReduct.push_back(rid);
		}
*/

GenuineGuessAndCheckModelGeneratorFactory::GenuineGuessAndCheckModelGeneratorFactory(
    ProgramCtx& ctx,
    const ComponentInfo& ci,
    ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig):
  FLPModelGeneratorFactoryBase(ctx.registry()),
  externalEvalConfig(externalEvalConfig),
  ctx(ctx),
  ci(ci),
  outerEatoms(ci.outerEatoms)
{
  // this model generator can handle any components
  // (and there is quite some room for more optimization)

  // copy rules and constraints to idb
  // TODO we do not really need this except for debugging (tiny optimization possibility)
  idb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
  idb.insert(idb.end(), ci.innerRules.begin(), ci.innerRules.end());
  idb.insert(idb.end(), ci.innerConstraints.begin(), ci.innerConstraints.end());

  innerEatoms = ci.innerEatoms;
  // create guessing rules "gidb" for innerEatoms in all inner rules and constraints
  createEatomGuessingRules();

  // transform original innerRules and innerConstraints to xidb with only auxiliaries
  xidb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
  std::back_insert_iterator<std::vector<ID> > inserter(xidb);
  std::transform(ci.innerRules.begin(), ci.innerRules.end(),
      inserter, boost::bind(&GenuineGuessAndCheckModelGeneratorFactory::convertRule, this, reg, _1));
  std::transform(ci.innerConstraints.begin(), ci.innerConstraints.end(),
      inserter, boost::bind(&GenuineGuessAndCheckModelGeneratorFactory::convertRule, this, reg, _1));

  // transform xidb for flp calculation
  createFLPRules();

  DBGLOG(DBG,"GenuineGuessAndCheckModelGeneratorFactory():");
  #ifndef NDEBUG
  {
    DBGLOG_INDENT(DBG);
    // verbose output
    std::stringstream s;
    print(s, true);
    DBGLOG(DBG,s.str());
  }
  #endif
}

GenuineGuessAndCheckModelGeneratorFactory::ModelGeneratorPtr
GenuineGuessAndCheckModelGeneratorFactory::createModelGenerator(
    InterpretationConstPtr input)
{ 
  return ModelGeneratorPtr(new GenuineGuessAndCheckModelGenerator(*this, input));
}

std::ostream& GenuineGuessAndCheckModelGeneratorFactory::print(
    std::ostream& o) const
{
  return print(o, false);
}

std::ostream& GenuineGuessAndCheckModelGeneratorFactory::print(
    std::ostream& o, bool verbose) const
{
  // item separator
  std::string isep(" ");
  // group separator
  std::string gsep(" ");
  if( verbose )
  {
    isep = "\n";
    gsep = "\n";
  }
  RawPrinter printer(o, ctx.registry());
  if( !outerEatoms.empty() )
  {
    o << "outer Eatoms={" << gsep;
    printer.printmany(outerEatoms,isep);
    o << gsep << "}" << gsep;
  }
  if( !innerEatoms.empty() )
  {
    o << "inner Eatoms={" << gsep;
    printer.printmany(innerEatoms,isep);
    o << gsep << "}" << gsep;
  }
  if( !gidb.empty() )
  {
    o << "gidb={" << gsep;
    printer.printmany(gidb,isep);
    o << gsep << "}" << gsep;
  }
  if( !idb.empty() )
  {
    o << "idb={" << gsep;
    printer.printmany(idb,isep);
    o << gsep << "}" << gsep;
  }
  if( !xidb.empty() )
  {
    o << "xidb={" << gsep;
    printer.printmany(xidb,isep);
    o << gsep << "}" << gsep;
  }
  if( !xidbflphead.empty() )
  {
    o << "xidbflphead={" << gsep;
    printer.printmany(xidbflphead,isep);
    o << gsep << "}" << gsep;
  }
  if( !xidbflpbody.empty() )
  {
    o << "xidbflpbody={" << gsep;
    printer.printmany(xidbflpbody,isep);
    o << gsep << "}" << gsep;
  }
  return o;
}

//
// the model generator
//

GenuineGuessAndCheckModelGenerator::GenuineGuessAndCheckModelGenerator(
    Factory& factory,
    InterpretationConstPtr input):
  FLPModelGeneratorBase(factory, input),
  factory(factory)
{
    DBGLOG(DBG, "Genuine GnC-ModelGenerator is instantiated for a " << (factory.ci.disjunctiveHeads ? "" : "non-") << "disjunctive component");

    RegistryPtr reg = factory.reg;

    // create new interpretation as copy
    InterpretationPtr postprocInput;
    if( input == 0 )
    {
      // empty construction
      postprocInput.reset(new Interpretation(reg));
    }
    else
    {
      // copy construction
      postprocInput.reset(new Interpretation(*input));
    }

    // augment input with edb
    #warning perhaps we can pass multiple partially preprocessed input edb's to the external solver and save a lot of processing here
    postprocInput->add(*factory.ctx.edb);

    // remember which facts we must remove
    mask.reset(new Interpretation(*postprocInput));

    // manage outer external atoms
    if( !factory.outerEatoms.empty() )
    {
      // augment input with result of external atom evaluation
      // use newint as input and as output interpretation
      IntegrateExternalAnswerIntoInterpretationCB cb(postprocInput);
      evaluateExternalAtoms(reg,
          factory.outerEatoms, postprocInput, cb);
      DLVHEX_BENCHMARK_REGISTER(sidcountexternalatomcomps,
          "outer external atom computations");
      DLVHEX_BENCHMARK_COUNT(sidcountexternalatomcomps,1);

      assert(!factory.xidb.empty() &&
          "the guess and check model generator is not required for "
          "non-idb components! (use plain)");
    }

    // assign to const member -> this value must stay the same from here on!
    postprocessedInput = postprocInput;

    // evaluate edb+xidb+gidb
    {
	DBGLOG(DBG,"evaluating guessing program");
	// no mask
	OrdinaryASPProgram program(reg, factory.xidb, postprocessedInput, factory.ctx.maxint);
	// append gidb to xidb
	program.idb.insert(program.idb.end(), factory.gidb.begin(), factory.gidb.end());

//	grounder = InternalGrounderPtr(new InternalGrounder(factory.ctx, program));
//	if (factory.ctx.config.getOption("Instantiate")){
//		std::cout << "% Component " << &(factory.ci) << std::endl;
//		std::cout << grounder->getGroundProgramString();
//	}

//	OrdinaryASPProgram gprogram = grounder->getGroundProgram();
//	igas = InternalGroundDASPSolverPtr(new InternalGroundDASPSolver(factory.ctx, gprogram));
	solver = GenuineSolver::getInstance(factory.ctx, program);
	factory.ctx.globalNogoods.addNogoodListener(solver);
	if (factory.ctx.config.getOption("ExternalLearningPartial") /* && false partial learning is currently not thread safe with clasp */){
		solver->addExternalLearner(this);
	}

	firstLearnCall = true;
    }
}

GenuineGuessAndCheckModelGenerator::~GenuineGuessAndCheckModelGenerator(){
	factory.ctx.globalNogoods.removeNogoodListener(solver);
	if (factory.ctx.config.getOption("ExternalLearningPartial")){
		solver->removeExternalLearner(this);
	}
	DBGLOG(DBG, "Final Statistics:" << std::endl << solver->getStatistics());
}

// generate and return next model, return null after last model
// see description of algorithm on top of this file
InterpretationPtr GenuineGuessAndCheckModelGenerator::generateNextModel()
{
  assert(!!solver && "solver must be set here, it is set in constructor");

	// for non-disjunctive components, generate one model and return it
	// for disjunctive components, generate all modes, do minimality check, and return one model
	//
	// !! disjunctive heads is not a sufficient criterion to make the distinction!
	// Consider the program: p(X) :- &ext[p](X), dom(X).
	// with the following definition of ext:
	//	{} -> {}
	//	{a} -> {a}
	// Then {a} is a compatible model but no answer set.
	// @TODO: find another criterion which allows for skipping the minimality check.
	if (factory.ctx.config.getOption("MinCheck")){
		DBGLOG(DBG, "Solving component with minimality check by GnC Model Generator");

		if( currentResults == 0 )
		{
			// Generate all compatible models
			InterpretationPtr cm;
			while ((cm = generateNextCompatibleModel()) != InterpretationPtr()){
				candidates.push_back(cm);
			} 

			// minimality check
			DBGLOG(DBG, "Doing minimality check");
			typedef std::list<InterpretationPtr> CandidateList;
			std::set<InterpretationPtr> erase;
			CandidateList::iterator it;
			for(it = candidates.begin(); it != candidates.end(); ++it)
			{
				DBGLOG(DBG,"checking with " << **it);
				for(CandidateList::iterator itv = candidates.begin();
				    itv != candidates.end(); ++itv)
				{
					// do not check against self
					if( itv == it ) continue;

					// (do not check against those already invalidated)
					if( erase.find(*itv) != erase.end() ) continue;

					DBGLOG(DBG,"  does it invalidate " << **itv << "?");

					// any_sub(b1, b2) checks if there is any bit in the bitset obtained by 'b1 - b2'
					// if this is not the case, we know that 'b1 \subseteq b2'
					if( !bm::any_sub( (*it)->getStorage(), (*itv)->getStorage() ) )
					{
						DBGLOG(DBG,"  yes it invalidates!");
						erase.insert(*itv);
					}
				}
			}
			// now all that must be erased are in set 'erase'

			DBGLOG(DBG,"minimal models are:");
			PreparedResults* pr = new PreparedResults;
			currentResults.reset(pr);
			BOOST_FOREACH(InterpretationPtr mdl, candidates)
			{
				if( erase.find(mdl) == erase.end() )
				{
					DBGLOG(DBG,"  " << *mdl);
					pr->add(AnswerSetPtr(new AnswerSet(mdl)));
				}
			}
		}

		assert(currentResults != 0);
		AnswerSet::Ptr ret = currentResults->getNextAnswerSet();
		if( ret == 0 )
		{
			currentResults.reset();
			return InterpretationPtr();
		}
		DLVHEX_BENCHMARK_REGISTER(sidcountgcanswersets,
		"GenuineGuessAndCheckMG answer sets");
		DLVHEX_BENCHMARK_COUNT(sidcountgcanswersets,1);

		return ret->interpretation;
	}else{
		DBGLOG(DBG, "Solving component without minimality check by GnC Model Generator");

		return generateNextCompatibleModel();
	}
}

InterpretationPtr GenuineGuessAndCheckModelGenerator::generateNextCompatibleModel()
{
	RegistryPtr reg = factory.reg;

	// now we have postprocessed input in postprocessedInput
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidgcsolve, "guess and check loop");

	InterpretationPtr modelCandidate;
	do
	{
		modelCandidate = solver->projectToOrdinaryAtoms(solver->getNextModel());
		DBGLOG(DBG, "Statistics:" << std::endl << solver->getStatistics());
		if( !modelCandidate ) return modelCandidate;

		DBGLOG_SCOPE(DBG,"gM", false);
		DBGLOG(DBG,"= got guess model " << *modelCandidate);

		DBGLOG(DBG, "doing compatibility check for model candidate " << *modelCandidate);
		bool compatible = isCompatibleSet(
		modelCandidate, postprocessedInput, factory.ctx,
		factory.ctx.config.getOption("ExternalLearning") ? solver : GenuineSolverPtr());
		DBGLOG(DBG, "Compatible: " << compatible);
		if (!compatible) continue;

		// FLP check
		if (factory.ctx.config.getOption("FLPCheck")){
			DBGLOG(DBG, "FLP Check");
			if( !isSubsetMinimalFLPModel<GenuineSolver>(modelCandidate, postprocessedInput, factory.ctx, solver) )
        			continue;
		}else{
			DBGLOG(DBG, "Skipping FLP Check");
		}

		// UFS check
		if (factory.ctx.config.getOption("UFSCheck")){
			DBGLOG(DBG, "UFS Check");
			std::vector<IDAddress> ufs = getUnfoundedSet(factory.ctx, solver->getGroundProgram(), modelCandidate);
			if (ufs.size() > 0){
				DBGLOG(DBG, "Got a UFS");
				if (factory.ctx.config.getOption("UFSLearning")){
					DBGLOG(DBG, "Learn from UFS");
					Nogood ufsng = getUFSNogood(factory.ctx, ufs, solver->getGroundProgram(), modelCandidate);
					solver->addNogood(ufsng);
				}
				continue;
			}
		}else{
			DBGLOG(DBG, "Skipping FLP Check");
		}

		// remove edb and the guess (from here we don't need the guess anymore)
		modelCandidate->getStorage() -= factory.gpMask.mask()->getStorage();
		modelCandidate->getStorage() -= factory.gnMask.mask()->getStorage();

		modelCandidate->getStorage() -= mask->getStorage();

		DBGLOG(DBG,"= final model candidate " << *modelCandidate);
		return modelCandidate;
	}while(true);
}

bool GenuineGuessAndCheckModelGenerator::learn(Interpretation::Ptr partialInterpretation, const bm::bvector<>& factWasSet, const bm::bvector<>& changed){

	RegistryPtr reg = factory.reg;

	// go through all external atoms
	bool learned = false;
	BOOST_FOREACH(ID eatomid, factory.innerEatoms)
	{
		const ExternalAtom& eatom = reg->eatoms.getByID(eatomid);
		eatom.updatePredicateInputMask();

		// check if input for external atom is complete
		DBGLOG(DBG, "Checking if input for " << eatom << " is complete");

#ifndef NDEBUG
		bm::bvector<>::enumerator en = factWasSet.first();
		bm::bvector<>::enumerator en_end = factWasSet.end();
		std::stringstream ss;
		ss << "Available input: { ";
		bool first = true;
		while (en < en_end){
			if (!first) ss << ", ";
			ss << *en;
			first = false;
			en++;
		}
		ss << " }";

		en = eatom.getPredicateInputMask()->getStorage().first();
		eatom.getPredicateInputMask()->getStorage().end();
		ss << std::endl << "Needed input: { ";
		first = true;
		while (en < en_end){
			if (!first) ss << ", ";
			ss << *en;
			first = false;
			en++;
		}
		ss << " }";
		DBGLOG(DBG, ss.str());
#endif
		/*

		// TODO: The if block below checks if the predicate input to the external atom changed, but not the aux input.
		//       If the predicate input changed, the external atom is reevaluated, otherwise it is not.
		//       As the aux input is not considered, we might miss some changes in the input and do not evaluate the external atom for learning, even though we could learn something new.
		//       This loop detects whether the aux input changed. However, the computational overhead is big. This needs to be considered when the heuristic is developed.

		bool auxChanged = false;

		  dlvhex::OrdinaryAtomTable::PredicateIterator it, it_end;
		  for(boost::tie(it, it_end) = reg->ogatoms.getRangeByPredicateID(eatom.auxInputPredicate); it != it_end; ++it)
		  {
		    const dlvhex::OrdinaryAtom& oatom = *it;
		    ID idoatom = reg->ogatoms.getIDByStorage(oatom);
		    if (changed.get_bit(idoatom.address) != 0) auxChanged = true;
		  }

		*/

		if ((eatom.getPredicateInputMask()->getStorage() & factWasSet).count() == eatom.getPredicateInputMask()->getStorage().count()){
			DBGLOG(DBG, "Input is complete");

			// check if at least one input fact changed (otherwise a reevaluation is pointless)
			if (/* auxChanged || */ firstLearnCall || (eatom.getPredicateInputMask()->getStorage() & changed).count() > 0){
// eatom.getPredicateInputMask()->getStorage().count() == 0
				DBGLOG(DBG, "Evaluating external atom");

				InterpretationPtr eaResult(new Interpretation(reg));
				IntegrateExternalAnswerIntoInterpretationCB intcb(eaResult);
				int i = solver->getNogoodCount();
				evaluateExternalAtom(reg, eatom, partialInterpretation, intcb, &factory.ctx, factory.ctx.config.getOption("ExternalLearning") ? solver : NogoodContainerPtr());
				DBGLOG(DBG, "Output has size " << eaResult->getStorage().count());
				if (solver->getNogoodCount() != i) learned = true;
			}else{
				DBGLOG(DBG, "Do not evaluate external atom because input did not change");
			}
		}else{
			DBGLOG(DBG, "Input is not complete");
		}
	}

	firstLearnCall = false;
	return learned;
}

DLVHEX_NAMESPACE_END
