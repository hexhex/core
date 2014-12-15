/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * Copyright (C) 2011, 2012, 2013, 2014 Christoph Redl
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
#include <boost/unordered_map.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/properties.hpp>
#include <boost/scoped_ptr.hpp>

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

  // just copy all rules and constraints to idb
  idb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
  idb.insert(idb.end(), ci.innerRules.begin(), ci.innerRules.end());
  idb.insert(idb.end(), ci.innerConstraints.begin(), ci.innerConstraints.end());

  // create program for domain exploration
  if (ctx.config.getOption("LiberalSafety")){
    // add domain predicates for all external atoms which are necessary to establish liberal domain-expansion safety
    // and extract the domain-exploration program from the IDB
    addDomainPredicatesAndCreateDomainExplorationProgram(ci, ctx, idb, deidb, deidbInnerEatoms, outerEatoms);
  }

  innerEatoms = ci.innerEatoms;
  // create guessing rules "gidb" for innerEatoms in all inner rules and constraints
  createEatomGuessingRules(ctx);

  // transform original innerRules and innerConstraints to xidb with only auxiliaries
  xidb.reserve(idb.size());
  std::back_insert_iterator<std::vector<ID> > inserter(xidb);
  std::transform(idb.begin(), idb.end(),
      inserter, boost::bind(&GenuineGuessAndCheckModelGeneratorFactory::convertRule, this, ctx, _1));

  // transform xidb for flp calculation
  if (ctx.config.getOption("FLPCheck")) createFLPRules();

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
  return print(o, true);
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
  reg(factory.reg),
  previousRuleCount(0)
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
    WARNING("perhaps we can pass multiple partially preprocessed input edb's to the external solver and save a lot of processing here")
    postprocInput->add(*factory.ctx.edb);

    // remember which facts we must remove
    mask.reset(new Interpretation(*postprocInput));

    // manage outer external atoms
    if( !factory.outerEatoms.empty() )
    {
      DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidhexground, "HEX grounder time");
		
      // augment input with result of external atom evaluation
      // use newint as input and as output interpretation
      IntegrateExternalAnswerIntoInterpretationCB cb(postprocInput);
      evaluateExternalAtoms(factory.ctx,
          factory.outerEatoms, postprocInput, cb);
      DLVHEX_BENCHMARK_REGISTER(sidcountexternalatomcomps,
          "outer eatom computations");
      DLVHEX_BENCHMARK_COUNT(sidcountexternalatomcomps,1);
    }

    // compute extensions of domain predicates and add it to the input
    if (factory.ctx.config.getOption("LiberalSafety")){
		if (!factory.ctx.config.getOption("IncrementalGrounding")){
			InterpretationConstPtr domPredictaesExtension = computeExtensionOfDomainPredicates(factory.ci, factory.ctx, postprocInput, factory.deidb, factory.deidbInnerEatoms);
			postprocInput->add(*domPredictaesExtension);
		}else{
			DBGLOG(DBG, "Creating subcomponent graph");
			DependencyGraphPtr subdepgraph(new DependencyGraph(factory.ctx, factory.ctx.registry()));
			std::vector<dlvhex::ID> idbWithoutAuxRules;
			BOOST_FOREACH (ID ruleID, factory.idb){
				const Rule& rule = reg->rules.getByID(ruleID);
				if (rule.head.size() > 0 && rule.head[0].isAuxiliary()) continue;
				else idbWithoutAuxRules.push_back(ruleID);
			}
			std::vector<dlvhex::ID> auxRules;
			subdepgraph->createDependencies(idbWithoutAuxRules, auxRules);

			// find for each component the set of domain predicates and the sub-(xidb+gidb)
			subcompgraph = ComponentGraphPtr(new ComponentGraph(*subdepgraph, factory.ctx.registry()));
			std::pair<ComponentGraph::ComponentIterator, ComponentGraph::ComponentIterator> comps = subcompgraph->getComponents();
			int nr = 0;
			for (ComponentGraph::ComponentIterator comp = comps.first; comp != comps.second; ++comp, ++nr){
				// find the set of domain predicates
				const ComponentGraph::ComponentInfo& ci = subcompgraph->getComponentInfo(*comp);
				PredicateMaskPtr pm(new PredicateMask());
				pm->setRegistry(reg);
				BOOST_FOREACH (ID ruleID, ci.innerRules){
					const Rule& rule = reg->rules.getByID(ruleID);
					BOOST_FOREACH (ID b, rule.body){
						if (b.isAuxiliary() && reg->getTypeByAuxiliaryConstantSymbol(reg->lookupOrdinaryAtom(b).tuple[0]) == 'd'){
							pm->addPredicate(reg->lookupOrdinaryAtom(b).tuple[0]);
						}
					}
				}				
				domainMaskPerComponent.push_back(pm);
				
				// add sub-xidb
				gxidbPerComponent.push_back(std::vector<ID>());
				gxidbPerComponent[nr].reserve(ci.innerRules.size() + ci.innerConstraints.size());
				std::back_insert_iterator<std::vector<ID> > inserter(gxidbPerComponent[nr]);
				std::transform(ci.innerRules.begin(), ci.innerRules.end(), inserter, boost::bind(&GenuineGuessAndCheckModelGeneratorFactory::convertRule, &factory, factory.ctx, _1));
				std::transform(ci.innerConstraints.begin(), ci.innerConstraints.end(), inserter, boost::bind(&GenuineGuessAndCheckModelGeneratorFactory::convertRule, &factory, factory.ctx, _1));

				// add sub-gidb
				BOOST_FOREACH (ID rid, ci.innerRules){
					if (rid.doesRuleContainExtatoms()){
						BOOST_FOREACH (ID lit, factory.reg->rules.getByID(rid).body){
							if (lit.isExternalAtom()) gxidbPerComponent[nr].push_back(factory.createEatomGuessingRule(factory.ctx, rid, lit));
						}
					}
				}
				BOOST_FOREACH (ID rid, ci.innerConstraints){
					if (rid.doesRuleContainExtatoms()){
						BOOST_FOREACH (ID lit, factory.reg->rules.getByID(rid).body){
							if (lit.isExternalAtom()) gxidbPerComponent[nr].push_back(factory.createEatomGuessingRule(factory.ctx, rid, lit));
						}
					}
				}
#ifndef NDEBUG
				{
				std::stringstream ss;
				RawPrinter rp(ss, reg);
				ss << "gxidb of subcomponent " << nr << std::endl;
				BOOST_FOREACH (ID ruleID, gxidbPerComponent[nr]){
					rp.print(ruleID);
					ss << std::endl;
				}
				DBGLOG(DBG, ss.str());
				}
#endif
			}

			if (factory.ctx.config.getOption("DumpCompGraph")){
					std::string fnamev = factory.ctx.config.getStringOption("DebugPrefix")+"_SubCompGraphVerbose.dot";
					LOG(INFO,"dumping verbose component graph to " << fnamev);
					std::ofstream filev(fnamev.c_str());
					subcompgraph->writeGraphViz(filev, true);
					
					std::string fnamet = factory.ctx.config.getStringOption("DebugPrefix")+"_SubCompGraphTerse.dot";
					LOG(INFO,"dumping terse component graph to " << fnamet);
					std::ofstream filet(fnamet.c_str());
					subcompgraph->writeGraphViz(filet, false);
			}

			// collect replacement atoms of inner external atoms and build a mask
			innerEatomOutputs.reset(new PredicateMask());
			innerEatomOutputs->setRegistry(factory.reg);
			previousInnerEatomOutputs.reset(new Interpretation(factory.reg));
			BOOST_FOREACH (ID eaID, factory.deidbInnerEatoms){
				innerEatomOutputs->addPredicate(factory.reg->getAuxiliaryConstantSymbol('r', factory.reg->eatoms.getByID(eaID).predicate));
			}

			// keep a set of rules currently in the grounding
			currentRules.reset(new Interpretation(factory.reg));

			// keep a set of hook atoms which need to be frozen
			frozenHookAtoms.reset(new Interpretation(factory.reg));
		}
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
		{
			DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidhexground, "HEX grounder time");
			grounder = GenuineGrounder::getInstance(factory.ctx, program);

			if (factory.ctx.config.getOption("IncrementalGrounding")){
				annotatedGroundProgram = AnnotatedGroundProgram(factory.ctx, grounder->getGroundProgram(), factory.innerEatoms, factory.idb);
			}else{
				annotatedGroundProgram = AnnotatedGroundProgram(factory.ctx, grounder->getGroundProgram(), factory.innerEatoms);
			}
			previousRuleCount = annotatedGroundProgram.getGroundProgram().idb.size();
		}
		// for incremental solving we need hook rules for all atoms which occur in the heads
		if (factory.ctx.config.getOption("IncrementalGrounding")){
			addHookRules();
			buildFrozenHookAtomAssumptions();
		}

		solver = GenuineGroundSolver::getInstance(
			factory.ctx, annotatedGroundProgram,
			frozenHookAtoms,
			// do the UFS check for disjunctions only if we don't do
			// a minimality check in this class;
			// this will not find unfounded sets due to external sources,
			// but at least unfounded sets due to disjunctions
			!factory.ctx.config.getOption("FLPCheck") && !factory.ctx.config.getOption("UFSCheck"));

		if (factory.ctx.config.getOption("IncrementalGrounding")) solver->restartWithAssumptions(hookAssumptions);
	}

    // external learning related initialization
    learnedEANogoods = SimpleNogoodContainerPtr(new SimpleNogoodContainer());
    learnedEANogoodsTransferredIndex = 0;
    nogoodGrounder = NogoodGrounderPtr(new ImmediateNogoodGrounder(factory.ctx.registry(), learnedEANogoods, learnedEANogoods, annotatedGroundProgram));
    if(factory.ctx.config.getOption("NoPropagator") == 0) solver->addPropagator(this);
    learnSupportSets();

    // external atom evaluation and unfounded set checking
    //   initialize UFS checker
    //     Concerning the last parameter, note that clasp backend uses choice rules for implementing disjunctions:
    //     this must be regarded in UFS checking (see examples/trickyufs.hex)
    ufscm = UnfoundedSetCheckerManagerPtr(new UnfoundedSetCheckerManager(*this, factory.ctx, annotatedGroundProgram,
                                                                         factory.ctx.config.getOption("GenuineSolver") >= 3,
                                                                         factory.ctx.config.getOption("ExternalLearning") ? learnedEANogoods : SimpleNogoodContainerPtr()));
    //   incremental algorithms
    setHeuristics();
}

