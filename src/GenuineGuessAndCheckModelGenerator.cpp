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
#include "dlvhex2/UnfoundedSetChecker.h"

#include <bm/bmalgo.h>

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

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

  // make an analysis of cyclic predicate input parameters
//  computeCyclicInputPredicates(reg, ctx, idb);

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
    DBGLOG(DBG, "Genuine GnC-ModelGenerator is instantiated for a " << (factory.ci.disjunctiveHeads ? "" : "non-") << "disjunctive component");

    RegistryPtr reg = factory.reg;

    setHeuristics();

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

	grounder = GenuineGrounder::getInstance(factory.ctx, program);
        annotatedGroundProgram = AnnotatedGroundProgram(reg, grounder->getGroundProgram(), factory.innerEatoms);
	solver = GenuineGroundSolver::getInstance(	factory.ctx, annotatedGroundProgram,
						eaVerificationMode != heuristics,		// prefer multithreaded mode, but this is not possible in heuristics mode
						!factory.ctx.config.getOption("FLPCheck") && !factory.ctx.config.getOption("UFSCheck")	// do the UFS check for disjunctions only if we don't do
																	// a minimality check in this class;
																	// this will not find unfounded sets due to external sources,
																	// but at least unfounded sets due to disjunctions
						);
	factory.ctx.globalNogoods.addNogoodListener(solver);
	learnedEANogoods = NogoodContainerPtr(new SimpleNogoodContainer());
	learnedEANogoodsTransferredIndex = 0;
	nogoodGrounder = NogoodGrounderPtr(new ImmediateNogoodGrounder(factory.ctx.registry(), learnedEANogoods, learnedEANogoods, annotatedGroundProgram));
	if (eaVerificationMode != post){
		solver->addExternalLearner(this);
	}
    }

    // initialize UFS checker
    //   Concerning the last parameter, note that clasp backend uses choice rules for implementing disjunctions:
    //   this must be regarded in UFS checking (see examples/trickyufs.hex)
    ufscm = UnfoundedSetCheckerManagerPtr(new UnfoundedSetCheckerManager(*this, factory.ctx, annotatedGroundProgram, factory.ctx.config.getOption("GenuineSolver") >= 3));
}

GenuineGuessAndCheckModelGenerator::~GenuineGuessAndCheckModelGenerator(){
	factory.ctx.globalNogoods.removeNogoodListener(solver);
	if (eaVerificationMode != post){
		solver->removeExternalLearner(this);
	}
	DBGLOG(DBG, "Final Statistics:" << std::endl << solver->getStatistics());
}

