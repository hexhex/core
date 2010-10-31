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

/*
 * TODO update paper/formal stuff:
 * definition of unifying dependency as in roman's thesis, not as in eswc paper
 * rule nodes added to graph
 * constraints have extra types of dependencies
 * negative dependencies unclear in roman's thesis: def 4.6.2 after second point 2:
 * "or b is a non-monotonic external atom" should imho be "or b \in B(r) and b a non-monotonic external atom"
 * (doesn't this give us fixed-point vs guess-and-check model generator choice for free?)
 * we will add those negative dependencies from rule to body atoms only
 * add external dependency if constant input has a variable created by output of other extatom (not covered by roman's thesis)
 *
 * for eval only:
 * create auxiliary input collecting predicates/rule before creating depedency graph
 * pos dependency from ext. atom to its auxiliary input collecting predicate
 */

DLVHEX_NAMESPACE_BEGIN

struct Registry;
typedef boost::shared_ptr<Registry> RegistryPtr;

#if 0
// TODO move this into plugin interface
struct ExternalAtomMetaInformation
{
	bool isMonotonic;

	enum InputType {
		PREDICATE,
		CONSTANT
	};
	std::vector<InputType> inputTypes;
	unsigned outputArity;

	ExternalAtomMetaInformation():
		isMonotonic(false), inputTypes(), outputArity(0) { }
	ExternalAtomMetaInformation(bool isMonotonic, InputType i1, unsigned outputArity):
		isMonotonic(isMonotonic), inputTypes(), outputArity(outputArity)
		{ inputTypes.push_back(i1); }
	ExternalAtomMetaInformation(bool isMonotonic, InputType i1, InputType i2, unsigned outputArity):
		isMonotonic(isMonotonic), inputTypes(), outputArity(outputArity)
		{ inputTypes.push_back(i1); inputTypes.push_back(i2); }
	ExternalAtomMetaInformation(bool isMonotonic, const std::vector<InputType>& inputTypes, unsigned outputArity):
		isMonotonic(isMonotonic), inputTypes(inputTypes), outputArity(outputArity) {}
};

// TODO move this into plugin interface
class ExternalAtomInformationProvider
{
public:
	// given id of constant term indicating external atom name,
	// return whether it is monotonic or not
	virtual const ExternalAtomMetaInformation& getInformation(ID name) const = 0;
};
#endif

class DependencyGraph
{
  //////////////////////////////////////////////////////////////////////////////
  // types
  //////////////////////////////////////////////////////////////////////////////
public:
  struct NodeInfo:
    public ostream_printable<NodeInfo>
  {
		// ID storage:
		// store rule as rule
		// store literal or atom as atom (in non-naf-negated form)
    ID id;

		// property of atom IDs (unused for rules):
		// at least one of them must be true
		// both may be true
		// this is independent from naf (naf is expressed only in dependency info)
		bool inBody;
		bool inHead;
		NodeInfo(ID id, bool inBody=false, bool inHead=false): id(id), inBody(inBody), inHead(inHead) {}
    std::ostream& print(std::ostream& o) const;
  };

  struct DependencyInfo:
    public ostream_printable<DependencyInfo>
  {
    // rule -> body dependencies to NAF literals are negative (=false)
    // rule -> body dependencies to nonmonotonic external atoms may be both positive and negative
    bool positive;
    bool negative;

		// body -> head external dependency (predicate inputs only)
		// body -> same body external dependency (constant inputs that are variables only)
    bool external;

    // dependency involves rule body or does not involve rule body
    bool involvesRule;

    // if does not involve rule body: dependency is unifying or disjunctive or both
    bool disjunctive; // head <-> head in same rule
    bool unifying; // body -(depends on)-> head in different or same rules, or head <-> head in different rules

    // if does involve rule body: 
    // rule is constraint or not
    bool constraint;

		DependencyInfo():
    	positive(false),
			negative(false),
			external(false),
      involvesRule(false),
      disjunctive(false), unifying(false),
      constraint(false) {}
    std::ostream& print(std::ostream& o) const;
  };

	//TODO: find out which adjacency list is best suited for subgraph/filtergraph
	// for edge list we need setS because we don't want to have duplicate edges
	// TODO: perhaps we do want to have duplicate edges after all
  typedef boost::adjacency_list<
    boost::listS, boost::setS, boost::bidirectionalS,
    NodeInfo, DependencyInfo> Graph;
  typedef boost::graph_traits<Graph> Traits;

  typedef Graph::vertex_descriptor Node;
  typedef Graph::edge_descriptor Dependency;
  typedef Traits::vertex_iterator NodeIterator;
  typedef Traits::edge_iterator DependencyIterator;
  typedef Traits::out_edge_iterator PredecessorIterator;
  typedef Traits::in_edge_iterator SuccessorIterator;

private:
	struct IDTag {};
	struct NodeMappingInfo
	{
		ID id;
		Node node;
		NodeMappingInfo(): id(ID_FAIL) {}
		NodeMappingInfo(ID id, Node node): id(id), node(node) {}
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
  RegistryPtr registry;
  Graph dg;
	NodeMapping nm;

  //////////////////////////////////////////////////////////////////////////////
  // methods
  //////////////////////////////////////////////////////////////////////////////
public:
	DependencyGraph(RegistryPtr registry);
	~DependencyGraph();

  void createNodesAndBasicDependencies(const std::vector<ID>& idb);
  void createUnifyingDependencies();
  void createExternalDependencies();
  void createAggregateDependencies();


  // output graph as graphviz source
  void writeGraphViz(std::ostream& o, bool verbose) const;

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

	// get node/dependency properties
	inline const NodeInfo& propsOf(Node n) const
		{ return dg[n]; }
	inline NodeInfo& propsOf(Node n)
		{ return dg[n]; }
	inline const DependencyInfo& propsOf(Dependency d) const
		{ return dg[d]; }
	inline DependencyInfo& propsOf(Dependency d)
		{ return dg[d]; }

	// counting -> mainly for allocating and testing
  inline unsigned countNodes() const
		{ return boost::num_vertices(dg); }
  inline unsigned countDependencies() const
		{ return boost::num_edges(dg); }
};

DLVHEX_NAMESPACE_END

#endif // DEPENDENCY_GRAPH_HPP_INCLUDED__18102010
