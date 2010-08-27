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
#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>
#define BOOST_TEST_MODULE __FILE__
#include <boost/test/included/unit_test.hpp>

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

private:
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
}; // class EvalGraph<...>

// for testing we use stupid types
typedef std::set<std::string> TestAtomSet;

class TestInterpretation
{
public:
  typedef boost::shared_ptr<TestInterpretation> Ptr;
  typedef boost::shared_ptr<const TestInterpretation> ConstPtr;

public:
  // create empty
  TestInterpretation(): atoms() {}
  // create from atom set
  TestInterpretation(const TestAtomSet& as): atoms(as) {}
  // create as union
  TestInterpretation(const TestInterpretation& i1, const TestInterpretation& i2);
  // destruct
  ~TestInterpretation() {}
  // output
  std::ostream& print(std::ostream& o) const
  {
    TestAtomSet::const_iterator it = atoms.begin();
    o << "{" << *it;
    ++it;
    for(;it != atoms.end(); ++it)
      o << "," << *it;
    o << "}";
    return o;
  }

private:
  TestAtomSet atoms;
}; // class Interpretation

// syntactic operator<< sugar for printing interpretations
std::ostream& operator<<(std::ostream& o, const TestInterpretation& i)
{
  return i.print(o);
}

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

//
// responsibility of a ProgramCtx class is to provide types of program and related objects
//

// the ProgramCtxTraits template checks if something is a ProgramCtx and gathers types
template<typename ProgramCtxT>
struct ProgramCtxTraits
{
  typedef typename ProgramCtxT::Rule Rule;
  typedef typename ProgramCtxT::Constraint Constraint;
};

// for testing we use stupid types
struct TestProgramCtx
{
  typedef std::string Rule;
  typedef std::string Constraint;
};

//
// A model generator does the following:
// * it is constructed by a ModelGeneratorFactory which knows the program
//   (and can precompute information for evaluation,
//    and may also provide this to the model generator)
// * it is evaluated on a (probably empty) input interpretation
// * this evaluation can be performed online
// * evaluation yields a (probably empty) set of output interpretations
//
template<typename InterpretationT>
class ModelGeneratorBase
{
  // types
public:
  typedef InterpretationT Interpretation;
  typedef boost::shared_ptr<ModelGeneratorBase<Interpretation> > Ptr;
  typedef typename Interpretation::Ptr InterpretationPtr;
  typedef typename Interpretation::ConstPtr InterpretationConstPtr;

  // storage
protected:
  InterpretationConstPtr input;

  // members
public:
  // initialize with factory and input interpretation
  ModelGeneratorBase(InterpretationConstPtr input):
    input(input) {}
  virtual ~ModelGeneratorBase() {}

  // generate and return next model, return null after last model
  virtual InterpretationPtr generateNextModel() = 0;
};

//
// a model generator factory provides model generators
// for a certain program ctx
//
template<typename InterpretationT, typename ProgramCtxT>
class ModelGeneratorFactoryBase
{
  // types
public:
  typedef InterpretationT Interpretation;
  typedef ProgramCtxT ProgramCtx;

public:
  typedef ModelGeneratorBase<InterpretationT> ModelGeneratorBase;
  typedef typename ModelGeneratorBase::Ptr ModelGeneratorPtr;
  typedef boost::shared_ptr<ModelGeneratorFactoryBase<InterpretationT, ProgramCtxT> > Ptr;

  // storage
public:
  const ProgramCtx& programCtx;

  // methods
public:
  ModelGeneratorFactoryBase(const ProgramCtx& programCtx):
    programCtx(programCtx) {}
  virtual ~ModelGeneratorFactoryBase() {}

  virtual ModelGeneratorPtr createModelGenerator(const Interpretation& input) = 0;
};

class TestModelGeneratorFactory:
  public ModelGeneratorFactoryBase<TestInterpretation, TestProgramCtx>
{
public:
  class ModelGenerator:
    public ModelGeneratorBase
  {
  public:
    ModelGenerator(
        InterpretationConstPtr input,
        TestModelGeneratorFactory& factory):
      ModelGeneratorBase(input) {}
    virtual ~ModelGenerator() {}

    virtual InterpretationPtr generateNextModel()
    {
      std::cerr << "wahoo generating next model!" << std::endl;
      // TODO
    }
  };

  virtual ModelGeneratorPtr createModelGenerator(
      ModelGenerator::InterpretationConstPtr input)
  {
    return ModelGeneratorPtr(new ModelGenerator(input, *this));
  }
};

