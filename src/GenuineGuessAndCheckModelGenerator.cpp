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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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
#include "dlvhex2/UnfoundedSetChecker.h"

#include <bm/bmalgo.h>

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

GenuineGuessAndCheckModelGeneratorFactory::GenuineGuessAndCheckModelGeneratorFactory(
    ProgramCtx& ctx,
    const ComponentInfo& ci,
    ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig):
  FLPModelGeneratorFactoryBase(ctx),
  externalEvalConfig(externalEvalConfig),
  ctx(ctx),
  ci(ci),
  outerEatoms(ci.outerEatoms)
{
  // this model generator can handle any components
  // (and there is quite some room for more optimization)

  // create program for domain exploration
  if (ctx.config.getOption("LiberalSafety")){
    std::vector<ID> deidb;
    deidb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
    deidb.insert(deidb.end(), ci.innerRules.begin(), ci.innerRules.end());
    deidb.insert(deidb.end(), ci.innerConstraints.begin(), ci.innerConstraints.end());
    createDomainExplorationProgram(ci, ctx, deidb);

    // add domain predicates
    idb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
    std::back_insert_iterator<std::vector<ID> > dinserter(idb);
    std::transform(ci.innerRules.begin(), ci.innerRules.end(),
        dinserter, boost::bind(&GenuineGuessAndCheckModelGeneratorFactory::addDomainPredicatesWhereNecessary, this, ci, reg, _1));
    std::transform(ci.innerConstraints.begin(), ci.innerConstraints.end(),
        dinserter, boost::bind(&GenuineGuessAndCheckModelGeneratorFactory::addDomainPredicatesWhereNecessary, this, ci, reg, _1));
  }else{
    // copy rules and constraints to idb
    // TODO we do not really need this except for debugging (tiny optimization possibility)
    idb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
    idb.insert(idb.end(), ci.innerRules.begin(), ci.innerRules.end());
    idb.insert(idb.end(), ci.innerConstraints.begin(), ci.innerConstraints.end());
  }

  innerEatoms = ci.innerEatoms;
  // create guessing rules "gidb" for innerEatoms in all inner rules and constraints
  createEatomGuessingRules(ctx);

  // transform original innerRules and innerConstraints to xidb with only auxiliaries
  xidb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
  std::back_insert_iterator<std::vector<ID> > inserter(xidb);
  std::transform(idb.begin(), idb.end(),
      inserter, boost::bind(&GenuineGuessAndCheckModelGeneratorFactory::convertRule, this, ctx, _1));

  // transform xidb for flp calculation
  if (ctx.config.getOption("FLPCheck")) createFLPRules();

  globalLearnedEANogoods = SimpleNogoodContainerPtr(new SimpleNogoodContainer());

  // output rules
  {
    std::ostringstream s;
    print(s, true);
    LOG(DBG,"GenuineGuessAndCheckModelGeneratorFactory(): " << s.str());
  }
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
  factory(factory),
  reg(factory.reg)
{
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidconstruct, "genuine g&c mg constructor");
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
      evaluateExternalAtoms(factory.ctx,
          factory.outerEatoms, postprocInput, cb);
      DLVHEX_BENCHMARK_REGISTER(sidcountexternalatomcomps,
          "outer eatom computations");
      DLVHEX_BENCHMARK_COUNT(sidcountexternalatomcomps,1);

      assert(!factory.xidb.empty() &&
          "the guess and check model generator is not required for "
          "non-idb components! (use plain)");
    }

    // compute extensions of domain predicates and add it to the input
    if (factory.ctx.config.getOption("LiberalSafety")){
      InterpretationConstPtr domPredictaesExtension = computeExtensionOfDomainPredicates<GenuineSolver>(factory.ci, factory.ctx, postprocInput);
      postprocInput->add(*domPredictaesExtension);
    }

    // assign to const member -> this value must stay the same from here on!
    postprocessedInput = postprocInput;

    // evaluate edb+xidb+gidb
    {
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"genuine g&c init guessprog");
	DBGLOG(DBG,"evaluating guessing program");
	// no mask
	OrdinaryASPProgram program(reg, factory.xidb, postprocessedInput, factory.ctx.maxint);
	// append gidb to xidb
	program.idb.insert(program.idb.end(), factory.gidb.begin(), factory.gidb.end());

	grounder = GenuineGrounder::getInstance(factory.ctx, program);
        annotatedGroundProgram = AnnotatedGroundProgram(factory.ctx, grounder->getGroundProgram(), factory.innerEatoms);
	solver = GenuineGroundSolver::getInstance(
						factory.ctx, annotatedGroundProgram,
						false, // interleaved threading (why here and only here false?)
						!factory.ctx.config.getOption("FLPCheck") && !factory.ctx.config.getOption("UFSCheck")	// do the UFS check for disjunctions only if we don't do
																	// a minimality check in this class;
																	// this will not find unfounded sets due to external sources,
																	// but at least unfounded sets due to disjunctions
						);
	learnedEANogoods = SimpleNogoodContainerPtr(new SimpleNogoodContainer());
	learnedEANogoodsTransferredIndex = 0;
	nogoodGrounder = NogoodGrounderPtr(new ImmediateNogoodGrounder(factory.ctx.registry(), learnedEANogoods, learnedEANogoods, annotatedGroundProgram));

	solver->addPropagator(this);
    }

    setHeuristics();

    // initialize UFS checker
    //   Concerning the last parameter, note that clasp backend uses choice rules for implementing disjunctions:
    //   this must be regarded in UFS checking (see examples/trickyufs.hex)
    ufscm = UnfoundedSetCheckerManagerPtr(new UnfoundedSetCheckerManager(*this, factory.ctx, annotatedGroundProgram, factory.ctx.config.getOption("GenuineSolver") >= 3));

    // overtake nogoods from the factory
    {
      DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"genuine g&c init nogoods");
      for (int i = 0; i < factory.globalLearnedEANogoods->getNogoodCount(); ++i){
        learnedEANogoods->addNogood(factory.globalLearnedEANogoods->getNogood(i));
      }
      updateEANogoods();
    }
}

