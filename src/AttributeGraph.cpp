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

AttributeGraph::Node AttributeGraph::getNode(ID predicate, std::vector<ID> inputList, ID ruleID, bool inputAttribute, int argumentIndex){

	std::vector<ID> ni;
	ni.push_back(ruleID);
	ni.push_back(predicate);
	BOOST_FOREACH (ID id, inputList) ni.push_back(id);
	ni.push_back(inputAttribute ? reg->storeConstantTerm("i") : reg->storeConstantTerm("o"));
	ni.push_back(ID::termFromInteger(argumentIndex));

	const NodeNodeInfoIndex& idx = nm.get<NodeInfoTag>();
	NodeNodeInfoIndex::const_iterator it = idx.find(ni);
	if(it != idx.end()){
		return it->node;
	}else{
		Node n = boost::add_vertex(ni, ag);
		NodeNodeInfoIndex::const_iterator it;
		bool success;
		boost::tie(it, success) = nm.insert(NodeMappingInfo(ni, n));
		assert(success);
		return n;
	}
}

AttributeGraph::Node AttributeGraph::getNode(ID predicate, int argumentIndex){

	std::vector<ID> ni;
	ni.push_back(predicate);
	ni.push_back(ID::termFromInteger(argumentIndex));

	const NodeNodeInfoIndex& idx = nm.get<NodeInfoTag>();
	NodeNodeInfoIndex::const_iterator it = idx.find(ni);
	if(it != idx.end()){
		return it->node;
	}else{
		Node n = boost::add_vertex(ni, ag);
		NodeNodeInfoIndex::const_iterator it;
		bool success;
		boost::tie(it, success) = nm.insert(NodeMappingInfo(ni, n));
		assert(success);
		return n;
	}
}

void AttributeGraph::createDependencies(){

	BOOST_FOREACH (ID ruleID, idb){
		const Rule& rule = reg->rules.getByID(ruleID);

		// head-body dependencies
		BOOST_FOREACH (ID hID, rule.head){
			const OrdinaryAtom& hAtom = reg->lookupOrdinaryAtom(hID);

			for (int hArg = 1; hArg < hAtom.tuple.size(); ++hArg){
				Node headNode = getNode(hAtom.tuple[0], hArg);

				BOOST_FOREACH (ID bID, rule.body){
					if (bID.isNaf()) continue;

					if (bID.isOrdinaryAtom()){
						const OrdinaryAtom& bAtom = reg->lookupOrdinaryAtom(bID);

						for (int bArg = 1; bArg < bAtom.tuple.size(); ++bArg){
							Node bodyNode = getNode(bAtom.tuple[0], bArg);

							if (hAtom.tuple[hArg].isVariableTerm() && bAtom.tuple[bArg].isVariableTerm() && hAtom.tuple[hArg] == bAtom.tuple[bArg]){
								boost::add_edge(bodyNode, headNode, ag);
							}
						}
					}

					if (bID.isExternalAtom()){
						const ExternalAtom& eAtom = reg->eatoms.getByID(bID);

						for (int bArg = 0; bArg < eAtom.tuple.size(); ++bArg){
							Node bodyNode = getNode(eAtom.predicate, eAtom.inputs, ruleID, false, (bArg + 1));

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
					Node bodyNode1 = getNode(bAtom.tuple[0], bArg1);

					BOOST_FOREACH (ID bID2, rule.body){
						if (bID2.isExternalAtom()){
							const ExternalAtom& eAtom = reg->eatoms.getByID(bID2);

							for (int bArg2 = 0; bArg2 < eAtom.inputs.size(); ++bArg2){
								Node bodyNode2 = getNode(eAtom.predicate, eAtom.inputs, ruleID, true, (bArg2 + 1));

								if (bAtom.tuple[bArg1].isVariableTerm() && eAtom.inputs[bArg2].isVariableTerm() && bAtom.tuple[bArg1] == eAtom.inputs[bArg2]){
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
					Node inputNode = getNode(eAtom.predicate, eAtom.inputs, ruleID, true, (i + 1));
					for (int o = 0; o < eAtom.tuple.size(); ++o){
						Node outputNode = getNode(eAtom.predicate, eAtom.inputs, ruleID, false, (o + 1));
						boost::add_edge(inputNode, outputNode, ag);
					}
				}
			}
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

	o << "digraph G {" << std::endl;

	// print vertices
	NodeIterator it, it_end;
	for(boost::tie(it, it_end) = boost::vertices(ag); it != it_end; ++it){
		o << graphviz_node_id(*it) << "[label=\"";
		{
			std::ostringstream ss;
			RawPrinter printer(ss, reg);

			if (ag[*it].size() == 2){
				// ordinary attribute
				for (int i = 0; i < ag[*it].size(); ++i){
					if (i == ag[*it].size() - 1) ss << "#";
					printer.print(ag[*it][i]);
				}
			}else{
				// external attribute
				ss << "r" << ag[*it][0].address;
				ss << "&";
				printer.print(ag[*it][1]);
				ss << "[";
				for (int i = 2; i < ag[*it].size() - 2; ++i){
					if (i > 2) ss << ",";
					printer.print(ag[*it][i]);
				}
				ss << "]";
				ss << "#";
				for (int i = ag[*it].size() - 2; i < ag[*it].size(); ++i){
					printer.print(ag[*it][i]);
				}
			}

			o << ss.str();
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

DLVHEX_NAMESPACE_END