void GenuineGuessAndCheckModelGenerator::setHeuristics(){

    // set external atom evaluation strategy according to selected heuristics
    switch (factory.ctx.config.getOption("VerificationHeuristics")){
      // post and immediate are hardcoded
      case 0:
        eaVerificationMode = post;
        DBGLOG(DBG, "EA-Verification Mode: post");
        break;
      case 1:
        eaVerificationMode = immediate;
        DBGLOG(DBG, "EA-Verification Mode: immediate");
        break;
      // other heuristics use the heuristics framework
      case 2:
        eaVerificationMode = heuristics;
        DBGLOG(DBG, "EA-Verification Mode: heuristics");
        for (int i = 0; i < factory.innerEatoms.size(); ++i){
          eaEvaluated.push_back(false);
          eaVerified.push_back(false);
        }
        externalAtomEvalHeuristics = factory.ctx.externalAtomEvaluationHeuristicsFactory->createHeuristics(this, reg);
        break;
      default: assert(false);
    }
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
		modelCandidate = solver->projectToOrdinaryAtoms(solver->getNextModel());
		DBGLOG(DBG, "Statistics:" << std::endl << solver->getStatistics());
		if( !modelCandidate ) return modelCandidate;
		DLVHEX_BENCHMARK_REGISTER_AND_COUNT(ssidmodelcandidates, "Candidate compatible sets", 1);

		DBGLOG_SCOPE(DBG,"gM", false);
		DBGLOG(DBG,"= got guess model " << *modelCandidate);

		DBGLOG(DBG, "doing compatibility check for model candidate " << *modelCandidate);
		if (!finalCompatibilityCheck(modelCandidate)) continue;

		DBGLOG(DBG, "Checking if model candidate is a model");
		if (!isModel(modelCandidate)) continue;

		// remove edb and the guess (from here we don't need the guess anymore)
		modelCandidate->getStorage() -= factory.gpMask.mask()->getStorage();
		modelCandidate->getStorage() -= factory.gnMask.mask()->getStorage();

		modelCandidate->getStorage() -= mask->getStorage();

		DBGLOG(DBG,"= final model " << *modelCandidate);
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

void GenuineGuessAndCheckModelGenerator::generalizeNogoods(){
	int max = learnedEANogoods->getNogoodCount();
	for (int i = learnedEANogoodsTransferredIndex; i < max; ++i){
		generalizeNogood(learnedEANogoods->getNogood(i));
	}
}

void GenuineGuessAndCheckModelGenerator::transferLearnedEANogoods(){

	for (int i = learnedEANogoodsTransferredIndex; i < learnedEANogoods->getNogoodCount(); ++i){
		DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sidcompatiblesets, "Learned EA-Nogoods", 1);
		if (factory.ctx.config.getOption("PrintLearnedNogoods")){
			if (factory.ctx.config.getOption("GenuineSolver") >= 3){
				if (i == 0) std::cerr << "( NOTE: With clasp backend, learned nogoods become effective with a delay because of multithreading! )" << std::endl << std::endl;
			}else{
				if (i == 0) std::cerr << "( NOTE: With i-backend, learned nogoods become effective AFTER the next model was printed ! )" << std::endl << std::endl;
			}
			std::cerr << "Learned nogood: " << learnedEANogoods->getNogood(i).getStringRepresentation(reg) << std::endl;
		}
		if (learnedEANogoods->getNogood(i).isGround()){
			solver->addNogood(learnedEANogoods->getNogood(i));
		}
	}
	learnedEANogoodsTransferredIndex = learnedEANogoods->getNogoodCount();
}

bool GenuineGuessAndCheckModelGenerator::finalCompatibilityCheck(InterpretationConstPtr modelCandidate){

	// did we already verify during model construction or do we have to do the verification now?
	bool compatible;
	int ngCount;
	switch (eaVerificationMode){
	case post:
		// no --> post-check
		DBGLOG(DBG, "(eaVerificationMode == post) Doing Compatibility Check");
		compatible = isCompatibleSet(modelCandidate, postprocessedInput, factory.ctx, factory.ctx.config.getOption("ExternalLearning") ? learnedEANogoods : GenuineSolverPtr());
		if (factory.ctx.config.getOption("ExternalLearningGeneralize")) generalizeNogoods();
		if (factory.ctx.config.getOption("NongroundNogoodInstantiation")) nogoodGrounder->update(modelCandidate);
		transferLearnedEANogoods();

		DBGLOG(DBG, "Compatible: " << compatible);
		break;
	case immediate:
		DBGLOG(DBG, "(eaVerificationMode == immediate) No Compatibility Check Required");
		compatible = true;
		break;
	case heuristics:
		DBGLOG(DBG, "(eaVerificationMode == heuristics) Checking if all external atoms were verified");
		compatible = true;
		for (int eaIndex = 0; eaIndex < factory.innerEatoms.size(); ++eaIndex){
			if (eaEvaluated[eaIndex] == false || eaVerified[eaIndex] == false){
				// try to verify
				DBGLOG(DBG, "External atom " << factory.innerEatoms[eaIndex] << " is not verified, trying to do this now");
				verifyExternalAtom(eaIndex, modelCandidate);
				DBGLOG(DBG, "Verification result: " << eaVerified[eaIndex]);

				if (eaVerified[eaIndex] == false){
					compatible = false;
					break;
				}
			}
		}
		DBGLOG(DBG, "Compatible: " << compatible);
		break;
	default:
		assert(false);
	}
	return compatible;
}

