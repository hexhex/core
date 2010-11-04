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
 * A component graph is a dependency graph with more capabilities,
 * required for building evaluation units.
 *
 * Capabilities are calculation of SCCs and other comfort data that is useful for
 * evaluation plan heuristics.
 */
class ComponentGraph:
  public DependencyGraph
{
  BOOST_CONCEPT_ASSERT((boost::Convertible<Node, unsigned int>));

  //////////////////////////////////////////////////////////////////////////////
  // types
  //////////////////////////////////////////////////////////////////////////////
public:
  // XXX use bidirectional map for this type?
  typedef std::vector<int> ComponentMap;
  typedef std::vector<Node> RepresentativeMap;
  typedef std::vector<std::set<Node> > SCCMap;
  typedef std::set<Node> RootContainer;
  typedef std::set<Node> LeafContainer;

  //////////////////////////////////////////////////////////////////////////////
  // members
  //////////////////////////////////////////////////////////////////////////////
protected:
  // map nodes to the ID of their SCC
  ComponentMap scc;
  // map nodes to one representative per SCC
  RepresentativeMap sccRepresentative;
  // map SCC indices to sets of nodes
  SCCMap sccMembers;

  // collect root nodes
  RootContainer roots;
  // collect leaf nodes
  LeafContainer leaves;

  //////////////////////////////////////////////////////////////////////////////
  // methods
  //////////////////////////////////////////////////////////////////////////////
public:
  // enforce
	ComponentGraph(RegistryPtr registry);
	virtual ~ComponentGraph();

  // calculate above members, based on underlying dependency graph
  void calculateComponentInfo();
  
  //
  // accessors
  //

  const ComponentMap& getSCC() const { return scc; }
  const RepresentativeMap& getSCCRepresentative() const { return sccRepresentative; }
  const SCCMap& getSCCMembers() const { return sccMembers; }
  const RootContainer& getRoots() const { return roots; }
  const LeafContainer& getLeaves() const { return leaves; }

protected:
  // calculate scc, sccRepresentative, sccMembers
  void calculateSCCs();
  // calculate roots, leaves
  void calculateSpecialNodeSets();

public:
  // add something to graphviz output
  virtual void writeGraphVizNodeLabel(std::ostream& o, Node n, bool verbose) const;
};

DLVHEX_NAMESPACE_END

#endif // DEPENDENCY_GRAPH_HPP_INCLUDED__18102010
