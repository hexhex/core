#include <iostream>
#include <set>
#include <list>
#include <string>
#include <vector>
#include <cassert>

//#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/property_map/vector_property_map.hpp>
#define BOOST_TEST_MODULE __FILE__
#include <boost/test/included/unit_test.hpp>

template<typename EvalUnitPropertiesT, typename EvalUnitDepPropertiesT>
class EvalGraph
{
  // types
public:
  struct EvalUnitPropertyBundle
  {
    bool iproject;
    bool oproject;
    EvalUnitPropertiesT props;

    EvalUnitPropertyBundle():
        iproject(false), oproject(false), props() {}
    EvalUnitPropertyBundle(
      const EvalUnitPropertiesT& props):
        iproject(false), oproject(false), props(props) {}
    EvalUnitPropertyBundle(
      bool iproject,
      bool oproject,
      const EvalUnitPropertiesT& props = EvalUnitPropertiesT()):
        iproject(iproject), oproject(oproject), props(props) {}
  };
  struct EvalUnitDepPropertyBundle
  {
    int joinOrder;
    EvalUnitDepPropertiesT props;

    EvalUnitDepPropertyBundle():
      joinOrder(0), props() {}
    EvalUnitDepPropertyBundle(
      const EvalUnitDepPropertiesT& props):
        joinOrder(0), props(props) {}
    EvalUnitDepPropertyBundle(
      int joinOrder,
      const EvalUnitDepPropertiesT& props = EvalUnitDepPropertiesT()):
        joinOrder(joinOrder), props(props) {}
  };

private:
  // vecS is not bad, because we will not really add a huge amount of eval units, and we won't remove any units.
  // vecS is important, because it creates an implicit vertex index (and descriptors of vecS are integers)
  typedef boost::adjacency_list<
    boost::vecS, boost::vecS, boost::bidirectionalS,
    EvalUnitPropertyBundle, EvalUnitDepPropertyBundle> EvalGraphInt;
public:
  typedef typename EvalGraphInt::vertex_descriptor EvalUnit;
  typedef typename EvalGraphInt::edge_descriptor EvalUnitDep;

  // members
private:
  EvalGraphInt eg;

  // methods
public:
  EvalUnit addUnit(const EvalUnitPropertyBundle& prop)
  {
    return boost::add_vertex(prop, eg);
  }
  EvalUnitDep addDependency(const EvalUnit& u1, const EvalUnit& u2,
    const EvalUnitDepPropertyBundle& prop)
  {
    bool success;
    EvalUnitDep dep;
    boost::tie(dep, success) = boost::add_edge(u1, u2, prop, eg);
    assert(success &&
      "EvalGraph::addDependency failed (foreign EvalUnit?)");
    return dep;
  }
};

// for testing we use stupid types
typedef std::set<std::string> AtomSet;

class Interpretation
{
public:
  // create empty
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

template<typename EvalGraphT, typename ModelPropertiesT, typename ModelDepPropertiesT>
class ModelGraph
{
  // types
public:
  typedef typename EvalGraphT::EvalUnit EvalUnit;
  typedef typename EvalGraphT::EvalUnitDep EvalUnitDep;

  struct ModelPropertyBundle
  {
    // location of this model
    typename EvalGraphT::EvalUnit location;
    // type of this model
    ModelType type;
    // other properties not managed by ModelGraph
    ModelPropertiesT props;
  };

  struct ModelDepPropertyBundle
  {
    // join order
    int joinOrder;
    // other properties not managed by ModelGraph
    ModelDepPropertiesT props;
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

  // members
private:
  // which eval graph is this model graph linked to
  EvalGraphT& eg;
  ModelGraphInt mg;
  // "exterior property map" for the eval graph: which models are present at which unit
  EvalUnitModelsPropertyMap mau;

  // methods
public:
  // initialize with link to eval graph
  ModelGraph(EvalGraphT& eg):
    eg(eg) {}
  // add a model including dependencies
  // if compiled for debugging, checks if
  // * output models depend on input if input exists
  // * input models depend on all units this unit depends on
  // * model type is correct (no IN_PROJ if no projection is configured, ...)
  // * projected/nonprojected dependencies are correct
  // join order is automatically created from eval graph
  // modelsAtUnit is automatically updated
  Model addModel(
    EvalUnit location,
    ModelType type,
    const std::list<Model>& deps=std::list<Model>());