GenuineGuessAndCheckModelGenerator::~GenuineGuessAndCheckModelGenerator(){
	solver->removePropagator(this);
	DBGLOG(DBG, "Final Statistics:" << std::endl << solver->getStatistics());
}

void GenuineGuessAndCheckModelGenerator::setHeuristics(){

    // set external atom evaluation strategy according to selected heuristics
    for (int i = 0; i < factory.innerEatoms.size(); ++i){
      eaEvaluated.push_back(false);
      eaVerified.push_back(false);

      // watch all atoms in the scope of the external atom for unverification
      bm::bvector<>::enumerator en = annotatedGroundProgram.getEAMask(i)->mask()->getStorage().first();
      bm::bvector<>::enumerator en_end = annotatedGroundProgram.getEAMask(i)->mask()->getStorage().end();
      if (en < en_end){
        // watch one input atom for verification
        verifyWatchList[*en].push_back(i);
      }
    }
    externalAtomEvalHeuristics = factory.ctx.externalAtomEvaluationHeuristicsFactory->createHeuristics(this, reg);

    // create ufs check heuristics as selected
    ufsCheckHeuristics = factory.ctx.unfoundedSetCheckHeuristicsFactory->createHeuristics(this, reg);
}

InterpretationPtr GenuineGuessAndCheckModelGenerator::generateNextModel()
{
	// now we have postprocessed input in postprocessedInput
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidgcsolve, "genuine guess and check loop");

	InterpretationPtr modelCandidate;
	do
	{
		LOG(DBG,"asking for next model");
		modelCandidate = solver->getNextModel();
		DBGLOG(DBG, "Statistics:" << std::endl << solver->getStatistics());
		if( !modelCandidate )
		{
			LOG(DBG,"unsatisfiable -> returning no model");
			return InterpretationPtr();
		}
		DLVHEX_BENCHMARK_REGISTER_AND_COUNT(ssidmodelcandidates, "Candidate compatible sets", 1);

		LOG_SCOPE(DBG,"gM", false);
		LOG(DBG,"got guess model, will do compatibility check on " << *modelCandidate);
		if (!finalCompatibilityCheck(modelCandidate))
		{
			LOG(DBG,"compatibility failed");
			continue;
		}

		DBGLOG(DBG, "Checking if model candidate is a model");
		if (!isModel(modelCandidate))
		{
			LOG(DBG,"isModel failed");
			continue;
		}

		// remove edb and the guess (from here we don't need the guess anymore)
		modelCandidate->getStorage() -= factory.gpMask.mask()->getStorage();
		modelCandidate->getStorage() -= factory.gnMask.mask()->getStorage();
		modelCandidate->getStorage() -= mask->getStorage();

		LOG(DBG,"returning model without guess: " << *modelCandidate);
		return modelCandidate;
	}while(true);
}

