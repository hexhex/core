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

#include "Logger.hpp"
#include "EvalGraph.hpp"
#include "ModelGraph.hpp"
#include "ModelGenerator.hpp"

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
template<typename TT>
inline std::ostream& printopt_main(std::ostream& o, boost::optional<typename std::list<TT>::const_iterator> it)
{
  if( !it )
    return o << "unset";
  else
    return o << *it;
}
template<typename TT>
inline std::ostream& printopt_main(std::ostream& o, boost::optional<typename std::list<TT>::iterator> it)
{
  if( !it )
    return o << "unset";
  else
    return o << *it;
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

// ******************** //

// 'print_method' usage:
// if some class has a method "std::ostream& print(std::ostream&) const"
// and you have an object o of this type
// then you can simply do "std::cerr << ... << print_method(o) << ... " to print it

// TODO: refactor print_opt to use print_container as base class

template<typename T>
struct print_container
{
  const T& t;
  print_container(const T& t): t(t) {}
  virtual ~print_container() {}
  virtual std::ostream& print(std::ostream& o) const = 0;
};

template<typename T>
inline std::ostream& operator<<(std::ostream& o, const print_container<T>& c)
{
  return c.print(o);
}

template<typename T>
struct print_method_container:
  public print_container<T>
{
  //typedef std::ostream& (T::*PrintFn)(std::ostream&);
  //PrintFn fn;

  print_method_container(const T& t)://, PrintFn fn):
    print_container<T>(t) {} // , fn(fn) {}
  virtual ~print_method_container() {}
  virtual std::ostream& print(std::ostream& o) const
    { return print_container<T>::t.print(o); }
};

template<typename T>
inline print_method_container<T> print_method(const T& t)
{
  return print_method_container<T>(t);
}



// model generator factory properties for eval units
// such properties are required by model builders
template<typename InterpretationT>
struct EvalUnitModelGeneratorFactoryProperties
{
	typedef InterpretationT Interpretation;

  // aka model generator factory
  typename ModelGeneratorFactoryBase<InterpretationT>::Ptr
		mgf; // aka model generator factory
};

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

  Rule rules;

  TestProgramCtx(const Rule& rules): rules(rules)
	{
		//std::cerr << this << " TestProgramCtx()" << std::endl;
	}

  ~TestProgramCtx()
	{
		//std::cerr << this << " ~TestProgramCtx()" << std::endl;
	}
};








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

class TestModelGeneratorFactory:
  public ModelGeneratorFactoryBase<TestInterpretation>
{
  //
  // types
  //
public:
  typedef ModelGeneratorFactoryBase<TestInterpretation>
		Base;

  class ModelGenerator:
    public ModelGeneratorBase<TestInterpretation>
  {
  public:
    typedef std::list<TestInterpretation::Ptr>
			TestModelList;

		TestModelGeneratorFactory& factory;

    // list of models
    TestModelList models;
    // next output model
    TestModelList::iterator mit;

  public:
    ModelGenerator(
        InterpretationConstPtr input,
        TestModelGeneratorFactory& factory):
      ModelGeneratorBase<TestInterpretation>(input),
			factory(factory),
      models(),
      mit()
    {
      LOG_METHOD("ModelGenerator()", this);
			const std::string& rules = factory.ctx.rules;
      LOG("rules '" << rules << "'");

      // hardcode models of given programs with given inputs
      if( rules == "plan(a) v plan(b)." )
      {
        assert(!input);
        TestAtomSet ma;
        ma.insert("plan(a)");
        TestAtomSet mb;
        mb.insert("plan(b)");
        models.push_back(TestInterpretation::Ptr(new TestInterpretation(ma)));
        models.push_back(TestInterpretation::Ptr(new TestInterpretation(mb)));
        mit = models.begin();
      }
      else
      {
        std::cerr << "TODO hardcode rules '" << rules << "'" << std::endl;
      }

      LOG_INDENT();
      BOOST_FOREACH(TestInterpretation::Ptr intp, models)
        LOG("model " << *intp);
    }

    virtual ~ModelGenerator()
    {
      LOG_METHOD("~ModelGenerator()", this);
    }

    virtual InterpretationPtr generateNextModel()
    {
			const std::string& rules = factory.ctx.rules;
      LOG_METHOD("gNM",this);
      LOG("returning next model for rules '" << rules << "':");
      if( mit == models.end() )
      {
        LOG("null");
        return InterpretationPtr();
      }
      else
      {
        InterpretationPtr ret = *mit;
        mit++;
        LOG(*ret);
        return ret;
      }
    }
  };

  //
  // storage
  //
public:
	const TestProgramCtx& ctx;

  //
  // members
  //
public:
  TestModelGeneratorFactory(const TestProgramCtx& ctx):
    ctx(ctx)
  {
    LOG_METHOD("TestModelGeneratorFactory()", this);
    LOG("rules='" << ctx.rules << "'");
  }

  virtual ~TestModelGeneratorFactory()
  {
    LOG_METHOD("~TestModelGeneratorFactory()", this);
  }

  virtual ModelGeneratorPtr createModelGenerator(
      InterpretationConstPtr input)
  {
    LOG_METHOD("createModelGenerator()", this);
    LOG("input=" << printopt(input));
    return ModelGeneratorPtr(new ModelGenerator(input, *this));
  }
};

