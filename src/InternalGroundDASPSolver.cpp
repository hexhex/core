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
 * @file InternalGroundDASPSolver.cpp
 * @author Christoph Redl
 *
 * @brief Extension of InternalGroundASPSolver to disjunctive programs.
 */

#define DLVHEX_BENCHMARK

#include "dlvhex2/PlainModelGenerator.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/ASPSolver.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/InternalGroundDASPSolver.h"

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

#if 0
Set<ID> InternalGroundDASPSolver::getDisjunctiveUnfoundedSetForComponent(int compNr){

	DBGLOG(DBG, "Checking if component " << compNr << " contains an unfounded set");
	Set<ID> comp;
	BOOST_FOREACH (IDAddress litadr, depSCC[compNr]){
		comp.insert(ID(ID::MAINKIND_LITERAL | ID::SUBKIND_ATOM_ORDINARYG, litadr));
	}

	// build subproblem for unfounded set detection
	NogoodSet subproblemUFSDetection;

	// for all rules which are not satisfied independently from comp
	BOOST_FOREACH (ID ruleID, program.getGroundProgram().idb){
		const Rule& r = reg->rules.getByID(ruleID);

		Set<ID> indSat = satisfiesIndependently(ruleID, comp);
		bool isSatisfied = false;
		BOOST_FOREACH (ID i, indSat){
			if (satisfied(i)){
				isSatisfied = true;
				break;
			}
		}

		if (!isSatisfied){
			// for an unfounded set, each rule must be excluded from justifying it
			// for each rule which is not already satisfied by literals in other components: either
			// 	1. a body literal is contained in the unfounded set, then the rule is excluded because it depends on the ufs; or
			// 	2. a true head literal is not contained in the ufs, then the rule is excluded because it is satisfied independently from the ufs
			// therefore, it must not happen that
			//	(i) all body literals are not contained
			//	(ii) all true head atoms are contained
			Nogood excludeRule;
			// (i)
			BOOST_FOREACH (ID bodyLit, r.body){
				if (comp.count(bodyLit) > 0){
					excludeRule.insert(negation(bodyLit));
				}
			}
			// (ii)
			BOOST_FOREACH (ID headLit, r.head){
				ID hl(ID::MAINKIND_LITERAL | ID::SUBKIND_ATOM_ORDINARYG, headLit.address);
				if (satisfied(hl)){
					excludeRule.insert(hl);
				}
			}
			subproblemUFSDetection.addNogood(excludeRule);
		}
	}

	// facts can never be in an unfounded set
	bm::bvector<>::enumerator en = program.getGroundProgram().edb->getStorage().first();
	bm::bvector<>::enumerator en_end = program.getGroundProgram().edb->getStorage().end();
	while (en < en_end){
		if (ordinaryFacts.count(*en) > 0){
			Nogood singular;
			singular.insert(ID(ID::MAINKIND_LITERAL | ID::SUBKIND_ATOM_ORDINARYG, *en));
			subproblemUFSDetection.addNogood(singular);
		}
		++en;
	}

	// we are looking for a non-empty unfounded set
	Nogood nonemptyNogood;
	BOOST_FOREACH (ID compLit, comp){
		if (satisfied(compLit)) nonemptyNogood.insert(negation(compLit));
	}
	subproblemUFSDetection.addNogood(nonemptyNogood);

	// check if there is a solution to this problem
	DBGLOG(DBG, "Solving the following subproblem for UFS detection: " << subproblemUFSDetection);
	CDNLSolver solver(ctx, subproblemUFSDetection);
	InterpretationConstPtr ufsSolution = solver.getNextModel();

	if (ufsSolution == InterpretationConstPtr()){
		DBGLOG(DBG, "No UFS caused by disjunctions exists");
		return Set<ID>();
	}else{
		// extract UFS from interpretation
		Set<ID> ufs;
		BOOST_FOREACH (ID compLit, comp){
			if (ufsSolution->getFact(compLit.address)){
				ufs.insert(compLit);
			}
		}
		DBGLOG(DBG, "Found an UFS caused by disjunctions: " << toString(ufs));
		return ufs;
	}
	
}

bool InternalGroundDASPSolver::isCompHCF(int compNr){

	// check if the component contains at least two distinct head literals of some rule
	BOOST_FOREACH (ID ruleID, program.getGroundProgram().idb){
		int numberOfHeadLits = 0;
		const Rule& r = reg->rules.getByID(ruleID);
		BOOST_FOREACH (ID headLit, r.head){
			if (depSCC[compNr].count(headLit.address) > 0) numberOfHeadLits++;
		}
		if (numberOfHeadLits > 1){
			return false;
		}
	}
	return true;
}