void GenuineGuessAndCheckModelGenerator::generalizeNogood(Nogood ng){

	if (!ng.isGround()) return;

	DBGLOG(DBG, "Generalizing " << ng.getStringRepresentation(reg));

	// find the external atom related to this nogood
	ID eaid = ID_FAIL;
	BOOST_FOREACH (ID l, ng){
		if (reg->ogatoms.getIDByAddress(l.address).isExternalAuxiliary() && annotatedGroundProgram.mapsAux(l.address)){
			eaid = l;
			break;
		}
	}
	if (eaid == ID_FAIL) return;

	assert(annotatedGroundProgram.getAuxToEA(eaid.address).size() > 0);
	DBGLOG(DBG, "External atom is " << annotatedGroundProgram.getAuxToEA(eaid.address)[0]);
	const ExternalAtom& ea = reg->eatoms.getByID(annotatedGroundProgram.getAuxToEA(eaid.address)[0]);

	// learn related nonground nogoods
	int oldCount = learnedEANogoods->getNogoodCount();
	ea.pluginAtom->generalizeNogood(ng, &factory.ctx, learnedEANogoods);
}

void GenuineGuessAndCheckModelGenerator::updateEANogoods(
	InterpretationConstPtr compatibleSet,
	InterpretationConstPtr factWasSet,
	InterpretationConstPtr changed){

	// generalize ground nogoods to nonground ones
	if (factory.ctx.config.getOption("ExternalLearningGeneralize")){
		int max = learnedEANogoods->getNogoodCount();
		for (int i = learnedEANogoodsTransferredIndex; i < max; ++i){
			generalizeNogood(learnedEANogoods->getNogood(i));
		}
	}

	// instantiate nonground nogoods
	if (factory.ctx.config.getOption("NongroundNogoodInstantiation")){
		nogoodGrounder->update(compatibleSet, factWasSet, changed);
	}

	// transfer nogoods to the solver
	for (int i = learnedEANogoodsTransferredIndex; i < learnedEANogoods->getNogoodCount(); ++i){
		DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sidcompatiblesets, "Learned EA-Nogoods", 1);
		if (factory.ctx.config.getOption("PrintLearnedNogoods")){
			if (factory.ctx.config.getOption("GenuineSolver") >= 3){
				if (i == 0) std::cerr << "( NOTE: With clasp backend, learned nogoods become effective with a delay because of multithreading! )" << std::endl << std::endl;
			}else{
				if (i == 0) std::cerr << "( NOTE: With i-backend, learned nogoods become effective AFTER the next model was printed ! )" << std::endl << std::endl;
			}
		}
		if (learnedEANogoods->getNogood(i).isGround()){
			solver->addNogood(learnedEANogoods->getNogood(i));
		}else{
			// keep nonground nogoods beyond the lifespan of this model generator
			factory.globalLearnedEANogoods->addNogood(learnedEANogoods->getNogood(i));
		}
	}

	// for encoding-based UFS checkers and explicit FLP checks, we need to keep learned nogoods (otherwise future UFS searches will not be able to use them)
	// for assumption-based UFS checkers we can delete them as soon as nogoods were added both to the main search and to the UFS search
	if (factory.ctx.config.getOption("UFSCheckAssumptionBased") ||
	    (annotatedGroundProgram.hasECycles() == 0 && factory.ctx.config.getOption("FLPDecisionCriterion"))){
		ufscm->learnNogoodsFromMainSearch();
		learnedEANogoods->clear();
	}else{
		learnedEANogoods->forgetLeastFrequentlyAdded();
	}
	learnedEANogoodsTransferredIndex = learnedEANogoods->getNogoodCount();
}

