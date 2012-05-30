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
 * @file   UnfoundedSetChecker.cpp
 * @author Chrisoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Unfounded set checker for programs with disjunctions and external atoms.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/BaseModelGenerator.h"
#include "dlvhex2/UnfoundedSetChecker.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Benchmarking.h"

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/visitors.hpp> 
#include <boost/graph/strong_components.hpp>

#include <fstream>

DLVHEX_NAMESPACE_BEGIN

UnfoundedSetChecker::UnfoundedSetChecker(
			ProgramCtx& ctx,
			const OrdinaryASPProgram& groundProgram,
			InterpretationConstPtr interpretation,
			std::set<ID> skipProgram,
			InterpretationConstPtr componentAtoms,
			NogoodContainerPtr ngc) :
	ggncmg(0),
	ctx(ctx),
	groundProgram(groundProgram),
	compatibleSet(interpretation),
	compatibleSetWithoutAux(interpretation),
	skipProgram(skipProgram),
	componentAtoms(componentAtoms),
	ngc(ngc),
	mode(Ordinary),
  domain(new Interpretation(ctx.registry())){

	reg = ctx.registry();

	// remove skipped rules from IDB
	BOOST_FOREACH (ID ruleId, groundProgram.idb){
		const Rule& rule = reg->rules.getByID(ruleId);
		if (std::find(skipProgram.begin(), skipProgram.end(), ruleId) != skipProgram.end()){	// ignored part of the program
			// skip it
		}else{
			ufsProgram.push_back(ruleId);
		}
	}

#ifndef NDEBUG
	std::stringstream programstring;
	RawPrinter printer(programstring, reg);
	if (groundProgram.edb) programstring << "EDB: " << *groundProgram.edb << std::endl;
	programstring << "IDB:" << std::endl;
	BOOST_FOREACH (ID ruleId, ufsProgram){
		printer.print(ruleId);
		programstring << std::endl;
	}
	DBGLOG(DBG, "Computing unfounded set of program:" << std::endl << programstring.str() << std::endl << "with respect to interpretation" << std::endl << *compatibleSetWithoutAux << " (" << *compatibleSet << ")");
#endif

	// construct the UFS detection problem
	constructUFSDetectionProblem();
}

UnfoundedSetChecker::UnfoundedSetChecker(
			GenuineGuessAndCheckModelGenerator& ggncmg,
			ProgramCtx& ctx,
			const OrdinaryASPProgram& groundProgram,
			const std::vector<ID>& innerEatoms,
			InterpretationConstPtr compatibleSet,
			std::set<ID> skipProgram,
			InterpretationConstPtr componentAtoms,
			NogoodContainerPtr ngc) :
	ggncmg(&ggncmg),
	innerEatoms(innerEatoms),
	ctx(ctx),
	groundProgram(groundProgram),
	compatibleSet(compatibleSet),
	skipProgram(skipProgram),
	componentAtoms(componentAtoms),
	ngc(ngc),
	mode(WithExt),
  domain(new Interpretation(ctx.registry())){

	reg = ctx.registry();

	// remove auxiliaries from interpretation
	compatibleSetWithoutAux = compatibleSet->getInterpretationWithoutExternalAtomAuxiliaries();

	// remove external atom guessing rules and skipped rules from IDB
	BOOST_FOREACH (ID ruleId, groundProgram.idb){
		const Rule& rule = reg->rules.getByID(ruleId);
		if (rule.isEAGuessingRule() ||								// EA-guessing rule
		    std::find(skipProgram.begin(), skipProgram.end(), ruleId) != skipProgram.end()){	// ignored part of the program
			// skip it
		}else{
			ufsProgram.push_back(ruleId);
		}
	}

#ifndef NDEBUG
	std::stringstream programstring;
	RawPrinter printer(programstring, reg);
	if (groundProgram.edb) programstring << "EDB: " << *groundProgram.edb << std::endl;
	programstring << "IDB:" << std::endl;
	BOOST_FOREACH (ID ruleId, ufsProgram){
		printer.print(ruleId);
		programstring << std::endl;
	}
	DBGLOG(DBG, "Computing unfounded set of program:" << std::endl << programstring.str() << std::endl << "with respect to interpretation" << std::endl << *compatibleSetWithoutAux << " (" << *compatibleSet << ")");
#endif

	// construct the UFS detection problem
	constructUFSDetectionProblem();
}

void UnfoundedSetChecker::constructUFSDetectionProblem(){

//DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidcc, "UFS Detection Problem Construction");

	constructUFSDetectionProblemNecessaryPart();
	constructUFSDetectionProblemOptimizationPart();
}

