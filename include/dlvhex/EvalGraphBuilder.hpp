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
 * @file   EvalGraphBuilder.hpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Framework for heuristics to build an EvalGraph from a ComponentGraph.
 */

#ifndef EVAL_GRAPH_BUILDER_HPP_INCLUDED__03112010
#define EVAL_GRAPH_BUILDER_HPP_INCLUDED__03112010

#include "dlvhex/FinalEvalGraph.hpp"
#include "dlvhex/ComponentGraph.hpp"
#include "dlvhex/Logger.hpp"

#include <boost/range/iterator_range.hpp>
#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/bimap/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/bimap/unordered_multiset_of.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
 * This template provides a framework for building an evaluation graph. It provides
 * one modifier method createEvalUnit() for creating an evaluation unit; this method
 * does all necessary checks.
 *
 * All evaluation planning heuristics must use this builder for creating evaluation
 * units and evaluation graphs.
 */
//template<typename EvalGraphT>
// TODO make this a template for EvalGraphT and ComponentGraphT, for faster prototyping we use fixed types for these graphs
class EvalGraphBuilder
{
  //////////////////////////////////////////////////////////////////////////////
  // types
  //////////////////////////////////////////////////////////////////////////////
public:
	typedef FinalEvalGraph EvalGraphT;
	typedef EvalGraphT::EvalUnit EvalUnit;

  #if 0
//  BOOST_CONCEPT_ASSERT((boost::Convertible<ComponentGraph::Node, unsigned>));
//  BOOST_CONCEPT_ASSERT((boost::Convertible<typename EvalGraphT::EvalUnit, unsigned>));


protected:
  // bidirectional mapping:
  // set of components -> one eval unit
  // set of components <- one eval unit
  // we use identity as hash function as nodes and eval units are distinct unsigned ints
  struct identity {
    inline unsigned operator()(unsigned u) { return u; }
  };
  typedef boost::bimaps::bimap<
      boost::bimaps::unordered_set_of<ComponentGraph::Node, identity >,
      boost::bimaps::unordered_multiset_of<typename EvalGraph::EvalUnit, identity >
    > NodeEvalUnitMapping;

  // for subgraph of component graph that still needs to be put into eval units:
  //
  // we cannot use subgraph to keep track of the rest of the component graph,
  // because subgraph does not allow for removing vertices and is furthermore
  // broken for bundled properties (see boost bugtracker)
  //
  // therefore we keep track of used components in an efficient way in the
  // following vector and filter the graph using boost::filtered_graph
  // (components are unsigned ints as verified by the following concept check)
  typedef std::vector<bool> UnusedNodesMap;
  struct UnusedVertexFilter
  {
    // unfortunately, this predicate must be default constructible
    UnusedVertexFilter(): ucmap(0) {}
    UnusedVertexFilter(const UnusedNodesMap* const ucmap): ucmap(ucmap) {}
    bool operator()(ComponentGraph::Node node) const { assert(ucmap); return (*ucmap)[static_cast<unsigned>(node)]; }
    const UnusedNodesMap* const ucmap;
  };
  struct UnusedEdgeFilter
  {
    // unfortunately, this predicate must be default constructible
    UnusedEdgeFilter(): cg(0), ucmap(0) {}
    UnusedEdgeFilter(const ComponentGraph* const cg, const UnusedNodesMap* const ucmap): cg(cg), ucmap(ucmap) {}
    bool operator()(ComponentGraph::Dependency dep) const;
    const ComponentGraph* const cg;
    const UnusedNodesMap* const ucmap;
  };
  typedef boost::filtered_graph<ComponentGraph::Graph,
          UnusedEdgeFilter, UnusedVertexFilter> ComponentGraphRest;
  #endif

  //////////////////////////////////////////////////////////////////////////////
  // members
  //////////////////////////////////////////////////////////////////////////////
protected:
	// component graph (this is an input -> const)
	ComponentGraph& cg;
	// eval graph
	EvalGraphT& eg;

  //NodeEvalUnitMapping mapping;

  #if 0
  //
  // subgraph of component graph that still needs to be put into eval units
  //
  // (see comment above)
  UnusedNodesMap unusedNodes; // true if unused
  UnusedEdgeFilter unusedEdgeFilter;
  UnusedVertexFilter unusedVertexFilter;
  // induced subgraph of cg, induced by unused nodes
  // for each change in the eval graph,
  // usedNodes will be updated
  //
  // the sources of boost::filtered_graph suggest that after an update to
  // usedNodes, iterators of cgrest should not be reused, but the graph
  // need not be reconstructed
  ComponentGraphRest cgrest;
  // leaves of cgrest
  ComponentGraph::LeafContainer cgrestLeaves;
  #endif

#if 0
  //
  // supporting information on cgrest/eg
  // XXX: perhaps move this into derived class
  //
  // this information is invalidated (=marked invalid) by createEvalUnit
  // and recalculated on demand by accessors to supporting information.
  //
  // the reason for this is, that calculating this information is costly,
  // and it might be useful to create multiple evaluation units with given
  // information
  //

  // need recalculation?
  bool supInvalid;
  // TODO: 
#endif

  //////////////////////////////////////////////////////////////////////////////
  // methods
  //////////////////////////////////////////////////////////////////////////////
public:
	EvalGraphBuilder(ComponentGraph& cg, EvalGraphT& eg);
	virtual ~EvalGraphBuilder();

  //
  // accessors
  // 
  inline const EvalGraphT& getEvaGraph() const { return eg; }
  inline ComponentGraph& getComponentGraph() { return cg; }
  inline const ComponentGraph& getComponentGraph() const { return cg; }
  //inline const ComponentGraph::LeafContainer& getRestLeaves() const { return cgrestLeaves; }

  //
  // modifiers
  //

	// this methods modifies the eval graph and invalidates the helping information
  // provided by this builder:
  //
  // it asserts that all requirements for evaluation units are fulfilled
  // it adds an evaluation unit created from given nodes, including dependencies
  //
	// NodeRange = range over nodes of component graph
	// (you can use a container as a range, or use 
	//  boost::iterator_range<Container>(first, beyond_last) )
  //
	// UnitRange = ordered range over eval units of the eval graph,
  // which will be used as dependencies of the new eval unit
	// (you can use a container as a range, or use 
	//  boost::iterator_range<Container>(first, beyond_last) )
  //
  // returns newly created eval unit
  // TODO: for constraint sharing distinguish between "consumedNodes" and "sharedNodes"
	template<typename NodeRange, typename UnitRange>
	virtual EvalUnit createEvalUnit(
      NodeRange nodes, UnitRange orderedDependencies);
#if 0
protected:
  inline void ensureSup()
    { if( supInvalid ) recalculateSupportingInformation(); }
  virtual void recalculateSupportingInformation();
#endif
};

DLVHEX_NAMESPACE_END

#endif // EVAL_GRAPH_BUILDER_HPP_INCLUDED__03112010
