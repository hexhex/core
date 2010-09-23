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

#include <iostream>
#include <set>
#include <list>
#include <string>
#include <vector>
#include <cassert>

#include <boost/foreach.hpp>
//#include <boost/type_traits/remove_const.hpp>
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
#include "OnlineModelBuilder.hpp"

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

class TestInterpretation:
  public InterpretationBase
{
public:
  typedef boost::shared_ptr<TestInterpretation> Ptr;
  typedef boost::shared_ptr<const TestInterpretation> ConstPtr;

public:
  // create empty
  TestInterpretation(): atoms() {}
  // create from atom set
  TestInterpretation(const TestAtomSet& as): atoms(as) {}
  // destruct
  ~TestInterpretation() {}

	void add(const TestAtomSet& as)
	{
		atoms.insert(as.begin(), as.end());
	}

	void add(const TestInterpretation& i)
	{
		add(i.getAtoms());
	}

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

  inline const TestAtomSet& getAtoms() const
    { return atoms; };

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
      mit(models.begin())
    {
      LOG_METHOD("ModelGenerator()", this);
			const std::string& rules = factory.ctx.rules;
      LOG("rules '" << rules << "'");
      if( input )
        LOG("input '" << *input << "'");

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
      else if( rules == "need(p,C) :- &cost[plan](C). :- need(_,money)." )
      {
        assert(input);
        const TestAtomSet& inp = input->getAtoms();
        assert(inp.size() == 1);
        if( inp.count("plan(a)") == 1 )
        {
          // no models (constraint violated)
        }
        else if( inp.count("plan(b)") == 1 )
        {
          TestAtomSet a;
          a.insert("need(p,time)");
          models.push_back(TestInterpretation::Ptr(new TestInterpretation(a)));
          mit = models.begin();
        }
        else
        {
          assert(false);
        }
      }
      else if( rules == "use(X) v use(Y) :- plan(P), choose(P,X,Y). choose(a,c,d). choose(b,e,f)." )
      {
        assert(input);
        const TestAtomSet& inp = input->getAtoms();
        assert(inp.size() == 1);
        if( inp.count("plan(a)") == 1 )
        {
          TestAtomSet ma;
          ma.insert("use(c)");
          TestAtomSet mb;
          mb.insert("use(d)");
          models.push_back(TestInterpretation::Ptr(new TestInterpretation(ma)));
          models.push_back(TestInterpretation::Ptr(new TestInterpretation(mb)));
          mit = models.begin();
        }
        else if( inp.count("plan(b)") == 1 )
        {
          TestAtomSet ma;
          ma.insert("use(e)");
          TestAtomSet mb;
          mb.insert("use(f)");
          models.push_back(TestInterpretation::Ptr(new TestInterpretation(ma)));
          models.push_back(TestInterpretation::Ptr(new TestInterpretation(mb)));
          mit = models.begin();
        }
        else
        {
          assert(false);
        }
      }
      else if( rules == "need(u,C) :- &cost[use](C). :- need(_,money)." )
      {
        assert(input);
        const TestAtomSet& inp = input->getAtoms();
        assert(inp.size() == 2);
        if( inp.count("need(p,time)") == 1 && inp.count("use(e)") )
        {
          TestAtomSet ma;
          ma.insert("need(u,time)");
          models.push_back(TestInterpretation::Ptr(new TestInterpretation(ma)));
          mit = models.begin();
        }
        else if( inp.count("need(p,time)") == 1 && inp.count("use(f)") )
        {
          // no models (constraint violated)
        }
        else
        {
          assert(false);
        }
      }
      else
      {
        std::cerr << "TODO hardcode rules '" << rules << "'" << std::endl;
        assert(false);
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
      LOG_METHOD("generateNextModel()",this);
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

    // debug output
    virtual std::ostream& print(std::ostream& o) const
    {
			const std::string& rules = factory.ctx.rules;
      return o << "TestMGF::ModelGenerator with rules '" << rules << "'";
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
      TestInterpretation::ConstPtr input)
      //InterpretationConstPtr input)
  {
    LOG_METHOD("createModelGenerator()", this);
    LOG("input=" << printptr(input));
    return ModelGeneratorPtr(new ModelGenerator(input, *this));
  }

  // debug output
  virtual std::ostream& print(std::ostream& o) const
  {
    return o << "TestModelGeneratorFactory with rules '" << ctx.rules << "'";
  }
};

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
    LOG_SCOPE("EvalGraphE2Fixture", true);
    typedef TestEvalUnitPropertyBase UnitCfg;
    typedef TestEvalGraph::EvalUnitDepPropertyBundle UnitDepCfg;

    BOOST_TEST_MESSAGE("adding u1");
    u1 = eg.addUnit(UnitCfg("plan(a) v plan(b)."));
    LOG("u1 = " << u1);
    BOOST_TEST_MESSAGE("adding u2");
    u2 = eg.addUnit(UnitCfg("need(p,C) :- &cost[plan](C). :- need(_,money).")); 
    LOG("u2 = " << u2);
    BOOST_TEST_MESSAGE("adding u3");
    // u3: EDB will NOT be part of this in the real system, but here it is useful to know what's going on
    u3 = eg.addUnit(UnitCfg("use(X) v use(Y) :- plan(P), choose(P,X,Y). choose(a,c,d). choose(b,e,f)."));
    LOG("u3 = " << u3);
    BOOST_TEST_MESSAGE("adding u4");
    u4 = eg.addUnit(UnitCfg("need(u,C) :- &cost[use](C). :- need(_,money)."));
    LOG("u4 = " << u4);
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

// setup eval graph $\cE_2$ with different join order between u_2 and u_3 (switched)
struct EvalGraphE2MirroredFixture
{
  TestEvalGraph eg;
  EvalUnit u1, u2, u3, u4;
  EvalUnitDep e21, e31, e42, e43;

  EvalGraphE2MirroredFixture()
  {
    LOG_SCOPE("EvalGraphE2MirroredFixture", true);
    typedef TestEvalUnitPropertyBase UnitCfg;
    typedef TestEvalGraph::EvalUnitDepPropertyBundle UnitDepCfg;

    BOOST_TEST_MESSAGE("adding u1");
    u1 = eg.addUnit(UnitCfg("plan(a) v plan(b)."));
    LOG("u1 = " << u1);
    BOOST_TEST_MESSAGE("adding u2");
    u2 = eg.addUnit(UnitCfg("need(p,C) :- &cost[plan](C). :- need(_,money).")); 
    LOG("u2 = " << u2);
    BOOST_TEST_MESSAGE("adding u3");
    // u3: EDB will NOT be part of this in the real system, but here it is useful to know what's going on
    u3 = eg.addUnit(UnitCfg("use(X) v use(Y) :- plan(P), choose(P,X,Y). choose(a,c,d). choose(b,e,f)."));
    LOG("u3 = " << u3);
    BOOST_TEST_MESSAGE("adding u4");
    u4 = eg.addUnit(UnitCfg("need(u,C) :- &cost[use](C). :- need(_,money)."));
    LOG("u4 = " << u4);
    BOOST_TEST_MESSAGE("adding e21");
    e21 = eg.addDependency(u2, u1, UnitDepCfg(0));
    BOOST_TEST_MESSAGE("adding e31");
    e31 = eg.addDependency(u3, u1, UnitDepCfg(0));
    BOOST_TEST_MESSAGE("adding e43");
    e43 = eg.addDependency(u4, u3, UnitDepCfg(0));
    BOOST_TEST_MESSAGE("adding e42");
    e42 = eg.addDependency(u4, u2, UnitDepCfg(1));
  }

  ~EvalGraphE2MirroredFixture() {}
};

// setup model graph $\cM_2$ (including setup of eval graph $\cE_2$
struct ModelGraphM2Fixture:
  public EvalGraphE2Fixture
{
  TestModelGraph mg;
  Model dummyi1;
  Model m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14;

  ModelGraphM2Fixture():
    EvalGraphE2Fixture(),
    mg(eg)
  {
    std::vector<Model> depm;
    depm.reserve(2);

    // u1
    BOOST_TEST_MESSAGE("adding dummyi1");
    dummyi1 = mg.addModel(u1, MT_IN);
    BOOST_TEST_MESSAGE("adding m1");
    depm.clear(); depm.push_back(dummyi1);
    m1 = mg.addModel(u1, MT_OUT, depm);
    BOOST_TEST_MESSAGE("adding m2");
    depm.clear(); depm.push_back(dummyi1);
    m2 = mg.addModel(u1, MT_OUT, depm);

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

// test online model building algorithm with graph E2
template<typename EvalGraphE2BaseFixtureT>
struct OnlineModelBuilderE2TFixture:
  public EvalGraphE2BaseFixtureT
{
  typedef EvalGraphE2BaseFixtureT Base;
  typedef OnlineModelBuilder<TestEvalGraph> ModelBuilder;
  typedef ModelBuilder::OptionalModel OptionalModel;

  ModelBuilder omb;
  EvalUnit ufinal;

  OnlineModelBuilderE2TFixture():
    EvalGraphE2BaseFixtureT(),
    omb(Base::eg)
  {
    TestEvalGraph& eg = Base::eg;
    TestEvalGraph::EvalUnit& u1 = Base::u1;
    TestEvalGraph::EvalUnit& u2 = Base::u2;
    TestEvalGraph::EvalUnit& u3 = Base::u3;
    TestEvalGraph::EvalUnit& u4 = Base::u4;

    LOG_SCOPE("OnlineModelBuilderE2TFixture<...>", true);
    typedef TestEvalUnitPropertyBase UnitCfg;
    typedef TestEvalGraph::EvalUnitDepPropertyBundle UnitDepCfg;

    // setup final unit
    BOOST_TEST_MESSAGE("adding ufinal");
    LOG("ufinal = " << ufinal);
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

  ~OnlineModelBuilderE2TFixture() {}
};

// create one normal E2 model building fixture
typedef OnlineModelBuilderE2TFixture<EvalGraphE2Fixture>
  OnlineModelBuilderE2Fixture;
// create one E2 model building fixture with mirrored join order u2/u3
typedef OnlineModelBuilderE2TFixture<EvalGraphE2MirroredFixture>
  OnlineModelBuilderE2MirroredFixture;

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

BOOST_FIXTURE_TEST_CASE(online_model_building_e2_u1_output, OnlineModelBuilderE2Fixture)
{
  BOOST_MESSAGE("requesting model #1");
  OptionalModel m1 = omb.getNextOModel(u1);
  BOOST_REQUIRE(!!m1);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m1.get()).interpretation);
    BOOST_CHECK(ti.getAtoms().size() == 1);
    BOOST_CHECK(ti.getAtoms().count("plan(a)") == 1);
  }

  BOOST_MESSAGE("requesting model #2");
  OptionalModel m2 = omb.getNextOModel(u1);
  BOOST_REQUIRE(!!m2);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m2.get()).interpretation);
    BOOST_CHECK(ti.getAtoms().size() == 1);
    BOOST_CHECK(ti.getAtoms().count("plan(b)") == 1);
  }

  BOOST_MESSAGE("requesting model #3");
  OptionalModel nfm = omb.getNextOModel(u1);
  BOOST_REQUIRE(!nfm);
}