void GenuineGuessAndCheckModelGenerator::addHookRules(){

	std::vector<ID> hookRules;
	bm::bvector<>::enumerator en = annotatedGroundProgram.getGroundProgram().edb->getStorage().first();
	bm::bvector<>::enumerator en_end = annotatedGroundProgram.getGroundProgram().edb->getStorage().end();
	if (en < en_end){
		ID fact = factory.reg->ogatoms.getIDByAddress(*en);
		if (hookAtoms.find(fact) == hookAtoms.end()){
			hookAtoms[fact] = factory.reg->getAuxiliaryAtom('k', fact);
			frozenHookAtoms->setFact(hookAtoms[fact].address);
		}
		hookRules.push_back(getIncrementalHookRule(fact, hookAtoms[fact]));
		en++;
	}
	BOOST_FOREACH (ID ruleID, annotatedGroundProgram.getGroundProgram().idb){
		currentRules->setFact(ruleID.address);
		const Rule& rule = factory.reg->rules.getByID(ruleID);
		BOOST_FOREACH (ID h, rule.head){
			if (hookAtoms.find(h) == hookAtoms.end()){
				hookAtoms[h] = factory.reg->getAuxiliaryAtom('k', h);
				frozenHookAtoms->setFact(hookAtoms[h].address);	
			}
			hookRules.push_back(getIncrementalHookRule(h, hookAtoms[h]));
		}
	}
	mask->add(*frozenHookAtoms);
	AnnotatedGroundProgram hookRulesAGP(factory.ctx, OrdinaryASPProgram(factory.reg, hookRules, InterpretationPtr(new Interpretation(factory.reg)), factory.ctx.maxint), factory.innerEatoms, factory.idb);
	annotatedGroundProgram.addProgram(hookRulesAGP);
}

void GenuineGuessAndCheckModelGenerator::buildFrozenHookAtomAssumptions(){

	hookAssumptions.clear();
	bm::bvector<>::enumerator en = frozenHookAtoms->getStorage().first();
	bm::bvector<>::enumerator en_end = frozenHookAtoms->getStorage().end();
	while (en < en_end){
		hookAssumptions.push_back(ID::nafLiteralFromAtom(factory.reg->ogatoms.getIDByAddress(*en)));
		en++;
	}
}

GenuineGuessAndCheckModelGenerator::~GenuineGuessAndCheckModelGenerator(){
	solver->removePropagator(this);
	DBGLOG(DBG, "Final Statistics:" << std::endl << solver->getStatistics());
}

void GenuineGuessAndCheckModelGenerator::setHeuristics(){

	defaultExternalAtomEvalHeuristics = factory.ctx.defaultExternalAtomEvaluationHeuristicsFactory->createHeuristics(reg);

	// set external atom evaluation strategy according to selected heuristics
	for (uint32_t i = 0; i < factory.innerEatoms.size(); ++i){
		const ExternalAtom& eatom = reg->eatoms.getByID(factory.innerEatoms[i]);

		eaEvaluated.push_back(false);
		eaVerified.push_back(false);
		changedAtomsPerExternalAtom.push_back(eatom.getExtSourceProperties().doesCareAboutChanged() ? InterpretationPtr(new Interpretation(reg)) : InterpretationPtr());

		// custom or default heuristics?
		if (eatom.pluginAtom->providesCustomExternalAtomEvaluationHeuristicsFactory()){
			DBGLOG(DBG, "Using custom external atom heuristics for external atom " << factory.innerEatoms[i]);
			eaEvalHeuristics.push_back(eatom.pluginAtom->getCustomExternalAtomEvaluationHeuristicsFactory()->createHeuristics(reg));
		}else{
			DBGLOG(DBG, "Using default external atom heuristics for external atom " << factory.innerEatoms[i]);
			eaEvalHeuristics.push_back(defaultExternalAtomEvalHeuristics);
		}

		// watch all atoms in the scope of the external atom for unverification
		bm::bvector<>::enumerator en = annotatedGroundProgram.getEAMask(i)->mask()->getStorage().first();
		bm::bvector<>::enumerator en_end = annotatedGroundProgram.getEAMask(i)->mask()->getStorage().end();
		if (en < en_end){
			// watch one input atom for verification
			verifyWatchList[*en].push_back(i);
		}
	}

	// create ufs check heuristics as selected
	ufsCheckHeuristics = factory.ctx.unfoundedSetCheckHeuristicsFactory->createHeuristics(annotatedGroundProgram, reg);
	verifiedAuxes = InterpretationPtr(new Interpretation(reg));
}

