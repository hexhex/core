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
 * @file ProgramCtx.h
 * @author Thomas Krennwallner
 * @author Peter Schueller
 *
 * @brief Program context
 */


#if !defined(_DLVHEX_PROGRAMCTX_H)
#define _DLVHEX_PROGRAMCTX_H

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/Configuration.h"
#include "dlvhex2/ASPSolverManager.h"
#include "dlvhex2/Interpretation.h"
#include "dlvhex2/PluginContainer.h"
#include "dlvhex2/InputProvider.h"
#include "dlvhex2/FinalEvalGraph.h"
#include "dlvhex2/EvalHeuristicBase.h"
#include "dlvhex2/EvalGraphBuilder.h"
#include "dlvhex2/ExternalAtomEvaluationHeuristics.h"
#include "dlvhex2/UnfoundedSetCheckHeuristics.h"
#include "dlvhex2/ModelBuilder.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Nogood.h"

#include <boost/shared_ptr.hpp>
#include <boost/functional/factory.hpp>

#include <typeinfo>
#include <vector>
#include <string>
#include <iosfwd>

DLVHEX_NAMESPACE_BEGIN

typedef boost::shared_ptr<EvalHeuristicBase<EvalGraphBuilder> >
  EvalHeuristicPtr;

typedef boost::shared_ptr<ModelBuilder<FinalEvalGraph> >
  ModelBuilderPtr;

typedef boost::function<ModelBuilder<FinalEvalGraph>*(FinalEvalGraph&)>
  ModelBuilderFactory;

typedef std::map<std::string, PluginAtomPtr>
	PluginAtomMap;

/**
 * @brief Program context class.
 *
 * A facade/state context for the subcomponents of dlvhex.
 */
class DLVHEX_EXPORT ProgramCtx
{
public:
	// previously globals
	Configuration config;

  const RegistryPtr& registry() const
    { return _registry; }
	const PluginContainerPtr& pluginContainer() const
    { return _pluginContainer; }

  // cannot change registry if something is already stored here
  void setupRegistry(RegistryPtr registry);

  // change registry 
  void changeRegistry(RegistryPtr registry);

  void setupPluginContainer(PluginContainerPtr pluginContainer);

  // factory for eval heuristics
  EvalHeuristicPtr evalHeuristic;
  // factory for model builders
  ModelBuilderFactory modelBuilderFactory;
  // factory for external atom evaluation heuristic and ufs check heuristic
  ExternalAtomEvaluationHeuristicsFactoryPtr externalAtomEvaluationHeuristicsFactory;
  UnfoundedSetCheckHeuristicsFactoryPtr unfoundedSetCheckHeuristicsFactory;

  ASPSolverManager::SoftwareConfigurationPtr aspsoftware;

	// program input provider (if a converter is used, the converter consumes this
	// input and replaces it by another input)
	InputProviderPtr inputProvider;

  // the input parser
  HexParserPtr parser;

  // idb 
  std::vector<ID> idb; 
  std::vector<std::vector<ID> > idbList;

  // edb 
  Interpretation::Ptr edb; 
  std::vector<InterpretationPtr> edbList;

  // global learning
  class GlobalNogoodRepository : public NogoodContainer{
  private:
    NogoodSet ns;
    std::vector<NogoodContainerPtr> listeners;

  public:
    int addNogood(const Nogood& ng){
      // notify all listeners about new nogoods
      DBGLOG(DBG, "Notifying " << listeners.size() << " listeners about new global nogood " << ng);
      BOOST_FOREACH (NogoodContainerPtr listener, listeners){
        listener->addNogood(ng);
      }

      // record global nogood
      return ns.addNogood(ng);
    }

    void removeNogood(int index){
      DBGLOG(DBG, "Removing global nogood " << ns.nogoods[index]);
      ns.removeNogood(index);
    }

    Nogood getNogood(int index){
      return ns.getNogood(index);
    }

    int getNogoodCount(){
      DBGLOG(DBG, "Have " << ns.nogoods.size() << " global nogoods");
      return ns.nogoods.size();
    }

    void addNogoodListener(NogoodContainerPtr nc, bool copyExistingNogoods = true){
      DBGLOG(DBG, "Adding global nogood listener");
      listeners.push_back(nc);

      // notify about existing nogoods
      if(copyExistingNogoods){
        DBGLOG(DBG, "Notifying new listener about " << ns.nogoods.size() << " existing global nogoods");
        BOOST_FOREACH (Nogood ng, ns.nogoods){
          nc->addNogood(ng);
        }
      }
    }

    void removeNogoodListener(NogoodContainerPtr nc){
      DBGLOG(DBG, "Removing global nogood listener");
      while (std::find(listeners.begin(), listeners.end(), nc) != listeners.end()){
        listeners.erase(std::find(listeners.begin(), listeners.end(), nc));
      }
    }
  };
  GlobalNogoodRepository globalNogoods;