void UnfoundedSetChecker::constructUFSDetectionProblemNecessaryPart(){

#ifndef NDEBUG
	std::stringstream programstring;
	RawPrinter printer(programstring, reg);
#endif

	DBGLOG(DBG, "Constructing necessary part of UFS detection problem");
	DBGLOG(DBG, "N: Facts");
	// facts cannot be in X
	if (groundProgram.edb) {
		bm::bvector<>::enumerator en = groundProgram.edb->getStorage().first();
		bm::bvector<>::enumerator en_end = groundProgram.edb->getStorage().end();
		while (en < en_end){
			domain->setFact(*en);
			Nogood ng;
			ng.insert(NogoodContainer::createLiteral(*en, true));
			ufsDetectionProblem.addNogood(ng);
			en++;
		}
	}

	// we want a UFS which intersects with I
	DBGLOG(DBG, "N: Intersection with I");
	{
		Nogood ng;
		bm::bvector<>::enumerator en = compatibleSetWithoutAux->getStorage().first();
		bm::bvector<>::enumerator en_end = compatibleSetWithoutAux->getStorage().end();
		while (en < en_end){
			if (!componentAtoms || componentAtoms->getFact(*en)){
				ng.insert(NogoodContainer::createLiteral(*en, false));
			}
			en++;
		}
		ufsDetectionProblem.addNogood(ng);
	}

	DBGLOG(DBG, "N: Rules");
	BOOST_FOREACH (ID ruleID, ufsProgram){
#ifndef NDEBUG
		programstring.str("");
		printer.print(ruleID);
		DBGLOG(DBG, "Processing rule " << programstring.str());
#endif

		const Rule& rule = reg->rules.getByID(ruleID);

		// condition 1 is handled directly: skip rules with unsatisfied body
		bool unsatisfied = false;
		BOOST_FOREACH (ID b, rule.body){
			if (compatibleSet->getFact(b.address) != !b.isNaf()){
				unsatisfied = true;
				break;
			}
		}
		if (unsatisfied) continue;

		// Compute the set of problem variables: this is the set of all atoms which (1) occur in the head of some rule; or (2) are external atom auxiliaries
		BOOST_FOREACH (ID h, rule.head) domain->setFact(h.address);
		BOOST_FOREACH (ID b, rule.body) domain->setFact(b.address);

		// Create two unique predicates and atoms for this rule
		OrdinaryAtom hratom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX);
		hratom.tuple.push_back(reg->getAuxiliaryConstantSymbol('k', ruleID));
		ID hr = reg->storeOrdinaryGAtom(hratom);

		// hr is true iff one of the rule's head atoms is in X
		{
			Nogood ng;
			ng.insert(NogoodContainer::createLiteral(hr.address, true));
			BOOST_FOREACH (ID h, rule.head){
				ng.insert(NogoodContainer::createLiteral(h.address, false));
			}
			ufsDetectionProblem.addNogood(ng);
		}
		{
			BOOST_FOREACH (ID h, rule.head){
				Nogood ng;
				ng.insert(NogoodContainer::createLiteral(hr.address, false));
				ng.insert(NogoodContainer::createLiteral(h.address, true));
				ufsDetectionProblem.addNogood(ng);
			}
		}

		{
			Nogood ng;
			// if hr is true, then it must not happen that neither Condition 2 nor Condition 3 is satisfied
			ng.insert(NogoodContainer::createLiteral(hr.address, true));

			// Condition 2: some body literal b, which is true in I, is false under I u -X
			// If b is ordinary (or considered to be ordinary), then this can only happen if b is positive because for a negative b, I \models b implies I u -X \models b
			// if b is external, then it can be either positive or negative because due to nonmonotonicity we might have I \models b but I u -X \not\models b (even if b is negative)
			// That is: It must not happen that
			//	1. all ordinary positive body atoms, which are true in I, are not in the unfounded set; and
			//	2. all external literals are true under I u -X 
			BOOST_FOREACH (ID b, rule.body){
				if (!b.isExternalAuxiliary() || mode == Ordinary){
					// ordinary literal
					if (!b.isNaf() && compatibleSet->getFact(b.address)){
						ng.insert(NogoodContainer::createLiteral(b.address, false));
					}
				}else{
					// external literal
					ng.insert(NogoodContainer::createLiteral(b.address, !b.isNaf()));
				}
			}

			// Condition 3: some head atom, which is true in I, is not in the unfounded set
			// That is: It must not happen, that all positive head atoms, which are true in I, are in the unfounded set (then the condition is not satisfied)
			BOOST_FOREACH (ID h, rule.head){
				if (compatibleSet->getFact(h.address)){
					ng.insert(NogoodContainer::createLiteral(h.address, true));
				}
			}
			ufsDetectionProblem.addNogood(ng);
		}
	}

	// the ufs must not contain a head atom of an ignored rule
	// (otherwise we cannot guarantee that the ufs remains an ufs for completed interpretations)
	DBGLOG(DBG, "N: Ignored rules");
	{
		BOOST_FOREACH (ID ruleId, skipProgram){
			const Rule& rule = reg->rules.getByID(ruleId);
			BOOST_FOREACH (ID h, rule.head){
				Nogood ng;
				ng.insert(NogoodContainer::createLiteral(h.address, true));
				ufsDetectionProblem.addNogood(ng);
			}
		}
	}

	// the ufs must not contain an atom which is external to the component
	if (componentAtoms){
		DBGLOG(DBG, "N: Restrict search to strongly connected component");
		bm::bvector<>::enumerator en = domain->getStorage().first();
		bm::bvector<>::enumerator en_end = domain->getStorage().end();
		while (en < en_end){
			if ((!reg->ogatoms.getIDByAddress(*en).isExternalAuxiliary() || mode == Ordinary) && !componentAtoms->getFact(*en)){
				Nogood ng;
				ng.insert(NogoodContainer::createLiteral(*en, true));
				ufsDetectionProblem.addNogood(ng);
			}
			en++;
		}
	}
}

void UnfoundedSetChecker::constructUFSDetectionProblemOptimizationPart(){

	DBGLOG(DBG, "Constructing optimization part of UFS detection problem");
	constructUFSDetectionProblemOptimizationPartRestrictToCompatibleSet();
	if (mode == WithExt){
		constructUFSDetectionProblemOptimizationPartBasicEAKnowledge();
		constructUFSDetectionProblemOptimizationPartLearnedFromMainSearch();

		// use this optimization only if external learning is off; the two optimizations can influence each other and cause spurious contradictions
		if (!ngc) constructUFSDetectionProblemOptimizationPartEAEnforement();
	}
}

void UnfoundedSetChecker::constructUFSDetectionProblemOptimizationPartRestrictToCompatibleSet(){

	// ordinary atoms not in I must not be in the unfounded set
	DBGLOG(DBG, "O: Ordinary atoms not in I must not be in the unfounded set");
	BOOST_FOREACH (ID ruleID, ufsProgram){
		const Rule& rule = reg->rules.getByID(ruleID);
		BOOST_FOREACH (ID h, rule.head){
			if (!compatibleSet->getFact(h.address)){
				Nogood ng;
				ng.insert(NogoodContainer::createLiteral(h.address, true));
				ufsDetectionProblem.addNogood(ng);
			}
		}
		BOOST_FOREACH (ID b, rule.body){
			if ((!b.isExternalAuxiliary() || mode == Ordinary) && !compatibleSet->getFact(b.address)){
				Nogood ng;
				ng.insert(NogoodContainer::createLiteral(b.address, true));
				ufsDetectionProblem.addNogood(ng);
			}
		}
	}
}

void UnfoundedSetChecker::constructUFSDetectionProblemOptimizationPartBasicEAKnowledge(){

	// if none of the input atoms to an external atom, which are true in I, are in the unfounded set, then the truth value of the external atom cannot change
	DBGLOG(DBG, "O: Adding basic knowledge about external atom behavior");
	for (int eaIndex = 0; eaIndex < innerEatoms.size(); ++eaIndex){
		const ExternalAtom& eatom = reg->eatoms.getByID(innerEatoms[eaIndex]);

		eatom.updatePredicateInputMask();

		// if none of the input atoms, which are true in I, are unfounded, then the output of the external atom does not change
		Nogood inputNogood;
		bm::bvector<>::enumerator en = eatom.getPredicateInputMask()->getStorage().first();
		bm::bvector<>::enumerator en_end = eatom.getPredicateInputMask()->getStorage().end();
		bool skip = false;
		while (en < en_end){
			if (compatibleSet->getFact(*en)){
				// T a \in I
				if ( !domain->getFact(*en) ){
					// atom is true for sure in I u -X
				}else{
					// atom might be true in I u -X
					inputNogood.insert(NogoodContainer::createLiteral(*en, false));
				}
			}else{
				// F a \in I
				if ( !domain->getFact(*en) ){
					// atom is also false for sure in I u -X
				}
			}
			en++;
		}
		if (skip){
			continue;
		}

		// go through the output atoms
		ggncmg->eaMasks[eaIndex].updateMask();
		en = ggncmg->eaMasks[eaIndex].mask()->getStorage().first();
		en_end = ggncmg->eaMasks[eaIndex].mask()->getStorage().end();
		while (en < en_end){
			if (reg->ogatoms.getIDByAddress(*en).isExternalAuxiliary()){
				// do not extend the variable domain (this is counterproductive)
				if ( domain->getFact(*en) ){
					Nogood ng = inputNogood;
					ng.insert(NogoodContainer::createLiteral(*en, !compatibleSet->getFact(*en)));
					ufsDetectionProblem.addNogood(ng);
				}
			}
			en++;
		}
	}
}

void UnfoundedSetChecker::constructUFSDetectionProblemOptimizationPartLearnedFromMainSearch(){

	// add the learned nogoods (in transformed form)
	if (ngc){
		DBGLOG(DBG, "O: Adding valid input-output relationships from nogood container");
		for (int i = 0; i < ngc->getNogoodCount(); ++i){
			const Nogood& ng = ngc->getNogood(i);
			DBGLOG(DBG, "Processing learned nogood " << ng);

			std::vector<Nogood> transformed = nogoodTransformation(ng, compatibleSet);
			BOOST_FOREACH (Nogood tng, transformed){
				ufsDetectionProblem.addNogood(tng);
			}
		}
	}
}