// Decision help for "putting properties into the base bundle vs
// putting properties into extra property maps":
// * stuff that may be required for optimizing the EvalGraph
//   should go into the base bundles
// * stuff that is used for model building only (after the EvalGraph is fixed)
//   should go into extra property maps


/*
template<
  typename EvalGraphT,
  typename ModelPropertyBaseT = none_t,
  typename ModelDepPropertyBaseT = none_t>
class OnlineModelBuildingModelGraph:
	public ModelGraph<EvalGraphT, ModelPropertyBaseT, ModelDepPropertyBaseT>
{
};
*/

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
        typename EvalGraphT::EvalUnitPropertyBundle::Interpretation> >));
  typedef typename EvalGraphT::EvalUnitPropertyBundle
		EvalUnitPropertyBundle;
  typedef typename EvalUnitPropertyBundle::Interpretation Interpretation;
  typedef typename EvalUnitPropertyBundle::Interpretation::Ptr InterpretationPtr;

  // create a model graph suited to our needs
  struct ModelProperties
  {
    // whether we already tried to create all output models for this (MT_IN/MT_INPROJ) model
    bool childrenCreated;
    // the interpretation data of this model
    InterpretationPtr interpretation;

    ModelProperties():
      childrenCreated(false), interpretation() {}
  };
  typedef ModelGraph<EvalGraphT, ModelProperties> MyModelGraph;
  typedef typename MyModelGraph::Model Model;
  typedef typename MyModelGraph::ModelPropertyBundle ModelPropertyBundle;
  typedef boost::optional<Model> OptionalModel;
  typedef boost::optional<typename MyModelGraph::ModelList::const_iterator>
    OptionalModelListIterator;
  typedef boost::optional<typename MyModelGraph::SuccessorIterator>
		OptionalModelSuccessorIterator;
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
  	typename ModelGeneratorBase<Interpretation>::Ptr currentmg;

    bool needInput;

    unsigned orefcount;

    // storage if needInput == true

		// imodel currently being present in iteration
    OptionalModel imodel;

		// successor of imodel
    OptionalModelSuccessorIterator omodel_s_current;

    // storage if needInput == false

		// this is the same as 'childrenGenerated' for Models,
		// but for eval units without inputs -> only valid if needInput == false
    bool modelsCreated;

    // iterator in mg.modelsAt(u, MT_OUT) or mg.modelsAt(u, MT_OUTPROJ)
		OptionalModelListIterator omodel_l_current;

    EvalUnitModelBuildingProperties():
      currentmg(), needInput(false), orefcount(0),
      imodel(), omodel_s_current(),
      omodel_l_current(), modelsCreated(false) {}

    /**
     * \brief advance the omodel to the next omodel
     *
     * two behaviors: needInput or !needInput
     * if needInput
     *   assert we have an imodel
     *   assert we have no omodel iterator
     *   if we have an omodel successor iterator:
     *     goto next and return it
     *   else
     *     get first omodel successor iterator to imodel and return it
     * else // if !needInput
     *   assert we have no imodel
     *   assert we have no omodel successor iterator
     *   if we have an omodel iterator:
     *     goto next and return it
     *   else
     *     get first and return it
     */
    OptionalModel advanceOModelToNextIfPossible(const MyModelGraph& mg);

    std::ostream& print(std::ostream& o) const
    {
      return o <<
        "currentmg = " << printopt(currentmg) <<
        ", needInput = " << needInput <<
        ", orefcount = " << orefcount <<
        ", imodel = " << printopt(imodel) <<
        ", omodel_s_current = " << printopt(omodel_s_current) <<
        ", omodel_l_current = " << printopt(omodel_l_current) <<
        ", modelsCreated = " << modelsCreated;
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
    typename std::vector<EvalUnitModelBuildingProperties>::const_iterator
			it, end;
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
   * nonrecursive "get next" wrt. a mandatory imodel
	 */
  OptionalModel advanceOModelForIModel(EvalUnit u);

	/**
   * nonrecursive "get next" without input
	 */
  OptionalModel advanceOModelWithoutInput(EvalUnit u);

	/**
   * nonrecursive "get next" wrt. an imodel or wrt. no input at all
   * we delegate to different methods for the case of needInput = yes vs needInput = no
	 */
  OptionalModel advanceOModel(EvalUnit u)
  {
		EvalUnitModelBuildingProperties& mbprops = mbp[u];
    assert(mbprops.orefcount <= 1);
    if( mbprops.needInput )
      // we need an imodel
      return advanceOModelForIModel(u);
    else
      return advanceOModelWithoutInput(u);
  }

  // get next output model (projected if projection is configured) at unit u
  OptionalModel getNextOModel(EvalUnit u);
};

