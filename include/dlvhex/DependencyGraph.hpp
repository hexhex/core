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
 * auxiliary rules do not take all body literals with the external atom's input variable, they only take positive literals!
 *   (this is stated differently in the thesis. perhaps it would be more efficient to take the
 *   transitive closure of dependencies over variables to make the body larger (and therefore reduce the grounding)
 *   example: &foo[X](), bar(X,Y), not baz(X,Z), boo(Z) -> is it more efficient to take all body atoms instead of only bar(X,Y)?)
 *
 * for eval only:
 * create auxiliary input collecting predicates/rule before creating depedency graph
 * pos dependency from ext. atom to its auxiliary input collecting predicate
 */

DLVHEX_NAMESPACE_BEGIN

// some forwards
struct Rule;
struct ExternalAtom;
struct OrdinaryAtom;
struct Registry;
typedef boost::shared_ptr<Registry> RegistryPtr;
class PluginAtom;
typedef boost::shared_ptr<PluginAtom> PluginAtomPtr;

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
		// store external atom body literal as atom (in non-naf-negated form)
    // store nothing else as node
    ID id;

		NodeInfo(ID id=ID_FAIL): id(id) {}
    std::ostream& print(std::ostream& o) const;
  };

  struct DependencyInfo:
    public ostream_printable<DependencyInfo>
  {
    // the following dependencies are stored in this graph:
    //
    // * dependency A -> B where A is a regular rule and B is a regular rule:
    //   * one of A's positive body ordinary atom literals
    //     unifies with one of B's head atoms -> "positiveRegularRule"
    //   * one of A's negative body ordinary atom literals
    //     unifies with one of B's head atoms -> "negativeRule"
    //   * one of A's head atoms unifies with one of B's head atoms
    //     -> "unifyingHead"
    //     if A or B has a disjunctive head -> "disjunctive"
    // * dependency A -> B where A is a constraint and B is a regular rule:
    //   * one of A's positive body ordinary atom literals
    //     unifies with one of B's head atoms -> "positiveConstraint"
    //   * one of A's negative body ordinary atom literals
    //     unifies with one of B's head atoms -> "negativeRule"
    // * dependency A -> X where A is a rule and X is an external atom:
    //   * X is present in the positive body of A and X is monotonic
    //     -> "positiveExternal"
    //   * X is present in the positive body of A and X is nonmonotonic
    //     -> "positiveExternal" and "negativeExternal"
    //   * X is present in the negative body of A and X is monotonic
    //     -> "negativeExternal"
    //   * X is present in the negative body of A and X is nonmonotonic
    //     -> "positiveExternal" and "negativeExternal"
    // * dependency X -> A where X is an external atom and A is a rule:
    //   * A is the auxiliary input rule providing input for X in rule/constraint B
    //     -> "externalConstantInput"
    //   * a predicate input of X matches one head of rule A
    //     -> "externalPredicateInput"
    bool positiveRegularRule;
    bool positiveConstraint;
    bool negativeRule;
    bool unifyingHead;
    bool disjunctive;
    bool positiveExternal;
    bool negativeExternal;
    bool externalConstantInput;
    bool externalPredicateInput;

		DependencyInfo():
    	positiveRegularRule(false),
      positiveConstraint(false),
			negativeRule(false),
			unifyingHead(false),
			disjunctive(false),
      positiveExternal(false),
      negativeExternal(false),
      externalConstantInput(false),
      externalPredicateInput(false)
      {}
		const DependencyInfo& operator|=(const DependencyInfo& other);
    std::ostream& print(std::ostream& o) const;
  };

  // for out-edge list we use vecS so we may have duplicate edges which is not a
  // problem (at least not for the SCC algorithm, for drawing the graph we must take
  // care a bit, but drawing a graph need not be efficient)
  //
  // for vertices it is necesssary to use vecS because so many nice algorithms need
  // implicit vertex_index
  //
  // TODO: do we need bidirectional? (at the moment yes, to find roots and leaves)
  typedef boost::adjacency_list<
    boost::vecS, boost::vecS, boost::bidirectionalS,
    NodeInfo, DependencyInfo> Graph;
  typedef boost::graph_traits<Graph> Traits;

  typedef Graph::vertex_descriptor Node;
  typedef Graph::edge_descriptor Dependency;
  typedef Traits::vertex_iterator NodeIterator;
  typedef Traits::edge_iterator DependencyIterator;
  typedef Traits::out_edge_iterator PredecessorIterator;
  typedef Traits::in_edge_iterator SuccessorIterator;

protected:
  // the node mapping maps IDs of external atoms and rules
  // to nodes of the dependency graph
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

