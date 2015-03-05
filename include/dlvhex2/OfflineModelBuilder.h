/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * Copyright (C) 2011, 2012, 2013 Christoph Redl
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
 * @file   OfflineModelBuilder.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Template for offline model building of a ModelGraph based on an EvalGraph.
 */

#ifndef OFFLINE_MODEL_BUILDER_HPP_INCLUDED__28092010
#define OFFLINE_MODEL_BUILDER_HPP_INCLUDED__28092010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/OnlineModelBuilder.h"
#include "dlvhex2/CAUAlgorithms.h"

DLVHEX_NAMESPACE_BEGIN

/** \brief Template for offline model building of a ModelGraph based on an EvalGraph. */
template<typename EvalGraphT>
class OfflineModelBuilder:
  public OnlineModelBuilder<EvalGraphT>
{
  // types
protected:
  typedef OnlineModelBuilder<EvalGraphT> Base;

public:
  typedef OfflineModelBuilder<EvalGraphT> Self;

  typedef typename Base::EvalUnit EvalUnit;
  typedef typename Base::EvalUnitDep EvalUnitDep;
  typedef typename Base::EvalUnitPropertyBundle EvalUnitPropertyBundle;
  typedef typename Base::MyModelGraph MyModelGraph;
  typedef typename Base::Model Model;
  typedef typename Base::ModelPropertyBundle ModelPropertyBundle;
  typedef typename Base::OptionalModel OptionalModel;
  typedef typename MyModelGraph::ModelList ModelList;
  typedef typename ModelList::const_iterator ModelListIterator;
  typedef boost::optional<ModelListIterator> OptionalModelListIterator;

protected:
  /** \brief Properties of offline model building. */
  struct OfflineModelBuildingProperties
  {
    /** \brief True if input models have been built. */
    bool builtIModels;
    /** \brief True if output models have been built. */
    bool builtOModels;

    // for non-joining iteration, we need this
    /** \brief Current input model. */
    OptionalModelListIterator currentIModel;
    /** \brief Current output model. */
    OptionalModelListIterator currentOModel;

    /** \brief Constructor. */
    OfflineModelBuildingProperties():
      builtIModels(false), builtOModels(false),
      currentIModel(), currentOModel() {}
  };
  typedef boost::vector_property_map<OfflineModelBuildingProperties>
    OfflineModelBuildingPropertyMap;

  // storage
protected:
  /** \brief Properties of models. */
  OfflineModelBuildingPropertyMap offmbp;
  /** \brief For each call we need this storage, this is not threadsafe! (but the rest of model building is unlikely to be threadsafe as well). */
  boost::optional<CAUAlgorithms::JoinRelevancePropertyMap> currentjrp;

  // methods
public:
  /** \brief Constructor.
    * @param cfg Configuration. */
  OfflineModelBuilder(ModelBuilderConfig<EvalGraphT>& cfg):
    Base(cfg), offmbp()
  {
    // allocate full mbp (plus one unit, as we will likely get an additional vertex)
    EvalGraphT& eg = cfg.eg;
    OfflineModelBuildingProperties& offmbproptemp = offmbp[eg.countEvalUnits()];
    (void)offmbproptemp;

    // (defaults for properties are ok)
  }
  /** \brief Destructor. */
  virtual ~OfflineModelBuilder() { }

  inline EvalGraphT& getEvalGraph() { return Base::getEvalGraph(); }
  inline MyModelGraph& getModelGraph() { return Base::getModelGraph(); }
  void printEvalGraphModelGraph(std::ostream& o) { Base::printEvalGraphModelGraph(o); }
  void printModelBuildingPropertyMap(std::ostream& o) { Base::printModelBuildingPropertyMap(o); }

  /** \brief Builds the input models at the given.
    * @param u Unit whose input models are to be computed. */
  virtual unsigned buildIModels(EvalUnit u);
  /** \brief Builds the output models at the given.
    * @param u Unit whose output models are to be computed. */
  virtual unsigned buildOModels(EvalUnit u);

  // automatically calls buildOModelsRecursively on any non-calculated predecessor
  virtual unsigned buildIModelsRecursively(EvalUnit u);
  // automatically calls buildIModelsRecursively if imodels are not calculated yet
  virtual unsigned buildOModelsRecursively(EvalUnit u);

protected:
  // get next input model (projected if projection is configured) at unit u
  virtual OptionalModel getNextIModel(EvalUnit u);
};