BOOST_FIXTURE_TEST_CASE(online_model_building_e2_u2_input, OnlineModelBuilderE2Fixture)
{
  BOOST_MESSAGE("requesting model #1");
  OptionalModel m3 = omb.getNextIModel(u2);
  BOOST_REQUIRE(!!m3);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m3.get()).interpretation);
    BOOST_CHECK(ti.getAtoms().size() == 1);
    BOOST_CHECK(ti.getAtoms().count("plan(a)") == 1);
  }

  BOOST_MESSAGE("requesting model #2");
  OptionalModel m4 = omb.getNextIModel(u2);
  BOOST_REQUIRE(!!m4);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m4.get()).interpretation);
    BOOST_CHECK(ti.getAtoms().size() == 1);
    BOOST_CHECK(ti.getAtoms().count("plan(b)") == 1);
  }

  BOOST_MESSAGE("requesting model #3");
  OptionalModel nfm = omb.getNextIModel(u2);
  BOOST_REQUIRE(!nfm);
}

BOOST_FIXTURE_TEST_CASE(online_model_building_e2_u3_input, OnlineModelBuilderE2Fixture)
{
  BOOST_MESSAGE("requesting model #1");
  OptionalModel m6 = omb.getNextIModel(u3);
  BOOST_REQUIRE(!!m6);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m6.get()).interpretation);
    BOOST_CHECK(ti.getAtoms().size() == 1);
    BOOST_CHECK(ti.getAtoms().count("plan(a)") == 1);
  }

  BOOST_MESSAGE("requesting model #2");
  OptionalModel m7 = omb.getNextIModel(u3);
  BOOST_REQUIRE(!!m7);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m7.get()).interpretation);
    BOOST_CHECK(ti.getAtoms().size() == 1);
    BOOST_CHECK(ti.getAtoms().count("plan(b)") == 1);
  }

  BOOST_MESSAGE("requesting model #3");
  OptionalModel nfm = omb.getNextIModel(u3);
  BOOST_REQUIRE(!nfm);
}

