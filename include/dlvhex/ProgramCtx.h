/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2007, 2008 Thomas Krennwallner
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

#include <vector>
#include <string>
#include <iosfwd>

#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

// forward declarations
class PluginContainer;
class PluginInterface;
class Program;
class AtomSet;
class NodeGraph;
class DependencyGraph;
class Process;
class ResultContainer;
class OutputBuilder;
class State;


/**
 * @brief Program context class.
 *
 * A facade/state context for the subcomponents of dlvhex.
 */
class DLVHEX_EXPORT ProgramCtx
{
 private:
  std::vector<std::string>* options;

  std::vector<std::string> inputsources;

  PluginContainer* container;
  std::vector<PluginInterface*>* plugins;

  std::istream* programstream;

  /// stores the rules of the program
  Program* IDB;
  /// stores the facts of the program
  AtomSet* EDB;

  NodeGraph* nodegraph;
  DependencyGraph* depgraph;

  Process* solver;

  ResultContainer* result;

  OutputBuilder* outputbuilder;

  boost::shared_ptr<State> state;


 protected:
  friend class State;

  void
  changeState(const boost::shared_ptr<State>&);


 public:
  ProgramCtx();

  virtual
  ~ProgramCtx();


  void
  setPluginContainer(PluginContainer*);

  PluginContainer* const
  getPluginContainer() const;
  

  void
  addPlugins(const std::vector<PluginInterface*>&);

  std::vector<PluginInterface*>* const
  getPlugins() const;

  void
  addOption(const std::string&);

  std::vector<std::string>* const
  getOptions() const;

  void
  addInputSource(const std::string&);

  const std::vector<std::string>&
  getInputSources() const;


  // the program's input stream
  std::istream&
  getInput();


  Program* const
  getIDB() const;

  AtomSet* const
  getEDB() const;


  NodeGraph* const
  getNodeGraph() const;

  void
  setNodeGraph(NodeGraph*);

  DependencyGraph* const
  getDependencyGraph() const;

  void
  setDependencyGraph(DependencyGraph*);


  Process* const
  getProcess() const;

  void
  setProcess(Process*);


  ResultContainer* const
  getResultContainer() const;

  void
  setResultContainer(ResultContainer*);


  OutputBuilder* const
  getOutputBuilder() const;

  void
  setOutputBuilder(OutputBuilder*);


  //
  // state processing
  //

  void
  openPlugins();

  void
  convert();

  void
  parse();

  void
  rewrite();

  void
  createNodeGraph();

  void
  optimize();

  void
  safetyCheck();

  void
  strongSafetyCheck();

  void
  createDependencyGraph();

  void
  evaluate();

  void
  postProcess();

  void
  output();

};

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_PROGRAMCTX_H */

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
