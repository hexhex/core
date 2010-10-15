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
 * @file ProgramCtx.cpp
 * @author Thomas Krennwallner
 * @date 
 *
 * @brief Program context.
 *
 *
 *
 */


#include "dlvhex/ProgramCtx.h"

#include "dlvhex/PluginContainer.h"

#if 0
#include "dlvhex/Program.h"
#include "dlvhex/AtomSet.h"
#endif

//#include "dlvhex/NodeGraph.h"
//#include "dlvhex/DependencyGraph.h"
//#include "dlvhex/ResultContainer.h"
//#include "dlvhex/OutputBuilder.h"
#include "dlvhex/State.h"
#include "dlvhex/DLVProcess.h"

#include <boost/shared_ptr.hpp>

#include <sstream>
#include <iostream>

DLVHEX_NAMESPACE_BEGIN

void Registry::logContents() const
{
  LOG_FUNCTION("Registry");
  terms.logContents("terms");
  ogatoms.logContents("ogatoms");
  onatoms.logContents("onatoms");
  batoms.logContents("batoms");
  aatoms.logContents("aatoms");
  rules.logContents("rules");
}

// TODO create something like a print visitor instead (reuse this code)
void Registry::printTerm(ID id, std::string& append) const
{
	assert(id.isTerm());
	append += terms.getByID(id).symbol;
}
	
ProgramCtx::ProgramCtx()
  :
		registry(),
		idb(),
		edb(),
		maxint(0),
		options(new std::vector<std::string>),
    container(0),
    plugins(new std::vector<PluginInterface*>),
    programstream(new std::istream(new std::stringbuf)),
 //   nodegraph(new NodeGraph),
    depgraph(0),
    result(0),
    outputbuilder(0),
    state()//boost::shared_ptr<State>(new OpenPluginsState))  // start in the OpenPlugin state
{ }


ProgramCtx::~ProgramCtx()
{
  delete outputbuilder;
  delete result;
  delete depgraph;
  //delete nodegraph;
  //  std::vector<PluginInterface*> plugins;
}
  

void
ProgramCtx::changeState(const boost::shared_ptr<State>& s)
{
  state = s;
}


void
ProgramCtx::addOption(const std::string& o)
{
  options->push_back(o);
}


std::vector<std::string>*
ProgramCtx::getOptions() const
{
  return this->options;
}


void
ProgramCtx::setPluginContainer(PluginContainer* c)
{
  if (this->container != c)
    {
      this->container = c;
    }
}


PluginContainer*
ProgramCtx::getPluginContainer() const
{
  return this->container;
} 


std::vector<PluginInterface*>*
ProgramCtx::getPlugins() const
{
  return this->plugins;
}


void
ProgramCtx::addPlugins(const std::vector<PluginInterface*>& p)
{
  plugins->insert(plugins->end(), p.begin(), p.end());
}


void
ProgramCtx::addInputSource(const std::string& s)
{
  inputsources.push_back(s);
}


const std::vector<std::string>&
ProgramCtx::getInputSources() const
{
  return inputsources;
}


std::istream&
ProgramCtx::getInput()
{
  return *programstream;
}


#if 0
Program*
ProgramCtx::getIDB() const
{
  return IDB;
}


AtomSet*
ProgramCtx::getEDB() const
{
  return EDB;
}


NodeGraph*
ProgramCtx::getNodeGraph() const
{
  return nodegraph;
}


void
ProgramCtx::setNodeGraph(NodeGraph* ng)
{
  if (ng != this->nodegraph)
    {
      delete this->nodegraph;
      this->nodegraph = ng;
    }
}


DependencyGraph*
ProgramCtx::getDependencyGraph() const
{
  return depgraph;
}


void
ProgramCtx::setDependencyGraph(DependencyGraph* dg)
{
  if (dg != this->depgraph)
    {
      delete this->depgraph;
      this->depgraph = dg;
    }
}

ASPSolverManager::SoftwareConfigurationPtr
ProgramCtx::getASPSoftware() const
{
  assert(aspsoftware != 0);
  return aspsoftware;
}

void
ProgramCtx::setASPSoftware(ASPSolverManager::SoftwareConfigurationPtr software)
{
  aspsoftware = software;
}


ResultContainer*
ProgramCtx::getResultContainer() const
{
  return result;
}


void
ProgramCtx::setResultContainer(ResultContainer* c)
{
  if (this->result != c)
    {
      delete this->result;
      this->result = c;
    }
}


OutputBuilder*
ProgramCtx::getOutputBuilder() const
{
  return outputbuilder;
}


void
ProgramCtx::setOutputBuilder(OutputBuilder* o)
{
  if (this->outputbuilder != o)
    {
      delete this->outputbuilder;
      this->outputbuilder = o;
    }
}
#endif


void
ProgramCtx::openPlugins()
{
  state->openPlugins(this);
}


void
ProgramCtx::convert()
{
  state->convert(this);
}


void
ProgramCtx::parse()
{
  state->parse(this);
}


void
ProgramCtx::rewrite()
{
  state->rewrite(this);
}


void
ProgramCtx::createNodeGraph()
{
  state->createNodeGraph(this);
}


void
ProgramCtx::optimize()
{
  state->optimize(this);
}


void
ProgramCtx::createDependencyGraph()
{
  state->createDependencyGraph(this);
}


void
ProgramCtx::safetyCheck()
{
  state->safetyCheck(this);
}


void
ProgramCtx::strongSafetyCheck()
{
  state->strongSafetyCheck(this);
}


void
ProgramCtx::setupProgramCtx()
{
  state->setupProgramCtx(this);
}


void
ProgramCtx::evaluate()
{
  state->evaluate(this);
}


void
ProgramCtx::postProcess()
{
  state->postProcess(this);
}


void
ProgramCtx::output()
{
  state->output(this);
}


DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
