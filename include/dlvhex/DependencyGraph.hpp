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
 * @file   DependencyGraph.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Dependency Graph interface.
 */

#ifndef DEPENDENCY_GRAPH_HPP_INCLUDED__18102010
#define DEPENDENCY_GRAPH_HPP_INCLUDED__18102010

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/Logger.hpp"
#include "dlvhex/ID.hpp"

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/shared_ptr.hpp>
//#include <boost/foreach.hpp>

#include <boost/multi_index/member.hpp>
#include <boost/multi_index/hashed_index.hpp>

//#include <cassert>

DLVHEX_NAMESPACE_BEGIN

struct Registry;
typedef boost::shared_ptr<Registry> RegistryPtr;

class DependencyGraph
{
  //////////////////////////////////////////////////////////////////////////////
  // types
  //////////////////////////////////////////////////////////////////////////////
public:
  struct NodeInfo
  {
    ID id;
  };

  struct DependencyInfo
  {
    // dependency head/body
    bool positive;
    bool negative;
    // head/head
    bool disjunctive;
    // external atoms
    bool external;
    // body/head for constraints
    bool positive_constraint;
    bool negative_constraint;

    // unification dependency (additional flag)
    bool unifying;
  };

	//TODO: find out which adjacency list is best suited for subgraph/filtergraph
  typedef boost::adjacency_list<
    boost::listS, boost::listS, boost::bidirectionalS,
    NodeInfo, DependencyInfo> Graph;
  typedef boost::graph_traits<Graph> Traits;

  typedef Graph::vertex_descriptor Node;
  typedef Graph::edge_descriptor Dependency;
  typedef Traits::out_edge_iterator PredecessorIterator;
  typedef Traits::in_edge_iterator SuccessorIterator;

private:
	struct IDTag {};
	struct NodeMappingInfo
	{
		ID id;
		Node node;
	};
	typedef boost::multi_index_container<
			NodeMappingInfo,
			boost::multi_index::indexed_by<
				boost::multi_index::hashed_unique<
					boost::multi_index::tag<IDTag>,
					BOOST_MULTI_INDEX_MEMBER(NodeMappingInfo,ID,id)
				>
			>
		> NodeMapping;
  typedef NodeMapping::index<IDTag>::type NodeIDIndex;

  //////////////////////////////////////////////////////////////////////////////
  // members
  //////////////////////////////////////////////////////////////////////////////
private:
  Graph dg;
	NodeMapping nm;

  //////////////////////////////////////////////////////////////////////////////
  // methods
  //////////////////////////////////////////////////////////////////////////////
public:
	DependencyGraph(RegistryPtr registry, const std::vector<ID>& idb);
	~DependencyGraph();

	// get node given some object id
	inline Node getNode(ID id) const
		{
			const NodeIDIndex& idx = nm.get<IDTag>();
			NodeIDIndex::const_iterator it = idx.find(id);
			assert(it != idx.end());
			return it->node;
		}

	// get node info given node
	inline const NodeInfo& getNodeInfo(Node node) const
		{ return dg[node]; }

	// get dependency info given dependency
	inline const DependencyInfo& getDependencyInfo(Dependency dep) const
		{ return dg[dep]; }

	// get dependencies (to predecessors) = arcs from this node to others
  inline std::pair<PredecessorIterator, PredecessorIterator>
  getDependencies(Node node) const
		{ return boost::out_edges(node, dg); }

	// get provides (dependencies to successors) = arcs from other nodes to this one
  inline std::pair<SuccessorIterator, SuccessorIterator>
  getProvides(Node node) const
		{ return boost::in_edges(node, dg); }

	// get source of dependency = node that depends
  inline Node sourceOf(Dependency d) const
		{ return boost::source(d, dg); }

	// get target of dependency = node upon which the source depends
  inline Node targetOf(Dependency d) const
		{ return boost::target(d, dg); }
};

DLVHEX_NAMESPACE_END

#endif // DEPENDENCY_GRAPH_HPP_INCLUDED__18102010