InterpretationPtr GenuineGuessAndCheckModelGenerator::generateNextModel()
{
	// now we have postprocessed input in postprocessedInput
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidgcsolve, "genuine guess and check loop");
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidhexsolve, "HEX solver time");

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

/*
#ifndef NDEBUG
		DBGLOG(DBG, "Checking if incremental instance is equivalent to full one");
		// check if the current model of the incrementally extended program also occurs in the models of the program if solved from scratch
		GenuineGroundSolverPtr solver2 = GenuineGroundSolver::getInstance(
			factory.ctx, annotatedGroundProgram,
			frozenHookAtoms,
			// do the UFS check for disjunctions only if we don't do
			// a minimality check in this class;
			// this will not find unfounded sets due to external sources,
			// but at least unfounded sets due to disjunctions
			!factory.ctx.config.getOption("FLPCheck") && !factory.ctx.config.getOption("UFSCheck"));
		InterpretationPtr model2;
		bool found = false;
		solver2->restartWithAssumptions(hookAssumptions);
		while (!!(model2 = solver2->getNextModel())){
			DBGLOG(DBG, "Checking if " << *modelCandidate << " is equivalent to " << *model2);
			if (model2->getStorage() == modelCandidate->getStorage()){
				found = true;
				break;
			}
		}

		assert (found && "model of incrementally extended program does not occur in models of the program built from scratch");

#endif
*/
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

		// check if new values were introduced which were not respected in the grounding
		if (factory.ctx.config.getOption("IncrementalGrounding")){
			DBGLOG(DBG, "Sending candidate to possible domain expansion: " << *modelCandidate);
			std::vector<int> expandedComponents;
			if (incrementalDomainExpansion(modelCandidate, expandedComponents)){
				DBGLOG(DBG, "Expanding program and restarting search");
				incrementalProgramExpansion(expandedComponents);
				buildFrozenHookAtomAssumptions();
				solver->restartWithAssumptions(hookAssumptions);
				for (int i = 0; i < factory.innerEatoms.size(); ++i){
					eaEvaluated[i] = eaVerified[i] = false;
				}
				continue;
			}
		}

		// remove edb and the guess (from here we don't need the guess anymore)
		DBGLOG(DBG, "Got a model, removing replacement atoms");
		modelCandidate->getStorage() -= factory.gpMask.mask()->getStorage();
		modelCandidate->getStorage() -= factory.gnMask.mask()->getStorage();
		modelCandidate->getStorage() -= mask->getStorage();

		LOG(DBG,"returning model without guess: " << *modelCandidate);

		if (factory.ctx.config.getOption("IncrementalGrounding")){
			// Add the model as constraint in the next incremental step because it will be output now and must not be enumerated again after the next incremental expansion.
			Rule modelConstraint(ID::MAINKIND_RULE | ID::SUBKIND_RULE_CONSTRAINT);
			bm::bvector<>::enumerator en = modelCandidate->getStorage().first();
			bm::bvector<>::enumerator en_end = modelCandidate->getStorage().end();
			while (en < en_end){
				modelConstraint.body.push_back(ID::posLiteralFromAtom(factory.reg->ogatoms.getIDByAddress(*en)));
				en++;
			}
			ID modelConstraintID = factory.reg->storeRule(modelConstraint);
			modelEliminationConstraints.push_back(modelConstraintID);
		}

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

void GenuineGuessAndCheckModelGenerator::learnSupportSets(){

	if (factory.ctx.config.getOption("SupportSets")){
		SimpleNogoodContainerPtr potentialSupportSets = SimpleNogoodContainerPtr(new SimpleNogoodContainer());
		SimpleNogoodContainerPtr supportSets = SimpleNogoodContainerPtr(new SimpleNogoodContainer());
		for(unsigned eaIndex = 0; eaIndex < factory.innerEatoms.size(); ++eaIndex){
			// evaluate the external atom if it provides support sets
			const ExternalAtom& eatom = reg->eatoms.getByID(factory.innerEatoms[eaIndex]);
			if (eatom.getExtSourceProperties().providesSupportSets()){
				DBGLOG(DBG, "Evaluating external atom " << factory.innerEatoms[eaIndex] << " for support set learning");
				learnSupportSetsForExternalAtom(factory.ctx, eatom, potentialSupportSets);
			}
		}

		DLVHEX_BENCHMARK_REGISTER(sidnongroundpsupportsets, "nonground potential supportsets");
		DLVHEX_BENCHMARK_COUNT(sidnongroundpsupportsets, potentialSupportSets->getNogoodCount());

		// ground the support sets exhaustively
		NogoodGrounderPtr nogoodgrounder = NogoodGrounderPtr(new ImmediateNogoodGrounder(factory.ctx.registry(), potentialSupportSets, potentialSupportSets, annotatedGroundProgram));

		int nc = 0;
		while (nc < potentialSupportSets->getNogoodCount()){
			nc = potentialSupportSets->getNogoodCount();
			nogoodgrounder->update();
		}
                DLVHEX_BENCHMARK_REGISTER(sidgroundpsupportsets, "ground potential supportsets");
                DLVHEX_BENCHMARK_COUNT(sidgroundpsupportsets, supportSets->getNogoodCount());

		// all support sets are also learned nogoods
		bool keep;
		for (int i = 0; i < potentialSupportSets->getNogoodCount(); ++i){
			const Nogood& ng = potentialSupportSets->getNogood(i);
			if (ng.isGround()){
				// determine the external atom replacement in ng
				ID eaAux = ID_FAIL;
				BOOST_FOREACH (ID lit, ng){
					if (reg->ogatoms.getIDByAddress(lit.address).isExternalAuxiliary()){
						if (eaAux != ID_FAIL) throw GeneralError("Set " + ng.getStringRepresentation(reg) + " is not a valid support set because it contains multiple external literals");
						eaAux = lit;
					}
				}
				if (eaAux == ID_FAIL) throw GeneralError("Set " + ng.getStringRepresentation(reg) + " is not a valid support set because it contains no external literals");

				// determine the according external atom
				if (annotatedGroundProgram.mapsAux(eaAux.address)){
					DBGLOG(DBG, "Evaluating guards of " << ng.getStringRepresentation(reg));
					keep = true;
					Nogood ng2 = ng;
					reg->eatoms.getByID(annotatedGroundProgram.getAuxToEA(eaAux.address)[0]).pluginAtom->guardSupportSet(keep, ng2, eaAux);
					if (keep){
#ifdef DEBUG
						// ng2 must be a subset of ng and still a valid support set
						ID aux = ID_FAIL;
						BOOST_FOREACH (ID id, ng2){
							if (reg->ogatoms.getIDByAddress(id.address).isExternalAuxiliary()) aux = id;
							assert(ng.count(id) > 0);
						}
						assert(aux != ID_FAIL);
#endif
						DBGLOG(DBG, "Keeping in form " << ng2.getStringRepresentation(reg));
						learnedEANogoods->addNogood(ng2);
						supportSets->addNogood(ng2);
#ifdef DEBUG
					}else{
						assert(ng == ng2);
						DBGLOG(DBG, "Rejecting " << ng2.getStringRepresentation(reg));
#endif
					}
				}
			}
		}

                DLVHEX_BENCHMARK_REGISTER(sidgroundsupportsets, "final ground supportsets");
                DLVHEX_BENCHMARK_COUNT(sidgroundsupportsets, supportSets->getNogoodCount());

		// add them to the annotated ground program to make use of them for verification
		DBGLOG(DBG, "Adding " << supportSets->getNogoodCount() << " support sets to annotated ground program");
		annotatedGroundProgram.setCompleteSupportSetsForVerification(supportSets);
	}
}

