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

#include "EvalGraph.hpp"
#include "ModelGraph.hpp"

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
  typedef typename ModelGeneratorBase<InterpretationT>::Ptr ModelGeneratorPtr;
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
    public ModelGeneratorBase<TestInterpretation>
  {
  public:
    ModelGenerator(
        InterpretationConstPtr input,
        TestModelGeneratorFactory& factory):
      ModelGeneratorBase<TestInterpretation>(input) {}
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

template<typename T>
inline std::ostream& printopt_main(std::ostream& o, const T& t)
{
  if( !t )
    return o << "unset";
  else
    return o << t;
}
template<typename TT>
inline std::ostream& printopt_main(std::ostream& o, const boost::shared_ptr<TT>& t)
{
  if( t == 0 )
    return o << "null";
  else
    return o << t;
}

template<typename T>
struct printopt_container
{
  const T& t;
  printopt_container(const T& t): t(t) {}
};

template<typename T>
inline std::ostream& operator<<(std::ostream& o, printopt_container<T> c)
{
  return printopt_main(o, c.t);
}

template<typename T>
inline printopt_container<T> printopt(const T& t)
{
  return printopt_container<T>(t);
}

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
  typedef typename EvalGraphT::EvalUnitPropertyBundle EvalUnitPropertyBundle;
  typedef typename EvalUnitPropertyBundle::Interpretation Interpretation;
  typedef typename EvalUnitPropertyBundle::ProgramCtx ProgramCtx;
  typedef typename EvalUnitPropertyBundle::ModelGeneratorFactory
      ModelGeneratorFactory;

  // create a model graph suited to our needs
  struct ModelProcessingProperties
  {
    // whether we already tried to create all output models for this (MT_IN/MT_INPROJ) model
    bool childrenCreated;
  };
  typedef ModelGraph<EvalGraphT, ModelProcessingProperties> MyModelGraph;
  typedef typename MyModelGraph::Model Model;
  typedef typename MyModelGraph::ModelPropertyBundle ModelPropertyBundle;
  typedef typename boost::optional<Model> OptionalModel;
  typedef typename MyModelGraph::ModelDep ModelDep;

  // properties required at each eval unit for model building:
  // model generator factory
  // current models and refcount
  struct EvalUnitModelBuildingProperties
  {
    // storage

    // currently running model generator
    // (such a model generator is bound to some input model)
    // (it is reinitialized for each new input model)
    typename ModelGeneratorFactory::ModelGeneratorPtr currentmg;
    bool needInput;
		// this is the same as 'childrenGenerated' for Models,
		// but for eval units without inputs -> only valid if needInput == false
    bool modelsGenerated;
    OptionalModel imodel;
    OptionalModel omodel;
    unsigned orefcount;

    EvalUnitModelBuildingProperties():
      currentmg(), needInput(false), imodel(), omodel(), orefcount(0) {}

    std::ostream& print(std::ostream& o) const
    {
      return o <<
        "currentmg = " << printopt(currentmg) <<
        ", needInput = " << needInput <<
        ", imodel = " << printopt(imodel) <<
        ", omodel = " << printopt(omodel) <<
        ", orefcount = " << orefcount;
    }
    #if 0
    {
      std::cerr << "EvalUnitMBP: this = " << this << " currentmg = " << currentmg << std::endl;
    }
    EvalUnitModelBuildingProperties(const EvalUnitModelBuildingProperties& p):
      currentmg(p.currentmg), imodel(p.imodel), omodel(p.omodel), orefcount(p.orefcount)
    {
      std::cerr << "EvalUnitMBP: copyconstructing this = " << this << " from " << &p << " currentmg = " << currentmg << std::endl;
    }
    ~EvalUnitModelBuildingProperties()
    {
      std::cerr << "EvalUnitMBP: destructing this = " << this << "currentmg was " << currentmg << std::endl;
    }

    EvalUnitModelBuildingProperties& operator=(const EvalUnitModelBuildingProperties& p)
    {
      std::cerr << "EvalUnitMBP: assigning " << &p << " to this = " << this << "currentmg was " << currentmg << " new currentmg = " << p.currentmg << std::endl;
      return *this;
    }
    #endif
  };
  typedef boost::vector_property_map<EvalUnitModelBuildingProperties>
    EvalUnitModelBuildingPropertyMap;

  // members
private:
  EvalGraphT& eg;
  MyModelGraph mg;
  EvalUnitModelBuildingPropertyMap mbp; // aka. model building properties

  // methods
public:
  OnlineModelBuilder(EvalGraphT& eg):
    eg(eg), mg(eg), mbp()
  {
    // initialize mbp for each vertex in eg:
    // * determine needInput
    typename EvalGraphT::EvalUnitIterator it, end;
    for(boost::tie(it, end) = eg.getEvalUnits(); it != end; ++it)
    {
      EvalUnit u = *it;
      EvalUnitModelBuildingProperties& mbprops = mbp[u];
      typename EvalGraphT::PredecessorIterator it, end;
      boost::tie(it, end) = eg.getPredecessors(u);
      if( it != end )
        mbprops.needInput = true;
      else
        mbprops.needInput = false;
    }
  }

  ~OnlineModelBuilder() { }

  EvalGraphT& getEvalGraph() { return eg; }
  MyModelGraph& getModelGraph() { return mg; }

  // get next input model (projected if projection is configured) at unit u
  OptionalModel getNextIModel(EvalUnit u)
  {
    // TODO

    return OptionalModel();
  }

protected:
  std::ostream& printModelBuildingPropertyMap(std::ostream& o)
  {
    o << "mbp contents:";
    typename std::vector<EvalUnitModelBuildingProperties>::const_iterator it, end;
    unsigned u = 0;
    it = mbp.storage_begin();
    end = mbp.storage_end();
    if( it == end )
      return o << " empty" << std::endl;
    else
      o << std::endl;
    for(; it != end; ++it, ++u)
    {
      const EvalUnitModelBuildingProperties& uprop = *it;
      uprop.print(o << "  [" << u << "]=>") << std::endl;
    }
    return o << std::endl;
  }

public:

	/**
	 * we have two very different situations:
	 * 1) all omodels for that imodel have been generated
	 * 2) otherwise, which we can split into
	 * 2a) no model has been generated (-> no currentmg)
	 * 2b) some models have been generated (-> currentmg)
	 *
	 * if the imodel is not set (i.e., u does not need input) then the properties of
	 * the unit say whether all omodels have been generated or not
	 */
  OptionalModel getNextOModelForIModel(
      EvalUnit u,
      OptionalModel imodel,
      OptionalModel previousOModel)
  {
		EvalUnitModelBuildingProperties& umbprops = mbp[u];
		const EvalUnitPropertyBundle& uprops = eg.propsOf(u);
		if( !umbprops.needInput )
		{
			assert(!imodel); // an imodel is present iff we need input
			if( umbprops.modelsGenerated )
			{
				// we don't need input and all omodels exist
			}
			else
			{
				// we may need to generate some more models
				if( !umbprops.currentmg )
				{
					// we need a model generator
				}
				else
				{
					// we have a model generator
				}
			}
		}
		else
		{
			// uprops.needInput
			assert(!!imodel); // an imodel is present iff we need input
			const ModelPropertyBundle& mprops = mg.propsOf(imodel.get());
			if( mprops.childrenCreated )
			{
				// we have all omodels to this imodel
			}
			else
			{
				// we may need to generate some more omodels
				if( !umbprops.currentmg )
				{
					// we need a model generator
				}
				else
				{
					// we have a model generator
				}
			}
		}
		// todo: this might not be a good idea
		return OptionalModel();
  }

  // get next output model (projected if projection is configured) at unit u
  OptionalModel getNextOModel(EvalUnit u)
  {
    printModelBuildingPropertyMap(std::cerr) << std::endl;
    std::cerr << "OnlineModelBuilder<...>::getNextOModel(" << u << "):" << std::endl;
    const EvalUnitPropertyBundle& uprops = eg.propsOf(u);
    std::cerr << "  rules = '" << uprops.rules << "'" << std::endl;
    EvalUnitModelBuildingProperties& mbprops = mbp[u];
    mbprops.print(std::cerr) << std::endl;

    // are we allowed to go to the next model here?
    if( mbprops.orefcount > 1 )
    {
      // no -> give up our model refcount and return no model at all
      mbprops.orefcount--;
      mbprops.omodel = OptionalModel();
      return OptionalModel();
    }

    // initialization?
    if( !mbprops.imodel && mbprops.needInput )
    {
      assert(mbprops.orefcount == 0);
      // get next input for this unit (stores into mprops.imodel)
      getNextIModel(u);
      mbprops.omodel = OptionalModel();
    }
    do
    {
      // fail if there is no input at this point
      if( !mbprops.imodel && mbprops.needInput )
      {
        assert(mbprops.orefcount == 0);
        return OptionalModel();
      }

      // remember previous omodel
      OptionalModel previousModel = mbprops.omodel;

      // unregister usage of previous omodel if there is one
      if( !!mbprops.omodel )
      {
        assert(mbprops.orefcount == 1);
        mbprops.orefcount--;
        mbprops.omodel = OptionalModel();
      }

      // try to advance omodel (stores into mbprops.omodel)
      getNextOModelForIModel(u, mbprops.imodel, previousModel);
      if( !!mbprops.omodel )
      {
        // no next omodel found (stores into mbprops.imodel)
        getNextIModel(u);
      }
    }
    while( !mbprops.omodel );
    // register imodel/omodel here
    mbprops.orefcount++;
    assert(mbprops.orefcount == 1);
    return mbprops.omodel;
  }
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
  typedef OnlineModelBuilder<TestEvalGraph> ModelBuilder;
  typedef ModelBuilder::OptionalModel OptionalModel;

  ModelBuilder omb;
  EvalUnit ufinal;

  OnlineModelBuilderM2Fixture():
    EvalGraphE2Fixture(),
    omb(eg)
  {
    typedef TestEvalUnitPropertyBase UnitCfg;
    typedef TestEvalGraph::EvalUnitDepPropertyBundle UnitDepCfg;

    BOOST_TEST_MESSAGE("adding ufinal");
    ufinal = eg.addUnit(UnitCfg());
    BOOST_TEST_MESSAGE("adding dependencies from ufinal to all other models");
    eg.addDependency(ufinal, u1, UnitDepCfg(0));
    eg.addDependency(ufinal, u2, UnitDepCfg(1));
    eg.addDependency(ufinal, u3, UnitDepCfg(2));
    eg.addDependency(ufinal, u4, UnitDepCfg(3));
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
	TestModelGraph::ModelList::const_iterator it;

  BOOST_REQUIRE(mg.modelsAt(u2, MT_OUT).size() == 1);
	it = mg.modelsAt(u2, MT_OUT).begin();
  BOOST_CHECK(*it == m5);

  BOOST_REQUIRE(mg.modelsAt(u2, MT_IN).size() == 2);
	it = mg.modelsAt(u2, MT_IN).begin();
  BOOST_CHECK(*it == m3);
	++it;
  BOOST_CHECK(*it == m4);

  BOOST_CHECK(mg.propsOf(m10).location == u3);
  BOOST_CHECK(mg.propsOf(m10).type == MT_OUT);
}

BOOST_FIXTURE_TEST_CASE(online_model_building_m2, OnlineModelBuilderM2Fixture)
{
  BOOST_MESSAGE("requesting model");
  OptionalModel m = omb.getNextOModel(u1);
  BOOST_CHECK(!!m);
  BOOST_MESSAGE("TODO: check model contents");

}

BOOST_AUTO_TEST_SUITE_END()
