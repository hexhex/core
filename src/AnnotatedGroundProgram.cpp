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
 * @file   AnnotatedGroundProgram.cpp
 * @author Christoph Redl
 * @date Wed May 30 2012
 * 
 * @brief  Stores an ordinary ground program with some meta information,
 * e.g. mapping of ground atoms back to external atoms, cycle information
 * 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dlvhex2/AnnotatedGroundProgram.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Benchmarking.h"

#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/visitors.hpp> 
#include <boost/graph/strong_components.hpp>
#include <boost/graph/filtered_graph.hpp>

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

AnnotatedGroundProgram::AnnotatedGroundProgram() : ctx(0), groundProgram(OrdinaryASPProgram(RegistryPtr(), std::vector<ID>(), InterpretationConstPtr())), haveGrounding(false){
}

AnnotatedGroundProgram::AnnotatedGroundProgram(ProgramCtx& ctx, const OrdinaryASPProgram& groundProgram, std::vector<ID> indexedEatoms, bool includeEDB) :
	ctx(&ctx), reg(ctx.registry()), groundProgram(groundProgram), indexedEatoms(indexedEatoms), haveGrounding(true), includeEDB(includeEDB){

	initialize(includeEDB);
}

AnnotatedGroundProgram::AnnotatedGroundProgram(ProgramCtx& ctx, std::vector<ID> indexedEatoms) :
	ctx(&ctx), reg(ctx.registry()), groundProgram(OrdinaryASPProgram(RegistryPtr(), std::vector<ID>(), InterpretationConstPtr())), indexedEatoms(indexedEatoms), haveGrounding(false){

	initialize(false);
}

// Incremental extension
// Note: program "other" MUST NOT cyclically depend on the current program (this condition is not checked but violation harms validity of the state of this object!)
void AnnotatedGroundProgram::addProgram(const AnnotatedGroundProgram& other){

	DBGLOG(DBG, "Adding program to AnnotatedGroundProgram");
	if (haveGrounding && other.haveGrounding){
		std::vector<ID> newGroundIdb = groundProgram.idb;
		newGroundIdb.insert(newGroundIdb.end(), other.groundProgram.idb.begin(), other.groundProgram.idb.end());

		InterpretationPtr newGroundEdb(new Interpretation(reg));
		if (!!groundProgram.edb) newGroundEdb->add(*groundProgram.edb);
		if (!!other.groundProgram.edb) newGroundEdb->add(*other.groundProgram.edb);

		InterpretationPtr newGroundMask(new Interpretation(reg));
		if (!!groundProgram.mask) newGroundMask->add(*groundProgram.mask);
		if (!!other.groundProgram.mask) newGroundMask->add(*other.groundProgram.mask);

		groundProgram = OrdinaryASPProgram(groundProgram.registry, newGroundIdb, newGroundEdb, groundProgram.maxint, newGroundMask);
		haveGrounding = true;
	}else{
		haveGrounding = false;
	}

	indexedEatoms.insert(indexedEatoms.end(), other.indexedEatoms.begin(), other.indexedEatoms.end());
	eaMasks.insert(eaMasks.end(), other.eaMasks.begin(), other.eaMasks.end());
	typedef const boost::unordered_map<IDAddress, std::vector<ID> >::value_type AuxToEAPair;
	BOOST_FOREACH (AuxToEAPair pair, other.auxToEA){
		auxToEA[pair.first].insert(auxToEA[pair.first].end(), pair.second.begin(), pair.second.end());
	}
	if (!!other.supportSets){
		if (!supportSets) supportSets = SimpleNogoodContainerPtr(new SimpleNogoodContainer());
		for (int i = 0; i < other.supportSets->getNogoodCount(); ++i){
			supportSets->addNogood(other.supportSets->getNogood(i));
		}
	}
	depSCC.insert(depSCC.end(), other.depSCC.begin(), other.depSCC.end());
	typedef const boost::unordered_map<IDAddress, int>::value_type ComponentOfAtomPair;
	BOOST_FOREACH (ComponentOfAtomPair pair, other.componentOfAtom){
		componentOfAtom[pair.first] = pair.second;
	}
	headCycles.insert(headCycles.end(), other.headCycles.begin(), other.headCycles.end());

	InterpretationPtr newHeadCyclicRules(new Interpretation(reg));
	newHeadCyclicRules->add(*headCyclicRules);
	newHeadCyclicRules->add(*other.headCyclicRules);
	headCyclicRules = newHeadCyclicRules;

	programComponents.insert(programComponents.end(), other.programComponents.begin(), other.programComponents.end());
	eaMasks.insert(eaMasks.end(), other.eaMasks.begin(), other.eaMasks.end());
	headCyclesTotal |= other.headCyclesTotal;
	eCyclesTotal |= other.eCyclesTotal;
	if (!!programMask  && !! other.programMask) programMask->add(*other.programMask);

	createEAMasks(includeEDB);
}