void GenuineGuessAndCheckModelGenerator::updateEANogoods(
	InterpretationConstPtr compatibleSet,
	InterpretationConstPtr assigned,
	InterpretationConstPtr changed){

	DBGLOG(DBG, "updateEANogoods");

	// generalize ground nogoods to nonground ones
	if (factory.ctx.config.getOption("ExternalLearningGeneralize")){
		int max = learnedEANogoods->getNogoodCount();
		for (int i = learnedEANogoodsTransferredIndex; i < max; ++i){
			generalizeNogood(learnedEANogoods->getNogood(i));
		}
	}

	// instantiate nonground nogoods
	if (factory.ctx.config.getOption("NongroundNogoodInstantiation")){
		nogoodGrounder->update(compatibleSet, assigned, changed);
	}

	// transfer nogoods to the solver
	DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sidcompatiblesets, "Learned EA-Nogoods", learnedEANogoods->getNogoodCount() - learnedEANogoodsTransferredIndex);
	for (int i = learnedEANogoodsTransferredIndex; i < learnedEANogoods->getNogoodCount(); ++i){
		const Nogood& ng = learnedEANogoods->getNogood(i);
		if (factory.ctx.config.getOption("PrintLearnedNogoods")){
		  	// we cannot use i==1 because of learnedEANogoods.clear() below in this function
		 	static bool first = true; 
			if( first )
			{
			  if (factory.ctx.config.getOption("GenuineSolver") >= 3){
				  LOG(DBG, "( NOTE: With clasp backend, learned nogoods become effective with a delay because of multithreading! )");
			  }else{
				  LOG(DBG, "( NOTE: With i-backend, learned nogoods become effective AFTER the next model was printed ! )");
			  }
			  first = false;
			}
			LOG(DBG,"learned nogood " << ng.getStringRepresentation(reg));
		}
		if (ng.isGround()) solver->addNogood(ng);
	}

	// for encoding-based UFS checkers and explicit FLP checks, we need to keep learned nogoods (otherwise future UFS searches will not be able to use them)
	// for assumption-based UFS checkers we can delete them as soon as nogoods were added both to the main search and to the UFS search
	if (factory.ctx.config.getOption("UFSCheckAssumptionBased") ||
	    (annotatedGroundProgram.hasECycles() == 0 && factory.ctx.config.getOption("FLPDecisionCriterionE"))){
		ufscm->learnNogoodsFromMainSearch(true);
		nogoodGrounder->resetWatched(learnedEANogoods);
		learnedEANogoods->clear();
	}else{
		learnedEANogoods->forgetLeastFrequentlyAdded();
	}
	learnedEANogoodsTransferredIndex = learnedEANogoods->getNogoodCount();
}

