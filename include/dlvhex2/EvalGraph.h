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
 * @file   EvalGraph.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Template for Evaluation Graph.
 */

#ifndef EVAL_GRAPH_HPP_INCLUDED__29082010
#define EVAL_GRAPH_HPP_INCLUDED__29082010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/Printhelpers.h"
#include "dlvhex2/GraphvizHelpers.h"

#include <cassert>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

struct none_t
{
  inline std::ostream& print(std::ostream& o) const { return o; }
};

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
    public EvalUnitPropertyBase,
    public ostream_printable<EvalUnitPropertyBundle>
  {
    EvalUnitPropertyBundle(
      const EvalUnitPropertyBase& base = EvalUnitPropertyBase()):
        EvalUnitPropertyBase(base) {}

    std::ostream& print(std::ostream& o) const
      { return o << static_cast<const EvalUnitPropertyBase>(*this); }
  };
  struct EvalUnitDepPropertyBundle:
    public EvalUnitDepPropertyBaseT,
    public ostream_printable<EvalUnitDepPropertyBundle>
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

    std::ostream& print(std::ostream& o) const
      { return o << "joinOrder=" << joinOrder << " " <<
        static_cast<const EvalUnitDepPropertyBase>(*this); }
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
  typedef typename Traits::edge_iterator DependencyIterator;
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
	EvalGraph()
    {}
  // this is not implemented on purpose (=linker error) because it is forbidden to use
  // (it cannot be private as this would cause concept checks to fail (TODO: check why clang produces such bad error messages in this case)
	EvalGraph(const EvalGraph& other);
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

  // output graph as graphviz source
  void writeGraphViz(std::ostream& o, bool verbose) const;

	struct Impl;
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

template<typename EvalUnitPropertyBaseT, typename EvalUnitDepPropertyBaseT>
struct EvalGraph<EvalUnitPropertyBaseT, EvalUnitDepPropertyBaseT>::Impl
{
  static std::string graphviz_node_id(EvalUnit u)
  {
    std::ostringstream os;
    os << "u" << u;
    return os.str();
  }
};

template<typename EvalUnitPropertyBaseT, typename EvalUnitDepPropertyBaseT>
void EvalGraph<EvalUnitPropertyBaseT, EvalUnitDepPropertyBaseT>::
	writeGraphViz(std::ostream& o, bool verbose) const
{
  // boost::graph::graphviz is horribly broken!
  // therefore we print it ourselves

  o << "digraph G {" << std::endl <<
    "rankdir=BT;" << std::endl; // print root nodes at bottom, leaves at top!

  // print vertices
  EvalUnitIterator it, it_end;
  unsigned index = 0;
  for(boost::tie(it, it_end) = boost::vertices(eg);
      it != it_end; ++it, ++index)
  {
    o << Impl::graphviz_node_id(*it) << "[shape=record,label=\"{" << *it << "|";
    {
      std::ostringstream ss;
			// write eval unit property
      ss << propsOf(*it);
			graphviz::escape(o, ss.str());
    }
    o << "}\"];" << std::endl;
  }

  // print edges
  DependencyIterator dit, dit_end;
  for(boost::tie(dit, dit_end) = boost::edges(eg);
      dit != dit_end; ++dit)
  {
    EvalUnit src = sourceOf(*dit);
    EvalUnit target = targetOf(*dit);
    o << Impl::graphviz_node_id(src) << " -> " << Impl::graphviz_node_id(target) <<
      "[label=\"";
    {
      std::ostringstream ss;
			ss << *dit;
			graphviz::escape(o, ss.str());
    }
    o << "\"];" << std::endl;
  }

  o << "}" << std::endl;
}

DLVHEX_NAMESPACE_END

#endif // EVAL_GRAPH_HPP_INCLUDED__29082010