  // return helper map that stores for each unit the set of i/omodels there
  // usage: modelsAtUnit()[EvalUnit u].
  const ModelList& modelsAt(EvalUnit unit, ModelType type) const
  {
    assert(0 <= type <= 4);
    return boost::get(mau, unit).models[type];
  }
};

template<typename EvalGraphT, typename ModelPropertiesT, typename ModelDepPropertiesT>
typename ModelGraph<EvalGraphT, ModelPropertiesT, ModelDepPropertiesT>::Model
ModelGraph<EvalGraphT, ModelPropertiesT, ModelDepPropertiesT>::addModel(
  EvalUnit location,
  ModelType type,
  const std::list<Model>& deps)
{
  #ifdef DEBUG
  #endif
  // TODO
}

#if 0
template<EvaluationGraphT>
class ModelBuilder
{
  // this is an "exterior property map" for the eval graph
  // it stores model building data
  struct EvalUnitCurrentModel
  {
    Model imodel;
    Model omodel;
    int orefcount;
  };
  typedef std::vector<EvalUnitCurrentModel> EvalUnitCurrentModelPropertyMap;
  EvalUnitCurrentModelPropertyMap current;
#endif

struct none_t {};


//
// test types
//

// eval unit properties = rules
// dependency properties = nothing
typedef EvalGraph<std::string, none_t>
  TestEvalGraph;
typedef TestEvalGraph::EvalUnit EvalUnit; 
typedef TestEvalGraph::EvalUnitDep EvalUnitDep; 
typedef TestEvalGraph::EvalUnitPropertyBundle UnitCfg;
typedef TestEvalGraph::EvalUnitDepPropertyBundle UnitDepCfg;

// model properties = interpretation
// dependency properties = nothing
typedef ModelGraph<TestEvalGraph, Interpretation, none_t>
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
    u1 = eg.addUnit(UnitCfg("plan(a) v plan(b)."));
    u2 = eg.addUnit(UnitCfg("need(p,C) :- &cost[plan](C). :- need(_,money).")); 
    u3 = eg.addUnit(UnitCfg("use(X) v use(Y).")); 
    u4 = eg.addUnit(UnitCfg("need(u,C) :- &cost[use](C). :- need(_,money)."));
    e21 = eg.addDependency(u2, u1, UnitDepCfg(0));
    e31 = eg.addDependency(u3, u1, UnitDepCfg(0));
    e42 = eg.addDependency(u4, u2, UnitDepCfg(0));
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
    std::list<Model> depm;

    // u1
    m1 = mg.addModel(u1, MT_OUT);
    m2 = mg.addModel(u1, MT_OUT);

    // u2
    depm.clear(); depm.push_back(m1);
    m3 = mg.addModel(u2, MT_IN, depm);
    depm.clear(); depm.push_back(m2);
    m4 = mg.addModel(u2, MT_IN, depm);
    depm.clear(); depm.push_back(m4);
    m5 = mg.addModel(u2, MT_OUT, depm);

    // u3
    depm.clear(); depm.push_back(m1);
    m6 = mg.addModel(u3, MT_IN, depm);
    depm.clear(); depm.push_back(m2);
    m7 = mg.addModel(u3, MT_IN, depm);
    depm.clear(); depm.push_back(m6);
    m8 = mg.addModel(u3, MT_OUT, depm);
    depm.clear(); depm.push_back(m6);
    m9 = mg.addModel(u3, MT_OUT, depm);
    depm.clear(); depm.push_back(m7);
    m10 = mg.addModel(u3, MT_OUT, depm);
    depm.clear(); depm.push_back(m7);
    m11 = mg.addModel(u3, MT_OUT, depm);

    // u4
    depm.clear(); depm.push_back(m5); depm.push_back(m10);
    m12 = mg.addModel(u4, MT_IN, depm);
    depm.clear(); depm.push_back(m5); depm.push_back(m11);
    m13 = mg.addModel(u4, MT_IN, depm);
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
  BOOST_CHECK(*(mg.modelsAt(u2, MT_OUT).begin()) == m5);
}

BOOST_AUTO_TEST_SUITE_END()