bool GenuineGuessAndCheckModelGenerator::isModel(InterpretationConstPtr compatibleSet){

	// which semantics?
	if (factory.ctx.config.getOption("WellJustified")){
		// well-justified FLP: fixpoint iteration
		InterpretationPtr fixpoint = getFixpoint(compatibleSet, grounder->getGroundProgram());
		InterpretationPtr reference = InterpretationPtr(new Interpretation(*compatibleSet));
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
		if (annotatedGroundProgram.hasHeadCycles() == 0 && annotatedGroundProgram.hasECycles() == 0){
			DBGLOG(DBG, "No head- or e-cycles --> No FLP/UFS check necessary");
			return true;
		}else{
			DBGLOG(DBG, "Head- or e-cycles --> FLP/UFS check necessary");

			// Explicit FLP check
			if (factory.ctx.config.getOption("FLPCheck")){
				DBGLOG(DBG, "FLP Check");
				// do FLP check (possibly with nogood learning) and add the learned nogoods to the main search
				bool result = isSubsetMinimalFLPModel<GenuineSolver>(compatibleSet, postprocessedInput, factory.ctx, factory.ctx.config.getOption("ExternalLearning") ? learnedEANogoods : GenuineSolverPtr());
				if (factory.ctx.config.getOption("ExternalLearningGeneralize")) generalizeNogoods();
				if (factory.ctx.config.getOption("NongroundNogoodInstantiation")) nogoodGrounder->update(compatibleSet);
				transferLearnedEANogoods();
				return result;
			}

			// UFS check
			if (factory.ctx.config.getOption("UFSCheck")){
				DBGLOG(DBG, "UFS Check");
				// do UFS check (possibly with nogood learning) and add the learned nogoods to the main search
//				UnfoundedSetChecker ufsc(*this, factory.ctx, solver->getGroundProgram(), factory.innerEatoms, compatibleSet, std::set<ID>(), InterpretationConstPtr(), factory.ctx.config.getOption("ExternalLearning") ? learnedEANogoods : GenuineSolverPtr());

//				std::vector<IDAddress> ufs = ufsc.getUnfoundedSet();
				std::vector<IDAddress> ufs = ufscm->getUnfoundedSet(compatibleSet, std::set<ID>(), factory.ctx.config.getOption("ExternalLearning") ? learnedEANogoods : GenuineSolverPtr());
				if (factory.ctx.config.getOption("ExternalLearningGeneralize")) generalizeNogoods();
				if (factory.ctx.config.getOption("NongroundNogoodInstantiation")) nogoodGrounder->update(compatibleSet);
				transferLearnedEANogoods();
				if (ufs.size() > 0){
					DBGLOG(DBG, "Got a UFS");
					if (factory.ctx.config.getOption("UFSLearning")){
						DBGLOG(DBG, "Learn from UFS");
//						Nogood ufsng = ufsc.getUFSNogood(ufs, compatibleSet);
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

	if (!factory.ctx.config.getOption("UFSCheck")) return false;

	// ufs check without nogood learning makes no sense if the interpretation is not complete
	if (factory.ctx.config.getOption("UFSLearning")){

		std::pair<bool, std::set<ID> > decision = ufsCheckHeuristics->doUFSCheck(partialInterpretation, factWasSet, changed);

		if (decision.first){

			DBGLOG(DBG, "Heuristic decides to do an UFS check");
			//UnfoundedSetChecker ufsc(*this, factory.ctx, solver->getGroundProgram(), factory.innerEatoms, solver->projectToOrdinaryAtoms(partialInterpretation), decision.second, InterpretationConstPtr(), factory.ctx.config.getOption("ExternalLearning") ? learnedEANogoods : GenuineSolverPtr());

			//std::vector<IDAddress> ufs = ufsc.getUnfoundedSet();
			std::vector<IDAddress> ufs = ufscm->getUnfoundedSet(partialInterpretation, decision.second, factory.ctx.config.getOption("ExternalLearning") ? learnedEANogoods : GenuineSolverPtr());
			DBGLOG(DBG, "UFS result: " << (ufs.size() == 0 ? "no" : "") << " UFS found (interpretation: " << *partialInterpretation << ", assigned: " << *factWasSet << ")");

			if (ufs.size() > 0){
				//Nogood ng = ufsc.getUFSNogood(ufs, partialInterpretation);
				Nogood ng = ufscm->getLastUFSNogood();
				int oldCount = solver->getNogoodCount();
				DBGLOG(DBG, "Adding UFS nogood: " << ng);
				solver->addNogood(ng);

				// check if nogood is violated
				BOOST_FOREACH (ID l, ng){
					if (!factWasSet->getFact(l.address) || l.isNaf() == partialInterpretation->getFact(l.address)) return false;
				}

				return solver->getNogoodCount() > oldCount;
			}
		}else{
			DBGLOG(DBG, "Heuristic decides not to do an UFS check");
		}
	}

	return false;
}

bool GenuineGuessAndCheckModelGenerator::isVerified(ID eaAux, InterpretationConstPtr factWasSet){

	if (eaVerificationMode == post){
		DBGLOG(DBG, "Auxiliary " << eaAux.address << " is not because model generator runs in post-mode");
		return false;
	}
	if (eaVerificationMode == immediate){
		// if the input to the external atom behind the auxiliary is complete,
		// then is was automatically verified due to verification strategy
		const ExternalAtom& ea = reg->eatoms.getByID(annotatedGroundProgram.getAuxToEA(eaAux.address)[0]);
		ea.updatePredicateInputMask();
		InterpretationConstPtr pm = ea.getPredicateInputMask();
		if (pm && (factWasSet->getStorage() & annotatedGroundProgram.getProgramMask()->getStorage() & pm->getStorage()).count() < (pm->getStorage() & annotatedGroundProgram.getProgramMask()->getStorage()).count()){
			DBGLOG(DBG, "Auxiliary " << eaAux.address << " is not verified because model generator runs in immediate-mode and input to " << annotatedGroundProgram.getAuxToEA(eaAux.address)[0] << " is incomplete");
			return false;
		}

		DBGLOG(DBG, "Auxiliary " << eaAux.address << " is verified because model generator runs in immediate-mode and input to " << annotatedGroundProgram.getAuxToEA(eaAux.address)[0] << " is complete");
		return true;
	}

	assert(eaVerificationMode == heuristics);
	assert(annotatedGroundProgram.getAuxToEA(eaAux.address).size() > 0);

	// check if at least one of the external atoms which can derive this auxiliary were verified
	int eaIndex = 0;
	BOOST_FOREACH (ID ea, annotatedGroundProgram.getAuxToEA(eaAux.address)){
		if (eaEvaluated[eaIndex] && eaVerified[eaIndex]){
			DBGLOG(DBG, "Auxiliary " << eaAux.address << " is verified by " << ea);
			return true;
		}
		eaIndex++;
	}
	DBGLOG(DBG, "Auxiliary " << eaAux.address << " is not verified");
	return false;
}

bool GenuineGuessAndCheckModelGenerator::verifyExternalAtoms(InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed){

	DBGLOG(DBG, "Evaluating External Atoms");

	// go through all external atoms and
	// - unverify them if relevant parts of the assignment changed
	// - verify them if possible and the heuristic decides to evaluate them (or we are in immediate mode)
	for (int eaIndex = 0; eaIndex < factory.innerEatoms.size(); ++eaIndex)
	{
		const ExternalAtom& eatom = reg->eatoms.getByID(factory.innerEatoms[eaIndex]);

		// if input to the external atom changed, it is not verified anymore
		if (eaVerificationMode == heuristics && eaEvaluated[eaIndex]){
			// check if one of its relevant atoms has changed
			if ((annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage() & changed->getStorage()).count() > 0){
				DBGLOG(DBG, "Unverifying " << factory.innerEatoms[eaIndex]);
				eaVerified[eaIndex] = false;
				eaEvaluated[eaIndex] = false;
			}
		}

		// here we evaluate an external atom if
		// - we are in immdiate mode: then we must verify because we must not produce candidate compatible sets which fail the compatibility check
		// - we are in heuristics mode and the heuristic tells us to verify
		//	currently, the heuristic verifies whenever the input is complete and the atom is not verified yet
		if (eaVerificationMode == immediate || (!eaEvaluated[eaIndex] && externalAtomEvalHeuristics->doEvaluate(eatom, partialInterpretation, factWasSet, changed))){
			if (verifyExternalAtom(eaIndex, partialInterpretation, factWasSet, changed)) return true;
		}
	}
	return false;
}

bool GenuineGuessAndCheckModelGenerator::verifyExternalAtom(int eaIndex, InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed){

	// 1. we need all relevant atoms to be assigned before we can do the verification
	// 2. reverification is only necessary and useful if relevant atoms changed
	if ((!factWasSet || ((annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage() & annotatedGroundProgram.getProgramMask()->getStorage() & factWasSet->getStorage()).count() == (annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage() & annotatedGroundProgram.getProgramMask()->getStorage()).count())) &&	// 1
	    (!changed || (annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage() & changed->getStorage()).count() > 0)){	// 2
		InterpretationConstPtr mask = (annotatedGroundProgram.getEAMask(eaIndex)->mask());
		DBGLOG(DBG, "External Atom " << factory.innerEatoms[eaIndex] << " is ready for verification, interpretation: " << *partialInterpretation << ", factWasSet: " << *factWasSet << ", mask: " << *mask);
		const ExternalAtom& eatom = reg->eatoms.getByID(factory.innerEatoms[eaIndex]);
		VerifyExternalAtomCB vcb(partialInterpretation, eatom, *(annotatedGroundProgram.getEAMask(eaIndex)));

		// make sure that ALL input auxiliary atoms are true, otherwise we might miss some output atoms and consider true output atoms wrongly as unfounded
		InterpretationPtr evalIntr = InterpretationPtr(new Interpretation(*partialInterpretation));
		BOOST_FOREACH (Tuple t, annotatedGroundProgram.getEAMask(eaIndex)->getAuxInputTuples()){
			OrdinaryAtom oa(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			oa.tuple.push_back(eatom.auxInputPredicate);
			oa.tuple.insert(oa.tuple.end(), t.begin(), t.end());
			ID oaid = reg->storeOrdinaryGAtom(oa);
#ifndef NDEBUG
			if (!evalIntr->getFact(oaid.address)){
				DBGLOG(DBG, "Setting aux input " << oaid.address);
			}
#endif
			evalIntr->setFact(oaid.address);
		}

		// evaluate the external atom (and learn nogoods if external learning is used)
		evaluateExternalAtom(reg, eatom, evalIntr, vcb, &factory.ctx, factory.ctx.config.getOption("ExternalLearning") ? learnedEANogoods : NogoodContainerPtr());
		if (factory.ctx.config.getOption("ExternalLearningGeneralize")) generalizeNogoods();
		if (factory.ctx.config.getOption("NongroundNogoodInstantiation")) nogoodGrounder->update(partialInterpretation, factWasSet, changed);
		transferLearnedEANogoods();


		// remember the verification result
		bool verify = vcb.verify();
		DBGLOG(DBG, "Verifying " << factory.innerEatoms[eaIndex] << " (Result: " << verify << ")");
		if (eaVerificationMode == heuristics){
			eaVerified[eaIndex] = verify;
			eaEvaluated[eaIndex] = true;
		}

		// in case of a conflict, the set of all atoms which are relevant to the external atom form a nogood
		if (!verify){
			eatom.updatePredicateInputMask();
			bm::bvector<>::enumerator en = annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage().first();
			bm::bvector<>::enumerator en_end = annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage().end();
			Nogood ng;
			while (en < en_end){
				// atoms which do not occur in the program can never be true
				if (annotatedGroundProgram.getProgramMask()->getFact(*en)){
					assert(factWasSet->getFact(*en));
					ng.insert(NogoodContainer::createLiteral(*en, partialInterpretation->getFact(*en)));
				}
				en++;
			}
			// note: this nogoods _must_ be learned in immediate mode (but it is very useful also in heuristics mode), even if external learning is off,
			// because the solver _must never_ generate conflicting assignments as there is no final compatibility check
			int oldCount = solver->getNogoodCount();
			int newIndex = solver->addNogood(ng);
			DBGLOG(DBG, "Conflict nogood: " << ng.getStringRepresentation(reg) << ", inconsistent: " << (newIndex >= oldCount));
			return newIndex >= oldCount;	// tell the solver if a new nogood was added
		}
	}

	return false;
}

const OrdinaryASPProgram& GenuineGuessAndCheckModelGenerator::getGroundProgram(){
	return grounder->getGroundProgram();
}

bool GenuineGuessAndCheckModelGenerator::learn(InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed){

	bool conflict = verifyExternalAtoms(partialInterpretation, factWasSet, changed);

	// UFS check requires a conflict-free interpretation
	if (conflict) return true;

	return partialUFSCheck(partialInterpretation, factWasSet, changed);
}

DLVHEX_NAMESPACE_END