template<typename EvalGraphT>
unsigned OfflineModelBuilder<EvalGraphT>::buildIModels(
    typename OfflineModelBuilder<EvalGraphT>::EvalUnit u)
{
  LOG_VSCOPE(MODELB,"bIM",u,true);
  DBGLOG(DBG,"=OfflineModelBuilder<...>::buildIModels(" << u << ")");
  //const EvalUnitPropertyBundle& uprops = Base::eg.propsOf(u);
  //LOG("rules: " << uprops.ctx.rules);

  typename EvalGraphT::PredecessorIterator pbegin, pend;
  boost::tie(pbegin, pend) = Base::eg.getPredecessors(u);

  #ifndef NDEBUG
  typename EvalGraphT::PredecessorIterator pit;
  for(pit = pbegin; pit != pend; ++pit)
  {
    EvalUnit upred = Base::eg.targetOf(*pit);
    assert(offmbp[upred].builtOModels == true);
  }
  #endif

  assert(offmbp[u].builtIModels == false);

  // if no predecessors
  //   call Base::getNextIModel while it returns (dummy) models
  // if 1 predecessor
  //   call Self::getNextIModel with limit where nothing is relevant
  //   (this returns (simply linked) models without backtracking in the model graph)
  // otherwise
  //   calculate CAUs from u
  //   calculate join-relevant units
  //   call Self::getNextIModel while it returns models
  // mark u as calculated
  // return number of generated models

  unsigned modelcounter = 0;
  if( pbegin == pend )
  {
    // no predecessors -> create dummy models using base class functionality
    LOG(MODELB,"asking for (dummy) models");
    while(Base::getNextIModel(u) != boost::none)
      modelcounter++;
  }
  else if( (pbegin+1) == pend )
  {
    // one predecessor -> create jrp directly (no CAUAlgorithms required, although they would do the job)
    LOG(MODELB,"one predecessor, manually creating join relevance");
    assert(!currentjrp);

    CAUAlgorithms::JoinRelevancePropertyMap jr;
    CAUAlgorithms::initJoinRelevance(jr, Base::eg);
    currentjrp = jr;

    DBGLOG(DBG,"asking for imodels");
    while(Base::getNextIModel(u) != boost::none)
      modelcounter++;
    LOG(MODELB,"created " << modelcounter << " imodels");

    currentjrp = boost::none;
  }
  else
  {
    LOG(MODELB,"more than one predecessor -> using CAUAlgorithms");
    assert(!currentjrp);

    CAUAlgorithms::AncestryPropertyMap apm;
    std::set<EvalUnit> caus;
    CAUAlgorithms::findCAUs(caus, Base::eg, u, apm);
    CAUAlgorithms::logAPM(apm);
    CAUAlgorithms::JoinRelevancePropertyMap jr;
    CAUAlgorithms::markJoinRelevance(jr, Base::eg, u, caus, apm);
    CAUAlgorithms::logJRPM(jr);
    currentjrp = jr;

    DBGLOG(DBG,"asking for imodels");
    while(Base::getNextIModel(u) != boost::none)
      modelcounter++;
    LOG(MODELB,"created " << modelcounter << " imodels");

    currentjrp = boost::none;
  }

  offmbp[u].builtIModels = true;
  return modelcounter;
}

template<typename EvalGraphT>
unsigned OfflineModelBuilder<EvalGraphT>::buildOModels(
    EvalUnit u)
{
  LOG_VSCOPE(MODELB,"bOM",u,true);
  DBGLOG(DBG,"=OfflineModelBuilder<...>::buildOModels(" << u << ")");
  //const EvalUnitPropertyBundle& uprops = Base::eg.propsOf(u);
  //LOG("rules: " << uprops.ctx.rules);

  assert(offmbp[u].builtIModels == true);
  assert(offmbp[u].builtOModels == false);

  // for all imodels present
  //   call Base::getNextOModel while it returns models
  // mark u as calculated
  // return number of generated models

  unsigned modelcounter = 0;

  assert(!currentjrp);
  CAUAlgorithms::JoinRelevancePropertyMap jr;
  CAUAlgorithms::initJoinRelevance(jr, Base::eg);
  currentjrp = jr;

  DBGLOG(DBG,"asking for omodels");
  while(Base::getNextOModel(u) != boost::none)
    modelcounter++;
  LOG(MODELB,"created " << modelcounter << " omodels");

  currentjrp = boost::none;
  offmbp[u].builtOModels = true;
  return modelcounter;
}

