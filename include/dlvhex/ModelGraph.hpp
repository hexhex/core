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

#include "Logger.hpp"
#include "EvalGraph.hpp"

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
  case MT_IN:      return "IN     ";
  case MT_INPROJ:  return "INPROJ ";
  case MT_OUT:     return "OUT    ";
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
    public ModelPropertyBaseT
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
    typedef std::map<EvalUnit, std::set<Model> > SuccessorModelMap;
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
      return o << toString(type) << " at unit " << location << ", " <<
        print_method(static_cast<ModelPropertyBaseT>(*this));
    }
    // the model graph will manage the set of successors
    friend class ModelGraph<EvalGraphT, ModelPropertyBaseT, ModelDepPropertyBaseT>;
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

  // "exterior property map" for the eval graph: which models are present at which unit
  typedef std::list<Model> ModelList;
  struct EvalUnitModels
  {
		// for each type of model we have a model list
    //boost::shared_ptr< std::vector<ModelList> > models;
    //EvalUnitModels(): models(new std::vector<ModelList>(4, ModelList()))
    std::vector<ModelList> models;
    EvalUnitModels(): models(4, ModelList())
      { LOG("EvalUnitModels()@" << this); }
    EvalUnitModels(const EvalUnitModels& eum): models(eum.models)
      { LOG("EvalUnitModels(const EvalUnitModels&)@" << this << " from " << &eum); }
		~EvalUnitModels()
      { LOG("~EvalUnitModels()@" << this); }
    inline ModelList& getModels(ModelType t)
      { assert(0 <= t <= 4); return models[t]; }
    inline const ModelList& getModels(ModelType t) const
      { assert(0 <= t <= 4); return models[t]; }
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
    // do it this way as we are not allowed to do
    // mau.store->reserve(lastUnit)
    mau[lastUnit] = EvalUnitModels();
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
  LOG("add_vertex returned " << m);

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
    std::set<Model>& successorsForThisEvalUnit =
      propsOf(deps[i]).successors[location];
    successorsForThisEvalUnit.insert(m);
  }

  // update modelsAt property map (models at each eval unit are registered there)
  LOG("updating mau");
  mau[location].getModels(type).push_back(m);

  return m;
} // ModelGraph<...>::addModel(...) implementation