void UnfoundedSetChecker::constructUFSDetectionProblemOptimizationPartEAEnforement(){

	// if there is no necessity to change the truth value of an external atom compared to compatibleSet, then do not do it
	// (this makes the postcheck cheaper)
	DBGLOG(DBG, "O: Enforcement of external atom truth values");

	// make aux('c', r) false iff one of B^{+}_o(r), which is true in compatibleSet, is true or one of H(r), which is true in compatibleSet, is false
	BOOST_FOREACH (ID ruleID, ufsProgram){
		const Rule& rule = reg->rules.getByID(ruleID);

		OrdinaryAtom cratom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX);
		cratom.tuple.push_back(reg->getAuxiliaryConstantSymbol('c', ruleID));
		ID cr = reg->storeOrdinaryGAtom(cratom);

		// check if condition 1 applies for this rule
		bool condition1 = false;
		BOOST_FOREACH (ID b, rule.body){
			if (compatibleSet->getFact(b.address) != !b.isNaf()){
				// yes: set aux('c', r) to false
				condition1 = true;
				Nogood falsifyCr;
				falsifyCr.insert(NogoodContainer::createLiteral(cr.address, true));
				ufsDetectionProblem.addNogood(falsifyCr);
				break;
			}
		}

		if (!condition1){
			Nogood ngnot;
			BOOST_FOREACH (ID b, rule.body){
				if (!b.isNaf() && !b.isExternalAuxiliary() && compatibleSet->getFact(b.address)){
					DBGLOG(DBG, "Binding positive body atom to c " << cr);
					Nogood ng;
					ng.insert(NogoodContainer::createLiteral(cr.address, true));
					ng.insert(NogoodContainer::createLiteral(b.address, true));
					ufsDetectionProblem.addNogood(ng);

					ngnot.insert(NogoodContainer::createLiteral(b.address, false));
				}
			}
			BOOST_FOREACH (ID h, rule.head){
				if (compatibleSet->getFact(h.address)){
					DBGLOG(DBG, "Binding head atom to c " << cr);
					Nogood ng;
					ng.insert(NogoodContainer::createLiteral(cr.address, true));
					ng.insert(NogoodContainer::createLiteral(h.address, false));
					ufsDetectionProblem.addNogood(ng);

					ngnot.insert(NogoodContainer::createLiteral(h.address, true));
				}
			}
			DBGLOG(DBG, "Negated nogood for c " << cr);
			ngnot.insert(NogoodContainer::createLiteral(cr.address, false));
			ufsDetectionProblem.addNogood(ngnot);
		}
	}

	// for all external atom auxiliaries
	std::set<IDAddress> eaAuxes;
	boost::unordered_map<IDAddress, std::vector<ID> > eaAuxToRule;
	bm::bvector<>::enumerator en = compatibleSet->getStorage().first();
	bm::bvector<>::enumerator en_end = compatibleSet->getStorage().end();
	while (en < en_end){
		if (reg->ogatoms.getIDByAddress(*en).isExternalAuxiliary()){
			eaAuxes.insert(*en);
		}
		en++;
	}
	BOOST_FOREACH (ID ruleID, ufsProgram){
		const Rule& rule = reg->rules.getByID(ruleID);
		BOOST_FOREACH (ID b, rule.body){
			if (b.isExternalAuxiliary()){
				eaAuxes.insert(b.address);
				eaAuxToRule[b.address].push_back(ruleID);
			}
		}
	}
	BOOST_FOREACH (IDAddress eaAux, eaAuxes){
		// if all aux('c', r) are false for all rules where eaAux occurs ...
		Nogood ng;
		BOOST_FOREACH (ID ruleID, eaAuxToRule[eaAux]){
			OrdinaryAtom cratom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX);
			cratom.tuple.push_back(reg->getAuxiliaryConstantSymbol('c', ruleID));
			ID cr = reg->storeOrdinaryGAtom(cratom);
			ng.insert(NogoodContainer::createLiteral(cr.address, false));
		}
		// then force aux to the same truth value as in compatibleSet
		ng.insert(NogoodContainer::createLiteral(eaAux, !compatibleSet->getFact(eaAux)));
		DBGLOG(DBG, "Enforcement of ea truth value");
		ufsDetectionProblem.addNogood(ng);
	}

/*
std::stringstream nss;
RawPrinter nprinter(nss, reg);
BOOST_FOREACH (Nogood ng, ns.nogoods){
nss << "{ ";
BOOST_FOREACH (ID l, ng){
nprinter.print(l);
nss << ", ";
}
nss << "}" << std::endl;
}
DBGLOG(DBG, "UFSDP: " << nss.str());
*/
}

