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
 * @file AttributeGraph.cpp
 * @author Christoph Redl
 *
 * @brief Stores dependencies between attributes in a program.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/AttributeGraph.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Rule.h"
#include "dlvhex2/Atoms.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/GraphvizHelpers.h"

#include <boost/property_map/property_map.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/range/join.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/visitors.hpp> 
#include <boost/graph/strong_components.hpp>

#include <sstream>

DLVHEX_NAMESPACE_BEGIN

bool AttributeGraph::Attribute::operator==(const Attribute& at2) const{
	return	type == at2.type &&
		predicate == at2.predicate &&
		inputList == at2.inputList &&
		ruleID == at2.ruleID &&
		input == at2.input &&
		argIndex == at2.argIndex;
}

bool AttributeGraph::Attribute::operator<(const Attribute& at2) const{
	if (type < at2.type) return true;
	if (predicate < at2.predicate) return true;
	if (inputList.size() < at2.inputList.size()) return true;
	for (int i = 0; i < inputList.size(); ++i)
		if (inputList[i] < at2.inputList[i]) return true;
	if (ruleID < at2.ruleID) return true;
	if (input < at2.input) return true;
	if (argIndex < at2.argIndex) return true;
	return false;
}

std::ostream& AttributeGraph::Attribute::print(std::ostream& o) const{

	RawPrinter printer(o, reg);
	if (type == Attribute::Ordinary){
		// ordinary attribute
		printer.print(predicate);
		o << "#";
		o << argIndex;
	}else{
		// external attribute
		o << "r" << ruleID.address << ":";
		o << "&";
		printer.print(predicate);
		o << "[";
		for (int i = 0; i < inputList.size(); ++i){
			if (i > 0) o << ",";
			printer.print(inputList[i]);
		}
		o << "]";
		o << "#";
		o << (input ? "i" : "o");
		o << argIndex;
	}
	return o;
}

AttributeGraph::Attribute AttributeGraph::getAttribute(ID eatomID, ID predicate, std::vector<ID> inputList, ID ruleID, bool inputAttribute, int argumentIndex){

	Attribute at;
	at.reg = reg;
	at.type = Attribute::External;
	at.ruleID = ruleID;
	at.eatomID = eatomID;
	at.predicate = predicate;
	at.inputList = inputList;
	at.input = inputAttribute;
	at.argIndex = argumentIndex;
	return at;
}

AttributeGraph::Attribute AttributeGraph::getAttribute(ID predicate, int argumentIndex){

	Attribute at;
	at.reg = reg;
	at.type = Attribute::Ordinary;
	at.ruleID = ID_FAIL;
	at.eatomID = ID_FAIL;
	at.predicate = predicate;
	at.input = false;
	at.argIndex = argumentIndex;
	predicateArity[predicate] = argumentIndex > predicateArity[predicate] ? argumentIndex : predicateArity[predicate];
	return at;
}

AttributeGraph::Node AttributeGraph::getNode(Attribute at){

	const NodeNodeInfoIndex& idx = nm.get<NodeInfoTag>();
	NodeNodeInfoIndex::const_iterator it = idx.find(at);
	if(it != idx.end()){
		return it->node;
	}else{
		Node n = boost::add_vertex(at, ag);
		if (at.type == Attribute::Ordinary) attributesOfPredicate[at.predicate].push_back(at);
		NodeNodeInfoIndex::const_iterator it;
		bool success;
		boost::tie(it, success) = nm.insert(NodeMappingInfo(at, n));
		assert(success);
		return n;
	}
}

bool AttributeGraph::isNewlySafe(Attribute at){
	return safetyPreconditions[at].first.size() == 0 && safetyPreconditions[at].second.size() == 0;
}

void AttributeGraph::addBoundedVariable(VariableLocation vl){

	// go through all attributes bounded by this variable
#ifndef NDEBUG
	std::stringstream ss;
	RawPrinter printer(ss, reg);
	printer.print(vl.second);
	DBGLOG(DBG, "Variable " << "r" << vl.first.address << "/" << ss.str() << " is bounded");
#endif
	boundedVariables.insert(vl);

	// notify all attributes which wait for this variable to become bounded
	while (attributesSafeByVariable[vl].size() > 0){
		Attribute sat = *attributesSafeByVariable[vl].begin();
		DBGLOG(DBG, "Fulfilled precondition of attribute " << sat);
		attributesSafeByVariable[vl].erase(attributesSafeByVariable[vl].begin());

		safetyPreconditions[sat].first.erase(vl);
		if (isNewlySafe(sat)){
			addDomainExpansionSafeAttribute(sat);
		}
	}
	attributesSafeByVariable[vl].clear();
}