bool GenuineGuessAndCheckModelGenerator::finalCompatibilityCheck(InterpretationConstPtr modelCandidate){

	// did we already verify during model construction or do we have to do the verification now?
	bool compatible;

	compatible = true;
	for (uint32_t eaIndex = 0; eaIndex < factory.innerEatoms.size(); ++eaIndex){
		if (eaEvaluated[eaIndex] == true && eaVerified[eaIndex] == true){
		}
		if (eaEvaluated[eaIndex] == true && eaVerified[eaIndex] == false){
			DBGLOG(DBG, "External atom " << factory.innerEatoms[eaIndex] << " was evaluated but falsified");
			compatible = false;
			if (!factory.ctx.config.getOption("AlwaysEvaluateAllExternalAtoms")) break;
		}
		if (eaEvaluated[eaIndex] == false){
			// try to verify
			DBGLOG(DBG, "External atom " << factory.innerEatoms[eaIndex] << " is not verified, trying to do this now");
			verifyExternalAtom(eaIndex, modelCandidate);
			DBGLOG(DBG, "Verification result: " << eaVerified[eaIndex]);

			if (eaVerified[eaIndex] == false){
				compatible = false;
				if (!factory.ctx.config.getOption("AlwaysEvaluateAllExternalAtoms")) break;
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
		InterpretationPtr fixpoint = welljustifiedSemanticsGetFixpoint(factory.ctx, compatibleSet, annotatedGroundProgram.getGroundProgram());
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
		if (annotatedGroundProgram.hasHeadCycles() == 0 && annotatedGroundProgram.hasECycles() == 0 && !factory.ctx.config.getOption("IncrementalGrounding") &&
		    factory.ctx.config.getOption("FLPDecisionCriterionHead") && factory.ctx.config.getOption("FLPDecisionCriterionE")){
			DBGLOG(DBG, "No head- or e-cycles --> No FLP/UFS check necessary");
			return true;
		}else{
			if (factory.ctx.config.getOption("IncrementalGrounding")){
				DBGLOG(DBG, "Incremental mode needs to call UFS-checker to determine if there are head- or e-cycles");
			}else{
				DBGLOG(DBG, "Head- or e-cycles --> FLP/UFS check necessary");
			}

			// Explicit FLP check
			if (factory.ctx.config.getOption("FLPCheck")){
				DBGLOG(DBG, "FLP Check");
				// do FLP check (possibly with nogood learning) and add the learned nogoods to the main search
				bool result = isSubsetMinimalFLPModel<GenuineSolver>(compatibleSet, postprocessedInput, factory.ctx,
				                                                     factory.ctx.config.getOption("ExternalLearning") ? learnedEANogoods : SimpleNogoodContainerPtr());
				updateEANogoods(compatibleSet);
				return result;
			}

			// UFS check
			if (factory.ctx.config.getOption("UFSCheck")){
				DBGLOG(DBG, "UFS Check");
				bool result = unfoundedSetCheck(compatibleSet);
				updateEANogoods(compatibleSet);
				return result;
			}

			// no check
			return true;
		}
	}
	assert (false);
}

bool GenuineGuessAndCheckModelGenerator::incrementalDomainExpansion(InterpretationConstPtr model, std::vector<int>& expandedComponents){

	bool expanded = false;

	// expand subcomponent-wise iteratively
	std::pair<ComponentGraph::ComponentIterator, ComponentGraph::ComponentIterator> comps = subcompgraph->getComponents();
	int nr = 0;
	InterpretationConstPtr domainAtomsFromCurrentEA;
	for (ComponentGraph::ComponentIterator comp = comps.first; comp != comps.second; ++comp, ++nr){
		const ComponentGraph::ComponentInfo& ci = subcompgraph->getComponentInfo(*comp);

		// check if there are new domain atoms for this component
		DBGLOG(DBG, "Checking if component " << nr << " needs to be expanded");
		int oldDomainAtomCount = domainMaskPerComponent[nr]->mask()->getStorage().count();
		domainAtomsFromCurrentEA = computeExtensionOfDomainPredicates(ci, factory.ctx, model, gxidbPerComponent[nr], factory.deidbInnerEatoms, false);
		domainMaskPerComponent[nr]->updateMask();
		int newDomainAtomCount = domainMaskPerComponent[nr]->mask()->getStorage().count();
		assert (newDomainAtomCount >= oldDomainAtomCount && "number of domain atoms decreased");

		if (newDomainAtomCount > oldDomainAtomCount){
			DBGLOG(DBG, "Expanded domain of external atoms in component " << nr << " from " << oldDomainAtomCount << " to " << newDomainAtomCount);
			expandedComponents.push_back(nr);
			expanded |= true;
		}else{
			DBGLOG(DBG, "Did not expand domain of external atoms");
		}
	}

	return expanded;
}

namespace
{
	// collect all components on the way
	struct DFSVisitor:
	  public boost::default_dfs_visitor
	{
	  const ComponentGraph& cg;
	  ComponentGraph::ComponentSet& comps;
	  DFSVisitor(const ComponentGraph& cg, ComponentGraph::ComponentSet& comps): boost::default_dfs_visitor(), cg(cg), comps(comps) {}
	  DFSVisitor(const DFSVisitor& other): boost::default_dfs_visitor(), cg(other.cg), comps(other.comps) {}
	  template<typename GraphT>
	  void discover_vertex(ComponentGraph::Component comp, const GraphT&)
	  {
	    comps.insert(comp);
	  }
	};

	void transitivePredecessorComponents(const ComponentGraph& compgraph, ComponentGraph::Component from, ComponentGraph::ComponentSet& preds)
	{
	  // we need a hash map, as component graph is no graph with vecS-storage
	  //
	  typedef boost::unordered_map<ComponentGraph::Component, boost::default_color_type> CompColorHashMap;
	  typedef boost::associative_property_map<CompColorHashMap> CompColorMap;
	  CompColorHashMap ccWhiteHashMap;
	  // fill white hash map
	  ComponentGraph::ComponentIterator cit, cit_end;
	  for(boost::tie(cit, cit_end) = compgraph.getComponents();
	      cit != cit_end; ++cit)
	  {
	    //boost::put(ccWhiteHashMap, *cit, boost::white_color);
	    ccWhiteHashMap[*cit] = boost::white_color;
	  }
	  CompColorHashMap ccHashMap(ccWhiteHashMap);
	
	  //
	  // do DFS
	  //
	  DFSVisitor dfs_vis(compgraph, preds);
	  //LOG("doing dfs visit for root " << *itr);
	  boost::depth_first_visit(
	      compgraph.getInternalGraph(),
	      from, 
	      dfs_vis,
	      CompColorMap(ccHashMap));
	  DBGLOG(DBG,"predecessors of " << from << " are " << printrange(preds));
	}
}

void GenuineGuessAndCheckModelGenerator::incrementalProgramExpansion(const std::vector<int>& expandedComponents){

	// update rule index
	DBGLOG(DBG, "Updating rule index");
	for (; previousRuleCount < annotatedGroundProgram.getGroundProgram().idb.size(); previousRuleCount++){
		currentRules->setFact(annotatedGroundProgram.getGroundProgram().idb[previousRuleCount].address);
	}

	if (expandedComponents.size() == 0){
		LOG(DBG,"domain was not expanded -> skipping program expansion");
		return;
	}

	LOG(DBG,"domain was expanded -> regrounding");
	InterpretationPtr edb(new Interpretation(factory.reg));
	edb->add(*postprocessedInput);
	std::vector<ID> idb;
	ComponentGraph::ComponentSet componentsToGround;
	BOOST_FOREACH (int nr, expandedComponents){
		ComponentGraph::ComponentIterator comp = subcompgraph->getComponents().first;
		for (int i = 0; i < nr; ++i) comp++;
		DBGLOG(DBG,"regrounding component " << nr << " over domain " << *domainMaskPerComponent[nr]->mask());
		edb->add(*domainMaskPerComponent[nr]->mask());
		idb.insert(idb.end(), gxidbPerComponent[nr].begin(), gxidbPerComponent[nr].end());
		componentsToGround.insert(*comp);
	}

	// also ground components which depend on such components
	std::pair<ComponentGraph::ComponentIterator, ComponentGraph::ComponentIterator> comps = subcompgraph->getComponents();
	int nr = 0;
	for (ComponentGraph::ComponentIterator comp = comps.first; comp != comps.second; ++comp, ++nr){
		if (componentsToGround.find(*comp) != componentsToGround.end()) continue;

		ComponentGraph::ComponentSet preds;
		transitivePredecessorComponents(*subcompgraph, *comp, preds);
		for (ComponentGraph::ComponentSet::iterator pcomp = preds.begin(); pcomp != preds.end(); ++pcomp){
			if (componentsToGround.find(*pcomp) != componentsToGround.end()){
				DBGLOG(DBG, "grounding depending component " << nr);
				edb->add(*domainMaskPerComponent[nr]->mask());
				idb.insert(idb.end(), gxidbPerComponent[nr].begin(), gxidbPerComponent[nr].end());
			}
		}
	}

	OrdinaryASPProgram program(reg, idb, edb, factory.ctx.maxint);

	DBGLOG(DBG, "Grounding with frozen atoms " << *annotatedGroundProgram.getProgramMask());
	grounder = GenuineGrounder::getInstance(factory.ctx, program, annotatedGroundProgram.getProgramMask());

	// if a previously defined atom is redefined, then use its hook instead and prepare a new hook for future expansions
	// if an atom which was not previously defined is now defined, then leave it unchanged and prepare a hook for future expansions
	std::map<ID, ID> redefinedHookAtoms;
	OrdinaryASPProgram gpAddition = grounder->getGroundProgram();
	gpAddition.edb = InterpretationPtr(new Interpretation(reg));
	gpAddition.idb.clear();
	DBGLOG(DBG, "Checking if the new ground program redefines previously defined atoms");
	BOOST_FOREACH (ID ruleID, grounder->getGroundProgram().idb){
		// no need to add a rule if it is already present
		if (currentRules->getFact(ruleID.address)) continue;

		// check if a previously defined atom is redefined
		DBGLOG(DBG, "Processing new rule");
		const Rule& rule = reg->rules.getByID(ruleID);
		int hID = 0;
		BOOST_FOREACH (ID headID, rule.head){
			// no need to process the same atom twice
			if (redefinedHookAtoms.find(headID) != redefinedHookAtoms.end()) continue;

			// was the atom defined in previous expansions?
			if (hookAtoms.find(headID) != hookAtoms.end()){
				// yes: prepare a new hook atom
				redefinedHookAtoms[headID] = factory.reg->getAuxiliaryAtom('k', hookAtoms[headID]);
				frozenHookAtoms->setFact(redefinedHookAtoms[headID].address);

				DBGLOG(DBG, "Atom " << printToString<RawPrinter>(headID, factory.reg) << " was previously defined, will replace it by its hook " << printToString<RawPrinter>(hookAtoms[headID], factory.reg) <<
							" and prepare book " << printToString<RawPrinter>(redefinedHookAtoms[headID], factory.reg) << " for future extensions");

				// the old hook is not undefined anymore
				frozenHookAtoms->clearFact(hookAtoms[headID].address);

				// prepare new hook rule
				ID hookRuleID = getIncrementalHookRule(hookAtoms[headID], redefinedHookAtoms[headID]);
				gpAddition.idb.push_back(hookRuleID);
				DBGLOG(DBG, "Adding hook rule " << printToString<RawPrinter>(hookRuleID, factory.reg));
			}else{
				// no: prepare a hook atom for future extensions
				redefinedHookAtoms[headID] = factory.reg->getAuxiliaryAtom('k', headID);
				frozenHookAtoms->setFact(redefinedHookAtoms[headID].address);
				
				DBGLOG(DBG, "Atom " << printToString<RawPrinter>(headID, factory.reg) << " was previously not defined, will not replace it but prepare book " <<
							printToString<RawPrinter>(redefinedHookAtoms[headID], factory.reg) << " for future extensions");

				// prepare hook rule
				ID hookRuleID = getIncrementalHookRule(headID, redefinedHookAtoms[headID]);
				gpAddition.idb.push_back(hookRuleID);
				DBGLOG(DBG, "Adding hook rule " << printToString<RawPrinter>(hookRuleID, factory.reg));
			}
			hID++;
		}

	}

	DBGLOG(DBG, "Substituting atoms by their hooks");
	// 1. If the atom has no hooks from previous expansions, then it is unchanged
	// 2. If the atom has a hook from previous expansions, then use this hook (not the one from the new expansion!)
	BOOST_FOREACH (ID ruleID, grounder->getGroundProgram().idb){
		// no need to add a rule if it is already present
		if (currentRules->getFact(ruleID.address)) continue;

		// check if a previously defined atom is redefined
		DBGLOG(DBG, "Processing new rule");
		Rule rule = reg->rules.getByID(ruleID);
		for (int i = 0; i < rule.head.size(); ++i){
			ID headID = rule.head[i];
			if (hookAtoms.find(headID) != hookAtoms.end()) rule.head[i] = hookAtoms[headID];
		}
		for (int i = 0; i < rule.body.size(); ++i){
			ID bodyID = rule.body[i];
			if (hookAtoms.find(bodyID) != hookAtoms.end()) rule.body[i] =  hookAtoms[bodyID];
		}
		
		// add the new rule
		ID newRuleID = factory.reg->storeRule(rule);
		DBGLOG(DBG, "Rewriting rule " << printToString<RawPrinter>(ruleID, factory.reg) << " to " << printToString<RawPrinter>(newRuleID, factory.reg));
		gpAddition.idb.push_back(newRuleID);
	}

	// update hook atoms
	typedef std::pair<ID, ID> HookAtomDefinition;
	BOOST_FOREACH (HookAtomDefinition had, redefinedHookAtoms){
		// update hook atom for future extensions
		hookAtoms[had.first] = had.second;
	}

	// add model elimination constraints to avoid repetition of models
#ifdef DEBUG
	DBGLOG(DBG, "Adding model elimination constraints");
	BOOST_FOREACH (ID id, modelEliminationConstraints){
		DBGLOG(DBG, printToString<RawPrinter>(id, factory.reg));
	}
#endif
	gpAddition.idb.insert(gpAddition.idb.end(), modelEliminationConstraints.begin(), modelEliminationConstraints.end());
	modelEliminationConstraints.clear();

	// program expansion
	DBGLOG(DBG, "Program expansion is finalized as follows");

	DBGLOG(DBG, "Previous program:");
	DBGLOG(DBG, *annotatedGroundProgram.getGroundProgram().edb);
	BOOST_FOREACH (ID id, annotatedGroundProgram.getGroundProgram().idb){
		DBGLOG(DBG, printToString<RawPrinter>(id, factory.reg));
	}

	mask->add(*frozenHookAtoms);
	AnnotatedGroundProgram expansion(factory.ctx, gpAddition, factory.innerEatoms);
	annotatedGroundProgram.addProgram(expansion);

	DBGLOG(DBG, "Adding the following " << gpAddition.idb.size() << " rules:");
	DBGLOG(DBG, *expansion.getGroundProgram().edb);
	BOOST_FOREACH (ID id, expansion.getGroundProgram().idb){
		DBGLOG(DBG, printToString<RawPrinter>(id, factory.reg));
	}

	DBGLOG(DBG, "Complete ground program is now:");
	DBGLOG(DBG, *annotatedGroundProgram.getGroundProgram().edb);
	BOOST_FOREACH (ID id, annotatedGroundProgram.getGroundProgram().idb){
		DBGLOG(DBG, printToString<RawPrinter>(id, factory.reg));
	}

	solver->addProgram(expansion, frozenHookAtoms);
}

ID GenuineGuessAndCheckModelGenerator::getIncrementalHookRule(ID headAtomID, ID hookAtomID){

	Rule hookRule(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);
	hookRule.head.push_back(headAtomID);
	hookRule.body.push_back(ID::posLiteralFromAtom(hookAtomID));
	return factory.reg->storeRule(hookRule);
}

bool GenuineGuessAndCheckModelGenerator::unfoundedSetCheck(InterpretationConstPtr partialInterpretation, InterpretationConstPtr assigned, InterpretationConstPtr changed, bool partial){

	assert ( (!partial || (!!assigned && !!changed)) && "partial UFS checks require information about the assigned atoms");

	DBGLOG(DBG, "GenuineGuessAndCheckModelGenerator::unfoundedSetCheck");
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "genuine g&c unfoundedSetCheck");

	DBGLOG(DBG, "unfoundedSetCheck was called to perform " << (partial ? "partial" : "full") << " UFS check");

	bool performCheck = false;
	static std::set<ID> emptySkipProgram;
	const std::set<ID>* skipProgram = &emptySkipProgram;

	if (partial){
		assert (!!assigned && !!changed);

		DBGLOG(DBG, "Calling UFS check heuristic");
		UnfoundedSetCheckHeuristics::UnfoundedSetCheckHeuristicsResult decision = ufsCheckHeuristics->doUFSCheck(verifiedAuxes, partialInterpretation, assigned, changed);

		if (decision.doUFSCheck()){

			if (!factory.ctx.config.getOption("UFSCheck") && !factory.ctx.config.getOption("UFSCheckAssumptionBased")){
				LOG(WARNING, "Partial unfounded set checks are only possible if FLP check method is set to unfounded set check; will skip the check");
				return true;
			}

			// ufs check without nogood learning makes no sense if the interpretation is not complete
			if (!factory.ctx.config.getOption("UFSLearning")){
				LOG(WARNING, "Partial unfounded set checks is useless if unfounded set learning is not enabled; will perform the check anyway, but result does not have any effect");
			}

			DBGLOG(DBG, "Heuristic decides to do a partial UFS check");
			performCheck = true;
			skipProgram = &decision.skipProgram();

#ifndef NDEBUG
			// check if all replacement atoms in the non-skipped program for UFS checking are verified
			BOOST_FOREACH (ID ruleID, annotatedGroundProgram.getGroundProgram().idb){
				const Rule& rule = reg->rules.getByID(ruleID);
				if (decision.skipProgram().count(ruleID) > 0) continue;	// check only non-skipped rules
				if (rule.isEAGuessingRule()) continue;		// guessing rules are irrelevant for the UFS check

				BOOST_FOREACH (ID h, rule.head){
					assert (assigned->getFact(h.address) && "UFS checker heuristic tried to perform UFS check over program part with unassigned head atoms");
				}
				BOOST_FOREACH (ID b, rule.body){
					assert (assigned->getFact(b.address) && "UFS checker heuristic tried to perform UFS check over program part with unassigned body atoms");
					if (b.isExternalAuxiliary() && !b.isExternalInputAuxiliary()){
						assert (verifiedAuxes->getFact(b.address) && "UFS checker heuristic tried to perform UFS check over program part with non-verified external atom replacements");
					}
				}
			}
#endif
		}else{
			DBGLOG(DBG, "Heuristic decides not to do an UFS check");
		}
	}else{



		DBGLOG(DBG, "Since the method was called for a full check, it will be performed");
		performCheck = true;
	}

	if (performCheck){
		std::vector<IDAddress> ufs = ufscm->getUnfoundedSet(partialInterpretation, *skipProgram,
			                                            factory.ctx.config.getOption("ExternalLearning") ? learnedEANogoods : SimpleNogoodContainerPtr());
		bool ufsFound = (ufs.size() > 0);
#ifndef NDEBUG
		std::stringstream ss;
		ss << "UFS result: " << (ufsFound ? "" : "no ") << "UFS found (interpretation: " << *partialInterpretation;
		if (!!assigned) ss << ", assigned: " << *assigned;
		ss << ")";
		DBGLOG(DBG, ss.str());
#endif

		if (ufsFound && factory.ctx.config.getOption("UFSLearning")){
			Nogood ng = ufscm->getLastUFSNogood();
			DBGLOG(DBG, "Adding UFS nogood: " << ng);

#ifndef NDEBUG
			// the learned nogood must not talk about unassigned atoms
			if (!!assigned){
				BOOST_FOREACH (ID lit, ng){
					assert(assigned->getFact(lit.address));
				}
			}
#endif
			solver->addNogood(ng);
		}
		return !ufsFound;
	}else{
		return true;
	}
}

IDAddress GenuineGuessAndCheckModelGenerator::getWatchedLiteral(int eaIndex, InterpretationConstPtr search, bool truthValue){

	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "getWatchedLiteral");
	bm::bvector<>::enumerator eaDepAtoms = annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage().first();
	bm::bvector<>::enumerator eaDepAtoms_end = annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage().end();
	bm::bvector<>::enumerator searchb = search->getStorage().first();
	bm::bvector<>::enumerator searchb_end = search->getStorage().end();

	// go through eamask
	while (eaDepAtoms < eaDepAtoms_end){
		// if search bitset has correct truth value
		if (search->getFact(*eaDepAtoms) == truthValue){
			DBGLOG(DBG, "Found watch " << *eaDepAtoms << " for atom " << factory.innerEatoms[eaIndex]);
			return *eaDepAtoms;
		}
		eaDepAtoms++;
	}

	return ID::ALL_ONES;
}

