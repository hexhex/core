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
 * @file   OfflineModelBuilder.hpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Template for offline model building of a ModelGraph based on an EvalGraph.
 */

#ifndef OFFLINE_MODEL_BUILDER_HPP_INCLUDED__28092010
#define OFFLINE_MODEL_BUILDER_HPP_INCLUDED__28092010

#include "dlvhex/Logger.hpp"
#include "dlvhex/EvalGraph.hpp"
#include "dlvhex/ModelGraph.hpp"
#include "dlvhex/ModelGenerator.hpp"
#include "dlvhex/OnlineModelBuilder.hpp"
#include "dlvhex/CAUAlgorithms.hpp"

template<typename EvalGraphT>
class OfflineModelBuilder:
  protected OnlineModelBuilder<EvalGraphT>
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
  struct OfflineModelBuildingProperties
  {
    bool builtIModels;
    bool builtOModels;

    // for non-joining iteration, we need this
    OptionalModelListIterator currentIModel;
    OptionalModelListIterator currentOModel;

    OfflineModelBuildingProperties():
      builtIModels(false), builtOModels(false),
      currentIModel(), currentOModel() {}
  };
  typedef boost::vector_property_map<OfflineModelBuildingProperties>
    OfflineModelBuildingPropertyMap;

  // storage
protected:
  OfflineModelBuildingPropertyMap offmbp;
  // TODO: for each call we need this storage, this is not threadsafe! (but the rest of model building is unlikely to be threadsafe as well)
  boost::optional<CAUAlgorithms::JoinRelevancePropertyMap> currentjrp;

  // methods
public:
  OfflineModelBuilder(EvalGraphT& eg):
    Base(eg), offmbp()
  {
    // allocate full mbp (plus one unit, as we will likely get an additional vertex)
    OfflineModelBuildingProperties& offmbproptemp = offmbp[eg.countEvalUnits()];
    (void)offmbproptemp;

    // (defaults for properties are ok)
  }

  virtual ~OfflineModelBuilder() { }

  inline EvalGraphT& getEvalGraph() { return Base::getEvalGraph(); }
  inline MyModelGraph& getModelGraph() { return Base::getModelGraph(); }
  void printEvalGraphModelGraph(std::ostream& o) { Base::printEvalGraphModelGraph(o); }
  void printModelBuildingPropertyMap(std::ostream& o) { Base::printModelBuildingPropertyMap(o); }

  virtual unsigned buildIModels(EvalUnit u);
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
  #ifndef NDEBUG
  std::ostringstream dbgstr;
  dbgstr << "bIM[" << u << "]";
  LOG_METHOD(dbgstr.str(),this);
  LOG("=OfflineModelBuilder<...>::buildIModels(" << u << ")");
  //const EvalUnitPropertyBundle& uprops = Base::eg.propsOf(u);
  //LOG("rules: " << uprops.ctx.rules);
  #endif

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
    LOG("asking for (dummy) models");
    while(Base::getNextIModel(u) != boost::none)
      modelcounter++;
  }
  else if( (pbegin+1) == pend )
  {
    // one predecessor -> create jrp directly (no CAUAlgorithms required, although they would do the job)
    LOG("one predecessor, manually creating join relevance");
    assert(!currentjrp);

    CAUAlgorithms::JoinRelevancePropertyMap jr;
    CAUAlgorithms::initJoinRelevance(jr, Base::eg);
    currentjrp = jr;

    LOG("asking for imodels");
    while(Base::getNextIModel(u) != boost::none)
      modelcounter++;
    LOG("created " << modelcounter << " imodels");

    currentjrp = boost::none;
  }
  else
  {
    LOG("more than one predecessor -> using CAUAlgorithms");
    assert(!currentjrp);

    CAUAlgorithms::AncestryPropertyMap apm;
    std::set<EvalUnit> caus;
    CAUAlgorithms::findCAUs(caus, Base::eg, u, apm);
    CAUAlgorithms::logAPM(apm);
    CAUAlgorithms::JoinRelevancePropertyMap jr;
    CAUAlgorithms::markJoinRelevance(jr, Base::eg, u, caus, apm);
    CAUAlgorithms::logJRPM(jr);
    currentjrp = jr;

    LOG("asking for imodels");
    while(Base::getNextIModel(u) != boost::none)
      modelcounter++;
    LOG("created " << modelcounter << " imodels");

    currentjrp = boost::none;
  }

  offmbp[u].builtIModels = true;
  return modelcounter;
}