void AttributeGraph::addDomainExpansionSafeAttribute(Attribute at){

	// go through all atoms where the attribute occurs
	DBGLOG(DBG, "Attribute " << at << " is domain-expansion safe");
	domainExpansionSafeAttributes.insert(at);

	// notify all attributes which wait for this attribute to become domain-expansion safe
	while (attributesSafeByAttribute[at].size() > 0){
		Attribute sat = *attributesSafeByAttribute[at].begin();
		DBGLOG(DBG, "Fulfilled precondition of attribute " << sat);
		attributesSafeByAttribute[at].erase(attributesSafeByAttribute[at].begin());

		assert(std::find(safetyPreconditions[sat].second.begin(), safetyPreconditions[sat].second.end(), at) != safetyPreconditions[sat].second.end());
		safetyPreconditions[sat].second.erase(at);
		if (isNewlySafe(at)){
			addDomainExpansionSafeAttribute(sat);
		}
	}

	// safe attributes may lead to safe variables
	// process safe variables due to ordinary atoms first (we want to use external atoms as rarely as possible in order to optimize them away)
//	for (int ordinary = 1; ordinary >= 0; ordinary--){
		BOOST_FOREACH (AtomLocation al, attributeOccursIn[at]){
			if (al.second.isOrdinaryAtom()){
				const OrdinaryAtom& oatom = reg->lookupOrdinaryAtom(al.second);
				if (oatom.tuple[at.argIndex].isVariableTerm()){
					addBoundedVariable(VariableLocation(al.first, oatom.tuple[at.argIndex]));
				}
			}
			if (al.second.isExternalAtom()){
				const ExternalAtom& eatom = reg->eatoms.getByID(al.second);
				for (int o = 0; o < eatom.tuple.size(); ++o){
					if (getAttribute(al.second, eatom.predicate, eatom.inputs, al.first, false, o + 1) == at){
						if (eatom.tuple[o].isVariableTerm()){
							VariableLocation vl(al.first, eatom.tuple[o]);
/*
							if (boundedVariables.count(vl) == 0){
								std::stringstream ss;	
								RawPrinter printer(ss, reg);
								printer.print(vl.second);
								DBGLOG(DBG, "External atom " << al.second << " is necessary to establish boundedness of " << "r" << vl.first.address << "/" << ss.str());
								necessaryExternalAtoms.insert(al.second);
							}
*/
							// here we COULD bound vl, but we don't do it yet, because
							// we want to check first if we can also make it safe without exploiting the external atom
							// (this would have the advantage that we can optimize the external atom away)
							boundedByExternals.insert(std::pair<ID, VariableLocation>(al.second, vl));
//							addBoundedVariable(vl);
						}
					}
				}
			}
		}
//	}
}

