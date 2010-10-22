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
#include <boost/foreach.hpp>

#include <cassert>

DLVHEX_NAMESPACE_USE

class DependencyGraph
{
  //////////////////////////////////////////////////////////////////////////////
  // types
  //////////////////////////////////////////////////////////////////////////////
public:
  struct NodeInfo
  {
    ID rule;
  };

  struct DependencyInfo
  {
    // dependenhead/body
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

  typedef boost::adjacency_list<
    boost::listS, boost::listS, boost::bidirectionalS,
    NodeInfo, DependencyInfo> Graph;
  typedef boost::graph_traits<Graph> Traits;

/*
  typedef typename Graph::vertex_descriptor Node;
  typedef typename Graph::edge_descriptor Dependency;
  typedef typename Traits::vertex_iterator EvalUnitIterator;
  typedef typename Traits::out_edge_iterator PredecessorIterator;
  typedef typename Traits::in_edge_iterator SuccessorIterator;

  class Observer
  {
  public:
    virtual ~Observer() {}
    virtual void addUnit(EvalUnit u) = 0;
    virtual void addDependency(EvalUnitDep d) = 0;
  };
  typedef boost::shared_ptr<Observer> ObserverPtr;
  */

  //////////////////////////////////////////////////////////////////////////////
  // members
  //////////////////////////////////////////////////////////////////////////////
private:
  Graph dg;

  //////////////////////////////////////////////////////////////////////////////
  // methods
  //////////////////////////////////////////////////////////////////////////////
public:
	/*
  inline const EvalGraphInt& getInt() const
    { return eg; }

  inline EvalUnit addUnit(const EvalUnitPropertyBundle& prop)
  {
    EvalUnit u = boost::add_vertex(prop, eg);
    BOOST_FOREACH(ObserverPtr o, observers)
      { o->addUnit(u); }
    return u;
  }

  inline EvalUnitDep addDependency(EvalUnit u1, EvalUnit u2,
    const EvalUnitDepPropertyBundle& prop)
  {
    #ifndef NDEBUG
    // check if the joinOrder is correct
    // (require that dependencies are added in join order)
    PredecessorIterator pit, pend;
    boost::tie(pit,pend) = getPredecessors(u1);
    unsigned count;
    for(count = 0; pit != pend; ++pit, ++count)
    {
      const EvalUnitDepPropertyBundle& predprop = propsOf(*pit);
      if( prop.joinOrder == predprop.joinOrder )
        throw std::runtime_error("EvalGraph::addDependency "
            "reusing join order not allowed");
    }
    if( count != prop.joinOrder )
      throw std::runtime_error("EvalGraph::addDependency "
          "using wrong (probably too high) join order");
    #endif

    bool success;
    EvalUnitDep dep;
    boost::tie(dep, success) = boost::add_edge(u1, u2, prop, eg);
    // if this fails, we tried to add a foreign eval unit or something strange like this
    assert(success);
    BOOST_FOREACH(ObserverPtr o, observers)
      { o->addDependency(dep); }
    return dep;
  }

  void addObserver(ObserverPtr o)
  {
    observers.insert(o);
  }

  void eraseObserver(ObserverPtr o)
  {
    observers.erase(o);
  }

  inline std::pair<EvalUnitIterator, EvalUnitIterator>
  getEvalUnits() const
  {
    return boost::vertices(eg);
  }

  // predecessors are eval units providing input to us,
  // edges are dependencies, so predecessors are at outgoing edges
  inline std::pair<PredecessorIterator, PredecessorIterator>
  getPredecessors(EvalUnit u) const
  {
    return boost::out_edges(u, eg);
  }

  // successors are eval units we provide input to,
  // edges are dependencies, so successors are at incoming edges
  inline std::pair<SuccessorIterator, SuccessorIterator>
  getSuccessors(EvalUnit u) const
  {
    return boost::in_edges(u, eg);
  }

  inline const EvalUnitDepPropertyBundle& propsOf(EvalUnitDep d) const
  {
    return eg[d];
  }

  inline EvalUnitDepPropertyBundle& propsOf(EvalUnitDep d)
  {
    return eg[d];
  }

  inline const EvalUnitPropertyBundle& propsOf(EvalUnit u) const
  {
    return eg[u];
  }

  inline EvalUnitPropertyBundle& propsOf(EvalUnit u)
  {
    return eg[u];
  }

  inline EvalUnit sourceOf(EvalUnitDep d) const
  {
    return boost::source(d, eg);
  }
  inline EvalUnit targetOf(EvalUnitDep d) const
  {
    return boost::target(d, eg);
  }

  inline unsigned countEvalUnits() const
  {
    return boost::num_vertices(eg);
  }
  inline unsigned countEvalUnitDeps() const
  {
    return boost::num_edges(eg);
  }
	*/
};

#endif // DEPENDENCY_GRAPH_HPP_INCLUDED__18102010
