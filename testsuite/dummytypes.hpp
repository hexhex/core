/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
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

/**
 * @file   dummytypes.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Dummy replacement types for testing (model building) templates.
 */

#ifndef DUMMYTYPES_HPP_INCLUDED__24092010
#define DUMMYTYPES_HPP_INCLUDED__24092010

#include "dlvhex/Logger.hpp"
#include "dlvhex/EvalGraph.hpp"
#include "dlvhex/ModelGraph.hpp"
#include "dlvhex/ModelGenerator.hpp"

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
inline std::ostream& operator<<(std::ostream& o, const TestInterpretation& i)
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

#endif // DUMMYTYPES_HPP_INCLUDED__24092010
