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
 * @file   FLPModelGeneratorBase.tcc
 * @author Peter Schüller
 * @author Christoph Redl
 * 
 * @brief  Implementation of generic FLP check.
 */

#ifndef DLVHEX_FLPMODELGENERATORBASE_TCC_INCLUDED
#define DLVHEX_FLPMODELGENERATORBASE_TCC_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/FLPModelGeneratorBase.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Nogood.h"
#include "dlvhex2/Benchmarking.h"

#include <fstream>

DLVHEX_NAMESPACE_BEGIN


namespace
{

// this is the naive external solver without nogoods
template<typename AnyOrdinaryASPSolverT>
struct ExternalSolverHelper
{
  static NogoodContainerPtr getNogoodContainer(
    boost::shared_ptr<AnyOrdinaryASPSolverT> solverPtr)
      { return NogoodContainerPtr(); }

  static int addNogood(
    boost::shared_ptr<AnyOrdinaryASPSolverT> solverPtr, Nogood ng)
      { throw std::runtime_error("nogoods not supported with this solver!"); }
};

// this is GenuineSolver from which we know it supports nogoods
// (create specializations for further nogood-capable external solvers)
template<>
struct ExternalSolverHelper<GenuineSolver>
{
  static NogoodContainerPtr getNogoodContainer(
    boost::shared_ptr<GenuineSolver> solverPtr)
      { return solverPtr; }
  static void addNogood(
    boost::shared_ptr<GenuineSolver> solverPtr, Nogood ng)
      { solverPtr->addNogood(ng); }
};

}

