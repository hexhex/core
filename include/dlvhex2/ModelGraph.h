/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2010 Peter Schüller
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

#ifndef MODEL_GRAPH_HPP_INCLUDED__29082010
#define MODEL_GRAPH_HPP_INCLUDED__29082010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/EvalGraph.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Printhelpers.h"

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include <cassert>
#include <iomanip>

DLVHEX_NAMESPACE_BEGIN

// this is used as index into an array by struct EvalUnitModels
enum ModelType
{
  MT_IN = 0,
  MT_INPROJ = 1,
  MT_OUT = 2,
  MT_OUTPROJ = 3,
};

inline const char* toString(ModelType mt)
{
  switch(mt)
  {
  case MT_IN:      return "IN";
  case MT_INPROJ:  return "INPROJ";
  case MT_OUT:     return "OUT";
  case MT_OUTPROJ: return "OUTPROJ";
  default: assert(false); return ""; // keep compiler happy with NDEBUG
  }
}

//
// the ModelGraph template manages a generic model graph,
// corresponding to an EvalGraph type:
// it manages projection for units and corresponding model types
// it manages correspondance of dependencies between models and units
// it manages correspondance of join orders between model and unit dependencies
//
template<
  typename EvalGraphT,
  typename ModelPropertyBaseT = none_t,
  typename ModelDepPropertyBaseT = none_t>
class ModelGraph
{
  //////////////////////////////////////////////////////////////////////////////
  // types
  //////////////////////////////////////////////////////////////////////////////
public:
  typedef ModelGraph<EvalGraphT, ModelPropertyBaseT, ModelDepPropertyBaseT> Self;
  typedef EvalGraphT MyEvalGraph;
  typedef ModelPropertyBaseT ModelPropertyBase;
  typedef ModelDepPropertyBaseT ModelDepPropertyBase;

  // concept check: must be an eval graph
  // TODO: solve not via convertible but via other helper
  BOOST_CONCEPT_ASSERT((boost::Convertible<
      EvalGraphT,
      EvalGraph<
        typename EvalGraphT::EvalUnitPropertyBase,
        typename EvalGraphT::EvalUnitDepPropertyBase> >));
  typedef typename EvalGraphT::EvalUnit EvalUnit;
  typedef typename EvalGraphT::EvalUnitDep EvalUnitDep;

  // concept check: eval graph must store projection properties for units
  BOOST_CONCEPT_ASSERT((boost::Convertible<
      typename EvalGraphT::EvalUnitPropertyBundle,
      EvalUnitProjectionProperties>));

  struct ModelPropertyBundle;
  struct ModelDepPropertyBundle;

private:
  typedef boost::adjacency_list<
    boost::listS, boost::listS, boost::bidirectionalS,
    ModelPropertyBundle, ModelDepPropertyBundle>
      ModelGraphInt;
  typedef typename boost::graph_traits<ModelGraphInt> Traits;

public:
  typedef typename ModelGraphInt::vertex_descriptor Model;
  typedef typename ModelGraphInt::edge_descriptor ModelDep;
  typedef typename Traits::vertex_iterator ModelIterator;
  typedef typename Traits::out_edge_iterator PredecessorIterator;
  typedef typename Traits::in_edge_iterator SuccessorIterator;

