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

#include <boost/shared_ptr.hpp>
#include <boost/functional/factory.hpp>
//#include <boost/bimap/bimap.hpp>
//#include <boost/bimap/set_of.hpp>
//#include <boost/bind.hpp>

#include <vector>
#include <string>
#include <iosfwd>

DLVHEX_NAMESPACE_BEGIN

// forward declarations
class Program;
class AtomSet;

//class Process;
//class ResultContainer;
//class OutputBuilder;
class State;
typedef boost::shared_ptr<State> StatePtr;

typedef boost::function<EvalHeuristicBase<EvalGraphBuilder>*(EvalGraphBuilder&)>
  EvalHeuristicFactory;

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

  // must be setup together
  // pluginContainer must be associated to registry
  void setupRegistryPluginContainer(
      RegistryPtr registry, PluginContainerPtr pluginContainer=PluginContainerPtr());

  // factory for eval heuristics
  EvalHeuristicFactory evalHeuristicFactory;

  ASPSolverManager::SoftwareConfigurationPtr aspsoftware;

	// program input provider (if a converter is used, the converter consumes this
	// input and replaces it by another input)
	InputProviderPtr inputProvider;

  // idb
  std::vector<ID> idb;

  // edb
  Interpretation::Ptr edb;

  // maxint setting, this is ID_FAIL if it is not specified, an integer term otherwise
  uint32_t maxint;


  // TODO: add visibility policy (as in clasp)

  // TODO: loaded external atoms

  // TODO: everything required for executing plain HEX programs (no rewriting involved)

  DependencyGraphPtr depgraph;
  ComponentGraphPtr compgraph;
  FinalEvalGraphPtr evalgraph;
  // ModelGraphPtr modelgraph;
  // ModelBuilderPtr modelbuilder;
  // ModelCallbackPtr modelcallback;
  // FinalizeCallbackPtr finalizecallback;

  StatePtr state;

// protected:
//  friend class State;

  void
  changeState(const boost::shared_ptr<State>&);


 public:
  ProgramCtx();

  virtual
  ~ProgramCtx();

  #if 0

  void
  setPluginContainer(PluginContainer*);

  PluginContainer*
  getPluginContainer() const;
  

  void
  addPlugins(const std::vector<PluginInterface*>&);

  std::vector<PluginInterface*>*
  getPlugins() const;


  Program*
  getIDB() const;

  AtomSet*
  getEDB() const;


  NodeGraph*
  getNodeGraph() const;

  void
  setNodeGraph(NodeGraph*);

  DependencyGraph*
  getDependencyGraph() const;

  void
  setDependencyGraph(DependencyGraph*);
	#endif

  ASPSolverManager::SoftwareConfigurationPtr
  getASPSoftware() const;

  void
  setASPSoftware(ASPSolverManager::SoftwareConfigurationPtr);

	#if 0
  ResultContainer*
  getResultContainer() const;

  void
  setResultContainer(ResultContainer*);


  OutputBuilder*
  getOutputBuilder() const;

  void
  setOutputBuilder(OutputBuilder*);
	#endif


  //
  // state processing
  //

  void showPlugins();
  void convert();
  void parse();
  void rewriteEDBIDB();
	void optimizeEDBDependencyGraph();
	void createComponentGraph();
	void createEvalGraph();
  void configureModelBuilder();
  void createDependencyGraph();
  void safetyCheck();
  void strongSafetyCheck();
  void evaluate();
  void postProcess();

protected:
  // symbol storage of this program context
  // (this is a shared ptr because we might want
  // to have multiple program contexts sharing the same registry)
  RegistryPtr _registry;

	// plugin container (this must be initialized with above registry!)
	PluginContainerPtr _pluginContainer;
};

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_PROGRAMCTX_H */

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
