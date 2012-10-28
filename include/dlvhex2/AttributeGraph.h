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
 * @file   AttributeGraph.h
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Stores dependencies between attributes in a program.
 */

#ifndef ATTRIBUTEGRAPH_H_
#define ATTRIBUTEGRAPH_H_

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/ID.h"

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>

#include <boost/multi_index/member.hpp>
#include <boost/multi_index/hashed_index.hpp>

DLVHEX_NAMESPACE_BEGIN

class AttributeGraph{
public:
	struct Attribute{
		enum Type{
			Ordinary, External
		};
		Type type;
		ID predicate;
		std::vector<ID> inputList;
		ID ruleID;
		bool input;
		int argIndex;

		bool operator==(const Attribute& at2) const;
	};

	typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, Attribute > Graph;
	typedef boost::graph_traits<Graph> Traits;

	typedef Graph::vertex_descriptor Node;
	typedef Graph::edge_descriptor Dependency;
	typedef Traits::vertex_iterator NodeIterator;
	typedef Traits::edge_iterator DependencyIterator;
	typedef Traits::out_edge_iterator PredecessorIterator;
	typedef Traits::in_edge_iterator SuccessorIterator;

private:
	RegistryPtr reg;
	const std::vector<ID>& idb;

	Graph ag;
	boost::unordered_map<ID, std::vector<Attribute> > attributesOfPredicate;

	struct NodeInfoTag {};
	struct NodeMappingInfo
	{
		Attribute at;
		Node node;
		NodeMappingInfo(Attribute at, Node node): at(at), node(node) {}
	};
	typedef boost::multi_index_container<
			NodeMappingInfo,
			boost::multi_index::indexed_by<
				boost::multi_index::hashed_unique<
					boost::multi_index::tag<NodeInfoTag>,
					BOOST_MULTI_INDEX_MEMBER(NodeMappingInfo,Attribute,at)
				>
			>
		> NodeMapping;
	NodeMapping nm;
	typedef NodeMapping::index<NodeInfoTag>::type NodeNodeInfoIndex;

	Attribute getAttribute(ID predicate, std::vector<ID> inputList, ID ruleID, bool inputAttribute, int argumentIndex);
	Attribute getAttribute(ID predicate, int argumentIndex);
	Node getNode(Attribute at);

	void createDependencies();
public:
	AttributeGraph(RegistryPtr reg, const std::vector<ID>& idb);

	// output graph as graphviz source
	virtual void writeGraphViz(std::ostream& o, bool verbose) const;
};

std::size_t hash_value(const AttributeGraph::Attribute& at);

DLVHEX_NAMESPACE_END

#endif

