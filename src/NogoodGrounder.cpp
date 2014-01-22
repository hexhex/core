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
 * @file NogoodGrounder.cpp
 * @author Christoph Redl
 *
 * @brief Implements a grounder for nonground nogoods.
 */

#define DLVHEX_BENCHMARK

#include "dlvhex2/NogoodGrounder.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/Nogood.h"

#include <bm/bmalgo.h>

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

NogoodGrounder::NogoodGrounder(RegistryPtr reg, SimpleNogoodContainerPtr watched, SimpleNogoodContainerPtr destination, AnnotatedGroundProgram& agp) :
	reg(reg), watched(watched), destination(destination), agp(agp){
}

void NogoodGrounder::resetWatched(SimpleNogoodContainerPtr watched){
	this->watched = watched;
}

ImmediateNogoodGrounder::ImmediateNogoodGrounder(RegistryPtr reg, SimpleNogoodContainerPtr watched, SimpleNogoodContainerPtr destination, AnnotatedGroundProgram& agp) :
	NogoodGrounder(reg, watched, destination, agp), instantiatedNongroundNogoodsIndex(0){
}

void ImmediateNogoodGrounder::update(InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed){

	// go through all nonground nogoods which have not been instantiated so far
	int max = watched->getNogoodCount();
	if (instantiatedNongroundNogoodsIndex >= max) instantiatedNongroundNogoodsIndex = 0;
	DBGLOG(DBG, "Updating nogood grounder from " << instantiatedNongroundNogoodsIndex << " to " << max);
	for (int i = instantiatedNongroundNogoodsIndex; i < max; ++i){
		Nogood ng = watched->getNogood(i);
		DBGLOG(DBG, "Checking nogood " << ng.getStringRepresentation(reg));
		if (ng.isGround()) continue;

		DBGLOG(DBG, "Searching for watched literal in nogood " << i);
		int maxBoundVariables = 0;
		ID watchedLit = ID_FAIL;
		BOOST_FOREACH (ID lit, ng){
			if (lit.isOrdinaryGroundAtom()) continue;

			const OrdinaryAtom& atom = reg->onatoms.getByID(lit);

			std::set<ID> distinctVar;
			int var = 0;
			BOOST_FOREACH (ID p, atom.tuple){
				if (p.isVariableTerm()){
					if (std::find(distinctVar.begin(), distinctVar.end(), p) == distinctVar.end()){
						distinctVar.insert(p);
						var++;
					}
				}
			}

			if (var > maxBoundVariables){
				maxBoundVariables = var;
				watchedLit = lit;
			}
		}
		assert (watchedLit != ID_FAIL);

		// watch the atom and the corresponding nogood
		DBGLOG(DBG, "Watching literal " << watchedLit << " in nogood " << i);
		const OrdinaryAtom& watchedAtom = reg->onatoms.getByAddress(watchedLit.address);

		// For each atom A of the program, check if the watched literal unifies with A
		bm::bvector<>::enumerator en = agp.getProgramMask()->getStorage().first();
		bm::bvector<>::enumerator en_end = agp.getProgramMask()->getStorage().end();

		DBGLOG(DBG, "Searching for unifying program atoms");
		while (en < en_end){

			DBGLOG(DBG, "Checking atom " << *en);

			const OrdinaryAtom& currentAtom = reg->ogatoms.getByAddress(*en);
			if (currentAtom.unifiesWith(watchedAtom, reg)){
				Nogood instantiatedNG;
				ng.match(reg, reg->ogatoms.getIDByAddress(*en), instantiatedNG);
				DBGLOG(DBG, "Instantiated " << instantiatedNG.getStringRepresentation(reg) << " from " << ng.getStringRepresentation(reg));

				// check if the instance of the nogood contains a ground literal which does not appear in the program
				bool relevant = true;
				BOOST_FOREACH (ID lit, ng){
					if (lit.isOrdinaryGroundAtom() && !agp.getProgramMask()->getFact(lit.address)){
						relevant = false;
						break;
					}
				}

				if (relevant){
					if (instantiatedNG.isGround()){
						destination->addNogood(instantiatedNG);
					}else{
						watched->addNogood(instantiatedNG);
					}
				}
			}

			en++;
		}

	}
	instantiatedNongroundNogoodsIndex = max;
}

void ImmediateNogoodGrounder::resetWatched(SimpleNogoodContainerPtr watched){
	NogoodGrounder::resetWatched(watched);
	instantiatedNongroundNogoodsIndex = 0;
}

LazyNogoodGrounder::LazyNogoodGrounder(RegistryPtr reg, SimpleNogoodContainerPtr watched, SimpleNogoodContainerPtr destination, AnnotatedGroundProgram& agp) :
	NogoodGrounder(reg, watched, destination, agp), watchedNogoodsCount(0){
}

void LazyNogoodGrounder::update(InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed){

	if (!factWasSet) return;

	// Watch for all new nonground nogoods the literal which binds the highest number of variables
	DBGLOG(DBG, "Updating watches of nonground nogoods");
	int max = watched->getNogoodCount();
	if (watchedNogoodsCount >= max) watchedNogoodsCount = 0;
	DBGLOG(DBG, "Updating nogood grounder from " << watchedNogoodsCount << " to " << max);
	for (int i = watchedNogoodsCount; i < max; ++i){
		Nogood ng = watched->getNogood(i);
		DBGLOG(DBG, "Checking nogood " << ng.getStringRepresentation(reg));
		if (ng.isGround()) continue;

		DBGLOG(DBG, "Searching for watched literal in nogood " << i);
		int maxBoundVariables = 0;
		ID watchedLit = ID_FAIL;
		BOOST_FOREACH (ID lit, ng){
			if (lit.isOrdinaryGroundAtom()) continue;

			const OrdinaryAtom& atom = reg->onatoms.getByID(lit);

			std::set<ID> distinctVar;
			int var = 0;
			BOOST_FOREACH (ID p, atom.tuple){
				if (p.isVariableTerm()){
					if (std::find(distinctVar.begin(), distinctVar.end(), p) == distinctVar.end()){
						distinctVar.insert(p);
						var++;
					}
				}
			}

			if (var > maxBoundVariables){
				maxBoundVariables = var;
				watchedLit = lit;
			}
		}
		assert (watchedLit != ID_FAIL);

		// watch the atom and the corresponding nogood
		DBGLOG(DBG, "Watching literal " << watchedLit << " in nogood " << i);
		watchedLiterals.push_back(std::pair<ID, int>(watchedLit, i));
	}
	watchedNogoodsCount = watched->getNogoodCount();

	// For each atom A with changed truth value: go through all watches and check if
	// 1. the watched literal unifies with A
	// 2. the corresponding clause has not been instantiated for A yet
	bm::bvector<>::enumerator en = changed->getStorage().first();
	bm::bvector<>::enumerator en_end = changed->getStorage().end();

	DBGLOG(DBG, "Instantiating nonground nogoods");
	while (en < en_end){

		DBGLOG(DBG, "Instantiating for atom " << *en);
		typedef std::pair<ID, int> Pair;
		BOOST_FOREACH (Pair p, watchedLiterals){
			DBGLOG(DBG, "Matching nonground nogood " << p.second);

			// 2.
			if (std::find(alreadyCompared.begin(), alreadyCompared.end(), std::pair<IDAddress, int>(*en, p.second)) != alreadyCompared.end()) continue;

			const OrdinaryAtom& currentAtom = reg->ogatoms.getByAddress(*en);
			const OrdinaryAtom& watchedAtom = reg->onatoms.getByAddress(p.first.address);
			// 1.
			if (currentAtom.unifiesWith(watchedAtom, reg)){
				Nogood instantiatedNG;
				watched->getNogood(p.second).match(reg, reg->ogatoms.getIDByAddress(*en), instantiatedNG);
				DBGLOG(DBG, "Instantiated " << instantiatedNG.getStringRepresentation(reg) << " from " << watched->getNogood(p.second).getStringRepresentation(reg));

				if (instantiatedNG.isGround()){
					destination->addNogood(instantiatedNG);
				}else{
					watched->addNogood(instantiatedNG);
				}
			}

			alreadyCompared.insert(std::pair<IDAddress, int>(*en, p.second));
		}
		en++;
	}
}

void LazyNogoodGrounder::resetWatched(SimpleNogoodContainerPtr watched){
	NogoodGrounder::resetWatched(watched);
}


DLVHEX_NAMESPACE_END

