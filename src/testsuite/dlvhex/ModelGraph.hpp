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

#include <cassert>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "EvalGraph.hpp"

// this is used as index into an array by struct EvalUnitModels
enum ModelType
{
  MT_IN = 0,
  MT_INPROJ = 1,
  MT_OUT = 2,
  MT_OUTPROJ = 3,
};

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
  typedef EvalGraphT MyEvalGraph;
  typedef ModelPropertyBaseT ModelPropertyBase;
  typedef ModelDepPropertyBaseT ModelDepPropertyBase;

  // concept check: must be an eval graph
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

  struct ModelPropertyBundle:
    public ModelPropertyBaseT
  {
    // storage

    // location of this model
    EvalUnit location;
    // type of this model
    ModelType type;

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
  };

  struct ModelDepPropertyBundle:
    public ModelDepPropertyBaseT
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

  // "exterior property map" for the eval graph: which models are present at which unit
  typedef std::list<Model> ModelList;
  struct EvalUnitModels
  {
		// for each type of model we have a model list
    boost::shared_ptr< std::vector<ModelList> > models;
    EvalUnitModels(): models(new std::vector<ModelList>(4, ModelList()))
      { LOG("EvalUnitModels()@" << this); }
    EvalUnitModels(const EvalUnitModels& eum): models(eum.models)
      { LOG("EvalUnitModels(const EvalUnitModels&)@" << this << " from " << &eum); }
		~EvalUnitModels()
      { LOG("~EvalUnitModels()@" << this << " sizes=" <<
				(*models)[0].size() << " " << (*models)[1].size() << " " <<
				(*models)[2].size() << " " << (*models)[3].size()); }
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
    // @todo: resize/reserve memory for mau s.t. it need not be reallocated too often?
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

  // return helper list that stores for each unit the set of i/omodels there
  inline const ModelList& modelsAt(EvalUnit unit, ModelType type) const
  {
    assert(0 <= type <= 4);
    assert(0 <= unit <= 4);
    return (*mau[unit].models)[type];
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

  // @todo make some properties non-writable! (i.e., those managed by modelgraph)!
  inline ModelPropertyBundle& propsOf(Model m)
  {
    return mg[m];
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

	inline const void* dbg(Model m) const
	{
		return reinterpret_cast<const void*>(&mg[m]);
	}
	inline const void* dbg(const boost::optional<Model>& m) const
	{
		if( !!m )
			return dbg(m.get());
		else
			return 0;
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
  LOG_METHOD("MG::addModel", this);

  #ifndef NDEBUG
  LOG("running debug checks");
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
      //   iff unit has predecessors
      PredecessorIterator it, end;
      boost::tie(it, end) = eg.getPredecessors(location);
      if( (it != end && deps.size() != 1) ||
          (it == end && deps.size() != 0) )
        throw std::runtime_error("ModelGraph::addModel MT_OUT "
          "must depend on one input model iff unit has predecessors");
      if( deps.size() == 1 )
      {
        const ModelPropertyBundle& depprop = propsOf(deps[0]);
        if( depprop.location != location )
          throw std::runtime_error("ModelGraph::addModel MT_OUT "
            "must depend on model at same eval unit");
        const EvalUnitPropertyBundle& unitprop = eg.propsOf(location);
        if( (unitprop.iproject && depprop.type != MT_INPROJ) ||
            (!unitprop.iproject && depprop.type != MT_IN) )
          throw std::runtime_error("ModelGraph::addModel MT_OUT "
            "must depend on MT_INPROJ model for iproject==true eval unit "
            "and on MT_IN model for iproject==false eval unit");
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
  LOG("add_vertex returned " << m);

  // add model dependencies
  for(unsigned i = 0; i < deps.size(); ++i)
  {
    ModelDepPropertyBundle dprop(i);
    ModelDep dep;
    bool success;
    boost::tie(dep, success) = boost::add_edge(m, deps[i], dprop, mg);
    assert(success);
  }

  // update modelsAt property map (models at each eval unit are registered there)
  LOG("updating mau");
  assert(0 <= type);
  assert(type < mau[location].models->size()) ;
  (*mau[location].models)[type].push_back(m);

  return m;
} // ModelGraph<...>::addModel(...) implementation

#endif // MODEL_GRAPH_HPP_INCLUDED__29082010
