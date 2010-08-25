#include <iostream>
#include <set>
#include <list>
#include <string>
#include <vector>
#include <cassert>

#include <boost/foreach.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/property_map/vector_property_map.hpp>
#define BOOST_TEST_MODULE __FILE__
#include <boost/test/included/unit_test.hpp>

struct none_t {};

template<
  typename EvalUnitPropertyBaseT = none_t,
  typename EvalUnitDepPropertyBaseT = none_t>
class EvalGraph
{
  //////////////////////////////////////////////////////////////////////////////
  // types
  //////////////////////////////////////////////////////////////////////////////
public:
  struct EvalUnitPropertyBundle:
    public EvalUnitPropertyBaseT
  {
    // storage

    // input projection or not
    bool iproject;
    // output projection or not
    bool oproject;

    // init
    EvalUnitPropertyBundle(
      bool iproject = false,
      bool oproject = false):
        iproject(iproject), oproject(oproject) {}
    EvalUnitPropertyBundle(
      const EvalUnitPropertyBaseT& base,
      bool iproject = false,
      bool oproject = false):
        EvalUnitPropertyBaseT(base),
        iproject(iproject), oproject(oproject) {}
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
      const EvalUnitDepPropertyBaseT& base,
      unsigned joinOrder = 0):
        EvalUnitDepPropertyBaseT(base),
        joinOrder(joinOrder) {}
  };

private:
  // vecS is not bad, because we will not really add a huge amount of eval units, and we won't remove any units.
  // vecS is important, because it creates an implicit vertex index (and descriptors of vecS are integers)
  typedef boost::adjacency_list<
    boost::vecS, boost::vecS, boost::bidirectionalS,
    EvalUnitPropertyBundle, EvalUnitDepPropertyBundle>
      EvalGraphInt;
  typedef typename boost::graph_traits<EvalGraphInt> Traits;

public:
  typedef typename EvalGraphInt::vertex_descriptor EvalUnit;
  typedef typename EvalGraphInt::edge_descriptor EvalUnitDep;
  typedef typename Traits::out_edge_iterator PredecessorIterator;
  typedef typename Traits::in_edge_iterator SuccessorIterator;

  //////////////////////////////////////////////////////////////////////////////
  // members
  //////////////////////////////////////////////////////////////////////////////
private:
  EvalGraphInt eg;

  //////////////////////////////////////////////////////////////////////////////
  // methods
  //////////////////////////////////////////////////////////////////////////////
public:
  inline EvalUnit addUnit(const EvalUnitPropertyBundle& prop)
  {
    return boost::add_vertex(prop, eg);
  }
  inline EvalUnitDep addDependency(EvalUnit u1, EvalUnit u2,
    const EvalUnitDepPropertyBundle& prop)
  {
    bool success;
    EvalUnitDep dep;
    boost::tie(dep, success) = boost::add_edge(u1, u2, prop, eg);
    // if this fails, we tried to add a foreign eval unit or something strange like this
    assert(success);
    return dep;
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

  inline const EvalUnitDepPropertyBundle& propsOf(EvalUnitDep u) const
  {
    return eg[u];
  }

  inline const EvalUnitPropertyBundle& propsOf(EvalUnit u) const
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
};

// for testing we use stupid types
typedef std::set<std::string> AtomSet;

class Interpretation
{
public:
  // create empty
  Interpretation(): atoms() {}
  // create from atom set
  Interpretation(const AtomSet& as): atoms(as) {}
  // create as union
  Interpretation(const Interpretation& i1, const Interpretation& i2);
  // destruct
  ~Interpretation() {}
  // output
  std::ostream& print(std::ostream& o) const
  {
    AtomSet::const_iterator it = atoms.begin();
    o << "{" << *it;
    ++it;
    for(;it != atoms.end(); ++it)
      o << "," << *it;
    o << "}";
    return o;
  }

private:
  AtomSet atoms;
};

// syntactic operator<< sugar for printing interpretations
std::ostream& operator<<(std::ostream& o, const Interpretation& i)
{
  return i.print(o);
}

// this is used as index into an array by struct EvalUnitModels
enum ModelType
{
  MT_IN = 0,
  MT_INPROJ = 1,
  MT_OUT = 2,
  MT_OUTPROJ = 3,
};

template<typename EvalGraphT, typename ModelPropertyBaseT, typename ModelDepPropertyBaseT>
class ModelGraph
{
  //////////////////////////////////////////////////////////////////////////////
  // types
  //////////////////////////////////////////////////////////////////////////////
public:
  typedef typename EvalGraphT::EvalUnit EvalUnit;
  typedef typename EvalGraphT::EvalUnitDep EvalUnitDep;

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
  typedef std::vector<Model> ModelList;
  struct EvalUnitModels
  {
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
    return boost::get(mau, unit).models[type];
  }

  inline const ModelPropertyBundle& propsOf(Model m) const
  {
    return mg[m];
  }
};

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


#if 0
template<typename EvaluationGraphT>
class ModelBuilder
{
  // types
public:
  // create a model graph suited to our needs
  typedef ... ModelGraph;

private:
  // "exterior property map" for the eval graph:
  // which models are currently used at which unit
  // (=model building state)
  typedef std::vector<Model> ModelList;
  struct EvalUnitModelBuildingData
  {
    Model imodel;
    Model omodel;
    unsigned orefcount;
  };
  typedef boost::vector_property_map<EvalUnitModelBuildingData>
    EvalUnitModelBuildingPropertyMap;

  // members
private:
  EvalUnitModelBuildingPropertyMap mbd;

  // methods
public:
};
#endif

//
// test types
//

// TestEvalGraph
struct TestEvalUnitPropertyBase
{
  // rules in the eval unit
  std::string rules;

  TestEvalUnitPropertyBase() {}
  TestEvalUnitPropertyBase(const std::string& rules):
    rules(rules) {}
};

typedef EvalGraph<TestEvalUnitPropertyBase>
  TestEvalGraph;
typedef TestEvalGraph::EvalUnit EvalUnit; 
typedef TestEvalGraph::EvalUnitDep EvalUnitDep; 
//typedef TestEvalGraph::EvalUnitPropertyBundle UnitCfg;
typedef TestEvalUnitPropertyBase UnitCfg;
typedef TestEvalGraph::EvalUnitDepPropertyBundle UnitDepCfg;

// TestModelGraph
struct TestModelPropertyBase
{
  // interpretation of the model
  Interpretation interpretation;

  TestModelPropertyBase() {}
  TestModelPropertyBase(const Interpretation& interpretation):
    interpretation(interpretation) {}
};

typedef ModelGraph<TestEvalGraph, TestModelPropertyBase, none_t>
  TestModelGraph;
typedef TestModelGraph::Model Model;
typedef TestModelGraph::ModelPropertyBundle ModelProp;
typedef TestModelGraph::ModelDep ModelDep;
typedef TestModelGraph::ModelDepPropertyBundle ModelDepProp;

//
// test fixtures (multiply used initializations)
//

// setup eval graph $\cE_2$
struct EvalGraphE2Fixture
{
  TestEvalGraph eg;
  EvalUnit u1, u2, u3, u4;
  EvalUnitDep e21, e31, e42, e43;

  EvalGraphE2Fixture()
  {
    BOOST_TEST_MESSAGE("adding u1");
    u1 = eg.addUnit(UnitCfg("plan(a) v plan(b)."));
    BOOST_TEST_MESSAGE("adding u2");
    u2 = eg.addUnit(UnitCfg("need(p,C) :- &cost[plan](C). :- need(_,money).")); 
    BOOST_TEST_MESSAGE("adding u3");
    u3 = eg.addUnit(UnitCfg("use(X) v use(Y).")); 
    BOOST_TEST_MESSAGE("adding u4");
    u4 = eg.addUnit(UnitCfg("need(u,C) :- &cost[use](C). :- need(_,money)."));
    BOOST_TEST_MESSAGE("adding e21");
    e21 = eg.addDependency(u2, u1, UnitDepCfg(0));
    BOOST_TEST_MESSAGE("adding e31");
    e31 = eg.addDependency(u3, u1, UnitDepCfg(0));
    BOOST_TEST_MESSAGE("adding e42");
    e42 = eg.addDependency(u4, u2, UnitDepCfg(0));
    BOOST_TEST_MESSAGE("adding e43");
    e43 = eg.addDependency(u4, u3, UnitDepCfg(1));
  }