bool GenuineGuessAndCheckModelGenerator::finalCompatibilityCheck(InterpretationConstPtr modelCandidate){

	// did we already verify during model construction or do we have to do the verification now?
	bool compatible;
	int ngCount;

	compatible = true;
	for (int eaIndex = 0; eaIndex < factory.innerEatoms.size(); ++eaIndex){
		if (eaEvaluated[eaIndex] == true && eaVerified[eaIndex] == true){
		}
		if (eaEvaluated[eaIndex] == true && eaVerified[eaIndex] == false){
			DBGLOG(DBG, "External atom " << factory.innerEatoms[eaIndex] << " was evaluated but falsified");
			compatible = false;
			break;
		}
		if (eaEvaluated[eaIndex] == false){
			// try to verify
			DBGLOG(DBG, "External atom " << factory.innerEatoms[eaIndex] << " is not verified, trying to do this now");
			verifyExternalAtom(eaIndex, modelCandidate);
			eaEvaluated[eaIndex] = true;
			DBGLOG(DBG, "Verification result: " << eaVerified[eaIndex]);

			if (eaVerified[eaIndex] == false){
				compatible = false;
				break;
			}
		}
	}
	DBGLOG(DBG, "Compatible: " << compatible);

	return compatible;
}

bool GenuineGuessAndCheckModelGenerator::isModel(InterpretationConstPtr compatibleSet){

	// which semantics?
	if (factory.ctx.config.getOption("WellJustified")){
		// well-justified FLP: fixpoint iteration
		InterpretationPtr fixpoint = welljustifiedSemanticsGetFixpoint(factory.ctx, compatibleSet, grounder->getGroundProgram());
		InterpretationPtr reference = InterpretationPtr(new Interpretation(*compatibleSet));
		factory.gpMask.updateMask();
		factory.gnMask.updateMask();
		reference->getStorage() -= factory.gpMask.mask()->getStorage();
		reference->getStorage() -= factory.gnMask.mask()->getStorage();

		DBGLOG(DBG, "Comparing fixpoint " << *fixpoint << " to reference " << *reference);
		if ((fixpoint->getStorage() & reference->getStorage()).count() == reference->getStorage().count()){
			DBGLOG(DBG, "Well-Justified FLP Semantics: Pass fixpoint test");
			return true;
		}else{
			DBGLOG(DBG, "Well-Justified FLP Semantics: Fail fixpoint test");
			return false;
		}
	}else{
		// FLP: ensure minimality of the compatible set wrt. the reduct (if necessary)
		if (annotatedGroundProgram.hasHeadCycles() == 0 && annotatedGroundProgram.hasECycles() == 0 && factory.ctx.config.getOption("FLPDecisionCriterion")){
			DBGLOG(DBG, "No head- or e-cycles --> No FLP/UFS check necessary");
			return true;
		}else{
			DBGLOG(DBG, "Head- or e-cycles --> FLP/UFS check necessary");

			// Explicit FLP check
			if (factory.ctx.config.getOption("FLPCheck")){
				DBGLOG(DBG, "FLP Check");
				// do FLP check (possibly with nogood learning) and add the learned nogoods to the main search
				bool result = isSubsetMinimalFLPModel<GenuineSolver>(compatibleSet, postprocessedInput, factory.ctx, factory.ctx.config.getOption("ExternalLearning") ? learnedEANogoods : SimpleNogoodContainerPtr());
				updateEANogoods(compatibleSet);
				return result;
			}

			// UFS check
			if (factory.ctx.config.getOption("UFSCheck")){
				DBGLOG(DBG, "UFS Check");
				std::vector<IDAddress> ufs = ufscm->getUnfoundedSet(compatibleSet, std::set<ID>(), factory.ctx.config.getOption("ExternalLearning") ? learnedEANogoods : SimpleNogoodContainerPtr());
				updateEANogoods(compatibleSet);
				if (ufs.size() > 0){
					DBGLOG(DBG, "Got a UFS");
					if (factory.ctx.config.getOption("UFSLearning")){
						DBGLOG(DBG, "Learn from UFS");
						Nogood ufsng = ufscm->getLastUFSNogood();
						solver->addNogood(ufsng);
					}
					return false;
				}else{
					return true;
				}
			}

			// no check
			return true;
		}
	}
}