  // maxint setting, this is ID_FAIL if it is not specified, an integer term otherwise
  uint32_t maxint;

  // used by plugins to store specific plugin data in ProgramCtx
  // default constructs PluginT::CtxData if it is not yet stored in ProgramCtx
  template<typename PluginT>
  typename PluginT::CtxData& getPluginData();

  // TODO: add visibility policy (as in clasp)

  DependencyGraphPtr depgraph;
  ComponentGraphPtr compgraph;
  FinalEvalGraphPtr evalgraph;
  FinalEvalGraph::EvalUnit ufinal;
  std::list<ModelCallbackPtr> modelCallbacks;
  std::list<FinalCallbackPtr> finalCallbacks;
  ModelBuilderPtr modelBuilder;
  // model graph is only accessible via modelbuilder->getModelGraph()!
  // (model graph is part of the model builder) TODO think about that

  StatePtr state;

  void
  changeState(const boost::shared_ptr<State>&);

public:
  ProgramCtx();
  // not virtual, we do not want to derive from this!
  ~ProgramCtx();

  ASPSolverManager::SoftwareConfigurationPtr
  getASPSoftware() const;

  void
  setASPSoftware(ASPSolverManager::SoftwareConfigurationPtr);

  //
  // plugin helpers
  //

	// process options for each plugin loaded in this ProgramCtx
	// (this is supposed to remove "recognized" options from pluginOptions)
	void processPluginOptions(std::list<const char*>& pluginOptions);

  // use _pluginContainer to get plugin atoms
  void addPluginAtomsFromPluginContainer();

  // add atom to this ProgramCtx and link it to registry of this ProgramCtx
  void addPluginAtom(PluginAtomPtr atom);

  // associate external atoms in registry of this ProgramCtx
  // with plugin atoms in given idb
  //
  // throws on unknown atom iff failOnUnknownAtom is true
  void associateExtAtomsWithPluginAtoms(const Tuple& idb, bool failOnUnknownAtom=true);

  // setup this ProgramCtx (using setupProgramCtx() for of all plugins)
  void setupByPlugins();

  //
  // state processing
  // the following functions are given in intended order of calling
  // optional functions may be omitted
  //

  void showPlugins();                // optional
  void convert();                    // optional
  void parse();
  void moduleSyntaxCheck();
  void mlpSolver();
  void rewriteEDBIDB();              // optional
  void safetyCheck();                // optional (if you know that your program is safe!)
  void createDependencyGraph();
	void optimizeEDBDependencyGraph(); // optional
	void createComponentGraph();
  void strongSafetyCheck();          // optional (if you know that your program is safe!)
	void createEvalGraph();
  void setupProgramCtx();
  void evaluate();
  void postProcess();

  // subprogram handling
  class SubprogramAnswerSetCallback : public ModelCallback{
  public:
    std::vector<InterpretationPtr> answersets;
    virtual bool operator()(AnswerSetPtr model);
    virtual ~SubprogramAnswerSetCallback();
  };
  std::vector<InterpretationPtr> evaluateSubprogram(InterpretationConstPtr edb, std::vector<ID>& idb);
  std::vector<InterpretationPtr> evaluateSubprogram(InputProviderPtr& ip, InterpretationConstPtr addFacts);
  std::vector<InterpretationPtr> evaluateSubprogram(ProgramCtx& pc, bool parse);

protected:
  // symbol storage of this program context
  // (this is a shared ptr because we might want
  // to have multiple program contexts sharing the same registry)
  RegistryPtr _registry;

	// plugin container (this must be initialized with above registry!)
	PluginContainerPtr _pluginContainer;

  // data associated with one specific plugin
  // externally we see this as a non-const reference, the shared_ptr is totally internal
  typedef std::map<std::string, boost::shared_ptr<PluginData> > PluginDataContainer;
  PluginDataContainer pluginData;

  // atoms usable for evaluation (loaded from plugins or manually added)
  PluginAtomMap pluginAtoms;
};

// used by plugins to store specific plugin data in ProgramCtx
// default constructs PluginT::CtxData if it is not yet stored in ProgramCtx
template<typename PluginT>
typename PluginT::CtxData& ProgramCtx::getPluginData()
{
  const std::string pluginTypeName(typeid(PluginT).name());
  PluginDataContainer::const_iterator it =
    pluginData.find(pluginTypeName);
  if( it == pluginData.end() )
  {
    it = pluginData.insert(std::make_pair(
          pluginTypeName,
          boost::shared_ptr<PluginData>(new typename PluginT::CtxData))
        ).first;
  }
  typename PluginT::CtxData* pret =
    dynamic_cast<typename PluginT::CtxData*>(it->second.get());
  assert(!!pret);
  return *pret;
}

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_PROGRAMCTX_H */

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