// model generator factory properties for eval units
// such properties are required by model builders
template<typename InterpretationT, typename ProgramCtxT>
struct EvalUnitModelGeneratorFactoryProperties
{
  typedef InterpretationT Interpretation;
  typedef ProgramCtxT ProgramCtx;
  typedef ModelGeneratorFactoryBase<InterpretationT, ProgramCtxT>
    ModelGeneratorFactory;

  typename ModelGeneratorFactory::Ptr mgf; // aka model generator factory
};

// Decision help for "putting properties into the base bundle vs
// putting properties into extra property maps":
// * stuff that may be required for optimizing the EvalGraph
//   should go into the base bundles
// * stuff that is used for model building only (after the EvalGraph is fixed)
//   should go into extra property maps

//TODO: create base class ModelBuiderBase
template<typename EvalGraphT>
class OnlineModelBuilder
{
  // types
public:
  // concept check: EvalGraphT must be an eval graph
  BOOST_CONCEPT_ASSERT((boost::Convertible<
      EvalGraphT,
      EvalGraph<
        typename EvalGraphT::EvalUnitPropertyBase,
        typename EvalGraphT::EvalUnitDepPropertyBase> >));
  typedef typename EvalGraphT::EvalUnit EvalUnit;
  typedef typename EvalGraphT::EvalUnitDep EvalUnitDep;

  // concept check: eval graph must store model generator factory properties for units
  BOOST_CONCEPT_ASSERT((boost::Convertible<
      typename EvalGraphT::EvalUnitPropertyBundle,
      EvalUnitModelGeneratorFactoryProperties<
        typename EvalGraphT::EvalUnitPropertyBundle::Interpretation,
        typename EvalGraphT::EvalUnitPropertyBundle::ProgramCtx> >));
  typedef typename EvalGraphT::EvalUnitPropertyBundle::Interpretation Interpretation;
  typedef typename EvalGraphT::EvalUnitPropertyBundle::ProgramCtx ProgramCtx;
  typedef typename EvalGraphT::EvalUnitPropertyBundle::ModelGeneratorFactory
      ModelGeneratorFactory;

  // create a model graph suited to our needs
  typedef ModelGraph<EvalGraphT> ModelGraph;
  typedef typename ModelGraph::Model Model;
  typedef typename ModelGraph::ModelDep ModelDep;

  // properties required at each eval unit for model building:
  // model generator factory
  // current models and refcount
  struct EvalUnitModelBuildingProperties
  {
    // storage

    // factory for creating model generators
    // (such a factory is bound to the program at the corresponding eval unit)
    // (it is initialized once)
    typename ModelGeneratorFactory::Ptr mgfactory;
    // currently running model generator
    // (such a model generator is bound to some input model)
    // (it is reinitialized for each new input model)
    typename ModelGeneratorFactory::ModelGeneratorBase::Ptr currentmg;
    Model imodel;
    Model omodel;
    unsigned orefcount;
  };
  typedef boost::vector_property_map<EvalUnitModelBuildingProperties>
    EvalUnitModelBuildingPropertyMap;

  // members
private:
  EvalGraphT& eg;
  ModelGraph mg;
  EvalUnitModelBuildingPropertyMap mbd; // aka. model building data

  // methods
public:
  OnlineModelBuilder(EvalGraphT& eg):
    eg(eg), mg(eg), mbd()
  {
  }
  ~OnlineModelBuilder() { }
};

//
// test types
//

// TestEvalGraph
struct TestEvalUnitPropertyBase:
  public EvalUnitProjectionProperties,
  public EvalUnitModelGeneratorFactoryProperties<TestInterpretation, TestProgramCtx>
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

// TestModelGraph
struct TestModelPropertyBase
{
  // interpretation of the model
  TestInterpretation interpretation;

  TestModelPropertyBase() {}
  TestModelPropertyBase(const TestInterpretation& interpretation):
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
    typedef TestEvalUnitPropertyBase UnitCfg;
    typedef TestEvalGraph::EvalUnitDepPropertyBundle UnitDepCfg;

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

// test online model building algorithm with graph M2
struct OnlineModelBuilderM2Fixture:
  public EvalGraphE2Fixture
{
  OnlineModelBuilder<TestEvalGraph> omb;

  OnlineModelBuilderM2Fixture():
    EvalGraphE2Fixture(),
    omb(eg)
  {
  }

  ~OnlineModelBuilderM2Fixture() {}
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