const AnnotatedGroundProgram&
AnnotatedGroundProgram::operator=(
    const AnnotatedGroundProgram& other)
{
	reg = other.reg;
	groundProgram = other.groundProgram;
	haveGrounding = other.haveGrounding;
	indexedEatoms = other.indexedEatoms;
	eaMasks = other.eaMasks;
	auxToEA = other.auxToEA;
	programMask = other.programMask;
	depGraph = other.depGraph;
	depSCC = other.depSCC;
	componentOfAtom = other.componentOfAtom;
	externalEdges = other.externalEdges;
	headCycles = other.headCycles;
	headCyclicRules = other.headCyclicRules;
	eCycles = other.eCycles;
	programComponents = other.programComponents;
	headCyclesTotal = other.headCyclesTotal;
	eCyclesTotal = other.eCyclesTotal;
	supportSets = other.supportSets;
  return *this;
}

void AnnotatedGroundProgram::createProgramMask(){

	// create mask of all atoms in the program
	programMask = InterpretationPtr(new Interpretation(reg));
	programMask->add(*groundProgram.edb);
	BOOST_FOREACH (ID ruleID, groundProgram.idb){
		const Rule& rule = reg->rules.getByID(ruleID);
		BOOST_FOREACH (ID h, rule.head) programMask->setFact(h.address);
		BOOST_FOREACH (ID b, rule.body) programMask->setFact(b.address);
	}
}

void AnnotatedGroundProgram::createEAMasks(bool includeEDB){
	eaMasks.resize(indexedEatoms.size());
	int eaIndex = 0;
	BOOST_FOREACH (ID eatom, indexedEatoms){
		// create an EAMask for each inner external atom
		eaMasks[eaIndex] = boost::shared_ptr<ExternalAtomMask>(new ExternalAtomMask);
		ExternalAtomMask& eaMask = *eaMasks[eaIndex];
		eaMask.setEAtom(*ctx, reg->eatoms.getByID(eatom), groundProgram.idb);
		if (includeEDB) eaMasks[eaIndex]->addOutputAtoms(groundProgram.edb);
		eaMask.updateMask();
		eaIndex++;
	}
}

void AnnotatedGroundProgram::mapAuxToEAtoms(){

	int eaIndex = 0;
	BOOST_FOREACH (ID eatom, indexedEatoms){
		// create an EAMask for each inner external atom
		ExternalAtomMask& eaMask = *eaMasks[eaIndex];
    // we already did this in createEAMasks
		//eaMask.setEAtom(*ctx, reg->eatoms.getByID(eatom), groundProgram.idb);
		//eaMask.updateMask();

		// map external auxiliaries back to their external atoms
		bm::bvector<>::enumerator en = eaMask.mask()->getStorage().first();
		bm::bvector<>::enumerator en_end = eaMask.mask()->getStorage().end();
		while (en < en_end){
			if (reg->ogatoms.getIDByAddress(*en).isExternalAuxiliary()){
				DBGLOG(DBG, "Auxiliary " << *en << " maps to " << indexedEatoms[eaIndex]);
				auxToEA[*en].push_back(indexedEatoms[eaIndex]);
			}
			en++;
		}
		eaIndex++;
	}
}

void AnnotatedGroundProgram::setIndexEAtoms(std::vector<ID> indexedEatoms){
	this->indexedEatoms = indexedEatoms;

	initialize(false);
}

void AnnotatedGroundProgram::initialize(bool includeEDB){
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"AnnotatedGroundProg init");

	headCyclicRules = InterpretationPtr(new Interpretation(reg));

	eaMasks.resize(0);
	if (haveGrounding) createProgramMask();
	createEAMasks(includeEDB);
	mapAuxToEAtoms();
	if (haveGrounding) computeAtomDependencyGraph();
	if (haveGrounding) computeStronglyConnectedComponents();
	if (haveGrounding) computeHeadCycles();
	if (haveGrounding) computeECycles();