/**
 * nonrecursive "get next" wrt. a mandatory imodel
 *
 * two situations:
 * 1) all omodels for that imodel have been generated
 *    -> use model graph only
 * 2) otherwise:
 *   a) no model has been generated (-> no currentmg)
 *      -> start model generator and get first model
 *   b) some models have been generated (-> currentmg)
 *      -> continue to use model generator currentmg
 */
template<typename EvalGraphT>
typename OnlineModelBuilder<EvalGraphT>::OptionalModel
OnlineModelBuilder<EvalGraphT>::advanceOModelForIModel(
    EvalUnit u)
{
  // TODO
}

/**
 * nonrecursive "get next" without input
 *
 * two situations:
 * 1) all omodels have been generated
 *    -> use model graph only
 * 2) otherwise:
 *   a) no model has been generated (-> no currentmg)
 *      -> start model generator and get first model
 *   b) some models have been generated (-> currentmg)
 *      -> continue to use model generator currentmg
 *
 * our strategy is as follows:
 * if possible, advance on model graph
 * if this yields no model
 *   if no model generator is running, start one
 *   use model generator
 */
template<typename EvalGraphT>
typename OnlineModelBuilder<EvalGraphT>::OptionalModel
OnlineModelBuilder<EvalGraphT>::advanceOModelWithoutInput(EvalUnit u)
{
  std::ostringstream dbgstr;
  dbgstr << "aOMwI[" << u << "]";
  LOG_METHOD(dbgstr.str(),this);
  LOG("=advanceOModelWithoutInput(" << u << ")");

  EvalUnitModelBuildingProperties& mbprops = mbp[u];
  const EvalUnitPropertyBundle& uprops = eg.propsOf(u);
  assert(!mbprops.needInput);
  assert(!mbprops.imodel);
  assert(!mbprops.omodel_s_current);

  typedef typename MyModelGraph::ModelList ModelList;
  const ModelList& rel_omodels = mg.relevantOModelsAt(u);

  if( !!mbprops.omodel_l_current )
  {
    // we have an omodel iterator
    LOG("omodel iterator is set");
    assert(mbprops.orefcount == 1);

    // try to advance iterator on model graph
    typename ModelList::const_iterator it =
      mbprops.omodel_l_current.get();
    it++;
    if( it != rel_omodels.end() )
    {
			LOG("advance successful");
      // advance was successful!
      mbprops.omodel_l_current = it;
      assert(mbprops.orefcount == 1);
      return *it;
    }
  }
  else
  {
    // we don't have an omodel iterator
		LOG("omodel iterator not set");
    assert(mbprops.orefcount == 0);

    if( !rel_omodels.empty() )
    {
      // but we have a nonempty list of models
      // use the first one
			LOG("omodels list is not empty");
      typename ModelList::const_iterator it = rel_omodels.begin();
      mbprops.omodel_l_current = it;
      mbprops.orefcount++;
      assert(mbprops.orefcount == 1);
      return *it;
    }
  }

  // here we know: we cannot advance on the model graph 
  
  // if we know that all models have been generated -> fail
  if( mbprops.modelsCreated )
  {
    LOG("all models have been created");
    mbprops.omodel_l_current = boost::none;
    mbprops.orefcount = 0;
    return boost::none;
  }

  // if not all models have been generated
  // -> create model generator if not existing
  // -> use model generator

  if( !mbprops.currentmg )
  {
    // mgf is of type ModelGeneratorFactory::Ptr
    LOG("creating model generator");
    mbprops.currentmg = eg.propsOf(u).mgf->createModelGenerator(
				typename Interpretation::ConstPtr());
  }

  // todo factorize model creation/storage?

  // use model generator to create new model
  LOG("generating next model");
  assert(mbprops.currentmg);
  InterpretationPtr intp =
    mbprops.currentmg->generateNextModel();

  if( intp )
  {
    // we got a new model
		LOG("got new model");

    // create model (no dependencies)
    // TODO: handle output projection here? (with no input ... there should not be anything to project?)
    assert(uprops.iproject == false);
    assert(uprops.oproject == false);
    Model m = mg.addModel(u, MT_OUT);

    // configure model
    mg.propsOf(m).interpretation = intp;

    // advance iterator to that model
    if( !mbprops.omodel_l_current )
    {
      LOG("starting at first model");
      mbprops.omodel_l_current = rel_omodels.begin();
      mbprops.orefcount++;
    }
    else
    {
			LOG("advancing model");
      assert(!!mbprops.omodel_l_current &&
					mbprops.omodel_l_current.get() != rel_omodels.end());
      mbprops.omodel_l_current.get()++;
      assert(!!mbprops.omodel_l_current &&
					mbprops.omodel_l_current.get() != rel_omodels.end());
    }
    assert(!!mbprops.omodel_l_current &&
				*(mbprops.omodel_l_current.get()) == m);
    assert(mbprops.orefcount == 1);
    LOG("returning model " << m);
    return m;
  }
  else
  {
    // no futher models for this model generator
    LOG("no further model");

    // mark this unit as finished for creating models
    mbprops.modelsCreated = true;

    // free model generator
    mbprops.currentmg.reset();

    // return failure
    mbprops.omodel_l_current = boost::none;
    mbprops.orefcount = 0;
		LOG("returning no model");
    return boost::none;
  }
}