// automatically calls buildOModelsRecursively on any non-calculated predecessor
template<typename EvalGraphT>
unsigned OfflineModelBuilder<EvalGraphT>::buildIModelsRecursively(
    EvalUnit u)
{
  LOG_VSCOPE(MODELB,"bIMR",u,true);
  DBGLOG(DBG,"=OfflineModelBuilder<...>::buildIModelsRecursively(" << u << ")@" << printptr(this));

  // no assertions here, we succeed if we already built the models
  if( offmbp[u].builtIModels == true )
  {
    unsigned count = Base::mg.modelsAt(u,MT_IN).size(); // TODO: how about iproject?
    LOG(MODELB,"already built -> counting " << count << " imodels");
    return count;
  }

  typename EvalGraphT::PredecessorIterator pbegin, pend;
  boost::tie(pbegin, pend) = Base::eg.getPredecessors(u);

  typename EvalGraphT::PredecessorIterator pit;
  for(pit = pbegin; pit != pend; ++pit)
  {
    EvalUnit upred = Base::eg.targetOf(*pit);
    if( offmbp[upred].builtOModels == false )
    {
      LOG(MODELB,"predecessor " << upred << " has no built omodels");
      unsigned count = buildOModelsRecursively(upred);
      LOG(MODELB,"built " << count << " models in predecessor");
    }
    else
    {
      LOG(MODELB,"predecessor " << upred << " has omodels");
    }
  }

  unsigned count = buildIModels(u);
  LOG(MODELB,"built " << count << " imodels here");
  return count;
}

template<typename EvalGraphT>
// automatically calls buildIModelsRecursively if imodels are not calculated yet
unsigned OfflineModelBuilder<EvalGraphT>::buildOModelsRecursively(
    EvalUnit u)
{
  LOG_VSCOPE(MODELB,"bOMR",u,true);
  DBGLOG(DBG,"=OfflineModelBuilder<...>::buildOModelsRecursively(" << u << ")@" << printptr(this));

  // no assertions here, we succeed if we already built the models
  if( offmbp[u].builtOModels == true )
  {
    unsigned count = Base::mg.modelsAt(u,MT_OUT).size(); // TODO: how about oproject?
    LOG(MODELB,"already built -> counting " << count << " omodels");
    return count;
  }

  if( offmbp[u].builtIModels == false )
  {
    LOG(MODELB,"have no imodels");
    unsigned count = buildIModelsRecursively(u);
    LOG(MODELB,"built " << count << " imodels here");
  }
  else
  {
    LOG(MODELB,"already have imodels");
  }
  
  unsigned count = buildOModels(u);
  LOG(MODELB,"built " << count << " omodels here");
  return count;
}

template<typename EvalGraphT>
typename OfflineModelBuilder<EvalGraphT>::OptionalModel
OfflineModelBuilder<EvalGraphT>::getNextIModel(
    EvalUnit u)
{
  LOG_VSCOPE(MODELB,"offgnIM",u,true);
  DBGLOG(DBG,"=OfflineModelBuilder<...>::getNextIModel(" << u << ")");
  //logEvalGraphModelGraph();

  assert(!!currentjrp);
  if( currentjrp.get()[u] )
  {
    LOG(MODELB,"join relevant");
    return Base::getNextIModel(u);
  }
  else
  {
    LOG(MODELB,"not join relevant");
    assert(offmbp[u].builtIModels);
    // TODO how about iproj?
    const ModelList& mlist = Base::mg.modelsAt(u, MT_IN);
    if( !!offmbp[u].currentIModel )
    {
      //assert(offmbp[u].currentIModel.get() != mlist.end());
      LOG(MODELB,"advancing iterator");
      //if( Base::mg.propsOf(*offmbp[u].currentIModel.get()).dummy == 1 )
      //  logEvalGraphModelGraph();
      offmbp[u].currentIModel.get()++;
    }
    else
    {
      LOG(MODELB,"initializing iterator");
      offmbp[u].currentIModel = mlist.begin();
    }
    if( offmbp[u].currentIModel.get() == mlist.end() )
    {
      LOG(MODELB,"no more models");
      Base::mbp[u].setIModel(boost::none);
      offmbp[u].currentIModel = boost::none;
      return boost::none;
    }
    else
    {
      Model m = *offmbp[u].currentIModel.get();
      Base::mbp[u].setIModel(m);
      LOG(MODELB,"got model " << m);
      return m;
    }
  }
}

DLVHEX_NAMESPACE_END

#endif // OFFLINE_MODEL_BUILDER_HPP_INCLUDED__28092010