#if 0
NogoodSet FLPModelGeneratorBase::getUFSDetectionProblem(
		ProgramCtx& ctx,
		OrdinaryASPProgram groundProgram,
		std::vector<ID> ufsProgram,
		InterpretationConstPtr compatibleSet /* I */,
		InterpretationConstPtr compatibleSetWithoutAux,
		std::set<ID> skipProgram){

	RegistryPtr reg = ctx.registry();

#ifndef NDEBUG
	std::stringstream programstring;
	RawPrinter printer(programstring, reg);
#endif

	// problem instance
	NogoodSet ns;

	// facts cannot be in X
	{
		bm::bvector<>::enumerator en = groundProgram.edb->getStorage().first();
		bm::bvector<>::enumerator en_end = groundProgram.edb->getStorage().end();
		while (en < en_end){
			Nogood ng;
			ng.insert(NogoodContainer::createLiteral(*en, true));
			ns.addNogood(ng);
			en++;
		}
	}
/*
	// atoms not in I must not be true in the unfounded set
	BOOST_FOREACH (ID ruleID, ufsProgram){
		const Rule& rule = reg->rules.getByID(ruleID);
		BOOST_FOREACH (ID h, rule.head){
			if (!compatibleSet->getFact(h.address)){
				Nogood ng;
				ng.insert(NogoodContainer::createLiteral(h.address, true));
				ns.addNogood(ng);
			}
		}
		BOOST_FOREACH (ID b, rule.body){
			if (!compatibleSet->getFact(b.address)){
				Nogood ng;
				ng.insert(NogoodContainer::createLiteral(b.address, true));
				ns.addNogood(ng);
			}
		}
	}
*/
	// create nogoods for all rules of the ufs program
	Nogood c2Relevance;
	BOOST_FOREACH (ID ruleID, ufsProgram){
#ifndef NDEBUG
		programstring.str("");
		printer.print(ruleID);
		DBGLOG(DBG, "Processing rule " << programstring.str());
#endif

		const Rule& rule = reg->rules.getByID(ruleID);

		// skip rules with unsatisfied body
		bool unsatisfied = false;
		BOOST_FOREACH (ID b, rule.body){
			if (compatibleSet->getFact(b.address) != !b.isNaf()){
				unsatisfied = true;
				break;
			}
		}
		if (unsatisfied) continue;

		// create two unique predicates and atoms for this rule
		OrdinaryAtom hratom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX);
		hratom.tuple.push_back(reg->getAuxiliaryConstantSymbol('k', ruleID));
		OrdinaryAtom cratom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX);
		cratom.tuple.push_back(reg->getAuxiliaryConstantSymbol('c', ruleID));
		ID hr = reg->storeOrdinaryGAtom(hratom);
		ID cr = reg->storeOrdinaryGAtom(cratom);

		// hr is true iff one of the rule's head atoms is in X
		{
			Nogood ng;
			ng.insert(NogoodContainer::createLiteral(hr.address, true));
			BOOST_FOREACH (ID h, rule.head){
				ng.insert(NogoodContainer::createLiteral(h.address, false));
			}
			ns.addNogood(ng);
		}
		{
			BOOST_FOREACH (ID h, rule.head){
				Nogood ng;
				ng.insert(NogoodContainer::createLiteral(hr.address, false));
				ng.insert(NogoodContainer::createLiteral(h.address, true));
				ns.addNogood(ng);
			}
		}

		// the 3 conditions of the unfounded set definition
		// (actually only 2, because rule satisfaction is already checked above)
		{
			// if hr is true, then either ...
			Nogood ng;
			ng.insert(NogoodContainer::createLiteral(hr.address, true));

			// (condition 3) a head literal, which is true in the interpretation I, is not in the unfounded set X
			BOOST_FOREACH (ID h, rule.head){
				if (compatibleSetWithoutAux->getFact(h.address)){
					ng.insert(NogoodContainer::createLiteral(h.address, true));
				}
			}

			// (condition 2) a body literal is false wrt "interpretation union negated unfounded set"
			ng.insert(NogoodContainer::createLiteral(cr.address, false));
			ns.addNogood(ng);
		}
		{
			// (o) if hr is false, then condition 2 does not matter, so do not enumerate the truth values in this case
			Nogood ng;
			ng.insert(NogoodContainer::createLiteral(hr.address, false));
			ng.insert(NogoodContainer::createLiteral(cr.address, true));
			ns.addNogood(ng);
		}
		// (condition 2) a body literal is false wrt "interpretation union negated unfounded set" (I u -X)
		{
			// condition 2 is satisfied if a positive ordinary body literal is in the unfounded set
			BOOST_FOREACH (ID b, rule.body){
				if (!b.isNaf() && !b.isExternalAuxiliary()){
					Nogood ng;
					ng.insert(NogoodContainer::createLiteral(cr.address, false));
					ng.insert(NogoodContainer::createLiteral(b.address, true));

					// this literal is very important: if the head is false, then do not derive T cr, otherwise we have a contradiction with the optimization (o)
					ng.insert(NogoodContainer::createLiteral(hr.address, true));
					ns.addNogood(ng);
				}
			}
			// condition 2 is falsified if (i) no positive ordinary body literal is in the unfounded set, and (ii) the relevant input atoms to the external sources in (I u -X) coincide with I
			Nogood ng;
			ng.insert(NogoodContainer::createLiteral(cr.address, true));
			BOOST_FOREACH (ID b, rule.body){
				if (!b.isNaf() && !b.isExternalAuxiliary()){
					ng.insert(NogoodContainer::createLiteral(b.address, false));
				}
			}
			// for all inner external atoms which occur in this rule
			DBGLOG(DBG, "Collecting the external atoms this rule depends on");
			BOOST_FOREACH (ID extatId, innerEatoms){
				const ExternalAtom& extat = reg->eatoms.getByID(extatId);

				ID eaauxPos = reg->getAuxiliaryConstantSymbol('r', extat.predicate);
				ID eaauxNeg = reg->getAuxiliaryConstantSymbol('n', extat.predicate);
				bool sign;

				DBGLOG(DBG, "Comparing EA-Aux " << eaauxPos << "/" << eaauxNeg	 << " to rule body");
				bool occurs = false;
				BOOST_FOREACH (ID b, rule.body){
					const OrdinaryAtom& ogatom = reg->ogatoms.getByID(b);
					DBGLOG(DBG, "Comparing External Atom with aux " << eaauxPos << "/" << eaauxNeg << " to atom " << ogatom << " (ID: " << b << ")");
					// compare predicate
					if (ogatom.tuple[0] == eaauxPos || ogatom.tuple[0] == eaauxNeg){
						// compare parameters
						occurs = true;
						for (int i = 0; i < extat.inputs.size(); ++i){
							if (extat.pluginAtom->getInputType(i) == PluginAtom::PREDICATE && extat.inputs[i] != ogatom.tuple[1 + i]){
								DBGLOG(DBG, "Mismatch at parameter position " << i);
								occurs = false;
								break;
							}
						}
						if (occurs){
							sign = (ogatom.tuple[0] == eaauxPos);
							break;
						}
					}
				}
				// add the input to this external atom iff
				// (i) it is over a nonmonotonic input predicate (neither monotonic nor antimonotonic)
				// (ii) the external atom is positive and the input atom is over a monotonic predicate
				// (iii) the external atom is negative and the input atom is over an antimonotonic predicate
				if (occurs){
					DBGLOG(DBG, "Depends on " << extatId);
					extat.updatePredicateInputMask();
					InterpretationConstPtr extInput = extat.getPredicateInputMask();
					DBGLOG(DBG, "Input is " << *extInput);

					bm::bvector<>::enumerator en = extInput->getStorage().first();
					bm::bvector<>::enumerator en_end = extInput->getStorage().end();
					while (en < en_end){
						// check predicate of this atom
						const OrdinaryAtom& inputAtom = reg->ogatoms.getByAddress(*en);
						int parIndex = 0;
						BOOST_FOREACH (ID p, extat.inputs){
							if (extat.pluginAtom->getInputType(parIndex) == PluginAtom::PREDICATE && p == inputAtom.tuple[0]) break;
							parIndex++;
						}
						assert(parIndex < extat.inputs.size());
						bool monotonic = extat.pluginAtom->isMonotonic(extat.useProp ? extat.prop : extat.pluginAtom->getExtSourceProperties(), parIndex);
						bool antimonotonic = extat.pluginAtom->isAntimonotonic(extat.useProp ? extat.prop : extat.pluginAtom->getExtSourceProperties(), parIndex);

						if ((!monotonic && !antimonotonic) || (monotonic && sign == true) || (antimonotonic && sign == false)){
							if (compatibleSetWithoutAux->getFact(*en)){
								ng.insert(NogoodContainer::createLiteral(*en, false));
							}
						}
						en++;
					}
				}
			}
			ns.addNogood(ng);
		}

		// condition 2 needs to be relevant at least once
		c2Relevance.insert(NogoodContainer::createLiteral(cr.address, false));
	}

	// we want a UFS which intersects with I
	{
		Nogood ng;
		bm::bvector<>::enumerator en = compatibleSetWithoutAux->getStorage().first();
		bm::bvector<>::enumerator en_end = compatibleSetWithoutAux->getStorage().end();
		while (en < en_end){
			ng.insert(NogoodContainer::createLiteral(*en, false));
			en++;
		}
		ns.addNogood(ng);
	}

	// condition 2 needs to be relevant at least once
	ns.addNogood(c2Relevance);

	// an unfounded set must contain at least one atom over a cyclic input predicate
	{
		factory.cyclicInputPredicatesMask.updateMask();	// make sure that new atoms are detected
		DBGLOG(DBG, "Cyclic input atoms: " << *factory.cyclicInputPredicatesMask.mask());
		Nogood cyclicInputNogood;
		bm::bvector<>::enumerator en = factory.cyclicInputPredicatesMask.mask()->getStorage().first();
		bm::bvector<>::enumerator en_end = factory.cyclicInputPredicatesMask.mask()->getStorage().end();
		while (en < en_end){
			cyclicInputNogood.insert(NogoodContainer::createLiteral(*en, false));
			en++;
		}
		ns.addNogood(cyclicInputNogood);
	}

	// the ufs must not contain a head atom of an ignored rule
	// (otherwise we cannot guarantee that the ufs remains an ufs for completed interpretations)
	{
		BOOST_FOREACH (ID ruleId, skipProgram){
			const Rule& rule = reg->rules.getByID(ruleId);
			BOOST_FOREACH (ID h, rule.head){
				Nogood ng;
				ng.insert(NogoodContainer::createLiteral(h));
				DBGLOG(DBG, "Adding nogood for skipped program: " << ng);
				ns.addNogood(ng);
			}
		}
	}

