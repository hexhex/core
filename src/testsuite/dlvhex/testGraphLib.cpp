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

class InterpretationBase
{
  // debug
  std::ostream& print(std::ostream& o) const
  { return o << "InterpretationBase::print() not overloaded"; }
};

// model generator factory properties for eval units
// such properties are required by model builders
template<typename InterpretationT>
struct EvalUnitModelGeneratorFactoryProperties
{
  BOOST_CONCEPT_ASSERT((boost::Convertible<InterpretationT, InterpretationBase>));
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

// Decision help for "putting properties into the base bundle vs
// putting properties into extra property maps":
// * stuff that may be required for optimizing the EvalGraph
//   should go into the base bundles
// * stuff that is used for model building only (after the EvalGraph is fixed)
//   should go into extra property maps


//TODO: create base class ModelBuiderBase?
template<typename EvalGraphT>
class OnlineModelBuilder
{
  // types
public:
  typedef OnlineModelBuilder<EvalGraphT>
    Self;

  // concept check: EvalGraphT must be an eval graph
  BOOST_CONCEPT_ASSERT((boost::Convertible<
      EvalGraphT,
      EvalGraph<
        typename EvalGraphT::EvalUnitPropertyBase,
        typename EvalGraphT::EvalUnitDepPropertyBase> >));
  typedef typename EvalGraphT::EvalUnit
    EvalUnit;
  typedef typename EvalGraphT::EvalUnitDep
    EvalUnitDep;

  // concept check: eval graph must store model generator factory properties for units
  BOOST_CONCEPT_ASSERT((boost::Convertible<
      typename EvalGraphT::EvalUnitPropertyBundle,
      EvalUnitModelGeneratorFactoryProperties<
        typename EvalGraphT::EvalUnitPropertyBundle::Interpretation> >));
  typedef typename EvalGraphT::EvalUnitPropertyBundle
		EvalUnitPropertyBundle;
  // from eval unit properties we get the interpretation type
  typedef typename EvalUnitPropertyBundle::Interpretation
    Interpretation;
  typedef typename EvalUnitPropertyBundle::Interpretation::Ptr
    InterpretationPtr;
  typedef typename EvalGraphT::PredecessorIterator
    EvalUnitPredecessorIterator;

  // we need special properties
  struct ModelProperties
  {
    // the interpretation data of this model
    InterpretationPtr interpretation;

    // for input models only:

    // whether this model is an input dummy for a root eval unit
    bool dummy;
    // whether we already tried to create all output models for this (MT_IN/MT_INPROJ) model
    bool childModelsGenerated;

    ModelProperties():
      interpretation(), dummy(false), childModelsGenerated(false) {}
    std::ostream& print(std::ostream& o) const
    {
      o <<
        "dummy=" << dummy <<
        ", childModelsGenerated=" << childModelsGenerated; 
      o <<
        ", interpretation=" << printptr(interpretation);
      if( interpretation )
        o << print_method(*interpretation);
      return o;
    }
  };

  typedef ModelGraph<EvalGraphT, ModelProperties>
    MyModelGraph;
  typedef typename MyModelGraph::Model
    Model;
  typedef typename MyModelGraph::ModelPropertyBundle
    ModelPropertyBundle;
  typedef boost::optional<Model>
    OptionalModel;
  typedef typename MyModelGraph::ModelList
    ModelList;
  typedef boost::optional<typename MyModelGraph::ModelList::const_iterator>
    OptionalModelListIterator;
  typedef typename MyModelGraph::PredecessorIterator
    ModelPredecessorIterator;
  typedef typename MyModelGraph::SuccessorIterator
    ModelSuccessorIterator;
  typedef boost::optional<typename MyModelGraph::SuccessorIterator>
		OptionalModelSuccessorIterator;
  typedef typename MyModelGraph::ModelDep
    ModelDep;

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

  protected:
		// imodel currently being present in iteration (dummy if !needInput)
    OptionalModel imodel;

  public:
		// current successor of imodel
    OptionalModelSuccessorIterator currentisuccessor;

    EvalUnitModelBuildingProperties():
      currentmg(), needInput(false), orefcount(0),
      imodel(), currentisuccessor()
			{}