bool GenuineGuessAndCheckModelGenerator::partialUFSCheck(InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed){
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "genuine g&c partialUFSchk");

	if (!factory.ctx.config.getOption("UFSCheck")) return false;

	// ufs check without nogood learning makes no sense if the interpretation is not complete
	if (factory.ctx.config.getOption("UFSLearning")){

		std::pair<bool, std::set<ID> > decision = ufsCheckHeuristics->doUFSCheck(partialInterpretation, factWasSet, changed);

		if (decision.first){

			DBGLOG(DBG, "Heuristic decides to do an UFS check");
			std::vector<IDAddress> ufs = ufscm->getUnfoundedSet(partialInterpretation, decision.second, factory.ctx.config.getOption("ExternalLearning") ? learnedEANogoods : SimpleNogoodContainerPtr());
			DBGLOG(DBG, "UFS result: " << (ufs.size() == 0 ? "no" : "") << " UFS found (interpretation: " << *partialInterpretation << ", assigned: " << *factWasSet << ")");

			if (ufs.size() > 0){
				Nogood ng = ufscm->getLastUFSNogood();
				DBGLOG(DBG, "Adding UFS nogood: " << ng);
				solver->addNogood(ng);

				// check if nogood is violated
				BOOST_FOREACH (ID l, ng){
					if (!factWasSet->getFact(l.address) || l.isNaf() == partialInterpretation->getFact(l.address)) return false;
				}

				return true;
			}
		}else{
			DBGLOG(DBG, "Heuristic decides not to do an UFS check");
		}
	}

	return false;
}

bool GenuineGuessAndCheckModelGenerator::isVerified(ID eaAux, InterpretationConstPtr factWasSet){

	assert(annotatedGroundProgram.getAuxToEA(eaAux.address).size() > 0);

	// check if at least one of the external atoms which can derive this auxiliary were verified
	BOOST_FOREACH (ID ea, annotatedGroundProgram.getAuxToEA(eaAux.address)){
		int eaIndex = 0;
		while (factory.innerEatoms[eaIndex] != ea) eaIndex++;

		if (eaEvaluated[eaIndex] && eaVerified[eaIndex]){
			DBGLOG(DBG, "Auxiliary " << eaAux.address << " is verified by " << ea);
			return true;
		}
	}
	DBGLOG(DBG, "Auxiliary " << eaAux.address << " is not verified");
	return false;
}