#ifndef NDEBUG
	if (haveGrounding){
		std::stringstream programstring;
		{
			RawPrinter printer(programstring, reg);
			if (groundProgram.edb) programstring << "EDB: " << *groundProgram.edb << std::endl;
			programstring << "IDB:" << std::endl;
			BOOST_FOREACH (ID ruleId, groundProgram.idb){
				printer.print(ruleId);
				programstring << std::endl;
			}
		}

		std::stringstream sccstring;
		{
			RawPrinter printer(sccstring, reg);
			int sai = 0;
			BOOST_FOREACH (std::set<IDAddress> sa, depSCC){
				sccstring << "{ ";
				bool first = true;
				BOOST_FOREACH (IDAddress ida, sa){
					if (!first) sccstring << ", ";
					first = false;
					printer.print(reg->ogatoms.getIDByAddress(ida));
				}
				sccstring << " } (HC: " << headCycles[sai] << ", EC: " << eCycles[sai] << ") ";
				sai++;
			}
		}

		DBGLOG(DBG, "Program:" << std::endl << programstring.str() << std::endl << "has SCC-decomposition: " << sccstring.str());
	}
#endif
}


void AnnotatedGroundProgram::computeAtomDependencyGraph(){

	// construct atom dependency graph
	DBGLOG(DBG, "Contructing atom dependency graph for " << groundProgram.idb.size() << " rules");
	BOOST_FOREACH (ID ruleID, groundProgram.idb){
		const Rule& rule = reg->rules.getByID(ruleID);

		BOOST_FOREACH (ID h, rule.head){
			if (depNodes.find(h.address) == depNodes.end()) depNodes[h.address] = boost::add_vertex(h.address, depGraph);
		}
		BOOST_FOREACH (ID b, rule.body){
			if (depNodes.find(b.address) == depNodes.end() && !b.isExternalAuxiliary()) depNodes[b.address] = boost::add_vertex(b.address, depGraph);
		}

		// add an arc from all head atoms to all positive body literals
		// literals in weight rules always count as positive body atoms, even if they are default negated (because the weighted body as a whole is positive)
		DBGLOG(DBG, "Adding ordinary edges");
		BOOST_FOREACH (ID h, rule.head){
			BOOST_FOREACH (ID b, rule.body){
				if ((!b.isNaf() || ruleID.isWeightRule()) && !b.isExternalAuxiliary()){
					DBGLOG(DBG, "Adding dependency from " << h.address << " to " << b.address);
					boost::add_edge(depNodes[h.address], depNodes[b.address], depGraph);
				}
			}
		}

		// add an arc from all head atoms to atoms which are input to some external atom in the rule body
		DBGLOG(DBG, "Adding e-edges");
		BOOST_FOREACH (ID b, rule.body){
			if (b.isExternalAuxiliary()){
				BOOST_FOREACH (ID eaID, auxToEA[b.address]){
					const ExternalAtom& ea = reg->eatoms.getByID(eaID);

					ea.updatePredicateInputMask();
					bm::bvector<>::enumerator en = ea.getPredicateInputMask()->getStorage().first();
					bm::bvector<>::enumerator en_end = ea.getPredicateInputMask()->getStorage().end();
					while (en < en_end){
						if (depNodes.find(*en) == depNodes.end()) depNodes[*en] = boost::add_vertex(*en, depGraph);

						BOOST_FOREACH (ID h, rule.head){
							if (!h.isExternalAuxiliary()){
								DBGLOG(DBG, "Adding dependency from " << h.address << " to " << *en);
								boost::add_edge(depNodes[h.address], depNodes[*en], depGraph);
								externalEdges.push_back(std::pair<IDAddress, IDAddress>(h.address, *en));
							}
						}
						en++;
					}
				}
			}
		}
	}
}

