#include "dlvhex2/AnnotatedGroundProgram.h"
#include "dlvhex2/Printer.h"

#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/visitors.hpp> 
#include <boost/graph/strong_components.hpp>
#include <boost/graph/filtered_graph.hpp>

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

AnnotatedGroundProgram::AnnotatedGroundProgram() : groundProgram(OrdinaryASPProgram(RegistryPtr(), std::vector<ID>(), InterpretationConstPtr())), haveGrounding(false){
}

AnnotatedGroundProgram::AnnotatedGroundProgram(RegistryPtr reg, const OrdinaryASPProgram& groundProgram, std::vector<ID> indexedEatoms) :
	reg(reg), groundProgram(groundProgram), indexedEatoms(indexedEatoms), haveGrounding(true){

	initialize();
}

AnnotatedGroundProgram::AnnotatedGroundProgram(RegistryPtr reg, std::vector<ID> indexedEatoms) :
	reg(reg), groundProgram(OrdinaryASPProgram(RegistryPtr(), std::vector<ID>(), InterpretationConstPtr())), indexedEatoms(indexedEatoms), haveGrounding(false){

	initialize();
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
	eCycles = other.eCycles;
	programComponents = other.programComponents;
	headCyclesTotal = other.headCyclesTotal;
  eCyclesTotal = other.eCyclesTotal;
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

void AnnotatedGroundProgram::createEAMasks(){

	eaMasks.resize(indexedEatoms.size());
	int eaIndex = 0;
	BOOST_FOREACH (ID eatom, indexedEatoms){
		// create an EAMask for each inner external atom
		eaMasks[eaIndex] = boost::shared_ptr<ExternalAtomMask>(new ExternalAtomMask);
		ExternalAtomMask& eaMask = *eaMasks[eaIndex];
		eaMask.setEAtom(reg->eatoms.getByID(eatom), groundProgram.idb);
		eaMask.updateMask();
		eaIndex++;
	}
}

void AnnotatedGroundProgram::mapAuxToEAtoms(){

	int eaIndex = 0;
	BOOST_FOREACH (ID eatom, indexedEatoms){
		// create an EAMask for each inner external atom
		ExternalAtomMask& eaMask = *eaMasks[eaIndex];
		eaMask.setEAtom(reg->eatoms.getByID(eatom), groundProgram.idb);
		eaMask.updateMask();

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

	initialize();
}

void AnnotatedGroundProgram::initialize(){

	eaMasks.resize(0);
	if (haveGrounding) createProgramMask();
	createEAMasks();
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
	DBGLOG(DBG, "Contructing atom dependency graph");
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
	int num = boost::strong_components(depGraph, &componentMap[0]);

	// translate into real map
	depSCC = std::vector<std::set<IDAddress> >(num);
	Node nodeNr = 0;

	BOOST_FOREACH (int componentOfNode, componentMap){
		depSCC[componentOfNode].insert(depGraph[nodeNr]);
		componentOfAtom[depGraph[nodeNr]] = componentOfNode;
		nodeNr++;
	}
#ifndef NDEBUG
	for (int comp = 0; comp < depSCC.size(); ++comp){
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
	for (int comp = 0; comp < depSCC.size(); ++comp){
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
	for (int comp = 0; comp < depSCC.size(); ++comp){
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
	}
}

void AnnotatedGroundProgram::computeECycles(){

	DBGLOG(DBG, "Computing e-cycles of components");

	eCyclesTotal = false;
	for (int comp = 0; comp < depSCC.size(); ++comp){

		// check for each e-edge x -> y if there is a path from y to x
		// if yes, then y is a cyclic predicate input
		std::vector<IDAddress> cyclicInputAtoms;
		typedef std::pair<IDAddress, IDAddress> Edge;
		BOOST_FOREACH (Edge e, externalEdges){
			if (std::find(depSCC[comp].begin(), depSCC[comp].end(), e.first) == depSCC[comp].end()) continue;
			if (std::find(depSCC[comp].begin(), depSCC[comp].end(), e.second) == depSCC[comp].end()) continue;

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
				if (std::find(cyclicInputAtoms.begin(), cyclicInputAtoms.end(), e.second) == cyclicInputAtoms.end()){
					cyclicInputAtoms.push_back(e.second);
				}
			}
		}
		eCycles.push_back(cyclicInputAtoms.size() > 0);
		eCyclesTotal |= eCycles[eCycles.size() - 1];

#ifndef NDEBUG
		std::stringstream ss;
		bool first = true;
		BOOST_FOREACH (IDAddress a, cyclicInputAtoms){
			if (!first) ss << ", ";
			first = false;
			ss << a;
		}
		if (cyclicInputAtoms.size() > 0){
			DBGLOG(DBG, "Component " << comp << ": 1 with cyclic input atoms " << ss.str());
		}else{
			DBGLOG(DBG, "Component " << comp << ": 0");
		}
#endif
	}
}

bool AnnotatedGroundProgram::containsHeadCycles(ID ruleID) const{

	for (int comp = 0; comp < depSCC.size(); ++comp){
		if (headCycles[comp] && std::find(programComponents[comp].program.idb.begin(), programComponents[comp].program.idb.end(), ruleID) != programComponents[comp].program.idb.end()) return true;
	}
	return false;
}

int AnnotatedGroundProgram::getComponentCount() const{
	return programComponents.size();
}

const OrdinaryASPProgram& AnnotatedGroundProgram::getProgramOfComponent(int compNr) const{
	assert(compNr >= 0 && compNr < depSCC.size());
	return programComponents[compNr].program;
}

InterpretationConstPtr AnnotatedGroundProgram::getAtomsOfComponent(int compNr) const{
	assert(compNr >= 0 && compNr < depSCC.size());
	return programComponents[compNr].componentAtoms;
}

bool AnnotatedGroundProgram::hasHeadCycles(int compNr) const{
	assert(compNr >= 0 && compNr < depSCC.size());
	return headCycles[compNr];
}

bool AnnotatedGroundProgram::hasECycles(int compNr) const{
	assert(compNr >= 0 && compNr < depSCC.size());
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
	for (tie(vi, vi_end) = edges(depGraph2); vi != vi_end; vi++){
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
	for (int i = 0; i < depSCC.size(); ++i){
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
	assert(eaIndex >= 0 && eaIndex < indexedEatoms.size());
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
	assert(index >= 0 && index < indexedEatoms.size());
	return indexedEatoms[index];
}

InterpretationConstPtr AnnotatedGroundProgram::getProgramMask() const{
	assert(!!programMask);
	return programMask;
}

DLVHEX_NAMESPACE_END

