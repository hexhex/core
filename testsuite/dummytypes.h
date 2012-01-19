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
 * @file   dummytypes.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Dummy replacement types for testing (model building) templates.
 */

#ifndef DUMMYTYPES_HPP_INCLUDED__24092010
#define DUMMYTYPES_HPP_INCLUDED__24092010

#include "dlvhex2/Logger.h"
#include "dlvhex2/EvalGraph.h"
#include "dlvhex2/ModelGraph.h"
#include "dlvhex2/ModelGenerator.h"
#include <boost/test/unit_test.hpp>

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
  public dlvhex::InterpretationBase
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
    o << "{";
    if( !atoms.empty() )
    {
      o << *it;
      ++it;
      for(;it != atoms.end(); ++it)
        o << "," << *it;
    }
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
  public dlvhex::ModelGeneratorFactoryBase<TestInterpretation>
{
  //
  // types
  //
public:
  typedef dlvhex::ModelGeneratorFactoryBase<TestInterpretation>
		Base;

  class ModelGenerator:
    public dlvhex::ModelGeneratorBase<TestInterpretation>
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
        TestModelGeneratorFactory& factory);

    virtual ~ModelGenerator()
    {
      LOG_VSCOPE(INFO,"~ModelGenerator()",this,true);
    }

    virtual InterpretationPtr generateNextModel()
    {
			const std::string& rules = factory.ctx.rules;
      LOG_VSCOPE(INFO,"generateNextModel()",this,true);
      factory.generateNextModelCount++;
      LOG(INFO,"returning next model for rules '" << rules << "':");
      if( mit == models.end() )
      {
        LOG(INFO,"null");
        return InterpretationPtr();
      }
      else
      {
        InterpretationPtr ret = *mit;
        mit++;
        LOG(INFO,*ret);
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
  unsigned generateNextModelCount;

  //
  // members
  //
public:
  TestModelGeneratorFactory(const TestProgramCtx& ctx):
    ctx(ctx),
    generateNextModelCount(0)
  {
    LOG_VSCOPE(INFO,"TestModelGeneratorFactory()",this,true);
    LOG(INFO,"rules='" << ctx.rules << "'");
  }

  virtual ~TestModelGeneratorFactory()
  {
    LOG_VSCOPE(INFO,"~TestModelGeneratorFactory()",this,true);
    LOG(INFO,"generateNextModelCount=" << generateNextModelCount);
  }

  virtual ModelGeneratorPtr createModelGenerator(
      TestInterpretation::ConstPtr input)
      //InterpretationConstPtr input)
  {
    LOG_VSCOPE(INFO,"createModelGenerator()",this,true);
    LOG(INFO,"input=" << printptr(input));
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
  public dlvhex::EvalUnitProjectionProperties,
  public dlvhex::EvalUnitModelGeneratorFactoryProperties<TestInterpretation>
{
	TestProgramCtx ctx;

  TestEvalUnitPropertyBase():
		dlvhex::EvalUnitProjectionProperties(),
		dlvhex::EvalUnitModelGeneratorFactoryProperties<TestInterpretation>(),
		ctx("unset")
	{ }
  TestEvalUnitPropertyBase(const std::string& rules):
		dlvhex::EvalUnitProjectionProperties(),
		dlvhex::EvalUnitModelGeneratorFactoryProperties<TestInterpretation>(),
		ctx(rules)
	{ }
};

typedef dlvhex::EvalGraph<TestEvalUnitPropertyBase>
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

typedef dlvhex::ModelGraph<TestEvalGraph, TestModelPropertyBase>
  TestModelGraph;
typedef TestModelGraph::Model Model;
typedef TestModelGraph::ModelPropertyBundle ModelProp;
typedef TestModelGraph::ModelDep ModelDep;
typedef TestModelGraph::ModelDepPropertyBundle ModelDepProp;

template<typename EvalGraphT>
class CounterVerification
{
protected:
  EvalGraphT& eg;

  typedef std::map<TestEvalGraph::EvalUnit, unsigned> IterCountMap;
  std::vector<IterCountMap> counters;

public:
  CounterVerification(EvalGraphT& eg, unsigned iterations):
    eg(eg),
    counters(iterations + 1, IterCountMap())
  {
    recordCounters(0);
  }

  void recordCounters(unsigned iteration)
  {
    assert(iteration <= counters.size());

    LOG_SCOPE(INFO,"CounterVerification", false);

    LOG(INFO,"recording iteration " << iteration);
    typename EvalGraphT::EvalUnitIterator unit, unitsbegin, unitsend;
    boost::tie(unitsbegin, unitsend) = eg.getEvalUnits();
    for(unit = unitsbegin; unit != unitsend; ++unit)
    {
      boost::shared_ptr<TestModelGeneratorFactory> mgf =
        boost::dynamic_pointer_cast<TestModelGeneratorFactory>(
          eg.propsOf(*unit).mgf);
      if( !mgf )
      {
        LOG(INFO,"could not downcast mgf of unit " << *unit << "!");
      }
      else
      {
        counters[iteration][*unit] = mgf->generateNextModelCount;
        LOG(INFO,"initial counter of mgf of unit " << *unit << " = " << counters[iteration][*unit]);
      }
    }
  }

  void printCounters()
  {
    typename EvalGraphT::EvalUnitIterator unit, unitsbegin, unitsend;
    boost::tie(unitsbegin, unitsend) = eg.getEvalUnits();
    for(unsigned counteridx = 0; counteridx != counters.size(); ++counteridx)
    {
      LOG(INFO,"model generation counter for iteration " << counteridx << ":");
      LOG_INDENT(INFO);
      for(unit = unitsbegin; unit != unitsend; ++unit)
      {
        LOG(INFO,"u" << *unit << " -> " << counters[counteridx][*unit]);
      }
    }
  }

  void verifyEqual(unsigned iterationa, unsigned iterationb)
  {
    assert(iterationa < iterationb <= counters.size());
    typename EvalGraphT::EvalUnitIterator unit, unitsbegin, unitsend;
    boost::tie(unitsbegin, unitsend) = eg.getEvalUnits();
    for(unit = unitsbegin; unit != unitsend; ++unit)
    {
      BOOST_CHECK((counters[iterationa][*unit] - counters[iterationb][*unit]) == 0);
    }
  }
};

#endif // DUMMYTYPES_HPP_INCLUDED__24092010