template<typename EvalGraphT>
unsigned OfflineModelBuilder<EvalGraphT>::buildOModels(
    EvalUnit u)
{
  #ifndef NDEBUG
  std::ostringstream dbgstr;
  dbgstr << "bOM[" << u << "]";
  LOG_METHOD(dbgstr.str(),this);
  LOG("=OfflineModelBuilder<...>::buildOModels(" << u << ")");
  //const EvalUnitPropertyBundle& uprops = Base::eg.propsOf(u);
  //LOG("rules: " << uprops.ctx.rules);
  #endif

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

  LOG("asking for omodels");
  while(Base::getNextOModel(u) != boost::none)
    modelcounter++;
  LOG("created " << modelcounter << " omodels");

  currentjrp = boost::none;
  offmbp[u].builtOModels = true;
  return modelcounter;
}

// automatically calls buildOModelsRecursively on any non-calculated predecessor
template<typename EvalGraphT>
unsigned OfflineModelBuilder<EvalGraphT>::buildIModelsRecursively(
    EvalUnit u)
{
  #ifndef NDEBUG
  std::ostringstream dbgstr;
  dbgstr << "bIMR[" << u << "]";
  LOG_FUNCTION(dbgstr.str());
  LOG("=OfflineModelBuilder<...>::buildIModelsRecursively(" << u << ")@" << printptr(this));
  #endif

  // no assertions here, we succeed if we already built the models
  if( offmbp[u].builtIModels == true )
  {
    unsigned count = Base::mg.modelsAt(u,MT_IN).size(); // TODO: how about iproject?
    LOG("already built -> counting " << count << " imodels");
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
      LOG("predecessor " << upred << " has no built omodels");
      unsigned count = buildOModelsRecursively(upred);
      LOG("built " << count << " models in predecessor");
    }
    else
    {
      LOG("predecessor " << upred << " has omodels");
    }
  }

  unsigned count = buildIModels(u);
  LOG("built " << count << " imodels here");
  return count;
}

template<typename EvalGraphT>
// automatically calls buildIModelsRecursively if imodels are not calculated yet
unsigned OfflineModelBuilder<EvalGraphT>::buildOModelsRecursively(
    EvalUnit u)
{
  #ifndef NDEBUG
  std::ostringstream dbgstr;
  dbgstr << "bOMR[" << u << "]";
  LOG_FUNCTION(dbgstr.str());
  LOG("=OfflineModelBuilder<...>::buildOModelsRecursively(" << u << ")@" << printptr(this));
  #endif

  // no assertions here, we succeed if we already built the models
  if( offmbp[u].builtOModels == true )
  {
    unsigned count = Base::mg.modelsAt(u,MT_OUT).size(); // TODO: how about oproject?
    LOG("already built -> counting " << count << " omodels");
    return count;
  }

  if( offmbp[u].builtIModels == false )
  {
    LOG("have no imodels");
    unsigned count = buildIModelsRecursively(u);
    LOG("built " << count << " imodels here");
  }
  else
  {
    LOG("already have imodels");
  }
  
  unsigned count = buildOModels(u);
  LOG("built " << count << " omodels here");
  return count;
}

template<typename EvalGraphT>
typename OfflineModelBuilder<EvalGraphT>::OptionalModel
OfflineModelBuilder<EvalGraphT>::getNextIModel(
    EvalUnit u)
{
  #ifndef NDEBUG
  std::ostringstream dbgstr;
  dbgstr << "offgnIM[" << u << "]";
  LOG_FUNCTION(dbgstr.str());
  LOG("=OfflineModelBuilder<...>::getNextIModel(" << u << ")");
  //logEvalGraphModelGraph();
  #endif

  assert(!!currentjrp);
  if( currentjrp.get()[u] )
  {
    LOG("join relevant");
    return Base::getNextIModel(u);
  }
  else
  {
    LOG("not join relevant");
    assert(offmbp[u].builtIModels);
    // TODO how about iproj?
    const ModelList& mlist = Base::mg.modelsAt(u, MT_IN);
    if( !!offmbp[u].currentIModel )
    {
      //assert(offmbp[u].currentIModel.get() != mlist.end());
      LOG("advancing iterator");
      //if( Base::mg.propsOf(*offmbp[u].currentIModel.get()).dummy == 1 )
      //  logEvalGraphModelGraph();
      offmbp[u].currentIModel.get()++;
    }
    else
    {
      LOG("initializing iterator");
      offmbp[u].currentIModel = mlist.begin();
    }
    if( offmbp[u].currentIModel.get() == mlist.end() )
    {
      LOG("no more models");
      Base::mbp[u].setIModel(boost::none);
      offmbp[u].currentIModel = boost::none;
      return boost::none;
    }
    else
    {
      Model m = *offmbp[u].currentIModel.get();
      Base::mbp[u].setIModel(m);
      LOG("got model " << m);
      return m;
    }
  }
}

#endif // OFFLINE_MODEL_BUILDER_HPP_INCLUDED__28092010