void AnnotatedGroundProgram::computeStronglyConnectedComponents(){

	// find strongly connected components in the dependency graph
	DBGLOG(DBG, "Computing strongly connected components");
	std::vector<int> componentMap(depNodes.size());
	int num = boost::strong_components(depGraph, boost::make_iterator_property_map(componentMap.begin(), get(boost::vertex_index, depGraph)));

	// translate into real map
	depSCC = std::vector<std::set<IDAddress> >(num);
	Node nodeNr = 0;

	BOOST_FOREACH (int componentOfNode, componentMap){
		depSCC[componentOfNode].insert(depGraph[nodeNr]);
		componentOfAtom[depGraph[nodeNr]] = componentOfNode;
		nodeNr++;
	}
#ifndef NDEBUG
	for (uint32_t comp = 0; comp < depSCC.size(); ++comp){
		std::stringstream ss;
		bool first = true;
		BOOST_FOREACH (IDAddress ida, depSCC[comp]){
			if (!first) ss << ", ";
			first = false;
			ss << ida;
		}
		DBGLOG(DBG, "Component " << comp << ": " << ss.str());
	}
#endif

	// partition the program according to the strongly connected components
	DBGLOG(DBG, "Partitioning program");
	std::map<IDAddress, std::vector<ID> > rulesWithHeadAtom;
	BOOST_FOREACH (ID ruleID, groundProgram.idb){
		const Rule& rule = reg->rules.getByID(ruleID);
		BOOST_FOREACH (ID h, rule.head){
			rulesWithHeadAtom[h.address].push_back(ruleID);
		}
	}
	for (uint32_t comp = 0; comp < depSCC.size(); ++comp){
		DBGLOG(DBG, "Partition " << comp);

		OrdinaryASPProgram componentProgram(reg, std::vector<ID>(), groundProgram.edb);
		InterpretationPtr componentAtoms = InterpretationPtr(new Interpretation(reg));
		ProgramComponent currentComp(componentAtoms, componentProgram);

		// set all atoms of this component
		BOOST_FOREACH (IDAddress ida, depSCC[comp]){
			componentAtoms->setFact(ida);
		}

		// compute the program partition
		bm::bvector<>::enumerator en = componentAtoms->getStorage().first();
		bm::bvector<>::enumerator en_end = componentAtoms->getStorage().end();
		while (en < en_end){
			BOOST_FOREACH (ID ruleID, rulesWithHeadAtom[*en]){
#ifndef NDEBUG
				std::stringstream programstring;
				RawPrinter printer(programstring, reg);
				printer.print(ruleID);
				DBGLOG(DBG, programstring.str());
#endif

				currentComp.program.idb.push_back(ruleID);
			}
			en++;
		}

		programComponents.push_back(currentComp);
	}
}

void AnnotatedGroundProgram::computeHeadCycles(){

	// check if the components contain head-cycles
	DBGLOG(DBG, "Computing head-cycles of components");
	headCyclesTotal = false;
	for (uint32_t comp = 0; comp < depSCC.size(); ++comp){
		int hcf = true;
		BOOST_FOREACH (ID ruleID, programComponents[comp].program.idb){
			const Rule& rule = reg->rules.getByID(ruleID);
			int intersectionCount = 0;
			BOOST_FOREACH (ID h, rule.head){
//				if (std::find(depSCC[comp].begin(), depSCC[comp].end(), h.address) != depSCC[comp].end()){
				if (programComponents[comp].componentAtoms->getFact(h.address)){
					intersectionCount++;
				}
				if (intersectionCount >= 2) break;
			}
			if (intersectionCount >= 2){
				hcf = false;
				break;
			}
		}
		headCycles.push_back(!hcf);
		headCyclesTotal |= headCycles[headCycles.size() - 1];
		DBGLOG(DBG, "Component " << comp << ": " << !hcf);

		if (!hcf){
			// all rules in the component are head-cyclic
			BOOST_FOREACH (ID ruleID, programComponents[comp].program.idb){
				headCyclicRules->setFact(ruleID.address);
			}
		}
	}
}