#ifndef NDEBUG
	std::stringstream ss;
	BOOST_FOREACH (Nogood ng, ns.nogoods){
		ss << ng << " ";
	}
	DBGLOG(DBG, "Constructed the following UFS detection program: " << ss.str());
#endif

	return ns;
}
#endif


bool UnfoundedSetChecker::isUnfoundedSet(InterpretationConstPtr ufsCandidate){

//DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(ssiufs1, "UnfoundedSetChecker::isUnfoundedSet1");

	// ordinary mode generates only real unfounded sets, hence there is no check required
	assert(mode == WithExt);

	DBGLOG(DBG, "Checking if " << *ufsCandidate << " is an unfounded set");

	// check for each EA auxiliary in the UFS, if the atom is indeed unfounded
	std::vector<IDAddress> auxiliariesToVerify;		// the auxiliaries which's falsity needs to be checked
	std::vector<std::set<ID> > auxiliaryDependsOnEA;	// stores for each auxiliary A the external atoms which are remain to be evaluated before the truth/falsity of A is certain
	std::map<ID, std::vector<int> > eaToAuxIndex;		// stores for each external atom index the indices in the above vector which depend on this external atom

/*
	// collect all external atom auxiliaries which changed their truth value from compatibleSet to ufsCandidate
	// and insert them into the above data structures
	DBGLOG(DBG, "Collecting auxiliaries with changed truth value");
	InterpretationPtr changed = InterpretationPtr(new Interpretation(reg));
	changed->getStorage() = ufsCandidate->getStorage() ^ compatibleSet->getStorage();
	bm::bvector<>::enumerator en = changed->getStorage().first();
	bm::bvector<>::enumerator en_end = changed->getStorage().end();

	Nogood ng;
  {
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(dbgscope,"isUnfoundedSet preparation");
    while (en < en_end){
      if( domain->getFact(*en) && reg->ogatoms.getIDByAddress(*en).isExternalAuxiliary() ){
        auxiliariesToVerify.push_back(*en);
        std::set<ID> s;
        s.insert(ggncmg->auxToEA[*en].begin(), ggncmg->auxToEA[*en].end());
        auxiliaryDependsOnEA.push_back(s);
        BOOST_FOREACH (ID eaID, ggncmg->auxToEA[*en]){
          eaToAuxIndex[eaID].push_back(auxiliaryDependsOnEA.size() - 1);
        }
      }
      en++;
    }
  }
*/
	typedef std::pair<IDAddress, std::vector<ID> > Pair;
	BOOST_FOREACH (Pair p, ggncmg->auxToEA){
		IDAddress aux = p.first;
		if (ufsCandidate->getFact(aux) != compatibleSet->getFact(aux)){
			if (domain->getFact(aux) && reg->ogatoms.getIDByAddress(aux).isExternalAuxiliary()){
				auxiliariesToVerify.push_back(aux);
				std::set<ID> s;
				s.insert(ggncmg->auxToEA[aux].begin(), ggncmg->auxToEA[aux].end());
				auxiliaryDependsOnEA.push_back(s);
				BOOST_FOREACH (ID eaID, ggncmg->auxToEA[aux]){
					eaToAuxIndex[eaID].push_back(auxiliaryDependsOnEA.size() - 1);
				}
			}
		}
	}

	// construct: compatibleSetWithoutAux - ufsCandidate
	DBGLOG(DBG, "Constructing input interpretation for external atom evaluation");
	InterpretationPtr eaResult = InterpretationPtr(new Interpretation(reg));
	eaResult->add(*compatibleSetWithoutAux);
	eaResult->getStorage() -= ufsCandidate->getStorage();

	BaseModelGenerator::IntegrateExternalAnswerIntoInterpretationCB cb(eaResult);

	// now evaluate one external atom after the other and check if the new truth value is justified
	DBGLOG(DBG, "Evaluating external atoms");
	for (int eaIndex = 0; eaIndex < innerEatoms.size(); ++eaIndex){
		ID eaID = innerEatoms[eaIndex];
		const ExternalAtom& eatom = reg->eatoms.getByID(eaID);

		// evaluate
		DBGLOG(DBG, "Evaluate " << eaID << " for UFS verification, ngc=" << (!!ngc ? "true" : "false"));

		if (ngc){
			// evaluate the external atom with learned, and add the learned nogoods in transformed form to the UFS detection problem
			int oldNogoodCount = ngc->getNogoodCount();
			ggncmg->evaluateExternalAtom(reg, eatom, eaResult, cb, &ctx, ngc);
			DBGLOG(DBG, "O: Adding new valid input-output relationships from nogood container");
			for (int i = oldNogoodCount; i < ngc->getNogoodCount(); ++i){
				const Nogood& ng = ngc->getNogood(i);
				DBGLOG(DBG, "Processing learned nogood " << ng);

				std::vector<Nogood> transformed = nogoodTransformation(ng, compatibleSet);
				BOOST_FOREACH (Nogood tng, transformed){
					solver->addNogood(tng);
				}
			}
		}else{
			ggncmg->evaluateExternalAtom(reg, eatom, eaResult, cb);
		}

		// remove the external atom from the remaining lists
		BOOST_FOREACH (int i, eaToAuxIndex[eaID]){
			if ( !auxiliaryDependsOnEA[i].empty() ){
				auxiliaryDependsOnEA[i].erase(eaID);
				// if no external atoms remain to be verified, then the truth/falsity of the auxiliary is finally known
				if ( auxiliaryDependsOnEA[i].empty() ){
					// check if the auxiliary, which was assumed to be unfounded, is indeed _not_ in eaResult
					if (eaResult->getFact(auxiliariesToVerify[i]) != ufsCandidate->getFact(auxiliariesToVerify[i])){
						// wrong guess: the auxiliary is _not_ unfounded
						DBGLOG(DBG, "Truth value of auxiliary " << auxiliariesToVerify[i] << " is not justified --> Candidate is not an unfounded set");
						DBGLOG(DBG, "Evaluated " << i << " of " << innerEatoms.size() << " external atoms");
						return false;
					}else{
						DBGLOG(DBG, "Truth value of auxiliary " << auxiliariesToVerify[i] << " is justified");
					}
				}
			}
    }
	}
	DBGLOG(DBG, "Evaluated " << innerEatoms.size() << " of " << innerEatoms.size() << " external atoms");

	DBGLOG(DBG, "Candidate is an unfounded set");
	return true;
}