  struct ModelPropertyBundle:
    public ModelPropertyBaseT,
    public ostream_printable<ModelPropertyBundle>
  {
    // storage

    // location of this model
    EvalUnit location;
    // type of this model
    ModelType type;

  protected:
    // successor models per successor eval unit, suitable for fast set intersection
    // (we also need the chronological ordering of adjacency_list,
    //  so we cannot replace that one by an ordered container)

    // (we must not use "Model" here because "Model" is defined using
    // "ModelPropertyBundle" (i.e., this class))
    //typedef std::map<EvalUnit, std::set<Model> > SuccessorModelMap;
    typedef std::map<EvalUnit, std::set<void*> > SuccessorModelMap;
    SuccessorModelMap successors;

  public:
    // init
    ModelPropertyBundle(
      EvalUnit location = EvalUnit(),
      ModelType type = MT_IN):
        location(location),
        type(type) {}
    ModelPropertyBundle(
      const ModelPropertyBaseT& base,
      EvalUnit location = EvalUnit(),
      ModelType type = MT_IN):
        ModelPropertyBaseT(base),
        location(location),
        type(type) {}
    std::ostream& print(std::ostream& o) const
    {
      return o << print_method(static_cast<ModelPropertyBaseT>(*this));
    }
    // the model graph will manage the set of successors
    friend class ModelGraph<EvalGraphT, ModelPropertyBaseT, ModelDepPropertyBaseT>;
  };

  struct ModelDepPropertyBundle:
    public ModelDepPropertyBaseT,
    public ostream_printable<ModelDepPropertyBundle>
  {
    // storage

    // join order
    unsigned joinOrder;

    // init
    ModelDepPropertyBundle(
      unsigned joinOrder = 0):
        joinOrder(joinOrder) {}
    ModelDepPropertyBundle(
      const ModelDepPropertyBaseT& base,
      unsigned joinOrder = 0):
        ModelDepPropertyBaseT(base),
        joinOrder(joinOrder) {}
  };

  // "exterior property map" for the eval graph: which models are present at which unit
  typedef std::list<Model> ModelList;
  struct EvalUnitModels
  {
  protected:
		// for each type of model we have a model list
    // (we need to use a pointer here, because otherwise resizing 
    //  the EvalUnitModelsPropertyMap will invalidate all iterators to the list)
    // (this additinoally makes resizing the property map cheaper)
    boost::shared_ptr< std::vector<ModelList> > models;
  public:
    EvalUnitModels(): models(new std::vector<ModelList>(4, ModelList()))
      { DBGLOG(DBG, "EvalUnitModels()@" << this); }
    EvalUnitModels(const EvalUnitModels& eum): models(eum.models)
      { DBGLOG(DBG, "EvalUnitModels(const EvalUnitModels&)@" << this << " from " << &eum); }
		~EvalUnitModels()
      { DBGLOG(DBG, "~EvalUnitModels()@" << this); }
    inline ModelList& getModels(ModelType t)
      { assert(0 <= t && t <= 4); assert(models.use_count() == 1); return (*models)[t]; }
    inline const ModelList& getModels(ModelType t) const
      { assert(0 <= t && t <= 4); assert(models.use_count() == 1); return (*models)[t]; }
    void reallocate()
      { models.reset(new std::vector<ModelList>(models->begin(), models->end())); }
  };
  typedef boost::vector_property_map<EvalUnitModels>
    EvalUnitModelsPropertyMap;

  //////////////////////////////////////////////////////////////////////////////
  // members
  //////////////////////////////////////////////////////////////////////////////
private:
  // which eval graph is this model graph linked to
  EvalGraphT& eg;
  ModelGraphInt mg;
  // "exterior property map" for the eval graph: which models are present at which unit
  EvalUnitModelsPropertyMap mau;

  //////////////////////////////////////////////////////////////////////////////
  // methods
  //////////////////////////////////////////////////////////////////////////////
public:
  // initialize with link to eval graph
  ModelGraph(EvalGraphT& eg):
    eg(eg), mg(), mau()
	{
    // get last unit
    // as eg uses vecS this is the maximum index we need in mau
    EvalUnit lastUnit = *(eg.getEvalUnits().second - 1);
    // this is an integer -> reserve for one more unit
    lastUnit++;
    // do it this way as we are not allowed to do
    // mau.store->reserve(lastUnit)
    mau[lastUnit] = EvalUnitModels();
    // now reallocate all (we do not want duplicate pointers to model lists)
    // @todo: this is a real hack, we should create our own vector property map with custom resizing
    for(unsigned u = 0; u <= lastUnit; ++u)
      mau[u].reallocate();
	}

