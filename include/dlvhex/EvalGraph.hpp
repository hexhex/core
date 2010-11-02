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
 * @file   EvalGraph.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Template for Evaluation Graph.
 */

#ifndef EVAL_GRAPH_HPP_INCLUDED__29082010
#define EVAL_GRAPH_HPP_INCLUDED__29082010

#include <cassert>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>

struct none_t {};

//
// the EvalGraph template manages a generic evaluation graph:
// it takes care of a correct join order among in-edges of units
//
template<
  typename EvalUnitPropertyBaseT = none_t,
  typename EvalUnitDepPropertyBaseT = none_t>
class EvalGraph
{
  //////////////////////////////////////////////////////////////////////////////
  // types
  //////////////////////////////////////////////////////////////////////////////
public:
  typedef EvalUnitPropertyBaseT EvalUnitPropertyBase;
  typedef EvalUnitDepPropertyBaseT EvalUnitDepPropertyBase;

  struct EvalUnitPropertyBundle:
    public EvalUnitPropertyBase
  {
    EvalUnitPropertyBundle(
      const EvalUnitPropertyBase& base = EvalUnitPropertyBase()):
        EvalUnitPropertyBase(base) {}
  };
  struct EvalUnitDepPropertyBundle:
    public EvalUnitDepPropertyBaseT
  {
    // storage
    unsigned joinOrder;

    // init
    EvalUnitDepPropertyBundle(
      unsigned joinOrder = 0):
        joinOrder(joinOrder) {}
    EvalUnitDepPropertyBundle(
      const EvalUnitDepPropertyBase& base,
      unsigned joinOrder = 0):
        EvalUnitDepPropertyBase(base),
        joinOrder(joinOrder) {}
  };

  // rationales for choice of vecS here:
  // * we will add eval units once and don't remove units later on,
  //   therefore the high cost of removing units is not problematic
  //   (if we need to modify the eval graph, this should be done before
  //    creating it in this form, and it should be done on a listS representation
  //    - for that we could add a template parameter StorageT to this class
  //    and convertibility from listS to vecS storage)
  // * vecS creates an implicit vertex index, as descriptors of vecS are integers
  // * therefore we can create vector_property_maps over EvalUnit and EvalUnitDep,
  //   and these property maps have efficient lookup.
  // * therefore we can distribute the properties among several such maps and
  //   need not put all into one property bundle
  typedef boost::adjacency_list<
    boost::vecS, boost::vecS, boost::bidirectionalS,
    EvalUnitPropertyBundle, EvalUnitDepPropertyBundle>
      EvalGraphInt;
  typedef typename boost::graph_traits<EvalGraphInt> Traits;

  typedef typename EvalGraphInt::vertex_descriptor EvalUnit;
  typedef typename EvalGraphInt::edge_descriptor EvalUnitDep;
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

  //////////////////////////////////////////////////////////////////////////////
  // members
  //////////////////////////////////////////////////////////////////////////////
private:
  EvalGraphInt eg;
  std::set<ObserverPtr> observers;

  //////////////////////////////////////////////////////////////////////////////
  // methods
  //////////////////////////////////////////////////////////////////////////////
public:
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
}; // class EvalGraph<...>

// projection properties for eval units
// such properties are required by the model graph
struct EvalUnitProjectionProperties
{
  // storage
  bool iproject;
  bool oproject;

  // init
  EvalUnitProjectionProperties(
    bool iproject = false,
    bool oproject = false):
      iproject(iproject), oproject(oproject) {}
};

#endif // EVAL_GRAPH_HPP_INCLUDED__29082010