#if 0
bool UnfoundedSetChecker::isUnfoundedSet(ProgramCtx& ctx, std::vector<ID> ufsProgram, InterpretationConstPtr ufsCandidate, InterpretationConstPtr compatibleSet, InterpretationConstPtr compatibleSetWithoutAux){

	RegistryPtr reg = ctx.registry();

	// check for each rule with aux('c', ruleID) = true if a positive ordinary body atom is in the unfounded set
	// if not, then the rule has to be verified wrt. external atoms
	std::vector<ID> verifyRule;
	std::vector<ID> verifyEA;
	BOOST_FOREACH (ID ruleID, ufsProgram){
		const Rule& rule = reg->rules.getByID(ruleID);
		OrdinaryAtom cratom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX);
		cratom.tuple.push_back(reg->getAuxiliaryConstantSymbol('c', ruleID));
		ID cr = reg->storeOrdinaryGAtom(cratom);

		// was aux('c', ruleID) derived?
		if (ufsCandidate->getFact(cr.address)){
			// is a positive ordinary atom in the UFS?
			bool c2satisfied = false;
			BOOST_FOREACH (ID b, rule.body){
				if (!b.isExternalAuxiliary() && !b.isNaf() && ufsCandidate->getFact(b.address)){
					c2satisfied = true;
					break;
				}
			}
			// if not, then satisfaction of condition 2 might be spurious, so we need to verify the condition
			if (!c2satisfied){
				DBGLOG(DBG, "Need to verify if condition 2 for " << ruleID << " is satisfied");
				verifyRule.push_back(ruleID);
				// we need to evaluate all external atoms involved in this rule
				BOOST_FOREACH (ID b, rule.body){
					if (b.isExternalAuxiliary()){
						BOOST_FOREACH (ID ea, ggncmg.auxToEA[b.address]){
							if (std::find(verifyEA.begin(), verifyEA.end(), ea) == verifyEA.end()){
								DBGLOG(DBG, "Need to evaluate EA " << ea);
								verifyEA.push_back(ea);
							}
						}
					}
				}
			}
		}
	}

	// if no rule needs to be verified, then the candidate is for sure an unfounded set
	if (verifyRule.size() == 0) return true;

	// compute I u -X
	InterpretationPtr intr2 = InterpretationPtr(new Interpretation(reg));
	intr2->add(*compatibleSetWithoutAux);
	intr2->getStorage() -= ufsCandidate->getStorage();
	DBGLOG(DBG, "I u -X: " << *intr2);



std::vector<std::set<ID> > toFalsify;			// one element from each set needs to be falsified
std::map<IDAddress, std::set<int> > toFalsifyIndex;	// stores for each address the vector elements where the address occurs
BOOST_FOREACH (ID ruleID, verifyRule){
	OrdinaryAtom cratom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX);
	cratom.tuple.push_back(reg->getAuxiliaryConstantSymbol('c', ruleID));
	ID cr = reg->storeOrdinaryGAtom(cratom);

#ifndef NDEBUG
	std::stringstream ss;
#endif

	std::set<ID> s;
	if (ufsCandidate->getFact(cr.address)){
		const Rule& rule = reg->rules.getByID(ruleID);
		// collect auxiliaries
		BOOST_FOREACH (ID b, rule.body){
			if (b.isExternalAuxiliary()){
				s.insert(b);
#ifndef NDEBUG
				ss << (b.isNaf() ? "-" : "") << b.address << " ";
#endif
				toFalsifyIndex[b.address].insert(toFalsify.size());
			}
		}
	}
#ifndef NDEBUG
	DBGLOG(DBG, toFalsify.size() << ": { " << ss.str() << "}");
#endif
	toFalsify.push_back(s);
}

// evaluate one external atom after the other
IntegrateExternalAnswerIntoInterpretationCB cbb(intr2);
BOOST_FOREACH (ID eatomid, verifyEA){
	DBGLOG(DBG, "Evaluating EA " << eatomid);
	const ExternalAtom& eatom = reg->eatoms.getByID(eatomid);
	evaluateExternalAtom(reg, eatom, intr2, cbb);

	int eaIndex = -1;
	for (int i = 0; i < innerEatoms.size(); ++i) if (innerEatoms[i] == eatomid) eaIndex = i;
	assert(eaIndex != -1);

	// go through the output atoms of this external atom
	DBGLOG(DBG, "Going through output atoms of " << eatomid << " (index: " << eaIndex << ")");
	ggncmg.eaMasks[eaIndex].updateMask();
	bm::bvector<>::enumerator en = ggncmg.eaMasks[eaIndex].mask()->getStorage().first();
	bm::bvector<>::enumerator en_end = ggncmg.eaMasks[eaIndex].mask()->getStorage().end();
	Nogood ng;
	while (en < en_end){
		if (reg->ogatoms.getIDByAddress(*en).isExternalAuxiliary()){
			// go through all sets in toFalsify and check if the element was verified or falsified
			DBGLOG(DBG, "Going through sets of auxiliary " << *en);
			typedef std::set<ID> S;
			BOOST_FOREACH (int setindex, toFalsifyIndex[*en]){
				S& set = toFalsify[setindex];
				DBGLOG(DBG, "Going through set with index " << setindex);
				BOOST_FOREACH (ID elem, set){
					if (elem.address == *en && elem.isNaf() != intr2->getFact(*en)){
						DBGLOG(DBG, "Verified");
						// verified: remove it from the set
						set.erase(elem);
						if (set.size() == 0) return false;	// at least one set is not falsified
						break;
					}
					if (elem.address == *en && elem.isNaf() == intr2->getFact(*en)){
						// falsified: remove the set
						DBGLOG(DBG, "Falsified");
						break;
					}
				}
			}
		}
		en++;
	}
}
return true;





	// evaluate (relevant) exteral atoms wrt. intr2
	InterpretationPtr eaValuesWrtIuNX = InterpretationPtr(new Interpretation(reg));
	IntegrateExternalAnswerIntoInterpretationCB cb(eaValuesWrtIuNX);
	evaluateExternalAtoms(reg, verifyEA, intr2, cb);

	// replace old EA values by new ones
	intr2->add(*eaValuesWrtIuNX);
	DBGLOG(DBG, "I u -X with EA values: " << *intr2);

	// check for each rule which needs to be verified if condition 2 is indeed satisfied
	bool isUfs = true;
	BOOST_FOREACH (ID ruleID, verifyRule){
		OrdinaryAtom cratom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG | ID::PROPERTY_AUX);
		cratom.tuple.push_back(reg->getAuxiliaryConstantSymbol('c', ruleID));
		ID cr = reg->storeOrdinaryGAtom(cratom);

		if (ufsCandidate->getFact(cr.address)){
			const Rule& rule = reg->rules.getByID(ruleID);
			bool bodyFalsified = false;
			BOOST_FOREACH (ID b, rule.body){
				if (intr2->getFact(b.address) != !b.isNaf()){
					bodyFalsified = true;
					break;
				}
			}
			if (!bodyFalsified){
				isUfs = false;
				break;
			}
		}
	}

	return isUfs;
}
#endif