template<typename OrdinaryASPSolverT>
bool FLPModelGeneratorBase::isSubsetMinimalFLPModel(
		InterpretationConstPtr compatibleSet,
		InterpretationConstPtr postprocessedInput,
		ProgramCtx& ctx,
		SimpleNogoodContainerPtr ngc)
{
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE_TPL(sidflpcheck, "Explicit FLP Check");

  typedef boost::shared_ptr<OrdinaryASPSolverT> OrdinaryASPSolverTPtr;

	RegistryPtr& reg = factory.reg;
  std::vector<ID>& innerEatoms = factory.innerEatoms;
  PredicateMask& gpMask = factory.gpMask;
  PredicateMask& gnMask = factory.gnMask;
  PredicateMask& fMask = factory.fMask;
  std::vector<ID>& xidbflphead = factory.xidbflphead;
  std::vector<ID>& xidbflpbody = factory.xidbflpbody;
  std::vector<ID>& gidb = factory.gidb;

	/*
	* FLP check:
	* Check if the flp reduct of the program has a model which is a proper subset of modelCandidate
	* 
	* This check is done as follows:
	* 1. evaluate edb + xidbflphead + M
	*    -> yields singleton answer set containing flp heads F for non-blocked rules
	* 2. evaluate edb + xidbflpbody + gidb + F
	*    -> yields candidate compatible models Cand[1], ..., Cand[n] of the reduct
	* 3. check each Cand[i] for compatibility (just as the check above for modelCandidate)
	*    -> yields compatible reduct models Comp[1], ...,, Comp[m], m <= n
	* 4. for all i: project modelCandidate and Comp[i] to ordinary atoms (remove flp and replacement atoms)
	* 5. if for some i, projected Comp[i] is a proper subset of projected modelCandidate, modelCandidate is rejected,
	*    otherwise it is a subset-minimal model of the flp reduct
	*/
	InterpretationPtr flpas;
	{
		DBGLOG(DBG,"evaluating flp head program");

		// here we can mask, we won't lose FLP heads
		OrdinaryASPProgram flpheadprogram(reg, xidbflphead, compatibleSet, ctx.maxint);
		OrdinaryASPSolverTPtr flpheadsolver = OrdinaryASPSolverT::getInstance(ctx, flpheadprogram);

		flpas = flpheadsolver->getNextModel();
		if( flpas == InterpretationPtr() )
		{
			DBGLOG(DBG, "FLP head program yielded no answer set");
			assert(false);
		}else{
			DBGLOG(DBG, "FLP head program yielded at least one answer set");
		}
	}
	DBGLOG(DBG,"got FLP head model " << *flpas);

	// evaluate xidbflpbody+gidb+edb+flp
	std::stringstream ss;
	RawPrinter printer(ss, ctx.registry());
	int flpm = 0;
	{
		DBGLOG(DBG, "evaluating flp body program");

		// build edb+flp
		Interpretation::Ptr reductEDB(new Interpretation(reg));
		fMask.updateMask();
		reductEDB->getStorage() |= flpas->getStorage() & fMask.mask()->getStorage();
		reductEDB->add(*postprocessedInput);

		std::vector<ID> simulatedReduct = xidbflpbody;
		// add guessing program to flpbody program
		BOOST_FOREACH (ID rid, gidb){
			simulatedReduct.push_back(rid);
		}

		static const bool encodeMinimalityCheckIntoReduct = true;

		std::map<ID, std::pair<int, ID> > shadowPredicates, unfoundedPredicates;
		// predicate postfix for shadow and unfounded predicates
		std::string shadowpostfix, unfoundedpostfix;
		computeShadowAndUnfoundedPredicates(reg, postprocessedInput, simulatedReduct, shadowPredicates, unfoundedPredicates, shadowpostfix, unfoundedpostfix);
		Interpretation::Ptr shadowInterpretation(new Interpretation(reg));
		addShadowInterpretation(reg, shadowPredicates, compatibleSet, shadowInterpretation);
		if (encodeMinimalityCheckIntoReduct){
			// add minimality rules to flpbody program
			createMinimalityRules(reg, shadowPredicates, shadowpostfix, simulatedReduct);
		}
		createFoundingRules(reg, shadowPredicates, unfoundedPredicates, simulatedReduct);
		reductEDB->add(*shadowInterpretation);		// make the FLP check know the compatible set in order to search for subsets
		if (postprocessedInput){
			reductEDB->add(*postprocessedInput);	// facts are always in the reduct
		}

		ss << "simulatedReduct: IDB={";
		printer.printmany(simulatedReduct, "\n");
		ss << "}\nEDB=" << *reductEDB;
		LOG(DBG, "Evaluating simulated reduct: " << ss.str());

		OrdinaryASPProgram flpbodyprogram(reg, simulatedReduct, reductEDB, ctx.maxint);
    OrdinaryASPSolverTPtr flpbodysolver = OrdinaryASPSolverT::getInstance(ctx, flpbodyprogram);

		// transfer learned nogoods to new solver
		if (ngc != NogoodContainerPtr()){
			for (int i = 0; i < ngc->getNogoodCount(); ++i){
				if (ngc->getNogood(i).isGround()){
				        ExternalSolverHelper<OrdinaryASPSolverT>::addNogood(flpbodysolver, ngc->getNogood(i));
				}
			}
		}

		DLVHEX_BENCHMARK_REGISTER(sidflpenum, "FLP-Reduct Solving");
		DLVHEX_BENCHMARK_START(sidflpenum);
		InterpretationPtr flpbodyas = flpbodysolver->getNextModel();
		DLVHEX_BENCHMARK_STOP(sidflpenum);
		DLVHEX_BENCHMARK_REGISTER(flpcandidates, "Checked FLP reduct models");
		while(flpbodyas != InterpretationPtr())
		{
			DLVHEX_BENCHMARK_COUNT(flpcandidates,1);

			// compatibility check
			DBGLOG(DBG, "doing compatibility check for reduct model candidate " << *flpbodyas);
      NogoodContainerPtr bodySolverNogoods =
        ExternalSolverHelper<OrdinaryASPSolverT>::getNogoodContainer(flpbodysolver);
			bool compatible;
			int ngCount = ngc ? ngc->getNogoodCount() : 0;
			compatible = isCompatibleSet(flpbodyas, postprocessedInput, ctx, ngc);
			if (ngc){
				for (int i = ngCount; i < ngc->getNogoodCount(); ++i){
					if (ngc->getNogood(i).isGround()){
						bodySolverNogoods->addNogood(ngc->getNogood(i));
					}
				}
			}
			DBGLOG(DBG, "Compatibility: " << compatible);

			// remove input and shadow input (because it not contained in modelCandidate neither)
			flpbodyas->getStorage() -= postprocessedInput->getStorage();
			DBGLOG(DBG, "Removed input facts: " << *flpbodyas);

			if (compatible){
				// check if the reduct model is smaller than modelCandidate
				if (encodeMinimalityCheckIntoReduct){
					// reduct model is a proper subset (this was already ensured by the program encoding)
					DBGLOG(DBG, "Model candidate " << *compatibleSet << " failed FLP check");
					DBGLOG(DBG, "Enumerated " << flpm << " FLP models");
/*
					{
						InterpretationPtr candidate(new Interpretation(*compatibleSet));
						candidate->getStorage() -= gpMask.mask()->getStorage();
						candidate->getStorage() -= gnMask.mask()->getStorage();
						candidate->getStorage() -= postprocessedInput->getStorage();

						flpbodyas->getStorage() -= gpMask.mask()->getStorage();
						flpbodyas->getStorage() -= gnMask.mask()->getStorage();
						flpbodyas->getStorage() -= fMask.mask()->getStorage();

						constructFLPNogood(ctx, groundProgram, compatibleSet, candidate, flpbodyas);
					}
*/
					DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sidfailedflpchecks, "Failed FLP Checks", 1);
					return false;
				}else{
					// project both the model candidate and the reduct model to ordinary atoms
					InterpretationPtr candidate(new Interpretation(*compatibleSet));
					candidate->getStorage() -= gpMask.mask()->getStorage();
					candidate->getStorage() -= gnMask.mask()->getStorage();
					candidate->getStorage() -= postprocessedInput->getStorage();

					flpbodyas->getStorage() -= gpMask.mask()->getStorage();
					flpbodyas->getStorage() -= gnMask.mask()->getStorage();
					flpbodyas->getStorage() -= fMask.mask()->getStorage();

					DBGLOG(DBG, "Checking if reduct model " << *flpbodyas << " is a subset of model candidate " << *candidate);

					if ((candidate->getStorage() & flpbodyas->getStorage()).count() == flpbodyas->getStorage().count() &&	// subset property
					     candidate->getStorage().count() > flpbodyas->getStorage().count()){				// proper subset property
						// found a smaller model of the reduct
						flpm++;
						DBGLOG(DBG, "Model candidate " << *candidate << " failed FLP check");
						DBGLOG(DBG, "Enumerated " << flpm << " FLP models");

						DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sidfailedflpchecks, "Failed FLP Checks", 1);
						return false;
					}else{
						DBGLOG(DBG, "Reduct model is no proper subset");
						flpm++;
					}
				}
			}

			DBGLOG(DBG, "Go to next model of reduct");
			DLVHEX_BENCHMARK_START(sidflpenum);
			flpbodyas = flpbodysolver->getNextModel();
			DLVHEX_BENCHMARK_STOP(sidflpenum);
		}
	}

	DBGLOG(DBG, "Model candidate " << *compatibleSet << " passed FLP check");			
	DBGLOG(DBG, "Enumerated " << flpm << " FLP models");

	return true;
}