  // create a new model including dependencies
  // returns the new model
  // modelsAtUnit is automatically updated
  // order of dependencies determines join order
  //
  // MT_IN models:
  // * checks if join order is equal to join order of eval graph
  // * checks if input models depend on all units this unit depends on
  //
  // MT_INPROJ models:
  // * checks if model depends on MT_IN model at same unit
  // * checks if projection is configured for unit
  //
  // MT_OUT models:
  // * checks if model depends on MT_IN or MT_INPROJ at same unit
  //   iff unit has predecessors
  //
  // MT_OUTPROJ models:
  // * checks if model depends on MT_OUT at same unit
  // * checks if projection is configured for unit
  Model addModel(
    EvalUnit location,
    ModelType type,
    const std::vector<Model>& deps=std::vector<Model>());

  // intersect sets of successors of models mm
  // return first intersected element, boost::none if none
  boost::optional<Model> getSuccessorIntersection(EvalUnit location, const std::vector<Model>& mm) const;

  inline std::pair<ModelIterator, ModelIterator> getModels() const
    { return boost::vertices(mg); }

  inline const ModelGraphInt& getInternalGraph() const
    { return mg; }

  // return helper list that stores for each unit the set of i/omodels there
  inline const ModelList& modelsAt(EvalUnit unit, ModelType type) const
  {
    return mau[unit].getModels(type);
  }

  // return list of relevant imodels at unit (depends on projection whether this is MT_IN or MT_INPROJ)
  inline const ModelList& relevantIModelsAt(EvalUnit unit) const
  {
    if( eg.propsOf(unit).iproject )
      return modelsAt(unit, MT_INPROJ);
    else
      return modelsAt(unit, MT_IN);
  }

  // return list of relevant omodels at unit (depends on projection whether this is MT_OUT or MT_OUTPROJ)
  inline const ModelList& relevantOModelsAt(EvalUnit unit) const
  {
    if( eg.propsOf(unit).oproject )
      return modelsAt(unit, MT_OUTPROJ);
    else
      return modelsAt(unit, MT_OUT);
  }

  inline const ModelPropertyBundle& propsOf(Model m) const
  {
    return mg[m];
  }

  inline ModelPropertyBundle& propsOf(Model m)
  {
    return mg[m];
  }

  inline const ModelDepPropertyBundle& propsOf(ModelDep d) const
  {
    return mg[d];
  }

  inline ModelDepPropertyBundle& propsOf(ModelDep d)
  {
    return mg[d];
  }

  // predecessors are models this model is based on
  // predecessors are dependencies, so predecessors are at targetOf(iterators)
  inline std::pair<PredecessorIterator, PredecessorIterator>
  getPredecessors(Model m) const
  {
    return boost::out_edges(m, mg);
  }

  // successors are models this model contributed to,
  // successors are dependencies, so successors are at sourceOf(iterators)
  inline std::pair<SuccessorIterator, SuccessorIterator>
  getSuccessors(Model m) const
  {
    return boost::in_edges(m, mg);
  }

  inline Model sourceOf(ModelDep d) const
  {
    return boost::source(d, mg);
  }
  inline Model targetOf(ModelDep d) const
  {
    return boost::target(d, mg);
  }

  inline unsigned countModels() const
  {
    return boost::num_vertices(mg);
  }
  inline unsigned countModelDeps() const
  {
    return boost::num_edges(mg);
  }
}; // class ModelGraph

// ModelGraph<...>::addModel(...) implementation
template<typename EvalGraphT, typename ModelPropertiesT, typename ModelDepPropertiesT>
typename ModelGraph<EvalGraphT, ModelPropertiesT, ModelDepPropertiesT>::Model
ModelGraph<EvalGraphT, ModelPropertiesT, ModelDepPropertiesT>::addModel(
  EvalUnit location,
  ModelType type,
  const std::vector<Model>& deps)
{
  typedef typename EvalGraphT::PredecessorIterator PredecessorIterator;
  typedef typename EvalGraphT::EvalUnitDepPropertyBundle EvalUnitDepPropertyBundle;
  typedef typename EvalGraphT::EvalUnitPropertyBundle EvalUnitPropertyBundle;
  LOG_VSCOPE(MODELB,"MG::addModel",this,true);

  #ifndef NDEBUG
  DBGLOG(DBG, "running debug checks");
  switch(type)
  {
  case MT_IN:
    {
      // input models:
      // * checks if join order is equal to join order of eval graph
      // * checks if input models depend on all units this unit depends on
      // (this is an implicit check if we exactly use all predecessor units)
      PredecessorIterator it, end;
      for(boost::tie(it, end) = eg.getPredecessors(location);
          it != end; ++it)
      {
        // check whether each predecessor is stored at the right position in the vector
        // the join order starts at 0, so we use it for indexing into the deps vector

        // check if joinOrder == index is within range of deps vector
        const EvalUnitDepPropertyBundle& predprop = eg.propsOf(*it);
        if( predprop.joinOrder >= deps.size() )
          throw std::runtime_error("ModelGraph::addModel MT_IN "
            "not enough join dependencies");

        // check if correct unit is referenced by model
        EvalUnit predunit = eg.targetOf(*it);
        const ModelPropertyBundle& depprop = propsOf(deps[predprop.joinOrder]);
        if( depprop.location != predunit )
          throw std::runtime_error("ModelGraph::addModel MT_IN "
            "with wrong join order");
      }
      // if we are here we found for each predecessor one unit in deps,
      // assuming joinOrder of predecessors are correct,
      // the models in the deps vector exactly use all predecessor units
    }
    break;

  case MT_INPROJ:
    {
      // projected input models
      // * checks if model depends on MT_IN model at same unit
      // * checks if projection is configured for unit
      if( deps.size() != 1 )
        throw std::runtime_error("ModelGraph::addModel MT_INPROJ "
          "must depend on exactly one MT_IN model");
      const ModelPropertyBundle& depprop = propsOf(deps[0]);
      if( depprop.location != location )
        throw std::runtime_error("ModelGraph::addModel MT_INPROJ "
          "must depend on model at same eval unit");
      if( depprop.type != MT_IN )
        throw std::runtime_error("ModelGraph::addModel MT_INPROJ "
          "must depend on exactly one MT_IN model");
      const EvalUnitPropertyBundle& unitprop = eg.propsOf(location);
      if( !unitprop.iproject )
        throw std::runtime_error("ModelGraph::addModel MT_INPROJ "
          "only possible for units with iproject==true");
    }
    break;

  case MT_OUT:
    {
      // output models:
      // * checks if model depends on MT_IN or MT_INPROJ at same unit
      PredecessorIterator it, end;
      boost::tie(it, end) = eg.getPredecessors(location);
      if( it != end )
      {
        const ModelPropertyBundle& depprop = propsOf(deps[0]);
        if( depprop.location != location )
          throw std::runtime_error("ModelGraph::addModel MT_OUT "
            "must depend on model at same eval unit");
        const EvalUnitPropertyBundle& unitprop = eg.propsOf(location);
        if( (unitprop.iproject && depprop.type != MT_INPROJ) ||
            (!unitprop.iproject && depprop.type != MT_IN) )
          throw std::runtime_error("ModelGraph::addModel MT_OUT "
            "must depend on MT_INPROJ model for iproject==true "
            "and on MT_IN model for iproject==false");
      }
    }
    break;

  case MT_OUTPROJ:
    {
      // projected output models:
      // * checks if model depends on MT_OUT at same unit
      // * checks if projection is configured for unit
      if( deps.size() != 1 )
        throw std::runtime_error("ModelGraph::addModel MT_OUTPROJ "
          "must depend on exactly one MT_OUT model");
      const ModelPropertyBundle& depprop = propsOf(deps[0]);
      if( depprop.location != location )
        throw std::runtime_error("ModelGraph::addModel MT_OUTPROJ "
          "must depend on model at same eval unit");
      if( depprop.type != MT_OUT )
        throw std::runtime_error("ModelGraph::addModel MT_OUTPROJ "
          "must depend on exactly one MT_OUT model");
      const EvalUnitPropertyBundle& unitprop = eg.propsOf(location);
      if( !unitprop.oproject )
        throw std::runtime_error("ModelGraph::addModel MT_OUTPROJ "
          "only possible for units with oproject==true");
    }
    break;
  }
  #endif

  // add model
  ModelPropertyBundle prop;
  prop.location = location;
  prop.type = type;
  Model m = boost::add_vertex(prop, mg);
  LOG(MODELB, "add_vertex returned " << m);

  // add model dependencies
  for(unsigned i = 0; i < deps.size(); ++i)
  {
    ModelDepPropertyBundle dprop(i);
    ModelDep dep;
    bool success;
    boost::tie(dep, success) = boost::add_edge(m, deps[i], dprop, mg);
    assert(success);

    // update ordered set of successors
    // (required for efficiently finding out whether for a given set of models
    //  there already exists a joined successor model at some eval unit)

    // if index does not exist, empty set will be created
    // TODO see getSuccessorIntersection
    std::set<void*>& successorsForThisEvalUnit =
      propsOf(deps[i]).successors[location];
    successorsForThisEvalUnit.insert(m);
  }

  // update modelsAt property map (models at each eval unit are registered there)
  LOG(MODELB, "updating mau");
  mau[location].getModels(type).push_back(m);

  return m;
} // ModelGraph<...>::addModel(...) implementation

// ModelGraph<...>::getSuccessorIntersection(...) implementation
//
// given an eval unit and for each predecessor of this unit a model,
// this method intersects the successor models of all these models
// return first intersected element, boost::none if none exists
template<typename EvalGraphT, typename ModelPropertiesT, typename ModelDepPropertiesT>
boost::optional<typename ModelGraph<EvalGraphT, ModelPropertiesT, ModelDepPropertiesT>::Model>
ModelGraph<EvalGraphT, ModelPropertiesT, ModelDepPropertiesT>::getSuccessorIntersection(
    //EvalUnit location, 
    //const std::vector<Model>& mm) const
    typename ModelGraph<EvalGraphT, ModelPropertiesT, ModelDepPropertiesT>::EvalUnit location,
    const std::vector<typename ModelGraph<EvalGraphT, ModelPropertiesT, ModelDepPropertiesT>::Model>& mm) const
{
  const unsigned predecessors = mm.size();

  DBGLOG_SCOPE(DBG, "gSI", false);
  DBGLOG(DBG, "=getSuccessorIntersection(" << location << "," << predecessors << ")");

  #ifndef NDEBUG
  BOOST_FOREACH(Model m, mm)
  {
    // assert that we only to this for output models
    assert(propsOf(m).type == MT_OUT || propsOf(m).type == MT_OUTPROJ);
    // assert that we only do this for models between eval units (i.e., for joins)
    assert(propsOf(m).location != location);
    // (rationale: for other models we do not need this functionality,
    //  and therefore for other models "successors" will not be managed by addModel
    //  in order to conserve space)
    // TODO: really do not do this for other models
  }
  #endif

  // shortcut if only one dependency
  if( predecessors == 1 )
  {
    DBGLOG(DBG, "one-dependency shortcut: simply finding corresponding model");
    typename ModelPropertyBundle::SuccessorModelMap::const_iterator itsucc =
      propsOf(mm.front()).successors.find(location);
    if( itsucc != propsOf(mm.front()).successors.end() )
    {
      // found successor set -> good (take first, which should be the only one)
      const std::set<void*>& succs = itsucc->second;
      DBGLOG(DBG, "found successor (" << succs.size() << ")");
      assert(succs.size() == 1);
      return *succs.begin();
    }
    else
    {
      // did not find successor set
      DBGLOG(DBG, "no successors");
      return boost::none;
    }
  }

  // regular processing
  typedef typename std::set<void*>::const_iterator SuccIter;
  // for each predecessor model we have a begin and an end iterator of all of their successors
  typename std::vector<SuccIter> iters;
  typename std::vector<SuccIter> ends;

  // store begin() and end() of each model's successor iterators into succs and ends
  BOOST_FOREACH(Model m, mm)
  {
    typename ModelPropertyBundle::SuccessorModelMap::const_iterator itsucc =
      propsOf(m).successors.find(location);
    if( itsucc != propsOf(m).successors.end() )
    {
      const std::set<void*>& succs = itsucc->second;
      iters.push_back(succs.begin());
      ends.push_back(succs.end());
      #ifndef NDEBUG
      DBGLOG(DBG, "model " << m << " at cursor index " <<
          static_cast<unsigned>(iters.size()-1) << " has successors:");
      DBGLOG_INDENT(DBG);
      for(SuccIter sit = succs.begin(); sit != succs.end(); ++sit)
      {
        DBGLOG(DBG, *sit);
      }
      #endif
    }
    else
    {
      // if one dependency has no successors, we can find no join
      DBGLOG(DBG, "model " << m << " at cursor index " <<
          static_cast<unsigned>(iters.size()) << " has no successors -> failing");
      return boost::none;
    }
  }

  // here we have the guarantee, that every pair of iterators has at least one element in the range

  // join loop

  // this is the position where we currently "are"
  unsigned at = 0;

  // invariant: *iters[at] == *iters[at-1] == ... == *iters[0] (they point to the same successor model)
  // -> if "at == predecessors-1" cursor is at the end, we found a common successor (stored in all iterators)

  // strategy:
  // try to find equal successor at [at+1] as it already is at [at]
  //   increment iters[at+1] while < iters[at]
  //     if equal -> at++
  //     else -> increment all iters[X<=at] until >= iters[at+1] and set at = 0
  do
  {
    #ifndef NDEBUG
    DBGLOG(DBG, "at loop begin with cursor at " << at << ", models:");
    for(unsigned u = 0; u < predecessors; ++u)
    {
      DBGLOG(DBG, "  iterator " << u << " pointing to model " << *iters[u]);
    }
    #endif
    // success condition
    if( at == (predecessors-1) )
    {
      Model m = *iters[0];
      DBGLOG(DBG, "found common successor model " << m << " -> returning");
      return m;
    }

    assert((at+1) < predecessors);

    // try to advance iters[at+1]
    while( *iters[at+1] < *iters[at] )
    {
      iters[at+1]++;
      if( iters[at+1] == ends[at+1] )
      {
        DBGLOG(DBG, "no suitable model at " << at << " -> returning none");
        return boost::none;
      }
      DBGLOG(DBG, "advancing " << (at+1) << " to model " << *iters[at+1]);
    }

    // check if same or greater
    if( *iters[at+1] == *iters[at] )
    {
      DBGLOG(DBG, "model at " << (at+1) << " equal to model at " << at << " -> next position");
      at++;
    }
    else
    {
      DBGLOG(DBG, "model at " << (at+1) << " bigger than model at " << at << " -> backtracking");
      for(unsigned u = 0; u <= at; ++u)
      {
        while( *iters[u] < *iters[at+1] )
        {
          iters[u]++;
          if( iters[u] == ends[u] )
          {
            DBGLOG(DBG, "no suitable model at " << u << " -> returning none");
            return boost::none;
          }
          DBGLOG(DBG, "advancing " << (u) << " to model " << *iters[u]);
        }
      }
      // restart loop
      at = 0;
    }
  }
  while(true);
} // ModelGraph<...>::getSuccessorIntersection(...) implementation

DLVHEX_NAMESPACE_END

#endif // MODEL_GRAPH_HPP_INCLUDED__29082010