std::vector<Nogood> UnfoundedSetChecker::nogoodTransformation(Nogood ng, InterpretationConstPtr assignment){

	bool skip = false;
	Nogood ngAdd;

	BOOST_FOREACH (ID id, ng){
		if (reg->ogatoms.getIDByAddress(id.address).isExternalAuxiliary()){	// we have to requery the ID because nogoods strip off unnecessary information (e.g. property flags)

			ID useID = id;

			// transform negative replacements to positive ones
			OrdinaryAtom ogatom = reg->ogatoms.getByID(id);
			if (ogatom.tuple[0] == reg->getAuxiliaryConstantSymbol('n', reg->getIDByAuxiliaryConstantSymbol(ogatom.tuple[0]))){
				ogatom.tuple[0] = reg->getAuxiliaryConstantSymbol('r', reg->getIDByAuxiliaryConstantSymbol(ogatom.tuple[0]));
				useID = reg->storeOrdinaryGAtom(ogatom);
				useID.kind |= ID::NAF_MASK;	// flip truth value
			}

			// do not add a nogood if it extends the variable domain (this is counterproductive)
			if ( !domain->getFact(useID.address) ){
				DBGLOG(DBG, "Skipping because " << useID.address << " expands the domain");
				skip = true;
				break;
			}else{
				DBGLOG(DBG, "Inserting EA-Aux " << (useID.isNaf() ? "-" : "") << useID.address);
				ngAdd.insert(NogoodContainer::createLiteral(useID));
			}
		}else{
			// input atom

			// we have the following relations between sign S of the atom in the nogood, truth in assignment C and the unfounded set
			// S=positive, C=false --> nogood can never fire, skip it
			// S=positive, C=true --> nogood fires if the atom is NOT in the unfounded set (because it is not in the domain or it is false)
			// S=negative, C=true --> nogood fires if the atom IS in the unfounded set (because then it is false in I u -X)
			// S=negative, C=false --> nogood will always fire (wrt. this literal), skip the literal
			if (!id.isNaf()){
				// positive
				if (assignment->getFact(id.address) == false){
					// false in I --> nogood can never fire unter I u -X
					DBGLOG(DBG, "Skipping because " << id.address << " can never be true under I u -X");
					skip = true;
					break;
				}else{
					// true in I --> nogood fires if X does not contain the atom
					if ( domain->getFact(id.address) ){
						DBGLOG(DBG, "Inserting ordinary -" << id.address << " because it is true in I");
						ngAdd.insert(NogoodContainer::createLiteral(id.address, false));
					}else{
						DBGLOG(DBG, "Skipping ordinary " << id.address << " because it is not in the domain and can therefore never be in the unfounded set");
					}
				}
			}else{
				// negative
				if (assignment->getFact(id.address) == true){
					// positive variant is true in I --> nogood fires if it is also in X
					if ( !domain->getFact(id.address) ){
						DBGLOG(DBG, "Skipping because " << id.address << " can never be false under I u -X");
						skip = true;
						break;
					}else{
						DBGLOG(DBG, "Inserting " << id.address << " because it is false in I u -X if it is in X");
						ngAdd.insert(NogoodContainer::createLiteral(id.address, true));
					}
				}else{
					// positive variant is false in I --> it is also false in I u -X, skip literal
					DBGLOG(DBG, "Skipping ordinary -" << id.address << " because it is false in I and therefore also in I u -X");
				}
			}
		}
	}
	if (skip){
		return std::vector<Nogood>();
	}else{
		DBGLOG(DBG, "Adding transformed nogood " << ngAdd);
		std::vector<Nogood> result;
		result.push_back(ngAdd);
		return result;
	}
}

std::vector<IDAddress> UnfoundedSetChecker::getUnfoundedSet(){

	// solve the ufs problem
	solver = SATSolver::getInstance(ctx, ufsDetectionProblem);
	InterpretationConstPtr model;

	int mCnt = 0;

  #ifdef DLVHEX_BENCHMARK
	DLVHEX_BENCHMARK_REGISTER(ufscheck, "UFS Check");
	DLVHEX_BENCHMARK_REGISTER(oufscheck, "Ordinary UFS Check");
	if( mode == WithExt ) {
		DLVHEX_BENCHMARK_START(ufscheck);
	}else{
		DLVHEX_BENCHMARK_START(oufscheck);
	}
	BOOST_SCOPE_EXIT( (ufscheck)(oufscheck)(mode) )
	{
		if( mode == WithExt ) {
			DLVHEX_BENCHMARK_STOP(ufscheck);
		}else{
			DLVHEX_BENCHMARK_STOP(oufscheck);
		}
	} BOOST_SCOPE_EXIT_END
  #endif

	DLVHEX_BENCHMARK_REGISTER(sidufsenum, "UFS-Detection Problem Solving");
	if (mode == WithExt){
		DLVHEX_BENCHMARK_START(sidufsenum);
	}
	model = solver->getNextModel();
	if (mode == WithExt){
		DLVHEX_BENCHMARK_STOP(sidufsenum);
	}
	while ( model != InterpretationConstPtr()){
		if (mode == WithExt){
			DLVHEX_BENCHMARK_REGISTER_AND_COUNT(ufscandidates, "Investigated number of UFS candidates", 1);
		}

		// check if the model is actually an unfounded set
		DBGLOG(DBG, "Got UFS candidate: " << *model);
		mCnt++;

		if (mode == Ordinary || isUnfoundedSet(model)){
			DBGLOG(DBG, "Found UFS: " << *model << " (interpretation: " << *compatibleSet << ")");

			std::vector<IDAddress> ufs;

			bm::bvector<>::enumerator en = model->getStorage().first();
			bm::bvector<>::enumerator en_end = model->getStorage().end();
			while (en < en_end){
				ufs.push_back(*en);
				en++;
			}

			DBGLOG(DBG, "Enumerated " << mCnt << " UFS candidates");

			solver.reset();

			if (mode == WithExt){
				DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sidfailedufscheckcount, "Failed UFS Checks", 1);
			}

			return ufs;
		}else{
			DBGLOG(DBG, "No UFS: " << *model);
		}

		if (mode == WithExt){
			DLVHEX_BENCHMARK_START(sidufsenum);
		}
		model = solver->getNextModel();
		if (mode == WithExt){
			DLVHEX_BENCHMARK_STOP(sidufsenum);
		}
	}

	DBGLOG(DBG, "Enumerated " << mCnt << " UFS candidates");
	solver.reset();
	// no ufs
	std::vector<IDAddress> ufs;
	return ufs;
}