/*

  if( !umbprops.needInput )
  {
    assert(!imodel); // an imodel is present iff we need input
    if( umbprops.modelsCreated )
    {
      // we don't need input and all omodels exist
          TODO: correctly factorize nextomodel to imodel vs nextomodel without imodel
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
*/

// get next output model (projected if projection is configured) at unit u
template<typename EvalGraphT>
typename OnlineModelBuilder<EvalGraphT>::OptionalModel
OnlineModelBuilder<EvalGraphT>::getNextOModel(
    EvalUnit u)
{
  std::ostringstream dbgstr;
  dbgstr << "gnOM[" << u << "] ";
  LOG_METHOD(dbgstr.str(), this);
  LOG("=OnlineModelBuilder<...>::getNextOModel(" << u << "):");
  const EvalUnitPropertyBundle& uprops = eg.propsOf(u);
  LOG("rules = '" << uprops.ctx.rules << "'");
  EvalUnitModelBuildingProperties& mbprops = mbp[u];
  LOG("mbprops = " << print_method(mbprops));

  // are we allowed to go to the next model here?
  if( mbprops.orefcount > 1 )
  {
    LOG("not allowed to continue because of orefcount > 1");
    // no -> give up our model refcount and return no model at all
    mbprops.orefcount--;
    /// @todo do we need to do the following here?
    /// mbprops.omodel = OptionalModel();
    return OptionalModel();
  }

  // initialization?
  if( !mbprops.imodel && mbprops.needInput )
  {
    LOG("getting next imodel (none present and we need one)");
    assert(mbprops.orefcount == 0);
    // get next input for this unit (stores into mprops.imodel)
    getNextIModel(u);
    assert(!mbprops.omodel_s_current);
    assert(!mbprops.omodel_l_current);
  }

  OptionalModel omodel;
  do
  {
    // fail if there is no input at this point
    if( !mbprops.imodel && mbprops.needInput )
    {
      LOG("failing with no input");
      assert(mbprops.orefcount == 0);
			return boost::none;
    }

    LOG("advancing omodel");
    // advance omodel, maybe advance to null model
    // advancing is only allowed if orefcount <= 1
    omodel = advanceOModel(u);
    if( !omodel )
    {
			if( mbprops.needInput )
			{
        LOG("no omodel and have input models -> advancing imodel");
				// no next omodel found
				// -> advance imodel (stores into mbprops.imodel)
				getNextIModel(u);
			}
			else
			{
        LOG("no omodel and do not need input models -> failing");
				return boost::none;
			}
    }
  }
  while( !omodel );
  assert(mbprops.orefcount == 1);
  LOG("returning omodel " << printopt(omodel));
  return omodel;
}


