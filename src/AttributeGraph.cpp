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

AttributeGraph::Attribute AttributeGraph::getAttribute(ID predicate, std::vector<ID> inputList, ID ruleID, bool inputAttribute, int argumentIndex){

	Attribute at;
	at.type = Attribute::External;
	at.ruleID = ruleID;
	at.predicate = predicate;
	at.inputList = inputList;
	at.input = inputAttribute;
	at.argIndex = argumentIndex;
	return at;
}

AttributeGraph::Attribute AttributeGraph::getAttribute(ID predicate, int argumentIndex){

	Attribute at;
	at.type = Attribute::Ordinary;
	at.ruleID = ID_FAIL;
	at.predicate = predicate;
	at.input = false;
	at.argIndex = argumentIndex;
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
							Node bodyNode = getNode(getAttribute(eAtom.predicate, eAtom.inputs, ruleID, false, (bArg + 1)));

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
								Node bodyNode2 = getNode(getAttribute(eAtom.predicate, eAtom.inputs, ruleID, true, (bArg2 + 1)));

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
					Node bodyNode1 = getNode(getAttribute(eAtom1.predicate, eAtom1.inputs, ruleID, false, (bArg1 + 1)));

					BOOST_FOREACH (ID bID2, rule.body){
						if (bID2.isExternalAtom()){
							const ExternalAtom& eAtom2 = reg->eatoms.getByID(bID2);

							for (int bArg2 = 0; bArg2 < eAtom2.inputs.size(); ++bArg2){
								Node bodyNode2 = getNode(getAttribute(eAtom2.predicate, eAtom2.inputs, ruleID, true, (bArg2 + 1)));

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
					Node inputNode = getNode(getAttribute(eAtom.predicate, eAtom.inputs, ruleID, true, (i + 1)));
					for (int o = 0; o < eAtom.tuple.size(); ++o){
						Node outputNode = getNode(getAttribute(eAtom.predicate, eAtom.inputs, ruleID, false, (o + 1)));
						boost::add_edge(inputNode, outputNode, ag);
					}
					if (eAtom.pluginAtom->getInputType(i) == PluginAtom::PREDICATE){
						predicateInputs.push_back(std::pair<Attribute, ID>(getAttribute(eAtom.predicate, eAtom.inputs, ruleID, true, (i + 1)), eAtom.inputs[i]));
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

AttributeGraph::AttributeGraph(RegistryPtr reg, const std::vector<ID>& idb) : reg(reg), idb(idb){
	createDependencies();
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
			RawPrinter printer(ss, reg);

			if (ag[*it].type == Attribute::Ordinary){
				// ordinary attribute
				printer.print(ag[*it].predicate);
				ss << "#";
				ss << ag[*it].argIndex;
			}else{
				// external attribute
				ss << "r" << ag[*it].ruleID.address << ":";
				ss << "&";
				printer.print(ag[*it].predicate);
				ss << "[";
				for (int i = 0; i < ag[*it].inputList.size(); ++i){
					if (i > 0) ss << ",";
					printer.print(ag[*it].inputList[i]);
				}
				ss << "]";
				ss << "#";
				ss << (ag[*it].input ? "i" : "o");
				ss << ag[*it].argIndex;
			}

			graphviz::escape(o, ss.str());
		}
		o << "\"";
		o << ",shape=box";
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
	boost::hash_combine(seed, at.predicate);
	BOOST_FOREACH (ID i, at.inputList) boost::hash_combine(seed, i);
	boost::hash_combine(seed, at.ruleID);
	boost::hash_combine(seed, at.input);
	boost::hash_combine(seed, at.argIndex);
	return seed;
}

DLVHEX_NAMESPACE_END