Nogood UnfoundedSetChecker::getUFSNogood(
		std::vector<IDAddress> ufs,
		InterpretationConstPtr interpretation){

	Nogood ng;

//	reduct-based stratey
#if 0

#ifndef NDEBUG
	std::stringstream ss;
	bool first = true;
	ss << "{ ";
	BOOST_FOREACH (IDAddress adr, ufs){
		ss << (!first ? ", " : "") << adr;
		first = false;
	}
	ss << " }";
	DBGLOG(DBG, "Constructing UFS nogood for UFS " << ss.str() << " wrt. " << *interpretation);
#endif

	// for each rule with unsatisfied body
	BOOST_FOREACH (ID ruleId, groundProgram.idb){
		const Rule& rule = reg->rules.getByID(ruleId);
		BOOST_FOREACH (ID b, rule.body){
			if (interpretation->getFact(b.address) != !b.isNaf()){
				// take an unsatisfied body literal
				ng.insert(NogoodContainer::createLiteral(b.address, interpretation->getFact(b.address)));
				break;
			}
		}
	}

	// add the smaller FLP model (interpretation minus unfounded set), restricted to ordinary atoms
	InterpretationPtr smallerFLPModel = InterpretationPtr(new Interpretation(*interpretation));
	BOOST_FOREACH (IDAddress adr, ufs){
		smallerFLPModel->clearFact(adr);
	}
	bm::bvector<>::enumerator en = smallerFLPModel->getStorage().first();
	bm::bvector<>::enumerator en_end = smallerFLPModel->getStorage().end();
	while (en < en_end){
		if (!reg->ogatoms.getIDByTuple(reg->ogatoms.getByAddress(*en).tuple).isAuxiliary()){
			ng.insert(NogoodContainer::createLiteral(*en));
		}
		en++;
	}

	// add one atom which is in the original interpretation but not in the flp model
	en = interpretation->getStorage().first();
	en_end = interpretation->getStorage().end();
	while (en < en_end){
		if (!smallerFLPModel->getFact(*en)){
			ng.insert(NogoodContainer::createLiteral(*en));
			break;
		}
		en++;
	}

	DBGLOG(DBG, "Constructed UFS nogood " << ng);
#endif

	// UFS-based strategy
#if 1
	// take an atom from the unfounded set which is true in the interpretation
	DBGLOG(DBG, "Constructing UFS nogood");
	BOOST_FOREACH (IDAddress adr, ufs){
		if (interpretation->getFact(adr)){
			ng.insert(NogoodContainer::createLiteral(adr, true));
			break;
		}
	}

	// find all rules r such that H(r) intersects with the unfounded set
	BOOST_FOREACH (ID ruleID, groundProgram.idb){
		const Rule& rule = reg->rules.getByID(ruleID);
		bool intersects = false;
		BOOST_FOREACH (ID h, rule.head){
			if (std::find(ufs.begin(), ufs.end(), h.address) != ufs.end()){
				intersects = true;
				break;
			}
		}
		if (!intersects) continue;

		// Check if the rule is external, i.e., it does _not_ contain a true ordinary unfounded atom in its positive body
		// (if the rule is not external, then condition (ii) will always be satisfied wrt. this unfounded set)
		bool external = true;
		BOOST_FOREACH (ID b, rule.body){
			if (interpretation->getFact(b.address) && !b.isNaf() && (!b.isExternalAuxiliary() || mode == Ordinary) && std::find(ufs.begin(), ufs.end(), b.address) != ufs.end()){
				external = false;
				break;
			}
		}
		if (!external) continue;

		// If available, find a literal which satisfies this rule independently of ufs;
		// this is either
		// (i) a head atom, which is true in the interpretation and not in the unfounded set
		// (ii) an ordinary positive body atom which is false in the interpretation
		// because then the rule is no justification for the ufs
		bool foundInd = false;
		BOOST_FOREACH (ID h, rule.head){
			if (interpretation->getFact(h.address) && std::find(ufs.begin(), ufs.end(), h.address) == ufs.end()){
				ng.insert(NogoodContainer::createLiteral(h.address, true));
				foundInd = true;
				break;
			}
		}
		if (!foundInd){
			BOOST_FOREACH (ID b, rule.body){
				if (!b.isNaf() != interpretation->getFact(b.address) && (!b.isExternalAuxiliary() || mode == Ordinary)){
					ng.insert(NogoodContainer::createLiteral(b.address, false));
					foundInd = true;
					break;
				}
			}
		}
		if (!foundInd){
			// This cannot happen if all atoms are taken as ordinary ones, because this would mean:
			// 1. No body atom is falsified by I (--> condition (i) does not apply)
			// 2. No positive body atom, which is true in I, is in the unfounded set (--> condition (ii) does not apply)
			// 3. All head atoms, which are true in I, are in the UFS (--> condition (iii) does not apply)
			// Therefore the UFS could not be an unfounded set
			assert (mode == WithExt);

			// alternatively: collect the truth values of all atoms relevant to the rule body
			BOOST_FOREACH (ID b, rule.body){
				if (!b.isExternalAuxiliary()){
					// this atom is satisfied by the interpretation (otherwise we had already foundInd),
					// therefore there must be another (external) atom which is false under I u -X
					// ng.insert(NogoodContainer::createLiteral(b.address, interpretation->getFact(b.address)));
				}else{
					const ExternalAtom& ea = reg->eatoms.getByID(ggncmg->auxToEA[b.address][0]);
					ea.updatePredicateInputMask();
					bm::bvector<>::enumerator en = ea.getPredicateInputMask()->getStorage().first();
					bm::bvector<>::enumerator en_end = ea.getPredicateInputMask()->getStorage().end();
					while (en < en_end){
						if (ggncmg->programMask->getFact(*en)){
							ng.insert(NogoodContainer::createLiteral(*en, interpretation->getFact(*en)));
						}
						en++;
					}
				}
			}
		}
	}
	DBGLOG(DBG, "Constructed UFS nogood " << ng);

#endif

	return ng;
}

UnfoundedSetCheckerManager::UnfoundedSetCheckerManager(
		GenuineGuessAndCheckModelGenerator& ggncmg,
		ProgramCtx& ctx,
		std::vector<ID>& innerEatoms,
		const AnnotatedGroundProgram& agp) :
			ggncmg(&ggncmg), ctx(ctx), innerEatoms(innerEatoms), agp(agp){
}

UnfoundedSetCheckerManager::UnfoundedSetCheckerManager(
		ProgramCtx& ctx,
		const AnnotatedGroundProgram& agp) :
			ctx(ctx), ggncmg(0), agp(agp){
}

std::vector<IDAddress> UnfoundedSetCheckerManager::getUnfoundedSet(
		InterpretationConstPtr interpretation,
		std::set<ID> skipProgram,
		NogoodContainerPtr ngc){

	if (ctx.config.getOption("UFSCheckMonolithic")){
		if (ggncmg){
			DBGLOG(DBG, "Checking UFS under consideration of external atoms");
			UnfoundedSetChecker ufsc(*ggncmg, ctx, agp.getGroundProgram(), innerEatoms, interpretation, skipProgram, InterpretationConstPtr(), ngc);
			std::vector<IDAddress> ufs = ufsc.getUnfoundedSet();
			if (ufs.size() > 0){
				DBGLOG(DBG, "Found a UFS");
				ufsnogood = ufsc.getUFSNogood(ufs, interpretation);
			}
			return ufs;
		}else{
			DBGLOG(DBG, "Checking UFS without considering external atoms");
			UnfoundedSetChecker ufsc(ctx, agp.getGroundProgram(), interpretation, skipProgram, InterpretationConstPtr(), ngc);
			std::vector<IDAddress> ufs = ufsc.getUnfoundedSet();
			if (ufs.size() > 0){
				DBGLOG(DBG, "Found a UFS");
				ufsnogood = ufsc.getUFSNogood(ufs, interpretation);
			}
			return ufs;
		}
	}else{
		// search in each component for unfounded sets
		DBGLOG(DBG, "UnfoundedSetCheckerManager::getUnfoundedSet");
		for (int comp = 0; comp < agp.getComponentCount(); ++comp){
			if (!agp.hasHeadCycles(comp) && (!ggncmg || !agp.hasECycles(comp))){
				DBGLOG(DBG, "Skipping component " << comp << " because it contains neither head-cycles not e-cycles");
				continue;
			}

			DBGLOG(DBG, "Checking for UFS in component " << comp);
			if (ggncmg && agp.hasECycles(comp)){
				DBGLOG(DBG, "Checking UFS under consideration of external atoms");
				UnfoundedSetChecker ufsc(*ggncmg, ctx, agp.getProgramOfComponent(comp), innerEatoms, interpretation, skipProgram, agp.getAtomsOfComponent(comp), ngc);
				std::vector<IDAddress> ufs = ufsc.getUnfoundedSet();
				if (ufs.size() > 0){
					DBGLOG(DBG, "Found a UFS");
					ufsnogood = ufsc.getUFSNogood(ufs, interpretation);
					return ufs;
				}
			}else{
				DBGLOG(DBG, "Checking UFS without considering external atoms");
				UnfoundedSetChecker ufsc(ctx, agp.getProgramOfComponent(comp), interpretation, skipProgram, agp.getAtomsOfComponent(comp));
				std::vector<IDAddress> ufs = ufsc.getUnfoundedSet();
				if (ufs.size() > 0){
					DBGLOG(DBG, "Found a UFS");
					ufsnogood = ufsc.getUFSNogood(ufs, interpretation);
					return ufs;
				}
			}
		}

		// no ufs found
		DBGLOG(DBG, "No component contains a UFS");
		return std::vector<IDAddress>();
	}
}

Nogood UnfoundedSetCheckerManager::getLastUFSNogood() const{
	return ufsnogood;
}

DLVHEX_NAMESPACE_END