BOOST_FIXTURE_TEST_CASE(online_model_building_e2_u2_output, OnlineModelBuilderE2Fixture)
{
  BOOST_MESSAGE("requesting model #1");
  OptionalModel m5 = omb.getNextOModel(u2);
  BOOST_REQUIRE(!!m5);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m5.get()).interpretation);
    BOOST_CHECK(ti.getAtoms().size() == 1);
    BOOST_CHECK(ti.getAtoms().count("need(p,time)") == 1);
  }

  BOOST_MESSAGE("requesting model #2");
  OptionalModel nfm = omb.getNextOModel(u2);
  BOOST_REQUIRE(!nfm);
}

BOOST_FIXTURE_TEST_CASE(online_model_building_e2_u3_output, OnlineModelBuilderE2Fixture)
{
  BOOST_MESSAGE("requesting model #1");
  OptionalModel m8 = omb.getNextOModel(u3);
  BOOST_REQUIRE(!!m8);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m8.get()).interpretation);
    BOOST_CHECK(ti.getAtoms().size() == 1);
    BOOST_CHECK(ti.getAtoms().count("use(c)") == 1);
  }

  BOOST_MESSAGE("requesting model #2");
  OptionalModel m9 = omb.getNextOModel(u3);
  BOOST_REQUIRE(!!m9);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m9.get()).interpretation);
    BOOST_CHECK(ti.getAtoms().size() == 1);
    BOOST_CHECK(ti.getAtoms().count("use(d)") == 1);
  }

  BOOST_MESSAGE("requesting model #3");
  OptionalModel m10 = omb.getNextOModel(u3);
  BOOST_REQUIRE(!!m10);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m10.get()).interpretation);
    BOOST_CHECK(ti.getAtoms().size() == 1);
    BOOST_CHECK(ti.getAtoms().count("use(e)") == 1);
  }

  BOOST_MESSAGE("requesting model #4");
  OptionalModel m11 = omb.getNextOModel(u3);
  BOOST_REQUIRE(!!m11);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m11.get()).interpretation);
    BOOST_CHECK(ti.getAtoms().size() == 1);
    BOOST_CHECK(ti.getAtoms().count("use(f)") == 1);
  }

  BOOST_MESSAGE("requesting model #5");
  OptionalModel nfm = omb.getNextOModel(u3);
  BOOST_REQUIRE(!nfm);
}