    inline const OptionalModel& getIModel() const
    {
      return imodel;
    }

    void setIModel(OptionalModel m)
    {
      // we can change the imodel iff currentmg is null
      assert(!(imodel != m && currentmg != 0));
      imodel = m;
    }

    bool hasOModel() const
      { return !!currentisuccessor; }
  };
  typedef boost::vector_property_map<EvalUnitModelBuildingProperties>
    EvalUnitModelBuildingPropertyMap;

  // helper for printEUMBP
  std::ostream& printEUMBPhelper(
      std::ostream& o, const EvalUnitModelBuildingProperties& p) const
  {
    o <<
      "currentmg = " << std::setw(9) << printptr(p.currentmg) <<
      ", needInput = " << p.needInput <<
      ", orefcount = " << p.orefcount <<
      ", imodel = " << std::setw(9) << printopt(p.getIModel()) <<
      ", currentisuccessor = ";
    if( !!p.currentisuccessor )
      o << mg.sourceOf(*p.currentisuccessor.get())
        << "->"
        << mg.targetOf(*p.currentisuccessor.get());
    else
      o << "unset";
    return o;
  }

  print_container* printEUMBP(
      const EvalUnitModelBuildingProperties& p) const
  {
    return print_function(boost::bind(&Self::printEUMBPhelper, this, _1, p));
  }

  Model getOModel(const EvalUnitModelBuildingProperties& p) const
  {
    assert(!!p.currentisuccessor);
    return mg.sourceOf(*p.currentisuccessor.get());
  }

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
    // @todo: do something like this if eval units are added afterwards
    typename EvalGraphT::EvalUnitIterator it, end;
    for(boost::tie(it, end) = eg.getEvalUnits(); it != end; ++it)
    {
      EvalUnit u = *it;
      EvalUnitModelBuildingProperties& mbprops = mbp[u];
      EvalUnitPredecessorIterator it, end;
      boost::tie(it, end) = eg.getPredecessors(u);
      if( it != end )
        mbprops.needInput = true;
      else
      {
        mbprops.needInput = false;
        assert(!eg.propsOf(u).iproject);
      }
    }
  }

  ~OnlineModelBuilder() { }

  EvalGraphT& getEvalGraph() { return eg; }
  MyModelGraph& getModelGraph() { return mg; }

protected:
	// helper for getNextIModel
	Model createIModelFromPredecessorOModels(EvalUnit u);

	/**
   * nonrecursive "get next" wrt. a mandatory imodel
	 */
  OptionalModel advanceOModelForIModel(EvalUnit u);
  // helper for advanceOModelForIModel
  OptionalModel createNextModel(EvalUnit u);
  // helper for advanceOModelForIModel
  boost::optional<EvalUnitPredecessorIterator>
  ensureModelIncrement(EvalUnit u, EvalUnitPredecessorIterator cursor);

public:
  // get next input model (projected if projection is configured) at unit u
  OptionalModel getNextIModel(EvalUnit u);

  // get next output model (projected if projection is configured) at unit u
  OptionalModel getNextOModel(EvalUnit u);

  // debugging methods
public:
  #ifndef NDEBUG
  void logEvalGraphModelGraph();
  void logModelBuildingPropertyMap();
  #else
  inline void logEvalGraphModelGraph() {}
  inline void logModelBuildingPropertyMap() {}
  #endif
};