// ModelGraph<...>::getSuccessorIntersection(...) implementation
// intersect sets of successors of models mm
// return first intersected element, boost::none if none
template<typename EvalGraphT, typename ModelPropertiesT, typename ModelDepPropertiesT>
boost::optional<typename ModelGraph<EvalGraphT, ModelPropertiesT, ModelDepPropertiesT>::Model>
ModelGraph<EvalGraphT, ModelPropertiesT, ModelDepPropertiesT>::getSuccessorIntersection(
    //EvalUnit location, 
    //const std::vector<Model>& mm) const
    typename ModelGraph<EvalGraphT, ModelPropertiesT, ModelDepPropertiesT>::EvalUnit location,
    const std::vector<typename ModelGraph<EvalGraphT, ModelPropertiesT, ModelDepPropertiesT>::Model>& mm) const
{
  LOG_SCOPE("gSI", false);
  LOG("=getSuccessorIntersection(" << location << "," << mm.size() << ")");

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
  if( mm.size() == 1 )
  {
    LOG("one-dependency shortcut");
    typename ModelPropertyBundle::SuccessorModelMap::const_iterator itsucc =
      propsOf(mm.front()).successors.find(location);
    if( itsucc != propsOf(mm.front()).successors.end() )
    {
      const std::set<Model>& succs = itsucc->second;
      LOG("found successor (" << succs.size() << ")");
      assert(succs.size() == 1);
      return *succs.begin();
    }
    else
    {
      LOG("succs empty");
      return boost::none;
    }
  }

  // regular processing
  typedef typename std::set<Model>::const_iterator SuccIter;
  typename std::vector<SuccIter> iters;
  typename std::vector<SuccIter> ends;

  // store begin() and end() of each model's successor iterators into succs and ends
  BOOST_FOREACH(Model m, mm)
  {
    typename ModelPropertyBundle::SuccessorModelMap::const_iterator itsucc =
      propsOf(m).successors.find(location);
    if( itsucc != propsOf(m).successors.end() )
    {
      const typename std::set<Model>& succs = itsucc->second;
      iters.push_back(succs.begin());
      ends.push_back(succs.end());
      #ifndef NDEBUG
      LOG("model " << m << " at cursor index " <<
          static_cast<unsigned>(iters.size()) << " has successors:");
      LOG_INDENT();
      for(SuccIter sit = succs.begin(); sit != succs.end(); ++sit)
      {
        LOG(*sit);
      }
      #endif
    }
    else
    {
      // if one dependency has no successors, we can find no join
      LOG("model " << m << " at cursor index " <<
          static_cast<unsigned>(iters.size()) << " has no successors -> failing");
      return boost::none;
    }
  }

  // here we have the guarantee, that every pair of iterators has at least one element in the range

  // join loop
  typedef typename std::vector<SuccIter>::iterator SuccIterIter;
  SuccIterIter cursorit = iters.begin();
  SuccIterIter cursorend = ends.begin();
  SuccIterIter lastcursorit = iters.end() - 1;
  SuccIterIter lastcursorend = ends.end() - 1;

  // invariant: elements at cursor and in front of cursor point to the same successor
  // -> if cursor is at the end, we found a common successor (stored in all iterators)

  // strategy:
  // try to find same successor at cursor+1:
  //   increment successor at cursor+1 as long as it is smaller than successor at cursor
  //     if it is equal -> cursor++
  //     else -> cursor = iters.begin() and increment *cursor
  do
  {
    #ifndef NDEBUG
    LOG("at loop begin with cursor at " << static_cast<unsigned>(cursorit - iters.begin()) << " at model " << **cursorit);
    for(SuccIterIter at = iters.begin();
        at != cursorit; ++at)
    {
      LOG("iterator " << static_cast<unsigned>(at - iters.begin()) << " pointing to model " << **at);
    }
    #endif
    // success condition
    if( cursorit == lastcursorit )
    {
      assert(cursorend == lastcursorend);
      Model m = **cursorit;
      LOG("found common successor model " << m << " -> returning");
      return m;
    }

    SuccIterIter nextcursorit = cursorit + 1;
    SuccIterIter nextcursorend = cursorend + 1;
    // these are not end() cursors!
    assert(nextcursorit != iters.end());
    assert(nextcursorend != ends.end());

    SuccIter& nextit = *nextcursorit;
    SuccIter& nextend = *nextcursorend;

    // try to increment here until we reach *cursorit or more
    while( (nextit != nextend) &&
           (*nextit < **cursorit) )
    {
      LOG("seeing model " << *nextit << " at position " <<
          static_cast<unsigned>(nextcursorit - iters.begin()));
      nextit++;
    }
    if( nextit == nextend )
    {
      LOG("beyond last element -> fail");
      return boost::none;
    }
    else
    {
      LOG("seeing model " << *nextit << " at position " <<
          static_cast<unsigned>(nextcursorit - iters.begin()));
      if( *nextit == **cursorit )
      {
        LOG("model is equal to model at cursor -> next cursor");
        cursorit++;
        cursorend++;
      }
      else
      {
        LOG("model is bigger than model at cursor -> backtrack");
        SuccIterIter backtrackercursorit = cursorit;
        SuccIterIter backtrackercursorend = cursorend;
        do
        {
          LOG("backtracking at " <<
            static_cast<unsigned>(backtrackercursorit - iters.begin()));
          // advance until we reach at least *nextit or fail
          SuccIter& backtrackerit = *backtrackercursorit;
          SuccIter& backtrackerend = *backtrackercursorend;
          if( backtrackerit == backtrackerend )
          {
            LOG("backtrack cursor already at end -> fail");
            return boost::none;
          }
          backtrackerit++;
          LOG("advanced to model " << *backtrackerit);
          while( (backtrackerit != backtrackerend) &&
                 (*backtrackerit < *nextit) )
          {
            backtrackerit++;
            LOG("advanced to model " << *backtrackerit);
          }
          if( backtrackercursorit == iters.begin() )
              break;
          LOG("going back further");
          backtrackercursorit--;
          backtrackercursorend--;
        }
        while(true);
        assert(backtrackercursorit == iters.begin());
        assert(backtrackercursorend == ends.begin());
      }
    }
  }
  while(cursorit != iters.end());

  LOG("returning failure");
  return boost::none;
} // ModelGraph<...>::getSuccessorIntersection(...) implementation

#endif // MODEL_GRAPH_HPP_INCLUDED__29082010