BOOST_FIXTURE_TEST_CASE(online_model_building_e2_u4_input, OnlineModelBuilderE2Fixture)
{
  omb.logEvalGraphModelGraph();
  BOOST_MESSAGE("requesting model #1");
  OptionalModel m12 = omb.getNextIModel(u4);
  omb.logEvalGraphModelGraph();
  BOOST_REQUIRE(!!m12);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m12.get()).interpretation);
    BOOST_CHECK(ti.getAtoms().size() == 2);
    BOOST_CHECK(ti.getAtoms().count("need(p,time)") == 1);
    BOOST_CHECK(ti.getAtoms().count("use(e)") == 1);
  }

  BOOST_MESSAGE("requesting model #2");
  OptionalModel m13 = omb.getNextIModel(u4);
  omb.logEvalGraphModelGraph();
  BOOST_REQUIRE(!!m13);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m13.get()).interpretation);
    BOOST_CHECK(ti.getAtoms().size() == 2);
    BOOST_CHECK(ti.getAtoms().count("need(p,time)") == 1);
    BOOST_CHECK(ti.getAtoms().count("use(f)") == 1);
  }

  BOOST_MESSAGE("requesting model #3");
  OptionalModel nfm = omb.getNextIModel(u4);
  omb.logEvalGraphModelGraph();
  BOOST_REQUIRE(!nfm);
}