protected:
  typedef std::vector<Node> NodeList;
  struct HeadBodyInfo
  {
    // ordinary ground or nonground atom id
    ID id;
    bool inHead;
    bool inBody;
    NodeList inHeadOfNondisjunctiveRules;
    NodeList inHeadOfDisjunctiveRules;
    NodeList inPosBodyOfRegularRules; // only non-constraint rules
    NodeList inPosBodyOfConstraints;
    NodeList inNegBodyOfRules; // any rules
    ID headPredicate; // constant term, only for inHead
    const OrdinaryAtom* oatom;

    HeadBodyInfo(const OrdinaryAtom* oatom = NULL):
      id(ID_FAIL), inHead(false), inBody(false),
      headPredicate(ID_FAIL), oatom(oatom) {}
  };

  struct InHeadTag {};
  struct InBodyTag {};
  struct HeadPredicateTag {};
  struct HeadBodyHelper
  {
    typedef boost::multi_index_container<
        HeadBodyInfo,
        boost::multi_index::indexed_by<
          boost::multi_index::hashed_unique<
            boost::multi_index::tag<IDTag>,
            BOOST_MULTI_INDEX_MEMBER(HeadBodyInfo,ID,id)
          >,
          boost::multi_index::hashed_non_unique<
            boost::multi_index::tag<InHeadTag>,
            BOOST_MULTI_INDEX_MEMBER(HeadBodyInfo,bool,inHead)
          >,
          boost::multi_index::hashed_non_unique<
            boost::multi_index::tag<InBodyTag>,
            BOOST_MULTI_INDEX_MEMBER(HeadBodyInfo,bool,inBody)
          >,
          boost::multi_index::hashed_non_unique<
            boost::multi_index::tag<HeadPredicateTag>,
            BOOST_MULTI_INDEX_MEMBER(HeadBodyInfo,ID,headPredicate)
          >
        >
      > HBInfos;
    typedef HBInfos::index<IDTag>::type IDIndex;
    typedef HBInfos::index<InHeadTag>::type InHeadIndex;
    typedef HBInfos::index<InBodyTag>::type InBodyIndex;
    typedef HBInfos::index<HeadPredicateTag>::type HeadPredicateIndex;

    HBInfos infos;
  };

  //////////////////////////////////////////////////////////////////////////////
  // members
  //////////////////////////////////////////////////////////////////////////////
protected:
  RegistryPtr registry;
  Graph dg;
	NodeMapping nm;

  //////////////////////////////////////////////////////////////////////////////
  // methods
  //////////////////////////////////////////////////////////////////////////////
private:
  // not implemented on purpose because forbidden to use
	DependencyGraph(const Dependency& other);
public:
	DependencyGraph(RegistryPtr registry);
	virtual ~DependencyGraph();

  // this method creates all dependencies
  void createDependencies(const std::vector<ID>& idb, std::vector<ID>& createdAuxRules);

  // output graph as graphviz source
  virtual void writeGraphViz(std::ostream& o, bool verbose) const;

  const Graph& getInternalGraph() const
    { return dg; }

	// get node given some object id
	inline Node getNode(ID id) const
		{
			const NodeIDIndex& idx = nm.get<IDTag>();
			NodeIDIndex::const_iterator it = idx.find(id);
			assert(it != idx.end());
			return it->node;
		}

  // get range over all nodes
  inline std::pair<NodeIterator, NodeIterator> getNodes() const
    { return boost::vertices(dg); }

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

protected:
  // create node, update mapping
  inline Node createNode(ID id);
  
protected:
  // create nodes for rules and external atoms
  // create "positiveExternal" and "negativeExternal" dependencies
  // create "externalConstantInput" dependencies and auxiliary rules
  // fill HeadBodyHelper (required for efficient unification)
  void createNodesAndIntraRuleDependencies(
      const std::vector<ID>& idb,
      std::vector<ID>& createdAuxRules,
      HeadBodyHelper& hbh);
    // helpers
    void createNodesAndIntraRuleDependenciesForRule(
        ID idrule,
        std::vector<ID>& createdAuxRules,
        HeadBodyHelper& hbh);
    void createAuxiliaryRuleIfRequired(
        ID idrule, Node nrule, const Rule& rule,
        ID idlit, ID idat, Node neatom, const ExternalAtom& eatom,
        const PluginAtomPtr& pluginAtom,
        std::vector<ID>& createdAuxRules,
        HeadBodyHelper& hbh);
    // create auxiliary rule head predicate (in registry) and return ID
    ID createAuxiliaryRuleHeadPredicate(ID forRule, ID forEAtom);
    // create auxiliary rule head (in registry) and return ID
    ID createAuxiliaryRuleHead(ID idauxpred, const std::list<ID>& variables);
    // create auxiliary rule (in registry) and return ID
    ID createAuxiliaryRule(ID head, const std::list<ID>& body);

	// create "externalPredicateInput" dependencies
  void createExternalPredicateInputDependencies(const HeadBodyHelper& hbh);
    // helpers
    void createExternalPredicateInputDependenciesForInput(
        const NodeMappingInfo& ni_eatom, ID predicate, const HeadBodyHelper& hbh);

  // build all unifying dependencies ("{positive,negative}{Rule,Constraint}", "unifyingHead")
  void createUnifyingDependencies(const HeadBodyHelper& hbh);
    // helpers
    // "unifyingHead" dependencies
    void createHeadHeadUnifyingDependencies(const HeadBodyHelper& hbh);
    // "{positive,negative}{Rule,Constraint}" dependencies
    void createHeadBodyUnifyingDependencies(const HeadBodyHelper& hbh);

protected:
  // helpers for writeGraphViz: extend for more output
  virtual void writeGraphVizNodeLabel(std::ostream& o, Node n, bool verbose) const;
  virtual void writeGraphVizDependencyLabel(std::ostream& o, Dependency dep, bool verbose) const;
};

DependencyGraph::Node DependencyGraph::createNode(ID id)
{
  DBGLOG(DBG,"creating node for ID " << id);
  Node n = boost::add_vertex(NodeInfo(id), dg);
  {
    NodeIDIndex::const_iterator it;
    bool success;
    boost::tie(it, success) = nm.insert(NodeMappingInfo(id, n));
    assert(success);
  }
  return n;
}

DLVHEX_NAMESPACE_END

#endif // DEPENDENCY_GRAPH_HPP_INCLUDED__18102010