  ~EvalGraphE2Fixture() {}
};

// setup model graph $\cM_2$ (including setup of eval graph $\cE_2$
struct ModelGraphM2Fixture:
  public EvalGraphE2Fixture
{
  TestModelGraph mg;
  Model m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14;

  ModelGraphM2Fixture():
    EvalGraphE2Fixture(),
    mg(eg)
  {
    std::vector<Model> depm;
    depm.reserve(2);

    // u1
    BOOST_TEST_MESSAGE("adding m1");
    m1 = mg.addModel(u1, MT_OUT);
    BOOST_TEST_MESSAGE("adding m2");
    m2 = mg.addModel(u1, MT_OUT);

    // u2
    BOOST_TEST_MESSAGE("adding m3");
    depm.clear(); depm.push_back(m1);
    m3 = mg.addModel(u2, MT_IN, depm);
    BOOST_TEST_MESSAGE("adding m4");
    depm.clear(); depm.push_back(m2);
    m4 = mg.addModel(u2, MT_IN, depm);
    BOOST_TEST_MESSAGE("adding m5");
    depm.clear(); depm.push_back(m4);
    m5 = mg.addModel(u2, MT_OUT, depm);

    // u3
    BOOST_TEST_MESSAGE("adding m6");
    depm.clear(); depm.push_back(m1);
    m6 = mg.addModel(u3, MT_IN, depm);
    BOOST_TEST_MESSAGE("adding m7");
    depm.clear(); depm.push_back(m2);
    m7 = mg.addModel(u3, MT_IN, depm);
    BOOST_TEST_MESSAGE("adding m8");
    depm.clear(); depm.push_back(m6);
    m8 = mg.addModel(u3, MT_OUT, depm);
    BOOST_TEST_MESSAGE("adding m9");
    depm.clear(); depm.push_back(m6);
    m9 = mg.addModel(u3, MT_OUT, depm);
    BOOST_TEST_MESSAGE("adding m10");
    depm.clear(); depm.push_back(m7);
    m10 = mg.addModel(u3, MT_OUT, depm);
    BOOST_TEST_MESSAGE("adding m11");
    depm.clear(); depm.push_back(m7);
    m11 = mg.addModel(u3, MT_OUT, depm);

    // u4
    BOOST_TEST_MESSAGE("adding m12");
    depm.clear(); depm.push_back(m5); depm.push_back(m10);
    m12 = mg.addModel(u4, MT_IN, depm);
    BOOST_TEST_MESSAGE("adding m13");
    depm.clear(); depm.push_back(m5); depm.push_back(m11);
    m13 = mg.addModel(u4, MT_IN, depm);
    BOOST_TEST_MESSAGE("adding m14");
    depm.clear(); depm.push_back(m12);
    m14 = mg.addModel(u4, MT_OUT, depm);
  }

  ~ModelGraphM2Fixture() {}
};

BOOST_AUTO_TEST_SUITE(root)

BOOST_FIXTURE_TEST_CASE(setup_eval_graph_e2, EvalGraphE2Fixture)
{
  BOOST_MESSAGE("TODO: check size of resulting graph or something like that");
}

BOOST_FIXTURE_TEST_CASE(setup_model_graph_m2, ModelGraphM2Fixture)
{
  BOOST_REQUIRE(mg.modelsAt(u2, MT_OUT).size() == 1);
  BOOST_CHECK(mg.modelsAt(u2, MT_OUT)[0] == m5);

  BOOST_REQUIRE(mg.modelsAt(u2, MT_IN).size() == 2);
  BOOST_CHECK(mg.modelsAt(u2, MT_IN)[0] == m3);
  BOOST_CHECK(mg.modelsAt(u2, MT_IN)[1] == m4);

  BOOST_CHECK(mg.propsOf(m10).location == u3);
  BOOST_CHECK(mg.propsOf(m10).type == MT_OUT);
}

BOOST_AUTO_TEST_SUITE_END()