#ifndef NDEBUG
template<typename EvalGraphT>
void
OnlineModelBuilder<EvalGraphT>::logEvalGraphModelGraph()
{
  LOG_SCOPE("egmg", false);
  LOG("=eval graph/model graph");
  typename EvalGraphT::EvalUnitIterator uit, ubegin, uend;
  boost::tie(ubegin, uend) = eg.getEvalUnits();
  for(uit = ubegin; uit != uend; ++uit)
  {
    EvalUnit u = *uit;
    std::stringstream s; s << "u " << u;
    LOG_SCOPE(s.str(), false);
    LOG("=unit " << u);

    // EvalUnitProjectionProperties
    LOG("iproject = " << eg.propsOf(u).iproject << " oproject = " << eg.propsOf(u).oproject);

    // EvalUnitModelGeneratorFactoryProperties
    if( eg.propsOf(u).mgf )
    {
      LOG("model generator factory = " << printptr(eg.propsOf(u).mgf) <<
          ":" << print_method(*eg.propsOf(u).mgf));
    }
    else
    {
      LOG("no model generator factory");
    }

    // unit dependencies
    typename EvalGraphT::PredecessorIterator pit, pbegin, pend;
    boost::tie(pbegin, pend) = eg.getPredecessors(u);
    for(pit = pbegin; pit != pend; ++pit)
    {
      LOG("-> depends on unit " << eg.targetOf(*pit) << "/join order " << eg.propsOf(*pit).joinOrder);
    }

    // models
    LOG_SCOPE("models", false);
    for(ModelType t = MT_IN; t <= MT_OUTPROJ; ++t)
    {
      const ModelList& modelsAt = mg.modelsAt(u, t);
      typename MyModelGraph::ModelList::const_iterator mit;
      for(mit = modelsAt.begin(); mit != modelsAt.end(); ++mit)
      {
        Model m = *mit;
        LOG(toString(t) << "@" << m << ": " << print_method(mg.propsOf(m)));
        // model dependencies (preds)
        ModelPredecessorIterator pit, pbegin, pend;
        boost::tie(pbegin, pend) = mg.getPredecessors(m);
        for(pit = pbegin; pit != pend; ++pit)
        {
          LOG("-> depends on model " << mg.targetOf(*pit) << "/join order " << mg.propsOf(*pit).joinOrder);
        }
        // model dependencies (succs)
        ModelSuccessorIterator sit, sbegin, send;
        boost::tie(sbegin, send) = mg.getSuccessors(m);
        for(sit = sbegin; sit != send; ++sit)
        {
          LOG("<- input for model  " << mg.sourceOf(*sit) << "/join order " << mg.propsOf(*sit).joinOrder);
        }
      }
      if( modelsAt.empty() )
        LOG(toString(t) << " empty");
    }
  }
}

template<typename EvalGraphT>
void
OnlineModelBuilder<EvalGraphT>::logModelBuildingPropertyMap()
{
  LOG_SCOPE("mbp", false);
  LOG("=model building property map");
  typename std::vector<EvalUnitModelBuildingProperties>::const_iterator
    it, end;
  unsigned u = 0;
  it = mbp.storage_begin();
  end = mbp.storage_end();
  if( it == end )
  {
    LOG("empty");
  }
  else
  {
    for(; it != end; ++it, ++u)
    {
      const EvalUnitModelBuildingProperties& uprop = *it;
      LOG(u << "=>" << printEUMBP(uprop));
    }
  }
}
#endif

template<typename EvalGraphT>
typename OnlineModelBuilder<EvalGraphT>::Model
OnlineModelBuilder<EvalGraphT>::createIModelFromPredecessorOModels(
    EvalUnit u)
{
  LOG_FUNCTION("cIMfPOM"); // only called from within object -> do not log this ptr
  LOG("=OnlineModelBuilder<...>::createIModelFromPredecessorOModels(" << u << ")");

	// create vector of dependencies
	std::vector<Model> deps;
	typename EvalGraphT::PredecessorIterator pit, pend;
	boost::tie(pit, pend) = eg.getPredecessors(u);
	for(; pit != pend; ++pit)
	{
		EvalUnit pred = eg.targetOf(*pit);
		EvalUnitModelBuildingProperties& predmbprops = mbp[pred];
		LOG("found predecessor unit " << pred << " with current omodel mbprops: " << printEUMBP(predmbprops));
		Model predmodel = getOModel(predmbprops);
		deps.push_back(predmodel);
	}
  
  // create interpretation
  InterpretationPtr pjoin;
  if( deps.size() == 1 )
  {
    // only link
    LOG("only one predecessor -> linking to omodel");
		pjoin = mg.propsOf(deps.front()).interpretation;
		assert(pjoin != 0);
  }
  else
  {
    // create joined interpretation
    LOG("more than one predecessor -> joining omodels");
    pjoin = InterpretationPtr(new Interpretation);
    LOG("new interpretation = " << printptr(pjoin));
    typename std::vector<Model>::const_iterator it;
    for(it = deps.begin(); it != deps.end(); ++it)
    {
      InterpretationPtr predinterpretation = mg.propsOf(*it).interpretation;
      LOG("predecessor omodel " << *it <<
          " has interpretation " << printptr(predinterpretation) <<
          " with contents " << print_method(*predinterpretation));
      assert(predinterpretation != 0);
      pjoin->add(*predinterpretation);
      LOG("pjoin now has contents " << print_method(*pjoin));
    }
  }

	// create model
	Model m = mg.addModel(u, MT_IN, deps);
	LOG("returning new MT_IN model " << m);
  mg.propsOf(m).interpretation = pjoin;
	return m;
}

// helper for advanceOModelForIModel
// TODO: comments from hexeval.tex
template<typename EvalGraphT>
boost::optional<typename OnlineModelBuilder<EvalGraphT>::EvalUnitPredecessorIterator>
OnlineModelBuilder<EvalGraphT>::ensureModelIncrement(
    EvalUnit u, typename OnlineModelBuilder<EvalGraphT>::EvalUnitPredecessorIterator cursor)
{
  #ifndef NDEBUG
  typename EvalGraphT::EvalUnit ucursor1 =
    eg.targetOf(*cursor);
  std::ostringstream dbgstr;
  dbgstr << "eMI[" << u << "," << ucursor1 << "]";
  LOG_FUNCTION(dbgstr.str()); // only called from within object -> do not log this ptr
  LOG("=OnlineModelBuilder<...>::ensureModelIncrement(" << u << "," << ucursor1 << ")");
  #endif

  EvalUnitPredecessorIterator pbegin =
    eg.getPredecessors(u).first;
  do
  {
    typename EvalGraphT::EvalUnit ucursor =
      eg.targetOf(*cursor);
    #ifndef NDEBUG
    EvalUnitModelBuildingProperties& ucursor_mbprops =
      mbp[ucursor];
    LOG("ucursor = " << ucursor << " with mbprops = {" << printEUMBP(ucursor_mbprops) << "}");
    assert(ucursor_mbprops.hasOModel());
    assert(ucursor_mbprops.orefcount >= 1);
    #endif

    OptionalModel om = getNextOModel(ucursor);
    if( !om )
    {
      LOG("advancing failed");
      if( cursor == pbegin )
      {
        LOG("cannot advance previous, returning null cursor");
        return boost::none;
      }
      else
      {
        LOG("trying to advance previous");
        cursor--;
      }
    }
    else
      break;
  }
  while(true);

  #ifndef NDEBUG
  typename EvalGraphT::EvalUnit ucursor2 =
    eg.targetOf(*cursor);
  EvalUnitModelBuildingProperties& ucursor2_mbprops =
    mbp[ucursor2];
  LOG("returning cursor: unit = " << ucursor2 << " with mbprops = {" << printEUMBP(ucursor2_mbprops) << "}");
  assert(ucursor2_mbprops.hasOModel());
  #endif
  return cursor;
}

/*
 * TODO get documentation from hexeval.tex
 */
template<typename EvalGraphT>
typename OnlineModelBuilder<EvalGraphT>::OptionalModel
OnlineModelBuilder<EvalGraphT>::getNextIModel(
    EvalUnit u)
{
  #ifndef NDEBUG
  std::ostringstream dbgstr;
  dbgstr << "gnIM[" << u << "]";
  LOG_METHOD(dbgstr.str(),this);
  LOG("=OnlineModelBuilder<...>::getNextIModel(" << u << ")");
  logModelBuildingPropertyMap();
  #endif

  const EvalUnitPropertyBundle& uprops = eg.propsOf(u);
  LOG("rules: " << uprops.ctx.rules);
  EvalUnitModelBuildingProperties& mbprops = mbp[u];
  LOG("mbprops: " << printEUMBP(mbprops));

  // did we have an imodel upon function entry?
  bool hadIModel = !!mbprops.getIModel();

  // dummy handling for units without input
  if( !mbprops.needInput )
  {
    LOG("unit needs no input");
    OptionalModel odummy;
    if( hadIModel )
    {
      LOG("removing dummy model and failing");
      odummy = boost::none;
    }
    else
    {
      Model dummy;
      if( mg.modelsAt(u, MT_IN).empty() )
      {
        dummy = mg.addModel(u, MT_IN);
        mg.propsOf(dummy).dummy = true;
        LOG("setting new dummy model " << dummy);
      }
      else
      {
        dummy = mg.modelsAt(u, MT_IN).front();
        assert(mg.propsOf(dummy).dummy);
        LOG("setting existing dummy model " << dummy);
      }
      odummy = dummy;
    }
    mbprops.setIModel(odummy);
    LOG("returning model " << printopt(odummy));
      logModelBuildingPropertyMap();
    return odummy;
  }

  LOG("unit needs input");

  // prepare cursor handling
  typename EvalGraphT::PredecessorIterator pbegin, pend;
  typename EvalGraphT::PredecessorIterator cursor;
  boost::tie(pbegin, pend) = eg.getPredecessors(u);

  if( hadIModel )
  {
    LOG("have imodel -> phase 1");
    boost::optional<EvalUnitPredecessorIterator> ncursor =
      ensureModelIncrement(u, pend - 1);
    if( !ncursor )
    {
      LOG("got null cursor, returning no imodel");
      mbprops.setIModel(boost::none);
      logModelBuildingPropertyMap();
      return boost::none;
    }
    else
    {
      LOG("got some increment");
      cursor = ncursor.get();
    }
    // if( cursor == (pend - 1) )
    // "cursor++;" will increment it to pend
    // phase 2 loop will not be executed
    // model will be created and returned
    cursor++;
  }
  else
  {
    cursor = pbegin;
  }
  
  // now, cursor is index of first unit where we do not hold a refcount 
  LOG("phase 2");

  while(cursor != pend)
  {
    typename EvalGraphT::EvalUnit ucursor =
      eg.targetOf(*cursor);
    EvalUnitModelBuildingProperties& ucursor_mbprops =
      mbp[ucursor];
    if( ucursor_mbprops.hasOModel() )
    {
      LOG("predecessor " << ucursor <<
          " has omodel " << mg.sourceOf(*ucursor_mbprops.currentisuccessor.get()) <<
          " with refcount " << ucursor_mbprops.orefcount);
      ucursor_mbprops.orefcount++;
    }
    else
    {
      LOG("predecessor " << ucursor << " has no omodel");
      OptionalModel om = getNextOModel(ucursor);
      LOG("got next omodel " << printopt(om) << " at unit " << ucursor);
      if( !om )
      {
        if( cursor == pbegin )
        {
          LOG("backtracking impossible, returning no imodel");
          mbprops.setIModel(boost::none);
          logModelBuildingPropertyMap();
          return boost::none;
        }
        else
        {
          LOG("backtracking");
          boost::optional<EvalUnitPredecessorIterator> ncursor =
            ensureModelIncrement(u, cursor - 1);
          if( !ncursor )
          {
            LOG("got null cursor, returning no imodel");
            mbprops.setIModel(boost::none);
            logModelBuildingPropertyMap();
            return boost::none;
          }
          else
          {
            LOG("backtracking was successful");
            cursor = ncursor.get();
          }
        }
      }
    }
    cursor++;
  } // while(cursor != pend)

  LOG("found full input model!");
  Model im = createIModelFromPredecessorOModels(u);
  LOG("returning newly created imodel " << im);
  mbprops.setIModel(im);
  logModelBuildingPropertyMap();
  return im;
}

// [checks if model generation is still possible given current input model]
// [checks if no model is currently stored as current omodel]
// if no model generator is running
//   determines input interpretation
//   start model generator
// get next model from model generator
// if successful
//   create in model graph
//   return model
// else
//   set finished for model generation
//   return null
template<typename EvalGraphT>
typename OnlineModelBuilder<EvalGraphT>::OptionalModel
OnlineModelBuilder<EvalGraphT>::createNextModel(
    EvalUnit u)
{
  #ifndef NDEBUG
  std::ostringstream dbgstr;
  dbgstr << "cNM[" << u << "]";
  LOG_FUNCTION(dbgstr.str()); // only called from within object -> do not log this ptr
  LOG("=createNextModel(" << u << ")");
  #endif

  EvalUnitModelBuildingProperties& mbprops = mbp[u];
  const EvalUnitPropertyBundle& uprops = eg.propsOf(u);

  #ifndef NDEBUG
  // check if there can be a next model
  assert(!!mbprops.getIModel());
  assert(!mg.propsOf(mbprops.getIModel().get()).childModelsGenerated);
  assert(!mbprops.currentisuccessor);
  assert(mbprops.orefcount == 0);
  #endif

  if( !mbprops.currentmg )
  {
    LOG("no model generator running");

    // determine input
    typename Interpretation::ConstPtr input;
    // input for creating model comes from current imodel
    // (this may be a dummy, so interpretation may be NULL which is ok)
    input = mg.propsOf(mbprops.getIModel().get()).interpretation;

    // mgf is of type ModelGeneratorFactory::Ptr
    LOG("creating model generator");
    mbprops.currentmg =
      eg.propsOf(u).mgf->createModelGenerator(input);
  }

  // use model generator to create new model
  LOG("generating next model");
  assert(mbprops.currentmg);
  InterpretationPtr intp =
    mbprops.currentmg->generateNextModel();

  if( intp )
  {
    // create model
    std::vector<Model> deps;
    deps.push_back(mbprops.getIModel().get());
    Model m = mg.addModel(u, MT_OUT, deps);
    // we got a new model
    LOG("stored new model " << m);

    // configure model
    mg.propsOf(m).interpretation = intp;

    // TODO: handle projection here?
    assert(uprops.iproject == false);
    assert(uprops.oproject == false);

    LOG("setting currentisuccessor iterator");
    ModelSuccessorIterator sbegin, send;
    boost::tie(sbegin, send) = mg.getSuccessors(mbprops.getIModel().get());
    /*{
      for(ModelSuccessorIterator it = sbegin; it != send; ++it)
      {
        LOG("found successor " << mg.sourceOf(*it));
      }
    }*/
    ModelSuccessorIterator sit = send;
    sit--;
    assert(mg.sourceOf(*sit) == m);
    mbprops.currentisuccessor = sit;

    LOG("setting refcount to 1");
    mbprops.orefcount = 1;
    LOG("returning model " << m);
    return m;
  }
  else
  {
    // no further models for this model generator
    LOG("no further model");

    // mark this input model as finished for creating models
    ModelPropertyBundle& imodelprops = mg.propsOf(mbprops.getIModel().get());
    imodelprops.childModelsGenerated = true;

    // free model generator
    mbprops.currentmg.reset();
    LOG("returning no model");
    return boost::none;
  }
}

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
 *
 * our strategy is as follows:
 * advance on model graph if possible
 * if this yields no model and not all models have been generated
 *   if no model generator is running, start one
 *   use model generator
 */
template<typename EvalGraphT>
typename OnlineModelBuilder<EvalGraphT>::OptionalModel
OnlineModelBuilder<EvalGraphT>::advanceOModelForIModel(
    EvalUnit u)
{
  #ifndef NDEBUG
  std::ostringstream dbgstr;
  dbgstr << "aOMfIM[" << u << "]";
  LOG_FUNCTION(dbgstr.str()); // only called from within object -> do not log this ptr
  LOG("=OnlineModelBuilder<...>::advanceOModelForIModel(" << u << ")");
  #endif

  // prepare
  EvalUnitModelBuildingProperties& mbprops = mbp[u];
  const EvalUnitPropertyBundle& uprops = eg.propsOf(u);
  assert(mbprops.orefcount <= 1);
  assert(!!mbprops.getIModel());

  // get imodel + properties
  Model imodel = mbprops.getIModel().get(); // Model == void* -> no ref!
  ModelPropertyBundle& imodelprops = mg.propsOf(imodel);
  LOG("have imodel " << imodel << ": " << print_method(imodelprops));

  // get successor list of imodel
  ModelSuccessorIterator sbegin, send;
  boost::tie(sbegin, send) = mg.getSuccessors(imodel);
  if( sbegin != send )
    LOG("imodel has at least one successor");

  LOG("trying to advance on model graph");
  if( !!mbprops.currentisuccessor )
  {
    LOG("currentisuccessor is set");
    assert(mbprops.orefcount == 1);

    ModelSuccessorIterator& currentisuccessor = mbprops.currentisuccessor.get();
    assert(currentisuccessor != send);
    currentisuccessor++;
    if( currentisuccessor != send )
    {
      Model m = mg.targetOf(*currentisuccessor);
      LOG("advance successful, returning model " << m);
      return m;
    }
    else
    {
      LOG("resetting iterator");
      // reset iterator here because we cannot be sure that it can
      // point to a "current" model anymore, and we need to set it anew
      // anyways in case we create a new model below
      mbprops.currentisuccessor = boost::none;
      mbprops.orefcount = 0;
    }
  }
  else
  {
    LOG("currentisuccessor not set");
    assert(mbprops.orefcount == 0);

    if( sbegin != send )
    {
      LOG("there are successors -> using them");
      mbprops.currentisuccessor = sbegin;
      mbprops.orefcount++;
      assert(mbprops.orefcount == 1);
      Model m = mg.targetOf(*sbegin);
      LOG("returning first successor model " << m);
      return m;
    }
  }

  // here we know: we cannot advance on the model graph 
  LOG("advancing on model graph failed");
  assert(!mbprops.currentisuccessor);
  assert(mbprops.orefcount == 0);

  if( imodelprops.childModelsGenerated )
  {
    LOG("all successors created -> returning no model");
    return boost::none;
  }

  // here, not all models have been generated
  // -> create model generator if not existing
  // -> use model generator

  LOG("attempting to create new model");
  OptionalModel m = createNextModel(u);
  LOG("returning model " << printopt(m));
  return m;
}

// get next output model (projected if projection is configured) at unit u
template<typename EvalGraphT>
typename OnlineModelBuilder<EvalGraphT>::OptionalModel
OnlineModelBuilder<EvalGraphT>::getNextOModel(
    EvalUnit u)
{
  #ifndef NDEBUG
  std::ostringstream dbgstr;
  dbgstr << "gnOM[" << u << "]";
  LOG_METHOD(dbgstr.str(), this);
  LOG("=OnlineModelBuilder<...>::getNextOModel(" << u << "):");
  #endif

  const EvalUnitPropertyBundle& uprops = eg.propsOf(u);
  logModelBuildingPropertyMap();
  LOG("rules = '" << uprops.ctx.rules << "'");
  EvalUnitModelBuildingProperties& mbprops = mbp[u];
  LOG("mbprops = " << printEUMBP(mbprops));

  // are we allowed to go to the next model here?
  if( mbprops.orefcount > 1 )
  {
    LOG("not allowed to continue because of orefcount > 1");
    // no -> give up our model refcount and return no model at all
    mbprops.orefcount--;
    logModelBuildingPropertyMap();
    return OptionalModel();
  }

  // initialization?
  if( !mbprops.getIModel() )
  {
    LOG("getting next imodel (none present and we need one)");
    assert(mbprops.orefcount == 0);
    // get next input for this unit (stores into mprops.imodel)
    getNextIModel(u);
    assert(!mbprops.currentisuccessor);
  }

  OptionalModel omodel;
  do
  {
    // fail if there is no input at this point
    if( !mbprops.getIModel() )
    {
      LOG("failing with no input");
      assert(mbprops.orefcount == 0);
      logModelBuildingPropertyMap();
			return boost::none;
    }

    LOG("advancing omodel");
    // advance omodel, maybe advance to null model
    // advancing is only allowed if orefcount <= 1
    omodel = advanceOModelForIModel(u);
    if( !omodel )
    {
      LOG("no omodel and have input models -> advancing imodel");
      // no next omodel found
      // -> advance imodel (stores into mbprops.imodel)
      getNextIModel(u);
    }
  }
  while( !omodel );
  assert(mbprops.orefcount == 1);
  LOG("returning omodel " << printopt(omodel));
  logModelBuildingPropertyMap();
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

#if 0
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
#endif

#if 1
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
#endif

BOOST_AUTO_TEST_SUITE_END()