void AttributeGraph::createDependencies(){

	std::vector<std::pair<Attribute, ID> > predicateInputs;

	DBGLOG(DBG, "AttributeGraph::createDependencies");
	BOOST_FOREACH (ID ruleID, idb){
		const Rule& rule = reg->rules.getByID(ruleID);

		// head-body dependencies
		BOOST_FOREACH (ID hID, rule.head){
			const OrdinaryAtom& hAtom = reg->lookupOrdinaryAtom(hID);

			for (int hArg = 1; hArg < hAtom.tuple.size(); ++hArg){
				Node headNode = getNode(getAttribute(hAtom.tuple[0], hArg));

				BOOST_FOREACH (ID bID, rule.body){
					if (bID.isNaf()) continue;

					if (bID.isOrdinaryAtom()){
						const OrdinaryAtom& bAtom = reg->lookupOrdinaryAtom(bID);

						for (int bArg = 1; bArg < bAtom.tuple.size(); ++bArg){
							Node bodyNode = getNode(getAttribute(bAtom.tuple[0], bArg));

							if (hAtom.tuple[hArg].isVariableTerm() && bAtom.tuple[bArg].isVariableTerm() && hAtom.tuple[hArg] == bAtom.tuple[bArg]){
								boost::add_edge(bodyNode, headNode, ag);
							}
						}
					}

					if (bID.isExternalAtom()){
						const ExternalAtom& eAtom = reg->eatoms.getByID(bID);

						for (int bArg = 0; bArg < eAtom.tuple.size(); ++bArg){
							Node bodyNode = getNode(getAttribute(bID, eAtom.predicate, eAtom.inputs, ruleID, false, (bArg + 1)));

							if (hAtom.tuple[hArg].isVariableTerm() && eAtom.tuple[bArg].isVariableTerm() && hAtom.tuple[hArg] == eAtom.tuple[bArg]){
								boost::add_edge(bodyNode, headNode, ag);
							}
						}
					}
				}
			}
		}

		// body-body dependencies
		BOOST_FOREACH (ID bID1, rule.body){
			if (bID1.isNaf()) continue;

			if (bID1.isOrdinaryAtom()){
				const OrdinaryAtom& bAtom = reg->lookupOrdinaryAtom(bID1);

				for (int bArg1 = 1; bArg1 < bAtom.tuple.size(); ++bArg1){
					Node bodyNode1 = getNode(getAttribute(bAtom.tuple[0], bArg1));

					BOOST_FOREACH (ID bID2, rule.body){
						if (bID2.isExternalAtom()){
							const ExternalAtom& eAtom = reg->eatoms.getByID(bID2);

							for (int bArg2 = 0; bArg2 < eAtom.inputs.size(); ++bArg2){
								Node bodyNode2 = getNode(getAttribute(bID2, eAtom.predicate, eAtom.inputs, ruleID, true, (bArg2 + 1)));

								if (bAtom.tuple[bArg1].isVariableTerm() && eAtom.inputs[bArg2].isVariableTerm() && bAtom.tuple[bArg1] == eAtom.inputs[bArg2]){
									boost::add_edge(bodyNode1, bodyNode2, ag);
								}
							}
						}
					}
				}
			}
			if (bID1.isExternalAtom()){
				const ExternalAtom& eAtom1 = reg->eatoms.getByID(bID1);

				for (int bArg1 = 0; bArg1 < eAtom1.tuple.size(); ++bArg1){
					Node bodyNode1 = getNode(getAttribute(bID1, eAtom1.predicate, eAtom1.inputs, ruleID, false, (bArg1 + 1)));

					BOOST_FOREACH (ID bID2, rule.body){
						if (bID2.isExternalAtom()){
							const ExternalAtom& eAtom2 = reg->eatoms.getByID(bID2);

							for (int bArg2 = 0; bArg2 < eAtom2.inputs.size(); ++bArg2){
								Node bodyNode2 = getNode(getAttribute(bID2, eAtom2.predicate, eAtom2.inputs, ruleID, true, (bArg2 + 1)));

								if (eAtom1.tuple[bArg1].isVariableTerm() && eAtom2.inputs[bArg2].isVariableTerm() && eAtom1.tuple[bArg1] == eAtom2.inputs[bArg2]){
									boost::add_edge(bodyNode1, bodyNode2, ag);
								}
							}
						}
					}
				}
			}
		}

		// EA input-output dependencies
		BOOST_FOREACH (ID bID, rule.body){
			if (bID.isExternalAtom()){
				const ExternalAtom& eAtom = reg->eatoms.getByID(bID);

				for (int i = 0; i < eAtom.inputs.size(); ++i){
					Node inputNode = getNode(getAttribute(bID, eAtom.predicate, eAtom.inputs, ruleID, true, (i + 1)));
					for (int o = 0; o < eAtom.tuple.size(); ++o){
						Node outputNode = getNode(getAttribute(bID, eAtom.predicate, eAtom.inputs, ruleID, false, (o + 1)));
						boost::add_edge(inputNode, outputNode, ag);
					}
					if (eAtom.pluginAtom->getInputType(i) == PluginAtom::PREDICATE){
						predicateInputs.push_back(std::pair<Attribute, ID>(getAttribute(bID, eAtom.predicate, eAtom.inputs, ruleID, true, (i + 1)), eAtom.inputs[i]));
					}
				}
			}
		}
	}

	// connect predicate input attributes
	typedef std::pair<Attribute, ID> AttPredPair;
	BOOST_FOREACH (AttPredPair p, predicateInputs){
		BOOST_FOREACH (Attribute ordinaryPredicateAttribute, attributesOfPredicate[p.second]){
			boost::add_edge(getNode(ordinaryPredicateAttribute), getNode(p.first), ag);
		}
	}
}