void AnnotatedGroundProgram::computeECycles(){

	DBGLOG(DBG, "Computing e-cycles of components");

	if (ctx->config.getOption("LegacyECycleDetection")){
	        eCyclesTotal = false;
        	for (uint32_t comp = 0; comp < depSCC.size(); ++comp){

               		// check for each e-edge x -> y if there is a path from y to x
	                // if yes, then y is a cyclic predicate input
        	        InterpretationPtr cyclicInputAtoms = InterpretationPtr(new Interpretation(reg));
                	typedef std::pair<IDAddress, IDAddress> Edge;
	                BOOST_FOREACH (Edge e, externalEdges){
        	                if (!programComponents[comp].componentAtoms->getFact(e.first)) continue;
                	        if (!programComponents[comp].componentAtoms->getFact(e.second)) continue;
                        	//if (std::find(depSCC[comp].begin(), depSCC[comp].end(), e.first) == depSCC[comp].end()) continue;
	                        //if (std::find(depSCC[comp].begin(), depSCC[comp].end(), e.second) == depSCC[comp].end()) continue;

	                        std::vector<Graph::vertex_descriptor> reachable;
        	                boost::breadth_first_search(depGraph, depNodes[e.second],
                	                boost::visitor(
                        	                boost::make_bfs_visitor(
                                	                boost::write_property(
                                        	                boost::identity_property_map(),
                                                	        std::back_inserter(reachable),
                                                        	boost::on_discover_vertex()))));

	                        if (std::find(reachable.begin(), reachable.end(), depNodes[e.second]) != reachable.end()){
        	                        // yes, there is a cycle
                	                cyclicInputAtoms->setFact(e.second);
                        	}
              		}
                	eCycles.push_back(cyclicInputAtoms->getStorage().count() > 0);
                	eCyclesTotal |= eCycles[eCycles.size() - 1];

#ifndef NDEBUG
	                std::stringstream ss;
        	        bool first = true;
                	bm::bvector<>::enumerator en = cyclicInputAtoms->getStorage().first();
	                bm::bvector<>::enumerator en_end = cyclicInputAtoms->getStorage().end();
        	        while (en < en_end){
                	        if (!first) ss << ", ";
                        	first = false;
                        	ss << *en;
                        	en++;
                	}
                	if (cyclicInputAtoms->getStorage().count() > 0){
                        	DBGLOG(DBG, "Component " << comp << ": 1 with cyclic input atoms " << ss.str());
               		}else{
                	        DBGLOG(DBG, "Component " << comp << ": 0");
                	}
#endif
        	}
	}else{
		for (uint32_t comp = 0; comp < depSCC.size(); ++comp){
			eCycles.push_back(false);
		}

		// for each e-edge x -> y: if x and y are in the same component, then y is cyclic
		typedef std::pair<IDAddress, IDAddress> Edge;
		BOOST_FOREACH (Edge e, externalEdges){
			if (componentOfAtom[e.first] == componentOfAtom[e.second]){
				eCycles[componentOfAtom[e.second]] = true;
			}
		}

		eCyclesTotal = false;
		for (uint32_t comp = 0; comp < depSCC.size(); ++comp){
			eCyclesTotal |= eCycles[comp];
		}
	}
}

bool AnnotatedGroundProgram::containsHeadCycles(ID ruleID) const{

	return headCyclicRules->getFact(ruleID.address);

//	for (int comp = 0; comp < depSCC.size(); ++comp){
//		if (headCycles[comp] && std::find(programComponents[comp].program.idb.begin(), programComponents[comp].program.idb.end(), ruleID) != programComponents[comp].program.idb.end()) return true;
//	}
//	return false;
}

int AnnotatedGroundProgram::getComponentCount() const{
	return programComponents.size();
}

const OrdinaryASPProgram& AnnotatedGroundProgram::getProgramOfComponent(int compNr) const{
	assert((uint32_t)compNr >= 0 && (uint32_t)compNr < depSCC.size());
	return programComponents[compNr].program;
}

InterpretationConstPtr AnnotatedGroundProgram::getAtomsOfComponent(int compNr) const{
	assert((uint32_t)compNr >= 0 && (uint32_t)compNr < depSCC.size());
	return programComponents[compNr].componentAtoms;
}

bool AnnotatedGroundProgram::hasHeadCycles(int compNr) const{
	assert((uint32_t)compNr >= 0 && (uint32_t)compNr < depSCC.size());
	return headCycles[compNr];
}

bool AnnotatedGroundProgram::hasECycles(int compNr) const{
	assert((uint32_t)compNr >= 0 && (uint32_t)compNr < depSCC.size());
	return eCycles[compNr];
}

namespace{

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, IDAddress> Graph;
typedef Graph::vertex_descriptor Node;

struct edge_filter {
	const std::set<Node>& skipnodes;

	edge_filter(std::set<Node>& skipnodes) : skipnodes(skipnodes) { }

	template <typename Edge>
	bool operator()(const Edge& e) const {
		return true;
	}
};

struct vertex_filter {
	const std::set<Node>& skipnodes;

	vertex_filter(std::set<Node>& skipnodes) : skipnodes(skipnodes) { }

	template <typename Vertex>
	bool operator()(const Vertex& v) const {
		return std::find(skipnodes.begin(), skipnodes.end(), v) == skipnodes.end();
	}
};

}

bool AnnotatedGroundProgram::hasECycles(int compNr, InterpretationConstPtr intr) const{

	// make a copy of the dependency graph
	Graph depGraph2;
	boost::copy_graph(depGraph, depGraph2);

	// remove atoms which are not in intr and corresponding edges
	std::set<Node> skipnodes;
	BOOST_FOREACH (IDAddress adr, depSCC[compNr]){
		if (!intr->getFact(adr)) skipnodes.insert(depNodes.at(adr));
	}
	boost::graph_traits<Graph>::edge_iterator vi, vi_end;
	std::vector<Graph::edge_descriptor> delEdges;
	for (boost::tuples::tie(vi, vi_end) = edges(depGraph2); vi != vi_end; vi++){
		if (std::find(skipnodes.begin(), skipnodes.end(), source(*vi, depGraph2)) != skipnodes.end() ||
		    std::find(skipnodes.begin(), skipnodes.end(), target(*vi, depGraph2)) != skipnodes.end()){
			delEdges.push_back(*vi);
		}
	}
	BOOST_FOREACH (Graph::edge_descriptor e, delEdges){
		remove_edge(e, depGraph2);
	}
	BOOST_FOREACH (Node n, skipnodes){
		remove_vertex(n, depGraph2);
	}

	// make a BFS in the reduced graph
	typedef std::pair<IDAddress, IDAddress> Edge;
	BOOST_FOREACH (Edge e, externalEdges){
		if (!intr->getFact(e.first)) continue;
		if (!intr->getFact(e.second)) continue;
		if (std::find(depSCC[compNr].begin(), depSCC[compNr].end(), e.first) == depSCC[compNr].end()) continue;
		if (std::find(depSCC[compNr].begin(), depSCC[compNr].end(), e.second) == depSCC[compNr].end()) continue;

		std::vector<Graph::vertex_descriptor> reachable;
		boost::breadth_first_search(depGraph2, depNodes.at(e.second),
			boost::visitor(
				boost::make_bfs_visitor(
					boost::write_property(
						boost::identity_property_map(),
						std::back_inserter(reachable),
						boost::on_discover_vertex())))); 

		if (std::find(reachable.begin(), reachable.end(), depNodes.at(e.second)) != reachable.end()){
			// yes, there is a cycle
			return true;
		}
	}
	if (hasHeadCycles(compNr)){
		DBGLOG(DBG, "Component " << compNr << " has no e-cycle wrt. interpretation, although it has in general e-cycles");
	}

	return false;
}

bool AnnotatedGroundProgram::hasHeadCycles() const{
	return headCyclesTotal;
}

bool AnnotatedGroundProgram::hasECycles(InterpretationConstPtr intr) const{
	for (uint32_t i = 0; i < depSCC.size(); ++i){
		if (hasECycles(i, intr)) return true;
	}
	if (hasHeadCycles()){
		DBGLOG(DBG, "Program has no e-cycle wrt. interpretation, although it has in general e-cycles");
	}
	return false;
}

bool AnnotatedGroundProgram::hasECycles() const{
	return eCyclesTotal;
}

bool AnnotatedGroundProgram::mapsAux(IDAddress ida) const{
	return auxToEA.find(ida) != auxToEA.end();
}

const boost::unordered_map<IDAddress, std::vector<ID> >& AnnotatedGroundProgram::getAuxToEA() const{
	return auxToEA;
}

const std::vector<ID>& AnnotatedGroundProgram::getAuxToEA(IDAddress ida) const{
	assert(auxToEA.find(ida) != auxToEA.end());
	return auxToEA.at(ida);
}

boost::shared_ptr<ExternalAtomMask> AnnotatedGroundProgram::getEAMask(int eaIndex){
	assert((uint32_t)eaIndex >= 0 && (uint32_t)eaIndex < indexedEatoms.size());
	eaMasks[eaIndex]->updateMask();
	return eaMasks[eaIndex];
}

const OrdinaryASPProgram& AnnotatedGroundProgram::getGroundProgram() const{
	return groundProgram;
}


const std::vector<ID>& AnnotatedGroundProgram::getIndexedEAtoms() const{
	return indexedEatoms;
}

ID AnnotatedGroundProgram::getIndexedEAtom(int index) const{
	assert((uint32_t)index >= 0 && (uint32_t)index < indexedEatoms.size());
	return indexedEatoms[index];
}

InterpretationConstPtr AnnotatedGroundProgram::getProgramMask() const{
	assert(!!programMask);
	return programMask;
}


void AnnotatedGroundProgram::setCompleteSupportSetsForVerification(SimpleNogoodContainerPtr supportSets){
	this->supportSets = supportSets;
}

bool AnnotatedGroundProgram::allowsForVerificationUsingCompleteSupportSets() const{
	return !!supportSets;
}

SimpleNogoodContainerPtr AnnotatedGroundProgram::getCompleteSupportSetsForVerification(){
	return supportSets;
}

bool AnnotatedGroundProgram::verifyExternalAtomsUsingCompleteSupportSets(int eaIndex, InterpretationConstPtr interpretation, InterpretationConstPtr auxiliariesToVerify){

	const ExternalAtom& eatom = reg->eatoms.getByID(indexedEatoms[eaIndex]);

	bool supportSetPolarity = eatom.getExtSourceProperties().providesCompletePositiveSupportSets();

	DBGLOG(DBG, "Verifying external atom " << indexedEatoms[eaIndex] << " using " << supportSets->getNogoodCount() << " complete support sets");

	// The external atom is verified wrt. interpretation I iff
	//      (i) it provides complete positive (negative) support sets
	//  and (ii) for each ground instance which is true (false) in I, there is a support set which contains this ground instance negatively (positively)
	//                                              and such that the remaining atoms are true in I.
	// This is checked as follows:
	//   1. Identify all support sets (Inp \cup { EA }) s.t. Inp \subseteq I is a set of ordinary literals and EA is an external atom replacement
	//   2. Keep the set S of all positive EAs that must be true (false)
	//   3. All positive ground instances which are true (false) in I must occur in S

#ifdef DEBUG
	Nogood impl_ng;
#endif
	InterpretationPtr implications(new Interpretation(reg));	// this is set S
	for (int i = 0; i < supportSets->getNogoodCount(); i++){
		ID mismatch = ID_FAIL;
		ID ea = ID_FAIL;
		const Nogood& ng = supportSets->getNogood(i);
		if (ng.isGround()){
			BOOST_FOREACH (ID id, ng){
				// because nogoods eliminate unnecessary flags from IDs in order to store them in a uniform way,
				// we need to lookup the atom here to get its attributes
				IDKind kind = reg->ogatoms.getIDByAddress(id.address).kind | (id.isNaf() ? ID::NAF_MASK : 0);
				if ((kind & ID::PROPERTY_EXTERNALAUX) == ID::PROPERTY_EXTERNALAUX){
					if (ea != ID_FAIL) throw GeneralError("Support set " + ng.getStringRepresentation(reg) + " is invalid becaues it contains multiple external atom replacement literals");
					ea = ID(kind, id.address);
				}else if (!id.isNaf() != interpretation->getFact(id.address)){
#ifdef DEBUG
					std::stringstream ss;
					RawPrinter printer(ss, reg);
					printer.print(id);
					ss << " is false in " << *interpretation;
					DBGLOG(DBG, "Mismatch: " << ss.str());
#endif
					mismatch = id;
					break;
				}
			}
			DBGLOG(DBG, "Analyzing support set " << ng.getStringRepresentation(reg) << " yielded " << (mismatch != ID_FAIL ? "mis" : "") << "match");
			if (mismatch == ID_FAIL){
				if (ea == ID_FAIL) throw GeneralError("Support set " + ng.getStringRepresentation(reg) + " is invalid becaues it contains no external atom replacement literal");

				if (supportSetPolarity == true){
					// store all and only the positive replacement atoms which must be true 
					if (reg->isPositiveExternalAtomAuxiliaryAtom(ea) && ea.isNaf()){
#ifdef DEBUG
						impl_ng.insert(ea);
#endif
						implications->setFact(ea.address);
					}else if(reg->isNegativeExternalAtomAuxiliaryAtom(ea) && !ea.isNaf()){
#ifdef DEBUG
						impl_ng.insert(reg->swapExternalAtomAuxiliaryAtom(ea));
#endif
						implications->setFact(reg->swapExternalAtomAuxiliaryAtom(ea).address);
					}else{
						throw GeneralError("Set " + ng.getStringRepresentation(reg) + " is an invalid positive support set");
					}
				}else{
					// store all and only the positive replacement atoms which must be false
					if (reg->isPositiveExternalAtomAuxiliaryAtom(ea) && !ea.isNaf()){
#ifdef DEBUG
						impl_ng.insert(reg->swapExternalAtomAuxiliaryAtom(ea));
#endif
						implications->setFact(reg->swapExternalAtomAuxiliaryAtom(ea).address);
					}else if(reg->isNegativeExternalAtomAuxiliaryAtom(ea) && ea.isNaf()){
#ifdef DEBUG
						impl_ng.insert(ea);
#endif
						implications->setFact(ea.address);
					}else{
						throw GeneralError("Set " + ng.getStringRepresentation(reg) + " is an invalid negative support set");
					}
				}
			}
		}
	}

	// if auxiliariesToVerify is not set, then verify all true atoms
	if (!auxiliariesToVerify) auxiliariesToVerify = interpretation;
#ifdef DEBUG
	DBGLOG(DBG, "Interpretation: " << *interpretation);
	DBGLOG(DBG, "Implications: " << *implications);
	DBGLOG(DBG, "Aux to verify: " << *auxiliariesToVerify);
	std::stringstream eamss;
	eamss << *getEAMask(eaIndex)->mask();
	DBGLOG(DBG, "EA-Mask: " << eamss.str());
#endif

	bool verify = true;
	bm::bvector<>::enumerator en = getEAMask(eaIndex)->mask()->getStorage().first();
	bm::bvector<>::enumerator en_end = getEAMask(eaIndex)->mask()->getStorage().end();
	while (en < en_end){
		if (auxiliariesToVerify->getFact(*en)){
			ID id = reg->ogatoms.getIDByAddress(*en);
			if (id.isExternalAuxiliary() && !id.isExternalInputAuxiliary()){

				// determine the guessed truth value of the external atom
				bool eaGuessedTruthValue;
				ID posId, negId;
				if (reg->isPositiveExternalAtomAuxiliaryAtom(id)){
					eaGuessedTruthValue = interpretation->getFact(id.address);
					posId = id;
					negId = reg->swapExternalAtomAuxiliaryAtom(id);
				}else{
					eaGuessedTruthValue = !interpretation->getFact(id.address);
					posId = reg->swapExternalAtomAuxiliaryAtom(id);
					negId = id;
				}

#ifdef DEBUG
				std::stringstream ss;
				RawPrinter printer(ss, reg);
				printer.print(posId);
				DBGLOG(DBG, "Verifying auxiliary " << ss.str() << "=" << eaGuessedTruthValue);
#endif

				// check it against the support sets
				if (eaGuessedTruthValue == true){
					if ( supportSetPolarity == true && !implications->getFact(posId.address) ){
						DBGLOG(DBG, "Failed because " << implications->getFact(id.address) << " == false");
						verify = false;
						break;
					}
					if ( supportSetPolarity == false && implications->getFact(negId.address) ){
						DBGLOG(DBG, "Failed because " << implications->getFact(negId.address) << " == true");
						verify = false;
						break;
					}
				}else{
					if ( supportSetPolarity == false && !implications->getFact(negId.address) ){
						DBGLOG(DBG, "Failed because " << implications->getFact(id.address) << " == false");
						verify = false;
						break;
					}
					if ( supportSetPolarity == true && implications->getFact(posId.address) ){
						DBGLOG(DBG, "Failed because " << implications->getFact(posId.address) << " == true");
						verify = false;
						break;
					}
				}
				DBGLOG(DBG, "Verified auxiliary " << ss.str() << "=" << eaGuessedTruthValue << " in " << *interpretation << " wrt. implications " << *implications);
			}
		}
		en++;
	}

	DBGLOG(DBG, "Verification done");
	return verify;
}

DLVHEX_NAMESPACE_END

