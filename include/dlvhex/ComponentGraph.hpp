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
 * @file   ComponentGraph.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Component Graph interface.
 */

#ifndef COMPONENT_GRAPH_HPP_INCLUDED__18102010
#define COMPONENT_GRAPH_HPP_INCLUDED__18102010

#include "dlvhex/DependencyGraph.hpp"

#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
 * A component graph is created from a dependency graph by collapsing all rule
 * nodes with their body and head nodes (except for external atoms),
 * and then by collapsing all rules in the same SCC (including external atoms).
 * Dependencies are collapsed as well.
 *
 * A component graph is a dag (acyclic by the above construction).
 *
 * Vertices (= components) store a set of rules and information about the dependencies
 * within the collapsed part of the dependency graph. These properties are calculated
 * by calculateCollapsedComponentProperties(...).
 *
 * Edges (= collapsed dependencies) store information about the collapsed
 * dependencies. These are calculated by calculateCollapsedDependencyProperties(...).
 */
class ComponentGraph
{
//  BOOST_CONCEPT_ASSERT((boost::Convertible<DependencyGraph::Node, unsigned int>));

  //////////////////////////////////////////////////////////////////////////////
  // types
  //////////////////////////////////////////////////////////////////////////////
public:
  struct ComponentInfo:
    public ostream_printable<ComponentInfo>
  {
    #ifndef NDEBUG
    std::set<DependencyGraph::Node> sources;
    #endif

		// ID storage:
		// store IDs of rules in component
    std::set<ID> rules;

    // store IDs of external atoms in component
    std::set<ID> eatoms;

    // TODO:
    // whether it contains a positive cycle of dependencies over a monotonic external atom (-> fixedpoint)
    // whether it contains a negative cycle of dependencies over a monotonic external atom (-> guess and check)
    // whether it contains any cycle of dependencies over a nonmonotonic external atom (-> guess and check)
		//bool posCycleMonotonicAtom;
    // ...

		ComponentInfo(): rules(), eatoms() {}
    std::ostream& print(std::ostream& o) const;
  };

  struct DependencyInfo:
    public ostream_printable<DependencyInfo>
  {
    #ifndef NDEBUG
    std::set<DependencyGraph::Dependency> sources;
    #endif

    // All those can be independently true:

    // whether it contains a positive rule dependency
    bool positiveRule;
    // whether it contains a negative rule dependency
    bool negativeRule;
    // whether it contains a positive constraint dependency
    bool positiveConstraint;
    // whether it contains a negative constraint dependency
    bool negativeConstraint;
    // whether it contains an external dependency
    bool external;

		DependencyInfo():
    	positiveRule(false),
			negativeRule(false),
    	positiveConstraint(false),
			negativeConstraint(false),
			external(false) {}
    std::ostream& print(std::ostream& o) const;
  };

  // we need listS because this graph will be changed a lot by collapsing nodes
  // TODO: perhaps for out-edges (first listS) we could risk vecS?
  typedef boost::adjacency_list<
    boost::listS, boost::listS, boost::bidirectionalS,
    ComponentInfo, DependencyInfo> Graph;
  typedef boost::graph_traits<Graph> Traits;

  typedef Graph::vertex_descriptor Component;
  typedef Graph::edge_descriptor Dependency;
  typedef Traits::vertex_iterator ComponentIterator;
  typedef Traits::edge_iterator DependencyIterator;
  typedef Traits::out_edge_iterator PredecessorIterator;
  typedef Traits::in_edge_iterator SuccessorIterator;

  //////////////////////////////////////////////////////////////////////////////
  // members
  //////////////////////////////////////////////////////////////////////////////
protected:
  // for debugging and printing
  RegistryPtr reg;
  #ifndef NDEBUG
  // in non-debug mode this graph's lifetime can end
  // after the constructor finished
  const DependencyGraph& dg;
  #endif
  Graph cg;

  //////////////////////////////////////////////////////////////////////////////
  // methods
  //////////////////////////////////////////////////////////////////////////////
public:
	ComponentGraph(const DependencyGraph& dg, RegistryPtr reg);
	virtual ~ComponentGraph();

  // output graph as graphviz source
  virtual void writeGraphViz(std::ostream& o, bool verbose) const;

  // get range over all components
  inline std::pair<ComponentIterator, ComponentIterator> getComponents() const
    { return boost::vertices(cg); }

	// get node info given node
	inline const ComponentInfo& getComponentInfo(Component c) const
		{ return cg[c]; }

	// get dependency info given dependency
	inline const DependencyInfo& getDependencyInfo(Dependency dep) const
		{ return cg[dep]; }

	// get dependencies (to predecessors) = arcs from this component to others
  inline std::pair<PredecessorIterator, PredecessorIterator>
  getDependencies(Component c) const
		{ return boost::out_edges(c, cg); }

	// get provides (dependencies to successors) = arcs from other component to this one
  inline std::pair<SuccessorIterator, SuccessorIterator>
  getProvides(Component c) const
		{ return boost::in_edges(c, cg); }

	// get source of dependency = component that depends
  inline Component sourceOf(Dependency d) const
		{ return boost::source(d, cg); }

	// get target of dependency = component upon which the source depends
  inline Component targetOf(Dependency d) const
		{ return boost::target(d, cg); }

	// get node/dependency properties
	inline const ComponentInfo& propsOf(Component c) const
		{ return cg[c]; }
	inline ComponentInfo& propsOf(Component c)
		{ return cg[c]; }
	inline const DependencyInfo& propsOf(Dependency d) const
		{ return cg[d]; }
	inline DependencyInfo& propsOf(Dependency d)
		{ return cg[d]; }

	// counting -> mainly for allocating and testing
  inline unsigned countComponents() const
		{ return boost::num_vertices(cg); }
  inline unsigned countDependencies() const
		{ return boost::num_edges(cg); }

protected:
  // helpers for writeGraphViz: extend for more output
  virtual void writeGraphVizComponentLabel(std::ostream& o, Component c, bool verbose) const;
  virtual void writeGraphVizDependencyLabel(std::ostream& o, Dependency dep, bool verbose) const;

  // helper for constructor
  void calculateComponents(const DependencyGraph& dg);
  void collapseComponent(Component c);
  // calculate ComponentInfo from dependencies within a collapsed part of the dependency graph.
  void calculateCollapsedComponentProperties(
    const std::set<DependencyGraph::Node>& sourceNodes,
    ComponentInfo& ci) const;
  // Calculate DependencyInfo from collapsed dependencies.
  void calculateCollapsedDependencyProperties(
    const std::set<DependencyGraph::Dependency>& sourceDependencies,
    DependencyInfo& di) const;
};

DLVHEX_NAMESPACE_END

#endif // DEPENDENCY_GRAPH_HPP_INCLUDED__18102010