Nogood InternalGroundDASPSolver::getViolatedLoopNogood(const Set<ID>& ufs){

	Nogood loopNogood;

	// there are exponentially many loop nogoods for ufs;
	// choose one l from
	// lamba(ufs) = { Ta | a in ufs} x Prod_{r in extsup(ufs)} indsat(r, ufs)
	// such that l is currently satisfied
	BOOST_FOREACH (ID at, ufs){
		if (satisfied(at)){
			loopNogood.insert(createLiteral(at));
			break;
		}
	}

	// choose for each external rule one literal which
	// (i) satisfies it independently from ufs; and
	// (ii) is currently true
	Set<ID> extSup = getExternalSupport(ufs);
	BOOST_FOREACH (ID ruleID, extSup){
		Set<ID> satInd = satisfiesIndependently(ruleID, ufs);	// (i)
		BOOST_FOREACH (ID indLit, satInd){
			if (satisfied(indLit)){				// (ii)
				loopNogood.insert(createLiteral(indLit));
				break;
			}
		}
	}
	DBGLOG(DBG, "Loop nogood for " << toString(ufs) << " is " << loopNogood);

	return loopNogood;
}

std::string InternalGroundDASPSolver::getStatistics(){

#ifndef NDEBUG
	std::stringstream ss;
	ss	<< InternalGroundASPSolver::getStatistics() << std::endl
		<< "Model candidates: " << cntModelCandidates << std::endl
		<< "Detected unfounded sets due to disjunctions: " << cntDUnfoundedSets;
	return ss.str();
#else
	std::stringstream ss;
	ss << "Only available in debug mode";
	return ss.str();
#endif
}
#endif

InternalGroundDASPSolver::InternalGroundDASPSolver(ProgramCtx& ctx, const AnnotatedGroundProgram& p) : InternalGroundASPSolver(ctx, p), ufscm(ctx, p)
#if 0
, cntModelCandidates(0), cntDUnfoundedSets(0)
#endif
{

#if 0
	// compute HCF status for all components
	for (unsigned int compNr = 0; compNr < depSCC.size(); ++compNr){
		hcf.push_back(isCompHCF(compNr));
		DBGLOG(DBG, "HCF of component " << compNr << ": " << hcf[compNr]);
	}
#endif
}

InterpretationConstPtr InternalGroundDASPSolver::getNextModel(){

	InterpretationConstPtr model = InternalGroundASPSolver::getNextModel();

	bool ufsFound = true;
	while (model && ufsFound){
		ufsFound = false;

		std::vector<IDAddress> ufs = ufscm.getUnfoundedSet(model);

//		UnfoundedSetChecker ufsc(ctx, program, model);
//		std::vector<IDAddress> ufs = ufsc.getUnfoundedSet();

		if (ufs.size() > 0){
			Nogood ng = ufscm.getLastUFSNogood(); // = ufscm.getUFSNogood(ufs, model);
//			Nogood ng = ufsc.getUFSNogood(ufs, model);
			addNogood(ng);
			ufsFound = true;
			model = InternalGroundASPSolver::getNextModel();
		}
	}
	return model;


#if 0
	Set<ID> ufs(0, 1);
	do{
		// generate model candidate
		InterpretationConstPtr mc = InternalGroundASPSolver::getNextModel();
		if (mc == InterpretationConstPtr()) return mc;
		DBGLOG(DBG, "Got model candidate: " << *mc);

#ifndef NDEBUG
		++cntModelCandidates;
#endif

		// for each non-HCF component do an (exponential) unfounded set check
		for (unsigned int compNr = 0; compNr < depSCC.size(); ++compNr){
			if (!hcf[compNr]){
				ufs = getDisjunctiveUnfoundedSetForComponent(compNr);
				if (ufs.size() > 0) break;
			}
		}

		// is there an unfounded set?
		if (ufs.size() == 0){
			// no: found an answer set
			DBGLOG(DBG, "Found an answer set");
			return mc;
		}else{
#ifndef NDEBUG
			++cntDUnfoundedSets;
#endif
			modelCount--;	// model was a spurious one

			Nogood loopNogood = getViolatedLoopNogood(ufs);
			DBGLOG(DBG, "Adding loop nogood: " << loopNogood);
			addNogood(loopNogood);

			// backtrack
			bool doBacktrack = false;
			BOOST_FOREACH (ID lit, loopNogood){
				if (decisionlevel[lit.address] > 0){
					doBacktrack = true;
					break;
				}
			}
			if (!doBacktrack){
				return InterpretationConstPtr();
			}else{
				Nogood learnedNogood;
				analysis(loopNogood, learnedNogood, currentDL);
				addNogood(learnedNogood);
				backtrack(currentDL);
			}
		}
	}while(true);
#endif
}

DLVHEX_NAMESPACE_END