BOOST_FIXTURE_TEST_CASE(online_model_building_e2mirrored_u4_input, OnlineModelBuilderE2MirroredFixture)
{
  omb.logEvalGraphModelGraph();
  BOOST_MESSAGE("requesting model #1");
  OptionalModel m12 = omb.getNextIModel(u4);
  omb.logEvalGraphModelGraph();
  BOOST_REQUIRE(!!m12);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m12.get()).interpretation);
    BOOST_CHECK(ti.getAtoms().size() == 2);
    BOOST_CHECK(ti.getAtoms().count("need(p,time)") == 1);
    BOOST_CHECK(ti.getAtoms().count("use(e)") == 1);
  }

  BOOST_MESSAGE("requesting model #2");
  OptionalModel m13 = omb.getNextIModel(u4);
  omb.logEvalGraphModelGraph();
  BOOST_REQUIRE(!!m13);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m13.get()).interpretation);
    BOOST_CHECK(ti.getAtoms().size() == 2);
    BOOST_CHECK(ti.getAtoms().count("need(p,time)") == 1);
    BOOST_CHECK(ti.getAtoms().count("use(f)") == 1);
  }

  BOOST_MESSAGE("requesting model #3");
  OptionalModel nfm = omb.getNextIModel(u4);
  omb.logEvalGraphModelGraph();
  BOOST_REQUIRE(!nfm);
}

BOOST_FIXTURE_TEST_CASE(online_model_building_e2_u4_output, OnlineModelBuilderE2Fixture)
{
  omb.logEvalGraphModelGraph();
  BOOST_MESSAGE("requesting model #1");
  OptionalModel m14 = omb.getNextOModel(u4);
  omb.logEvalGraphModelGraph();
  BOOST_REQUIRE(!!m14);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(m14.get()).interpretation);
    BOOST_CHECK(ti.getAtoms().size() == 1);
    BOOST_CHECK(ti.getAtoms().count("need(u,time)") == 1);
  }

  BOOST_MESSAGE("requesting model #2");
  OptionalModel nfm = omb.getNextOModel(u4);
  omb.logEvalGraphModelGraph();
  BOOST_REQUIRE(!nfm);
}

BOOST_FIXTURE_TEST_CASE(online_model_building_e2_ufinal_input, OnlineModelBuilderE2Fixture)
{
  omb.logEvalGraphModelGraph();
  BOOST_MESSAGE("requesting model #1");
  OptionalModel mcomplete = omb.getNextIModel(ufinal);
  omb.logEvalGraphModelGraph();
  BOOST_REQUIRE(!!mcomplete);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(mcomplete.get()).interpretation);
    BOOST_CHECK(ti.getAtoms().size() == 4);
    BOOST_CHECK(ti.getAtoms().count("need(u,time)") == 1);
    BOOST_CHECK(ti.getAtoms().count("need(u,time)") == 1);
    BOOST_CHECK(ti.getAtoms().count("use(e)") == 1);
    BOOST_CHECK(ti.getAtoms().count("plan(b)") == 1);
  }

  BOOST_MESSAGE("requesting model #2");
  OptionalModel nfm = omb.getNextIModel(ufinal);
  omb.logEvalGraphModelGraph();
  BOOST_REQUIRE(!nfm);
}

BOOST_FIXTURE_TEST_CASE(online_model_building_e2mirrored_ufinal_input, OnlineModelBuilderE2MirroredFixture)
{
  omb.logEvalGraphModelGraph();
  BOOST_MESSAGE("requesting model #1");
  OptionalModel mcomplete = omb.getNextIModel(ufinal);
  omb.logEvalGraphModelGraph();
  BOOST_REQUIRE(!!mcomplete);
  {
    TestInterpretation& ti = *(omb.getModelGraph().propsOf(mcomplete.get()).interpretation);
    BOOST_CHECK(ti.getAtoms().size() == 4);
    BOOST_CHECK(ti.getAtoms().count("need(u,time)") == 1);
    BOOST_CHECK(ti.getAtoms().count("need(u,time)") == 1);
    BOOST_CHECK(ti.getAtoms().count("use(e)") == 1);
    BOOST_CHECK(ti.getAtoms().count("plan(b)") == 1);
  }

  BOOST_MESSAGE("requesting model #2");
  OptionalModel nfm = omb.getNextIModel(ufinal);
  omb.logEvalGraphModelGraph();
  BOOST_REQUIRE(!nfm);
}


BOOST_AUTO_TEST_SUITE_END()