//
// test types
//

// TestEvalGraph
struct TestEvalUnitPropertyBase:
  public EvalUnitProjectionProperties,
  public EvalUnitModelGeneratorFactoryProperties<TestInterpretation>
{
	TestProgramCtx ctx;

  TestEvalUnitPropertyBase():
		EvalUnitProjectionProperties(),
		EvalUnitModelGeneratorFactoryProperties<TestInterpretation>(),
		ctx("unset")
	{ }
  TestEvalUnitPropertyBase(const std::string& rules):
		EvalUnitProjectionProperties(),
		EvalUnitModelGeneratorFactoryProperties<TestInterpretation>(),
		ctx(rules)
	{ }
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

    // setup final unit
    BOOST_TEST_MESSAGE("adding ufinal");
    ufinal = eg.addUnit(UnitCfg());
    BOOST_TEST_MESSAGE("adding dependencies from ufinal to all other models");
    eg.addDependency(ufinal, u1, UnitDepCfg(0));
    eg.addDependency(ufinal, u2, UnitDepCfg(1));
    eg.addDependency(ufinal, u3, UnitDepCfg(2));
    eg.addDependency(ufinal, u4, UnitDepCfg(3));

    // setup model generator factories
    eg.propsOf(u1).mgf.reset( 
      new TestModelGeneratorFactory(eg.propsOf(u1).ctx));
    eg.propsOf(u2).mgf.reset(
      new TestModelGeneratorFactory(eg.propsOf(u2).ctx));
    eg.propsOf(u3).mgf.reset(
      new TestModelGeneratorFactory(eg.propsOf(u3).ctx));
    eg.propsOf(u4).mgf.reset(
      new TestModelGeneratorFactory(eg.propsOf(u4).ctx));

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
  OptionalModel m1 = omb.getNextOModel(u1);
  BOOST_CHECK(!!m1);
  BOOST_MESSAGE("TODO: check model m1 contents");
  OptionalModel m2 = omb.getNextOModel(u1);
  BOOST_CHECK(!!m2);
  BOOST_MESSAGE("TODO: check model m2 contents");
  OptionalModel nom3 = omb.getNextOModel(u1);
  BOOST_CHECK(!nom3);
}

BOOST_AUTO_TEST_SUITE_END()