void AttributeGraph::createIndices(){

	BOOST_FOREACH (ID ruleID, idb){
		const Rule& rule = reg->rules.getByID(ruleID);

		// store for each attribute of a head atom the variable on which it depends
		BOOST_FOREACH (ID hID, rule.head){
			const OrdinaryAtom& oatom = reg->lookupOrdinaryAtom(hID);
			for (int i = 1; i < oatom.tuple.size(); ++i){
				if (oatom.tuple[i].isVariableTerm()){
					safetyPreconditions[getAttribute(oatom.tuple[0], i)].first.insert(VariableLocation(ruleID, oatom.tuple[i]));
					attributesSafeByVariable[VariableLocation(ruleID, oatom.tuple[i])].insert(getAttribute(oatom.tuple[0], i));
				}
			}
		}

		// 1. store for body attributes in which ordinary or external atoms they occur
		// 2. store for external atoms:
		//	- for which variables they wait
		//	- for which attributes they wait
		BOOST_FOREACH (ID bID, rule.body){
			if (bID.isNaf()) continue;

			// attributes which occur in ordinary body atoms
			if (bID.isOrdinaryAtom()){
				const OrdinaryAtom& oatom = reg->lookupOrdinaryAtom(bID);
				for (int i = 1; i < oatom.tuple.size(); ++i){
					attributeOccursIn[getAttribute(oatom.tuple[0], i)].insert(AtomLocation(ruleID, bID));
				}
			}

			// attributes which occur as predicate input to external atoms
			// also store the preconditions for an external attribute to become domain-expansion safe
			if (bID.isExternalAtom()){
				const ExternalAtom& eatom = reg->eatoms.getByID(bID);
				for (int i = 0; i < eatom.inputs.size(); ++i){
					Attribute iattr = getAttribute(bID, eatom.predicate, eatom.inputs, ruleID, true, i + 1);

					// for predicate input parameters, we have to wait for all attributes of the according predicate to become safe
					if (eatom.pluginAtom->getInputType(i) == PluginAtom::PREDICATE){
						for (int a = 1; a <= predicateArity[eatom.inputs[i]]; ++a){
							attributeOccursIn[getAttribute(eatom.inputs[i], a)].insert(AtomLocation(ruleID, bID));
							safetyPreconditions[iattr].second.insert(getAttribute(eatom.inputs[i], a));
							attributesSafeByAttribute[getAttribute(eatom.inputs[i], a)].insert(iattr);
						}
					}
					// for variables in place of constant parameters, we have to wait for the variable to become bounded
					if (eatom.pluginAtom->getInputType(i) != PluginAtom::PREDICATE && eatom.inputs[i].isVariableTerm()){
						safetyPreconditions[iattr].first.insert(VariableLocation(ruleID, eatom.inputs[i]));
						attributesSafeByVariable[VariableLocation(ruleID, eatom.inputs[i])].insert(iattr);
					}

					// for output attributes, we have to wait for all input attributes to become safe
					for (int o = 0; o < eatom.tuple.size(); ++o){
						Attribute oattr = getAttribute(bID, eatom.predicate, eatom.inputs, ruleID, false, o + 1);
						attributeOccursIn[oattr].insert(AtomLocation(ruleID, bID));
						safetyPreconditions[oattr].second.insert(iattr);
						attributesSafeByAttribute[iattr].insert(oattr);
					}
				}
			}
		}
	}
}

void AttributeGraph::computeDomainExpansionSafety(){

	// find strongly connected components in the graph
	DBGLOG(DBG, "Computing strongly connected components in attribute dependency graph");
	std::vector<int> componentMap(num_vertices(ag));
	int num = boost::strong_components(ag, &componentMap[0]);
	std::vector<std::vector<Attribute> > depSCC(num);
	int nodeNr = 0;
	BOOST_FOREACH (int componentOfNode, componentMap){
		depSCC[componentOfNode].push_back(ag[nodeNr++]);
	}

	// find cyclic external attributes
	std::vector<Attribute> cyclicExternal;
	for (int c = 0; c < depSCC.size(); ++c){
		BOOST_FOREACH (Attribute at, depSCC[c]){
			if (at.type == Attribute::External && depSCC[c].size() > 1){
				DBGLOG(DBG, "Found cyclic external attribute of " << at.predicate);
				cyclicExternal.push_back(at);
				break;
			}
		}
	}

	// find all attributes which depend on such cyclic external attributes
	BOOST_FOREACH (Attribute at, cyclicExternal){
		const NodeNodeInfoIndex& idx = nm.get<NodeInfoTag>();
		NodeNodeInfoIndex::const_iterator it = idx.find(at);
		boost::breadth_first_search(ag, it->node,
			boost::visitor(
				boost::make_bfs_visitor(
					boost::write_property(
						boost::identity_property_map(),
						std::inserter(cyclicAttributes, cyclicAttributes.end()),
						boost::on_discover_vertex())))); 
	}
	DBGLOG(DBG, "" << cyclicAttributes.size() << " attributes depend cyclically on external attributes");

	NodeIterator it, it_end;
	for(boost::tie(it, it_end) = boost::vertices(ag); it != it_end; ++it){
		// 1. make all attributes safe, except those in cyclicAttributes
		if (cyclicAttributes.count(*it) == 0){
			DBGLOG(DBG, "Attribute " << ag[*it] << " is externally acyclic");
			addDomainExpansionSafeAttribute(ag[*it]);
		}
		// 2. attributes are safe if there are no more variables which need to be bounded
		else if (safetyPreconditions[ag[*it]].first.size() == 0 && safetyPreconditions[ag[*it]].second.size() == 0){
			DBGLOG(DBG, "Attribute " << ag[*it] << " does not depend on any unbounded variables or unsafe attributes");
			addDomainExpansionSafeAttribute(ag[*it]);
		}
	}

	// exploit external atoms to establish further boundings of variables
	while (boundedByExternals.size() > 0){
		VariableLocation vl = boundedByExternals.begin()->second;
		ID eatom = boundedByExternals.begin()->first;
		boundedByExternals.erase(boundedByExternals.begin());
		if (boundedVariables.count(vl) == 0){
			DBGLOG(DBG, "Exploiting " << eatom);
			necessaryExternalAtoms.insert(eatom);
			addBoundedVariable(vl);
		}
	}

	DBGLOG(DBG, "Domain Expansion Safety: " << isDomainExpansionSafe() << " (" << domainExpansionSafeAttributes.size() << " out of " << num_vertices(ag) << " attributes are safe)");
}

