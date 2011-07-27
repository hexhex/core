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
 * @date
 *
 * @brief Program context
 *
 */


#if !defined(_DLVHEX_PROGRAMCTX_H)
#define _DLVHEX_PROGRAMCTX_H

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/fwd.hpp"
#include "dlvhex/Configuration.hpp"
#include "dlvhex/ASPSolverManager.h"
#include "dlvhex/Interpretation.hpp"
#include "dlvhex/PluginContainer.h"
#include "dlvhex/InputProvider.hpp"
#include "dlvhex/FinalEvalGraph.hpp"
#include "dlvhex/EvalHeuristicBase.hpp"
#include "dlvhex/EvalGraphBuilder.hpp"
#include "dlvhex/ModelBuilder.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/functional/factory.hpp>

#include <typeinfo>
#include <vector>
#include <string>
#include <iosfwd>

DLVHEX_NAMESPACE_BEGIN

typedef boost::function<EvalHeuristicBase<EvalGraphBuilder>*(EvalGraphBuilder&)>
  EvalHeuristicFactory;

typedef boost::shared_ptr<ModelBuilder<FinalEvalGraph> >
  ModelBuilderPtr;

typedef boost::function<ModelBuilder<FinalEvalGraph>*(FinalEvalGraph&)>
  ModelBuilderFactory;

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

  void setupPluginContainer(PluginContainerPtr pluginContainer);

  // must be setup together
  // pluginContainer must be associated to registry
  #warning deprecated
  void setupRegistryPluginContainer(
      RegistryPtr registry, PluginContainerPtr pluginContainer=PluginContainerPtr())
    { setupRegistry(registry); setupPluginContainer(pluginContainer); }

  // factory for eval heuristics
  EvalHeuristicFactory evalHeuristicFactory;
  // factory for model builders
  ModelBuilderFactory modelBuilderFactory;

  ASPSolverManager::SoftwareConfigurationPtr aspsoftware;

	// program input provider (if a converter is used, the converter consumes this
	// input and replaces it by another input)
	InputProviderPtr inputProvider;

  // the input parser
  HexParserPtr parser;

  // idb
  std::vector<ID> idb;

  // edb
  Interpretation::Ptr edb;

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