void GenuineGuessAndCheckModelGenerator::unverifyExternalAtoms(InterpretationConstPtr changed){
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "genuine g&c unverifyEAtoms");

	DBGLOG(DBG, "Unverify External Atoms");

	bm::bvector<>::enumerator en;
	bm::bvector<>::enumerator en_begin;
	if (!!changed) en_begin = changed->getStorage().first();
	bm::bvector<>::enumerator en_end;
	if (!!changed) en_end = changed->getStorage().end();

	// for each eatom, if it is evaluated:
	// * look if there is a bit in changed that matches the eatom mask
	// * if yes:
	//     * unverify eatom and use the changed bit as new watch
	//     * if a watched atom was assigned, possibly evaluate the external atom (driven by a heuristics)

	// unverify/unfalsify external atoms which watch this atom
	for(unsigned eaIndex = 0; eaIndex < factory.innerEatoms.size(); ++eaIndex){
		if( !eaEvaluated[eaIndex] )
			// we don't need to verify watches because the eatom was not evaluated
			continue;

		  const InterpretationConstPtr& mask = annotatedGroundProgram.getEAMask(eaIndex)->mask();

		  // check if something in its mask was changed
		  bool unverify = false;
		  if (!!changed){
			  DBGLOG(DBG, "Checking if a changed atom belongs to the input of external atom " << factory.innerEatoms[eaIndex]);
			  en = en_begin;
			  while (en < en_end){
				if(mask->getFact(*en)){
					// yes, it changed, so we unverify and leave
					DBGLOG(DBG, "atom " << printToString<RawPrinter>(reg->ogatoms.getIDByAddress(*en), reg) <<
						    " changed and unverified external atom " <<
					printToString<RawPrinter>(factory.innerEatoms[eaIndex], reg));

					unverify = true;
					break;
				}
				en++;
			  }
		  }else{
			// everything changed (potentially): unverify all external atoms
			const ExternalAtom& eatom = reg->eatoms.getByID(factory.innerEatoms[eaIndex]);
		  	unverify = true;
		  }
		  if (unverify){
			// unverify
			eaVerified[eaIndex] = false;
			eaEvaluated[eaIndex] = false;
			verifiedAuxes->getStorage() -= annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage();

			// *en is our new watch (as it is either undefined or was recently changed)
			verifyWatchList[*en].push_back(eaIndex);
		}
	}

	DBGLOG(DBG, "Unverify External Atoms finished");
}

bool GenuineGuessAndCheckModelGenerator::verifyExternalAtoms(InterpretationConstPtr partialInterpretation, InterpretationConstPtr assigned, InterpretationConstPtr changed){

	// If there is no information about assigned or changed atoms, then we do not do anything.
	// This is because we would need to assume the worst (any atom might have changed and no atom is currently assigned).
	// Under these assumptions we cannot do any useful computation since we could only blindly evaluate any external atom,
	// but this can also be done later (when we have a concrete compatible set).
	if (!assigned || !changed) return false;

	DBGLOG(DBG, "Verify External Atoms");

	// go through all changed atoms which are now assigned
	bool conflict = false;
	bm::bvector<>::enumerator en = changed->getStorage().first();
	bm::bvector<>::enumerator en_end = changed->getStorage().end();
	while (en < en_end){
		if (assigned->getFact(*en)){

			// for all external atoms which watch this atom
			BOOST_FOREACH (int eaIndex, verifyWatchList[*en]){
				if (!eaEvaluated[eaIndex]){
					const ExternalAtom& eatom = reg->eatoms.getByID(factory.innerEatoms[eaIndex]);

					// update set of changed input atoms
					if (eatom.getExtSourceProperties().doesCareAboutChanged()){
						assert (!!changedAtomsPerExternalAtom[eaIndex]);
						changedAtomsPerExternalAtom[eaIndex]->add(*changed);
					}

					// evaluate external atom if the heuristics decides so
					bool doEval;
					{
						DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "genuine g&c verifyEAtoms eh");
						assert (!!eaEvalHeuristics[eaIndex]);
						doEval = eaEvalHeuristics[eaIndex]->doEvaluate(eatom,
						                                                annotatedGroundProgram.getEAMask(eaIndex)->mask(),
						                                                annotatedGroundProgram.getProgramMask(),
						                                                partialInterpretation, assigned, changed);
					}
					if (doEval){
						// evaluate it
						DBGLOG(DBG, "Heuristic decides to evaluate external atom " << factory.innerEatoms[eaIndex]);
						conflict |= (verifyExternalAtom(eaIndex, partialInterpretation, assigned, changed));

						// update set of changed input atoms
						if (eatom.getExtSourceProperties().doesCareAboutChanged()){
							assert (!!changedAtomsPerExternalAtom[eaIndex]);
							changedAtomsPerExternalAtom[eaIndex]->clear();
						}
					}
					if (!eaEvaluated[eaIndex]){
						// find a new yet unassigned atom to watch
						IDAddress id = getWatchedLiteral(eaIndex, assigned, false);
						if (id != ID::ALL_ONES) verifyWatchList[id].push_back(eaIndex);
					}
				}
			}
			// current atom was set, so remove all watches
			verifyWatchList[*en].clear();
		}

		en++;
	}

	DBGLOG(DBG, "Verify External Atoms finished " << (conflict ? "with" : "without") << " conflict");

	return conflict;
}

bool GenuineGuessAndCheckModelGenerator::verifyExternalAtom(int eaIndex, InterpretationConstPtr partialInterpretation, InterpretationConstPtr assigned, InterpretationConstPtr changed){

	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "genuine g&c verifyEAtom");

	const ExternalAtom& eatom = reg->eatoms.getByID(factory.innerEatoms[eaIndex]);

	// if support sets are enabled, and the external atom provides complete support sets, we use them for verification
	if (!assigned && !changed && factory.ctx.config.getOption("SupportSets") &&
	    (eatom.getExtSourceProperties().providesCompletePositiveSupportSets() || eatom.getExtSourceProperties().providesCompleteNegativeSupportSets()) &&
	    annotatedGroundProgram.allowsForVerificationUsingCompleteSupportSets()){
		return verifyExternalAtomBySupportSets(eaIndex, partialInterpretation, assigned, changed);
	}else{
		return verifyExternalAtomByEvaluation(eaIndex, partialInterpretation, assigned, changed);
	}
}

bool GenuineGuessAndCheckModelGenerator::verifyExternalAtomByEvaluation(int eaIndex, InterpretationConstPtr partialInterpretation, InterpretationConstPtr assigned, InterpretationConstPtr changed){

	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "genuine g&c verifyEAtom by evaluation");

	const ExternalAtom& eatom = reg->eatoms.getByID(factory.innerEatoms[eaIndex]);

	// prepare EA evaluation
	InterpretationConstPtr mask = (annotatedGroundProgram.getEAMask(eaIndex)->mask());
	VerifyExternalAtomCB vcb(partialInterpretation, eatom, *(annotatedGroundProgram.getEAMask(eaIndex)));

	InterpretationConstPtr evalIntr = partialInterpretation;
	if (!factory.ctx.config.getOption("IncludeAuxInputInAuxiliaries")){
		// make sure that ALL input auxiliary atoms are true, otherwise we might miss some output atoms and consider true output atoms wrongly as unfounded
		// clone and extend
		InterpretationPtr ncevalIntr(new Interpretation(*partialInterpretation));
		ncevalIntr->getStorage() |= annotatedGroundProgram.getEAMask(eaIndex)->getAuxInputMask()->getStorage();
		evalIntr = ncevalIntr;
	}

	// evaluate the external atom and learn nogoods if external learning is used
	DBGLOG(DBG, "Verifying external Atom " << factory.innerEatoms[eaIndex] << " under " << *evalIntr);
	evaluateExternalAtom(factory.ctx, eatom, evalIntr, vcb,
	                     factory.ctx.config.getOption("ExternalLearning") ? learnedEANogoods : NogoodContainerPtr(), assigned, changed);
	updateEANogoods(partialInterpretation, assigned, changed);

	// if the input to the external atom was complete, then remember the verification result;
	// for incomplete input we cannot yet decide this yet, evaluation is only done for learning purposes in this case
	if( !assigned ||
	    !bm::any_sub(annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage() & annotatedGroundProgram.getProgramMask()->getStorage(),
	                 assigned->getStorage() & annotatedGroundProgram.getProgramMask()->getStorage() ) ) {
		eaVerified[eaIndex] = vcb.verify();

		DBGLOG(DBG, "Verifying " << factory.innerEatoms[eaIndex] << " (Result: " << eaVerified[eaIndex] << ")");

		// we remember that we evaluated, only if there is a propagator that can undo this memory (that can unverify an eatom during model search)
		if(factory.ctx.config.getOption("NoPropagator") == 0){
			eaEvaluated[eaIndex] = true;
			if (eaVerified[eaIndex]) verifiedAuxes->getStorage() |= annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage();
		}

		return !eaVerified[eaIndex];
	}else{
		return false;
	}
}

bool GenuineGuessAndCheckModelGenerator::verifyExternalAtomBySupportSets(int eaIndex, InterpretationConstPtr partialInterpretation, InterpretationConstPtr assigned, InterpretationConstPtr changed){

	assert (!assigned && !changed && " verification using complete support sets is only possible wrt. complete interpretations");
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "genuine g&c verifyEAtom by suoport sets");

	eaVerified[eaIndex] = annotatedGroundProgram.verifyExternalAtomsUsingCompleteSupportSets(eaIndex, partialInterpretation, InterpretationPtr());
	if (eaVerified[eaIndex]) verifiedAuxes->getStorage() |= annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage();

	// we remember that we evaluated, only if there is a propagator that can undo this memory (that can unverify an eatom during model search)
	if( factory.ctx.config.getOption("NoPropagator") == 0 ){
		eaEvaluated[eaIndex] = true;
	}

	return !eaVerified[eaIndex];
}

const OrdinaryASPProgram& GenuineGuessAndCheckModelGenerator::getGroundProgram(){
	return annotatedGroundProgram.getGroundProgram();
}

void GenuineGuessAndCheckModelGenerator::propagate(InterpretationConstPtr partialAssignment, InterpretationConstPtr assigned, InterpretationConstPtr changed){

	assert (!!partialAssignment && !!assigned && !!changed);

	// update external atom verification results
	// (1) unverify external atoms if atoms, which are relevant to this external atom, have (potentially) changed
	unverifyExternalAtoms(changed);
	// (2) now verify external atoms (driven by a heuristic)
	bool conflict = verifyExternalAtoms(partialAssignment, assigned, changed);

	// UFS check can in principle also applied to conflicting assignments
	// since the heuristic knows which external atoms are correct and which ones not.
	// The check can be restricted to the non-conflicting part of the program.
	// Although there is already another reason for backtracking,
	// we still need to notify the heuristics such that it can update its internal information about assigned atoms.
	assert (!!ufsCheckHeuristics);
	if (!conflict) unfoundedSetCheck(partialAssignment, assigned, changed, true);
	else ufsCheckHeuristics->notify(verifiedAuxes, partialAssignment, assigned, changed);
}

DLVHEX_NAMESPACE_END

// vi:ts=8:noexpandtab:
