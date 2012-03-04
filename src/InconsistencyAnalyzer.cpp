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
 * @file InconsistencyAnalyzer.cpp
 * @author Christoph Redl
 *
 * @brief Analyzes the inconsistency in a program wrt. selected input facts.
 */

#include "dlvhex2/InconsistencyAnalyzer.h"

#include "dlvhex2/HexParser.h"
#include "dlvhex2/Printer.h"

DLVHEX_NAMESPACE_BEGIN

InconsistencyAnalyzer::InconsistencyAnalyzer(ProgramCtx& ctx) : ctx(ctx) {
	reg = ctx.registry();
}

void InconsistencyAnalyzer::registerTerms(){

	neg_id = reg->getNewConstantTerm("neg");
	inc_id = reg->storeConstantTerm("inc");
}


Nogood InconsistencyAnalyzer::getIncNogood(const OrdinaryASPProgram& groundProgram, InterpretationConstPtr answerset){

	Nogood ng;

	// go through interpretation
	bm::bvector<>::enumerator en = answerset->getStorage().first();
	bm::bvector<>::enumerator en_end = answerset->getStorage().end();
	while (en < en_end){
		// check if the atom is over "inc"
		const OrdinaryAtom& ogatom = reg->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
		if (ogatom.tuple.front() == inc_id){
			// if the "inconsistent fact" was true, it must not be true anymore, if it was false, it must not be false anymore
			ng.insert(NogoodContainer::createLiteral(ogatom.tuple[1].address, groundProgram.edb->getFact(ogatom.tuple[1].address)));
		}
		en++;
	}

	return ng;
}

void InconsistencyAnalyzer::extractExplanationFromInterpretation(Nogood& ng, const OrdinaryASPProgram& groundProgram, InterpretationConstPtr intr){

	// go through interpretation
	bm::bvector<>::enumerator en = intr->getStorage().first();
	bm::bvector<>::enumerator en_end = intr->getStorage().end();
	while (en < en_end){
		// check if the atom is over "inc"
		const OrdinaryAtom& ogatom = reg->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
		if (ogatom.tuple.front() == inc_id){
			// if the "inconsistent fact" was true, it must not be true anymore, if it was false, it must not be false anymore
			ng.insert(NogoodContainer::createLiteral(ogatom.tuple[1].address, groundProgram.edb->getFact(ogatom.tuple[1].address)));
		}
		en++;
	}
}

HittingSetDetector<ID>::Hyperedge InconsistencyAnalyzer::extractHyperedgeFromInterpretation(const OrdinaryASPProgram& groundProgram, InterpretationConstPtr answerset){

	HittingSetDetector<ID>::Hyperedge edge;

	// go through interpretation
	bm::bvector<>::enumerator en = answerset->getStorage().first();
	bm::bvector<>::enumerator en_end = answerset->getStorage().end();
	while (en < en_end){
		// check if the atom is over "inc"
		const OrdinaryAtom& ogatom = reg->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
		if (ogatom.tuple.front() == inc_id){
			// if the "inconsistent fact" was true, it must not be true anymore, if it was false, it must not be false anymore
			edge.push_back(NogoodContainer::createLiteral(ogatom.tuple[1].address, groundProgram.edb->getFact(ogatom.tuple[1].address)));
		}
		en++;
	}

	DBGLOG(DBG, "Created hyperedge of size " << edge.size());
	return edge;
}

Nogood InconsistencyAnalyzer::getExplanationFromHypergraph(HittingSetDetector<ID>::Hypergraph graph){

	DBGLOG(DBG, "Solving hitting set problem for " << graph.size() << " hyperedges");
	Nogood ng;
	std::vector<ID> hittingset = HittingSetDetector<ID>::getHittingSet(graph);
	BOOST_FOREACH (ID id, hittingset){
		ng.insert(id);
	}
	return ng;
}

int InconsistencyAnalyzer::getNumberOfIncAtoms(InterpretationConstPtr intr){

	int nr = 0;

	// go through interpretation
	bm::bvector<>::enumerator en = intr->getStorage().first();
	bm::bvector<>::enumerator en_end = intr->getStorage().end();
	while (en < en_end){
		// check if the atom is over "inc"
		const OrdinaryAtom& ogatom = reg->ogatoms.getByID(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
		if (ogatom.tuple.front() == inc_id){
			nr++;
		}
		en++;
	}
	return nr;
}

Nogood InconsistencyAnalyzer::explainInconsistency(const OrdinaryASPProgram& groundProgram, InterpretationConstPtr explanationAtoms){

	DBGLOG(DBG, "Analyze inconsistency of program:");

#ifndef NDEBUG
	std::stringstream ss;
	ss << *groundProgram.edb << std::endl;
	RawPrinter p(ss, reg);
	BOOST_FOREACH (ID ruleID, groundProgram.idb){
		p.print(ruleID);
	}
	DBGLOG(DBG, ss.str());
#endif

	DBGLOG(DBG, "With respect to explanation atoms: " << *explanationAtoms);

	DBGLOG(DBG, "Initialization");
	registerTerms();

	DBGLOG(DBG, "Constructing analysis program");
	std::vector<ID> analysisIDB;
	InterpretationPtr analysisEDB = InterpretationPtr(new Interpretation(*groundProgram.edb));
	OrdinaryASPProgram analysisProgram(reg, analysisIDB, analysisEDB);
	BOOST_FOREACH (ID id, groundProgram.idb){
		analysisProgram.idb.push_back(id);
	}

	// go through explanation atoms
	bm::bvector<>::enumerator en = explanationAtoms->getStorage().first();
	bm::bvector<>::enumerator en_end = explanationAtoms->getStorage().end();
	while (en < en_end){
		// remove it from the analysis program's edb (we make a guess instead)
		analysisEDB->clearFact(*en);

		// create a choice rule between original explanation atom and dummy negation
		{
			Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR | ID::PROPERTY_RULE_DISJ);
			r.head.push_back(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *en));
			OrdinaryAtom neg(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			neg.tuple.push_back(neg_id);
			neg.tuple.push_back(ID::termFromInteger(*en));
			r.head.push_back(reg->storeOrdinaryGAtom(neg));
			analysisProgram.idb.push_back(reg->storeRule(r));
		}

		// create an "inconsistency detection rule" of kind
		//	inc(*en) :- not *en	if the truth value of *en is true
		// and
		//	inc(*en) :- *en		otherwise
		{
			Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);
			OrdinaryAtom inc(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
			inc.tuple.push_back(inc_id);
			inc.tuple.push_back(ID::termFromInteger(*en));
			r.head.push_back(reg->storeOrdinaryGAtom(inc));

			r.body.push_back(ID(ID::MAINKIND_LITERAL | ID::SUBKIND_ATOM_ORDINARYG | ( groundProgram.edb->getFact(*en) ? ID::NAF_MASK : 0), *en));
			analysisProgram.idb.push_back(reg->storeRule(r));
		}
		en++;
	}

	// create a weak constraint to minimize the extension of "inc"
	{
		Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_WEAKCONSTRAINT);
		OrdinaryAtom inc(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
		inc.tuple.push_back(inc_id);
		inc.tuple.push_back(reg->storeVariableTerm("X"));
		r.body.push_back(reg->storeOrdinaryGAtom(inc));
//		analysisProgram.idb.push_back(reg->storeRule(r));
	}

#ifndef NDEBUG
	DBGLOG(DBG, "Analysis program is:");
	ss.str("");
	ss << *analysisProgram.edb << std::endl;
	BOOST_FOREACH (ID ruleID, analysisProgram.idb){
		p.print(ruleID);
	}
	DBGLOG(DBG, ss.str());
#endif

	DBGLOG(DBG, "Evaluating analysis program");
	GenuineSolverPtr solver = GenuineSolver::getInstance(ctx, analysisProgram);

	DBGLOG(DBG, "Extracting explanation from answer sets");
	InterpretationConstPtr explanation;

	HittingSetDetector<ID>::Hypergraph graph;
	while ((explanation = solver->getNextModel()) != InterpretationConstPtr()){
		// optimization: avoid the generation of supersets of this explanation
		Nogood ng = getIncNogood(groundProgram, explanation);
		DBGLOG(DBG, "Adding subset minimality nogood " << ng);
		solver->addNogood(ng);

		// extract hyperedge
		graph.push_back(extractHyperedgeFromInterpretation(groundProgram, explanation));
	}

	Nogood ng = getExplanationFromHypergraph(graph);
	DBGLOG(DBG, "Explanation is: " << ng);
	return ng;
}

DLVHEX_NAMESPACE_END