template<typename OrdinaryASPSolverT>
InterpretationConstPtr FLPModelGeneratorBase::computeExtensionOfDomainPredicates(const ComponentGraph::ComponentInfo& ci, ProgramCtx& ctx, InterpretationConstPtr edb){
#define nAdvancedTechnique

	InterpretationPtr domintr = InterpretationPtr(new Interpretation(factory.reg));
	int oldDomintrCount = 0;
	int loop = 1;

	// if there are no inner external atoms, then there is nothing to do
	if (factory.deidbInnerEatoms.size() == 0) return InterpretationPtr(new Interpretation(factory.reg));

	typedef boost::shared_ptr<OrdinaryASPSolverT> OrdinaryASPSolverTPtr;

	// compute the fixpoint of the positive program wrt. edb
	DBGLOG(DBG, "Computing fixpoint of extensions of domain predicates");
	std::vector<InterpretationPtr> ints(2);
	unsigned current = 0;
	ints[0] = InterpretationPtr(new Interpretation(*edb));
	ints[1] = InterpretationPtr(new Interpretation(*edb));
	do
	{
		InterpretationPtr src = ints[current];
		InterpretationPtr dst = ints[1-current];
		DBGLOG(DBG, "Loop with src=" << *src << " and dst=" << *dst);

		// evaluate inner external atoms
		IntegrateExternalAnswerIntoInterpretationCB cb(dst);
		BOOST_FOREACH (ID eaid, factory.deidbInnerEatoms){
			const ExternalAtom& ea = factory.reg->eatoms.getByID(eaid);

			// remove all atoms over antimonotonic parameters from the input interpretation (both in standard and in higher-order notation)
			// in order to maximize the output;
			// for nonmonotonic input atoms, enumerate all exponentially many assignments
			boost::unordered_map<IDAddress, bool> nonmonotonicinput;
			InterpretationPtr input(new Interpretation(factory.reg));
			input->add(*src);
			ea.updatePredicateInputMask();
			bm::bvector<>::enumerator en = ea.getPredicateInputMask()->getStorage().first();
			bm::bvector<>::enumerator en_end = ea.getPredicateInputMask()->getStorage().end();
			while (en < en_end){
				const OrdinaryAtom& ogatom = factory.reg->ogatoms.getByAddress(*en);

				for (int i = 0; i < ea.inputs.size(); ++i){
					if (ea.pluginAtom->getInputType(i) == PluginAtom::PREDICATE &&
					    ea.getExtSourceProperties().isAntimonotonic(i) &&
					    ogatom.tuple[0] == ea.inputs[i]){
						DBGLOG(DBG, "Setting " << *en << " to false because it is an antimonotonic input atom");
						input->clearFact(*en);
					}
					if (ea.pluginAtom->getInputType(i) == PluginAtom::PREDICATE &&
					    !ea.getExtSourceProperties().isAntimonotonic(i) &&
					    !ea.getExtSourceProperties().isMonotonic(i) &&
					    ogatom.tuple[0] == ea.inputs[i]){
						// if the predicate is defined in this component, enumerate all possible assignments
						if (ci.predicatesInComponent.count(ea.inputs[i]) > 0){
							DBGLOG(DBG, "Must guess all assignments to " << *en << " because it is a nonmonotonic and unstratified input atom");
							nonmonotonicinput[*en] = false;
						}
						// otherwise: take the truth value from the edb
						else{
							if (!edb->getFact(*en)){
								DBGLOG(DBG, "Setting " << *en << " to false because it is stratified and false in the edb");
								input->clearFact(*en);
							}
						}
					}
				}
				en++;
			}

			DBGLOG(DBG, "Enumerating nonmonotonic input assignments to " << eaid);
			bool allOnes;
			do
			{
				// set nonmonotonic input
				allOnes = true;
				typedef std::pair<IDAddress, bool> Pair;
				BOOST_FOREACH (Pair p, nonmonotonicinput){
					if (p.second) input->setFact(p.first);
					else{
						input->clearFact(p.first);
						allOnes = false;
					}
				}

				// evalute external atom
				DBGLOG(DBG, "Evaluating external atom " << eaid << " under " << *input);
				evaluateExternalAtom(ctx, ea, input, cb);

				// enumerate next assignment to nonmonotonic input atoms
				if (!allOnes){
					std::vector<IDAddress> clear;
					BOOST_FOREACH (Pair p, nonmonotonicinput){
						if (p.second) clear.push_back(p.first);
						else{
							nonmonotonicinput[p.first] = true;
							break;
						}
					}
					BOOST_FOREACH (IDAddress c, clear) nonmonotonicinput[c] = false;
				}
			}while(!allOnes);
			DBGLOG(DBG, "Enumerated all nonmonotonic input assignments to " << eaid);
		}

#ifdef AdvancedTechnique
		// translate new EA-replacements to domain atoms
		bm::bvector<>::enumerator en = dst->getStorage().first();
		bm::bvector<>::enumerator en_end = dst->getStorage().end();
		while (en < en_end){
			ID id = factory.reg->ogatoms.getIDByAddress(*en);
			if (id.isExternalAuxiliary()){
				DBGLOG(DBG, "Converting atom with address " << *en);

				const OrdinaryAtom& ogatom = factory.reg->ogatoms.getByAddress(*en);
				BOOST_FOREACH (ID eaid, factory.deidbInnerEatoms){
					const ExternalAtom ea = factory.reg->eatoms.getByID(eaid);
					if (ea.predicate == factory.reg->getIDByAuxiliaryConstantSymbol(ogatom.tuple[0])){

						OrdinaryAtom domatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN | ID::PROPERTY_AUX);
						domatom.tuple.push_back(factory.reg->getAuxiliaryConstantSymbol('d', eaid));
						int io = 1;
						if (ea.auxInputPredicate != ID_FAIL && factory.ctx.config.getOption("IncludeAuxInputInAuxiliaries")) io = 2;
						for (int i = io + ea.inputs.size(); i < ogatom.tuple.size(); ++i){
							domatom.tuple.push_back(ogatom.tuple[i]);
						}
						domintr->setFact(factory.reg->storeOrdinaryGAtom(domatom).address);
					}
				}
			}
			en++;
		}
		dst->getStorage() |= domintr->getStorage();
		DBGLOG(DBG, "Domain extension interpretation: " << *domintr);

		// ground program
		domintr->getStorage() |= edb->getStorage();
		OrdinaryASPProgram program(factory.reg, factory.deidb, domintr, ctx.maxint);
		GenuineGrounderPtr grounder = GenuineGrounder::getInstance(ctx, program);

		// retrieve maximum set of ground atoms in the ground program
		dst->getStorage() |= grounder->getGroundProgram().edb->getStorage();
		BOOST_FOREACH (ID rid, grounder->getGroundProgram().idb){
			const Rule& r = factory.reg->rules.getByID(rid);
			BOOST_FOREACH (ID h, r.head) dst->setFact(h.address);
			BOOST_FOREACH (ID b, r.body) dst->setFact(b.address);
		}

		// grounding and EA-evaluation can both expand the active domain,
		// so the overall loop must run 2 times without changed before we can be sure that we have reached the fixpoint
		if (domintr->getStorage().count() == oldDomintrCount){
			if (loop == 0){
				DBGLOG(DBG, "Reached fixpoint: " << *dst);
				break;
			}
			loop--;
		}else loop = 1;
		oldDomintrCount = domintr->getStorage().count();

		// switch interpretations
		current = 1 - current;
	}while(true);