AttributeGraph::AttributeGraph(RegistryPtr reg, const std::vector<ID>& idb) : reg(reg), idb(idb){
	createDependencies();
	createIndices();
	computeDomainExpansionSafety();
}

bool AttributeGraph::isDomainExpansionSafe() const{
	return domainExpansionSafeAttributes.size() == num_vertices(ag);
}

bool AttributeGraph::isExternalAtomNecessaryForDomainExpansionSafety(ID eatomID) const{
	assert(isDomainExpansionSafe());
	return necessaryExternalAtoms.count(eatomID) > 0;
}

namespace
{
	inline std::string graphviz_node_id(AttributeGraph::Node n)
	{
		std::ostringstream os;
		os << "n" << n;
		return os.str();
	}
}

void AttributeGraph::writeGraphViz(std::ostream& o, bool verbose) const{

	DBGLOG(DBG, "AttributeGraph::writeGraphViz");

	o << "digraph G {" << std::endl;

	// print vertices
	NodeIterator it, it_end;
	for(boost::tie(it, it_end) = boost::vertices(ag); it != it_end; ++it){
		o << graphviz_node_id(*it) << "[label=\"";
		{
			std::ostringstream ss;
			ss << ag[*it];
			graphviz::escape(o, ss.str());
		}
		o << "\"";
		o << ",shape=box";
		std::vector<std::string> style;
		if (cyclicAttributes.count(*it) > 0){
			if (std::find(domainExpansionSafeAttributes.begin(), domainExpansionSafeAttributes.end(), ag[*it]) == domainExpansionSafeAttributes.end()){
				o << ",fillcolor=red";
			}else{
				o << ",fillcolor=yellow";
			}
			style.push_back("filled");
		}
		if (ag[*it].type == Attribute::External && necessaryExternalAtoms.count(ag[*it].eatomID) == 0){
			style.push_back("dashed");
		}
		o << ",style=\"";
		for (int i = 0; i < style.size(); ++i) o << (i > 0 ? "," : "") << style[i];
		o << "\"";
		o << "];" << std::endl;
	}

	// print edges
	DependencyIterator dit, dit_end;
	for(boost::tie(dit, dit_end) = boost::edges(ag); dit != dit_end; ++dit) {
		Node src = boost::source(*dit, ag);
		Node target = boost::target(*dit, ag);
		o << graphviz_node_id(src) << " -> " << graphviz_node_id(target) <<
		"[label=\"";
		{
		std::ostringstream ss;
		}
		o << "\"];" << std::endl;
	}

	o << "}" << std::endl;
}

std::size_t hash_value(const AttributeGraph::Attribute& at)
{
	std::size_t seed = 0;
	boost::hash_combine(seed, at.type);
	boost::hash_combine(seed, at.eatomID);
	boost::hash_combine(seed, at.predicate);
	BOOST_FOREACH (ID i, at.inputList) boost::hash_combine(seed, i);
	boost::hash_combine(seed, at.ruleID);
	boost::hash_combine(seed, at.input);
	boost::hash_combine(seed, at.argIndex);
	return seed;
}

std::size_t hash_value(const AttributeGraph::VariableLocation& vl)
{
	std::size_t seed = 0;
	boost::hash_combine(seed, vl.first);
	boost::hash_combine(seed, vl.second);
	return seed;
}

DLVHEX_NAMESPACE_END

