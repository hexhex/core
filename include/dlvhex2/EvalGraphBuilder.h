/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * Copyright (C) 2011, 2012, 2013 Christoph Redl
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
 * @file   EvalGraphBuilder.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Framework for heuristics to build an EvalGraph from a ComponentGraph.
 */

#ifndef EVAL_GRAPH_BUILDER_HPP_INCLUDED__03112010
#define EVAL_GRAPH_BUILDER_HPP_INCLUDED__03112010

#include "dlvhex2/FinalEvalGraph.h"
#include "dlvhex2/ComponentGraph.h"
#include "dlvhex2/ASPSolverManager.h"
#include "dlvhex2/Logger.h"

#include <boost/range/iterator_range.hpp>
#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/bimap/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/bimap/unordered_multiset_of.hpp>
#include <boost/scoped_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

class ProgramCtx;

/** \brief This template provides a framework for building an evaluation graph.
 *
 * It provides
 * one modifier method createEvalUnit() for creating an evaluation unit; this method
 * does all necessary checks.
 *
 * All evaluation planning heuristics must use this builder for creating evaluation
 * units and evaluation graphs.
 */
//template<typename EvalGraphT>
// TODO make this a template for EvalGraphT and ComponentGraphT, for faster prototyping we use fixed types for these graphs
class DLVHEX_EXPORT EvalGraphBuilder
{
  //////////////////////////////////////////////////////////////////////////////
  // types
  //////////////////////////////////////////////////////////////////////////////
public:
	typedef FinalEvalGraph EvalGraphT;
	typedef EvalGraphT::EvalUnit EvalUnit;
  typedef ComponentGraph::Component Component;
  typedef ComponentGraph::Dependency Dependency;

protected:
  // we use identity as hash function as eval units are distinct unsigned ints

  BOOST_CONCEPT_ASSERT((boost::Convertible<Component, void*>));
  BOOST_CONCEPT_ASSERT((boost::Convertible<EvalUnit, unsigned>));

  /** \brief Identity function. */
  struct identity {
    /** @param u.
      * @return \p u. */
    inline unsigned operator()(unsigned u) const { return u; }
  };

  // bidirectional mapping:
  // set of components -> one eval unit
  // set of components <- one eval unit
  // constraint components that have been pushed up are ignored here
	// (nothing can depend on them, and they are not "used" until all their
	// dependencies have been fulfilled)
  typedef boost::bimaps::bimap<
      boost::bimaps::unordered_set_of<Component>,
      boost::bimaps::unordered_set_of<EvalUnit>
    > ComponentEvalUnitMapping;

protected:
  /** \brief For subgraph of component graph that still needs to be put into eval units.
    *
    * We cannot use subgraph to keep track of the rest of the component graph,
    * because subgraph does not allow for removing vertices and is furthermore
    * broken for bundled properties (see boost bugtracker).
    *
    * Therefore we use the mapping to keep track of used components
    * and we filter the graph using boost::filtered_graph
    * (components are unsigned ints as verified by the following concept check). */
  struct UnusedVertexFilter
  {
    // unfortunately, this predicate must be default constructible
    /** \brief Constructor. */
    UnusedVertexFilter(): ceum(0) {}
    /** \brief Constructor.
      * @param ceum See UnusedVertexFilter::ceum. */
    UnusedVertexFilter(const ComponentEvalUnitMapping* ceum): ceum(ceum) {}
    /** \brief Copy-constructor.
      * @param other Second UnusedVertexFilter. */
    UnusedVertexFilter(const UnusedVertexFilter& other): ceum(other.ceum) {}
		UnusedVertexFilter& operator=(const UnusedVertexFilter& other)
			{ ceum = other.ceum; return *this; }
    /* Execution.
     * @return True if vertex is still in graph -> return true if not mapped yet. */
    bool operator()(Component comp) const
			{ assert(ceum); return ceum->left.find(comp) == ceum->left.end(); }
    /** \brief Assigns evaluation units to components. */
    const ComponentEvalUnitMapping* ceum;
  };
  /** \brief Edges of component graph that still need to be put into eval units. */
  struct UnusedEdgeFilter
  {
    // unfortunately, this predicate must be default constructible
    /** \brief Constructor. */
    UnusedEdgeFilter(): cg(0), ceum(0) {}
    /** \brief Constructor.
      * @param cg ComponentGraph.
      * @param ceum See UnusedEdgeFilter::ceum. */
    UnusedEdgeFilter(const ComponentGraph* const cg,
				const ComponentEvalUnitMapping* const ceum): cg(cg), ceum(ceum) {}
    /** \brief Copy-constructor.
      * @param other Second UnusedEdgeFilter. */
    UnusedEdgeFilter(const UnusedEdgeFilter& other): cg(other.cg), ceum(other.ceum) {}
		UnusedEdgeFilter& operator=(const UnusedEdgeFilter& other)
			{ cg = other.cg; ceum = other.ceum; return *this; }
    /* Execution.
     * @return True if edge is still in graph -> return true if not mapped yet. */
    bool operator()(Dependency dep) const
			{ assert(cg && ceum);
				return
					(ceum->left.find(cg->targetOf(dep)) == ceum->left.end()) &&
					(ceum->left.find(cg->sourceOf(dep)) == ceum->left.end()); }
    /** \brief ComponentGraph to process. */
    const ComponentGraph* cg;
    /** \brief Assigns evaluation units to components. */
    const ComponentEvalUnitMapping* ceum;
  };

public:
  typedef boost::filtered_graph<ComponentGraph::Graph,
          UnusedEdgeFilter, UnusedVertexFilter> ComponentGraphRest;

  //////////////////////////////////////////////////////////////////////////////
  // members
  //////////////////////////////////////////////////////////////////////////////
protected:
  /** \brief Overall program context. */
  ProgramCtx& ctx;
  /** \brief Component graph (we clone it and store it here in the constructor). */
  boost::scoped_ptr<ComponentGraph> clonedcgptr;
  /** \brief Component graph (reference to cloned storage). */
  ComponentGraph& cg;
	/** \brief Eval graph. */
	EvalGraphT& eg;
  /** \brief Configuration for model generator factory. */
  ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig;

	/** \brief Mapping of nonshared components to eval units. */
  ComponentEvalUnitMapping mapping;

  /** \brief Subgraph of component graph that still needs to be put into eval units.
   *
   * See comment UnusedVertexFilter. */
  UnusedEdgeFilter unusedEdgeFilter;
  /** \brief Edges of component graph that still need to be put into eval units.
   *
   * See comment UnusedEdgeFilter. */
  UnusedVertexFilter unusedVertexFilter;
  /** \brief Induced subgraph of cg:
    * * Nodes not in mapping are part of this graph
    * * Edges where both nodes are not in mapping are part of this graph.
    *
    * The sources of boost::filtered_graph suggest that after an update to
    * usedNodes, iterators of cgrest should not be reused, but the graph
    * need not be reconstructed. */
  ComponentGraphRest cgrest;

  //////////////////////////////////////////////////////////////////////////////
  // methods
  //////////////////////////////////////////////////////////////////////////////
public:
	/** \brief Constructor.
	  * @param ctx See EvalGraphBuilder::ctx.
	  * @param cg See EvalGraphBuilder::cg.
	  * @param eg Evaluation graph to write the result to.
	  * @param externalEvalConfig See ASPSolverManager::SoftwareConfiguration. */
	EvalGraphBuilder(
      ProgramCtx& ctx, ComponentGraph& cg, EvalGraphT& eg,
      ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig);
	/** \brief Destructor. */
	virtual ~EvalGraphBuilder();

  //
  // accessors
  // 
  /** \brief Retrieve internal evaluation graph.
    * @return Evaluation graph. */
  inline const EvalGraphT& getEvalGraph() const { return eg; }
  /** \brief Retrieve internal component graph.
    * @return Component graph. */
  inline ComponentGraph& getComponentGraph() { return cg; }
  /** \brief Returns a graph consisting of all components that still need to be built into some evaluation unit.
    * @return Remaining components. */
  inline const ComponentGraphRest& getComponentGraphRest() const { return cgrest; }
  /** \brief Get component corresponding to given unit (previously generated using createEvalUnit).
    * @param u Evaluation unit.
    * @retrn \p u as Component. */
  Component getComponentForUnit(EvalUnit u) const;

	/** \brief Returns the registry (useful for printing, cannot do this inline as ProgramCtx depends on this header).
	  * @return Registry. */
  RegistryPtr registry();

	/** \brief Returns the ProgramCtx.
	  * @return ProgramCtx. */
  inline ProgramCtx& getProgramCtx(){ return ctx; }

  //
  // modifiers
  //

//  NodeRange nodes, UnitRange orderedDependencies);
  /** \brief This methods modifies the eval graph.
    *
    * It asserts that all requirements for evaluation units are fulfilled
    * it adds an evaluation unit created from given nodes, including dependencies.
    *
    * @todo add ordered unit dependencies
    * @todo ComponentRange
    * @param comps List of components to directly put into eval unit.
    * @param ccomps List of components to copy into eval unit
    * (these copied components may only contain constraints, and these must obey the constraint pushing restrictions (this will be asserted by createEvalUnit)).
    * @return Evaluation unit. */
  virtual EvalUnit createEvalUnit(
			const std::list<Component>& comps, const std::list<Component>& ccomps);
//template<typename NodeRange, typename UnitRange>
//virtual EvalUnit createEvalUnit(
};
typedef boost::shared_ptr<EvalGraphBuilder> EvalGraphBuilderPtr;

DLVHEX_NAMESPACE_END

#endif // EVAL_GRAPH_BUILDER_HPP_INCLUDED__03112010