#else
		// solve program
		OrdinaryASPProgram program(factory.reg, factory.deidb, dst, ctx.maxint);
		OrdinaryASPSolverTPtr solver = OrdinaryASPSolverT::getInstance(ctx, program);

		InterpretationPtr model = solver->getNextModel();
		assert(model != InterpretationPtr());
		InterpretationPtr model2 = solver->getNextModel();
		assert(model2 == InterpretationPtr());
		dst->getStorage().swap(model->getStorage());

		int cmpresult = dst->getStorage().compare(src->getStorage());
		if( cmpresult == 0 )
		{
			DBGLOG(DBG, "Reached fixpoint: " << *dst);
			break;
		}

		// switch interpretations
		current = 1 - current;
	}while(true);

	// create explicit representation of domain predicate extensions
	DBGLOG(DBG, "Creating extension of domain predicates");

	// translate EA-replacements to domain atoms
	bm::bvector<>::enumerator en = ints[current]->getStorage().first();
	bm::bvector<>::enumerator en_end = ints[current]->getStorage().end();
	while (en < en_end){
		ID id = factory.reg->ogatoms.getIDByAddress(*en);
		if (id.isExternalAuxiliary()){
			DBGLOG(DBG, "Converting atom with address " << *en);

			const OrdinaryAtom& ogatom = factory.reg->ogatoms.getByAddress(*en);
			BOOST_FOREACH (ID eaid, factory.deidbInnerEatoms){
				const ExternalAtom ea = factory.reg->eatoms.getByID(eaid);
				if (ea.predicate == factory.reg->getIDByAuxiliaryConstantSymbol(ogatom.tuple[0])){

					OrdinaryAtom domatom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYN | ID::PROPERTY_AUX);
					domatom.tuple.push_back(factory.reg->getAuxiliaryConstantSymbol('d', eaid));
					int io = 1;
					if (ea.auxInputPredicate != ID_FAIL && factory.ctx.config.getOption("IncludeAuxInputInAuxiliaries")) io = 2;
					for (int i = io + ea.inputs.size(); i < ogatom.tuple.size(); ++i){
						domatom.tuple.push_back(ogatom.tuple[i]);
					}
					domintr->setFact(factory.reg->storeOrdinaryGAtom(domatom).address);
				}
			}
		}
		en++;
	}
	DBGLOG(DBG, "Domain extension interpretation: " << *domintr);

#endif

	return domintr;
}

DLVHEX_NAMESPACE_END

#endif // DLVHEX_FLPMODELGENERATORBASE_TCC_INCLUDED

// Local Variables:
// mode: C++
// End:
