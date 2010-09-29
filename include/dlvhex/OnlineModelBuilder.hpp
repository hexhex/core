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
 * @file   OnlineModelBuilder.hpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Template for online model building of a ModelGraph based on an EvalGraph.
 */

#ifndef ONLINE_MODEL_BUILDER_HPP_INCLUDED__23092010
#define ONLINE_MODEL_BUILDER_HPP_INCLUDED__23092010

#include "Logger.hpp"
#include "EvalGraph.hpp"
#include "ModelGraph.hpp"
#include "ModelGenerator.hpp"

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
      assert(!(!!m && imodel != m && currentmg != 0));
      // log warning if we unset the imodel if currentmg is not null
      if( !m && imodel != m && currentmg != 0 )
      {
        LOG("WARNING: unsetting imodel while currentmg is null -> unsetting currentmg too");
        currentmg.reset();
      }
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
        << " -> "
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

private:
  typedef typename EvalGraphT::Observer EvalGraphObserverBase;
  class EvalGraphObserver:
    public EvalGraphObserverBase
  {
  public:
    EvalGraphObserver(Self& omb): omb(omb) {}
    virtual ~EvalGraphObserver() {}
    virtual void addUnit(EvalUnit u)
    {
      LOG("observing addUnit(" << u << ")");
      EvalUnitModelBuildingProperties& mbprops =
        omb.mbp[u];
      mbprops.needInput = false;
    }
    virtual void addDependency(EvalUnitDep d)
    {
      LOG("observing addDependency(" << omb.eg.sourceOf(d) << " -> " << omb.eg.targetOf(d) << ")");
      EvalUnitModelBuildingProperties& mbprops =
        omb.mbp[omb.eg.sourceOf(d)];
      mbprops.needInput = true;
    }

  protected:
    Self& omb;
  };

  // members
protected:
  EvalGraphT& eg;
  MyModelGraph mg;
  EvalUnitModelBuildingPropertyMap mbp; // aka. model building properties
  boost::shared_ptr<EvalGraphObserver> ego;

  // methods
public:
  OnlineModelBuilder(EvalGraphT& eg):
    eg(eg), mg(eg), mbp(),
    // setup observer to do the things below in case EvalGraph is changed
    // after the creation of this OnlineModelBuilder
    ego(new EvalGraphObserver(*this))
  {
    // allocate full mbp (plus one unit, as we will likely get an additional vertex)
    EvalUnitModelBuildingProperties& mbproptemp = mbp[eg.countEvalUnits()];
    (void)mbproptemp;

    // initialize mbp for each vertex in eg
    typename EvalGraphT::EvalUnitIterator it, end;
    for(boost::tie(it, end) = eg.getEvalUnits(); it != end; ++it)
    {
      EvalUnit u = *it;
      LOG("initializing mbp for unit " << u);
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
    eg.addObserver(ego);
  }

  virtual ~OnlineModelBuilder() { }

  inline EvalGraphT& getEvalGraph() { return eg; }
  inline MyModelGraph& getModelGraph() { return mg; }

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
  virtual OptionalModel getNextIModel(EvalUnit u);

  // get next output model (projected if projection is configured) at unit u
  virtual OptionalModel getNextOModel(EvalUnit u);

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

  // check if there is an existing model created from these predecessors
  OptionalModel oexisting = mg.getSuccessorIntersection(u, deps);
  if( !!oexisting )
  {
    LOG("found and will return existing successor imodel " << oexisting.get());
    return oexisting.get();
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

  const EvalUnitPropertyBundle& uprops = eg.propsOf(u);
  LOG("rules: " << uprops.ctx.rules);
  #endif

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

  const EvalUnitPropertyBundle& uprops = eg.propsOf(u);
  #endif

  EvalUnitModelBuildingProperties& mbprops = mbp[u];

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
      Model m = mg.sourceOf(*currentisuccessor);
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
      Model m = mg.sourceOf(*sbegin);
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

  const EvalUnitPropertyBundle& uprops = eg.propsOf(u);
  #endif

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

#endif // ONLINE_MODEL_BUILDER_HPP_INCLUDED__23092010