IDAddress GenuineGuessAndCheckModelGenerator::getWatchedLiteral(int eaIndex, InterpretationConstPtr search, bool truthValue){

	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "getWatchedLiteral");
	bm::bvector<>::enumerator eaDepAtoms = annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage().first();
	bm::bvector<>::enumerator eaDepAtoms_end = annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage().end();
	bm::bvector<>::enumerator searchb = search->getStorage().first();
	bm::bvector<>::enumerator searchb_end = search->getStorage().end();

	#if 1
	// go through eamask
	while (eaDepAtoms < eaDepAtoms_end){
		// if search bitset has correct truth value
		if (search->getFact(*eaDepAtoms) == truthValue){
			DBGLOG(DBG, "Found watch " << *eaDepAtoms << " for atom " << factory.innerEatoms[eaIndex]);
			return *eaDepAtoms; // reg->ogatoms.getIDByAddress(*eaDepAtoms);
		}
		eaDepAtoms++;
	}
	#else
	// optimized for truthValue (not a good optimization, as optimization on small eamask is the best we can do, and it is done above) XXX only thing we could improve is a queue of watches so that we have constant time for getWatchedLiteral
	if( truthValue )
	{
	  // looking for the first common bit in eaDepAtoms and searchb
	  while( eaDepAtoms != eaDepAtoms_end && searchb != searchb_end ) {
	    if( *eaDepAtoms == *searchb ) {
	      DBGLOG(DBG, "Found watch " << *eaDepAtoms << " for atom " << factory.innerEatoms[eaIndex]);
	      return *eaDepAtoms; // reg->ogatoms.getIDByAddress(*eaDepAtoms);
	    } else if( *eaDepAtoms < *searchb ) {
	      eaDepAtoms++;
	    } else { assert( *eaDepAtoms > *searchb );
	      searchb++;
	    }
	  }
	} else {
	  // looking for the first bit in eaDepAtoms that is not in searchb
	  while( eaDepAtoms != eaDepAtoms_end && searchb != searchb_end ) {
	    if( *eaDepAtoms == *searchb ) { // we need to find a different pair
	      eaDepAtoms++;
	      searchb++;
	    } else if( *eaDepAtoms < *searchb ) { // found it!
	      DBGLOG(DBG, "Found watch " << *eaDepAtoms << " for atom " << factory.innerEatoms[eaIndex]);
	      return *eaDepAtoms; //reg->ogatoms.getIDByAddress(*eaDepAtoms);
	    } else { assert( *eaDepAtoms > *searchb ); // we found a bit that is in searchb but not in eaDepAtoms
	      searchb++;
	    }
	  }
	}
	#endif

	return ID::ALL_ONES; //ID_FAIL;
}

bool GenuineGuessAndCheckModelGenerator::verifyExternalAtoms(InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed){
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "genuine g&c verifyEAtoms");

	DBGLOG(DBG, "Evaluating External Atoms");

	bm::bvector<>::enumerator en;
	bm::bvector<>::enumerator en_begin = changed->getStorage().first();
	bm::bvector<>::enumerator en_end = changed->getStorage().end();

	{
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "genuine g&c verifyEAtoms wl");

	// for each eatom, if it is evaluated:
	// * look if there is a bit in changed that matches the eatom mask
	// * if yes, unverify eatom and update watches:
	//   * if bit became assigned/defined set watch on unassigned bit that matches eatom mask
	//   * if bit became unassigned/undefined set watch on changed bit that matches eatom mask

	// unverify/unfalsify external atoms which watch this atom
	for(unsigned eaIndex = 0; eaIndex < factory.innerEatoms.size(); ++eaIndex){
	  if( !eaEvaluated[eaIndex] )
	    // we don't need to verify watches because the eatom was not evaluated
	    continue;

	  const InterpretationConstPtr& mask = annotatedGroundProgram.getEAMask(eaIndex)->mask();

	  // check if something in its mask was changed
	  en = en_begin;
	  while (en < en_end){
	    if( mask->getFact(*en) )
	    {
	      // yes, it changed, so we unverify and leave
	      DBGLOG(DBG, "atom " << printToString<RawPrinter>(reg->ogatoms.getIDByAddress(*en), reg) <<
		 " changed and unverified external atom " <<
		 printToString<RawPrinter>(factory.innerEatoms[eaIndex], reg));

	      // unverify
	      eaVerified[eaIndex] = false;
	      eaEvaluated[eaIndex] = false;

	      // new watches
	      if (!factWasSet->getFact(*en)){
		      // watch a yet unassigned atom such that the external atom depends on it
		      IDAddress id = getWatchedLiteral(eaIndex, factWasSet, false);
		      assert(id != ID::ALL_ONES);
		      verifyWatchList[id].push_back(eaIndex);
	      }else{
		      // watch a changed atom
		      IDAddress id = getWatchedLiteral(eaIndex, changed, true);
		      assert(id != ID::ALL_ONES);
		      verifyWatchList[id].push_back(eaIndex);
	      }

	      // leave, we are done with this external atom as we set eaEvaluated to false
	      break;
	    }
	    en++;
	  }
	}
	}

	bool conflict = false;
	en = changed->getStorage().first();
	en_end = changed->getStorage().end();
	while (en < en_end){
		// for all external atoms which watch this atom
		if (factWasSet->getFact(*en)){
			BOOST_FOREACH (int eaIndex, verifyWatchList[*en]){
				if (!eaEvaluated[eaIndex]){
					// evaluate external atom if the heuristics decides so
					const ExternalAtom& eatom = reg->eatoms.getByID(factory.innerEatoms[eaIndex]);
					if (externalAtomEvalHeuristics->doEvaluate(eatom, annotatedGroundProgram.getProgramMask(), partialInterpretation, factWasSet, changed)){
						// evaluate it
						conflict |= (verifyExternalAtom(eaIndex, partialInterpretation, factWasSet, changed));
					}
					if (!eaEvaluated[eaIndex]){
						// find a new yet unassigned atom to watch
						IDAddress id = getWatchedLiteral(eaIndex, factWasSet, false);
						if (id != ID::ALL_ONES)
						  verifyWatchList[id].push_back(eaIndex);
					}
				}
			}
			// current atom was set, so remove all watches
			verifyWatchList[*en].clear();
		}

		en++;
	}

	return conflict;
}

bool GenuineGuessAndCheckModelGenerator::verifyExternalAtom(int eaIndex, InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed){

	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "genuine g&c verifyEAtom");

	// prepare EA evaluation
	InterpretationConstPtr mask = (annotatedGroundProgram.getEAMask(eaIndex)->mask());
	const ExternalAtom& eatom = reg->eatoms.getByID(factory.innerEatoms[eaIndex]);
	VerifyExternalAtomCB vcb(partialInterpretation, eatom, *(annotatedGroundProgram.getEAMask(eaIndex)));

	// make sure that ALL input auxiliary atoms are true, otherwise we might miss some output atoms and consider true output atoms wrongly as unfounded
	InterpretationPtr evalIntr = InterpretationPtr(new Interpretation(*partialInterpretation));
	if (!factory.ctx.config.getOption("IncludeAuxInputInAuxiliaries")){
		evalIntr->getStorage() |= annotatedGroundProgram.getEAMask(eaIndex)->getAuxInputMask()->getStorage();
	}
	// evaluate the external atom (and learn nogoods if external learning is used)
	DBGLOG(DBG, "Verifying external Atom " << factory.innerEatoms[eaIndex] << " under " << *evalIntr);
	evaluateExternalAtom(factory.ctx, eatom, evalIntr, vcb, factory.ctx.config.getOption("ExternalLearning") ? learnedEANogoods : NogoodContainerPtr());
	updateEANogoods(partialInterpretation, factWasSet, changed);

	// if the input to the external atom was complete, then remember the verification result
	// (for incomplete input we cannot yet decide this)
	if (!factWasSet || (
                (annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage() & annotatedGroundProgram.getProgramMask()->getStorage() & factWasSet->getStorage()).count()
                ==
                (annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage() & annotatedGroundProgram.getProgramMask()->getStorage()).count()   )){

		bool verify = vcb.verify();
		DBGLOG(DBG, "Verifying " << factory.innerEatoms[eaIndex] << " (Result: " << verify << ")");
		eaVerified[eaIndex] = verify;
		eaEvaluated[eaIndex] = true;

		return !verify;
	}else{
		return false;
	}
}

const OrdinaryASPProgram& GenuineGuessAndCheckModelGenerator::getGroundProgram(){
	return grounder->getGroundProgram();
}

void GenuineGuessAndCheckModelGenerator::propagate(InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed){

	bool conflict = verifyExternalAtoms(partialInterpretation, factWasSet, changed);

	// UFS check requires a conflict-free interpretation
	if (conflict) return;

	partialUFSCheck(partialInterpretation, factWasSet, changed);
}

DLVHEX_NAMESPACE_END

// vi:ts=8:noexpandtab:
