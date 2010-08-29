#ifndef MODEL_GRAPH_HPP_INCLUDED__29082010
#define MODEL_GRAPH_HPP_INCLUDED__29082010

#include <cassert>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>

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
    boost::listS, boost::listS, boost::directedS,
    ModelPropertyBundle, ModelDepPropertyBundle>
      ModelGraphInt;

public:
  typedef typename ModelGraphInt::vertex_descriptor Model;
  typedef typename ModelGraphInt::edge_descriptor ModelDep;

  // "exterior property map" for the eval graph: which models are present at which unit
  typedef std::list<Model> ModelList;
  struct EvalUnitModels
  {
		// for each type of model we have a model list
    std::vector<ModelList> models;
    EvalUnitModels(): models(4, ModelList()) {}
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
    eg(eg) {}

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
  // usage: modelsAtUnit()[EvalUnit u].
  inline const ModelList& modelsAt(EvalUnit unit, ModelType type) const
  {
    assert(0 <= type <= 4);
    assert(0 <= unit <= 4);
    return boost::get(mau, unit).models[type];
  }

  inline const ModelPropertyBundle& propsOf(Model m) const
  {
    return mg[m];
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

  #ifndef NDEBUG
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
  assert(0 <= type);
  assert(type < boost::get(mau, location).models.size()) ;
  boost::get(mau, location).models[type].push_back(m);

  return m;
} // ModelGraph<...>::addModel(...) implementation

#endif // MODEL_GRAPH_HPP_INCLUDED__29082010
