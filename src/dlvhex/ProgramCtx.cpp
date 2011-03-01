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
#include "dlvhex/Registry.hpp"
#include "dlvhex/PluginContainer.h"
#include "dlvhex/State.h"
//#include "dlvhex/DLVProcess.h"

#include <boost/shared_ptr.hpp>

#include <sstream>
#include <iostream>

DLVHEX_NAMESPACE_BEGIN


ProgramCtx::ProgramCtx():
		maxint(0)
{
}


ProgramCtx::~ProgramCtx()
{
  DBGLOG(DBG,"resetting state");
  state.reset();

  DBGLOG(DBG,"resetting callbacks");
  modelCallbacks.clear();
  finalCallbacks.clear();

  DBGLOG(DBG,"resetting modelBuilder");
  modelBuilder.reset();

  DBGLOG(DBG,"resetting parser");
  parser.reset();

  DBGLOG(DBG,"resetting evalgraph");
  evalgraph.reset();

  DBGLOG(DBG,"resetting compgraph");
  compgraph.reset();

  DBGLOG(DBG,"resetting depgraph");
  depgraph.reset();

  DBGLOG(DBG,"resetting edbList");
  std::vector<InterpretationPtr>::iterator it = edbList.begin();
  while ( it != edbList.end() )
    {
      it->reset();
      it++;
    }
  // edb.reset();

  DBGLOG(DBG,"resetting inputProvider");
  inputProvider.reset();

  DBGLOG(DBG,"resetting aspsoftware");
  aspsoftware.reset();

  DBGLOG(DBG,"resetting pluginData");
  pluginData.clear();

  DBGLOG(DBG,"resetting registry, usage count was " << _registry.use_count() << " (it should be 2)");
  _registry.reset();

  DBGLOG(DBG,"resetting pluginAtoms");
  pluginAtoms.clear();

  DBGLOG(DBG,"resetting pluginContainer, usage count was " << _pluginContainer.use_count() << " (it should be 1)");
  _pluginContainer.reset();
}
  
void
ProgramCtx::changeState(const boost::shared_ptr<State>& s)
{
  state = s;
}

#if 0
// must be setup together
// pluginContainer must be associated to registry
#warning deprecated
void ProgramCtx::setupRegistryPluginContainer(
    RegistryPtr registry, PluginContainerPtr pluginContainer)
{
  assert(!pluginContainer ||
      (pluginContainer->getRegistry() == registry &&
      "PluginContainer in ProgramCtx must be associated to registry of programCtx"));
  _registry = registry;
  _pluginContainer = pluginContainer;
}
#endif

// cannot change registry if something is already stored here
void ProgramCtx::setupRegistry(
    RegistryPtr registry)
{
  assert(
      (
        !_registry || // allow to set from nothing
        (idbList.size()==0 && edbList.size()==0 && pluginAtoms.empty()) // allow to change if empty
      )
      &&
      "cannot change registry once idb or edb or pluginAtoms contains data");
  _registry = registry;
}

void ProgramCtx::setupPluginContainer(
    PluginContainerPtr pluginContainer)
{
  assert(
      (
        !_pluginContainer || // allow to set if unset
        pluginAtoms.empty() // allow to change if no atoms stored
      )
      &&
      "cannot change pluginContainer once pluginAtoms are used");
  _pluginContainer = pluginContainer;
  #warning here we could reset the pointers in all ExternalAtoms if we unset the pluginContainer
}

ASPSolverManager::SoftwareConfigurationPtr
ProgramCtx::getASPSoftware() const
{
  assert(aspsoftware != 0);
  return aspsoftware;
}

void ProgramCtx::setASPSoftware(ASPSolverManager::SoftwareConfigurationPtr software)
{
  aspsoftware = software;
}

void ProgramCtx::showPlugins() { state->showPlugins(this); }
void ProgramCtx::convert() { state->convert(this); }
void ProgramCtx::parse() { state->parse(this); }
void ProgramCtx::rewriteEDBIDB() { state->rewriteEDBIDB(this); }
void ProgramCtx::safetyCheck() { state->safetyCheck(this); }
void ProgramCtx::createDependencyGraph() { state->createDependencyGraph(this); }
void ProgramCtx::optimizeEDBDependencyGraph() { state->optimizeEDBDependencyGraph(this); }
void ProgramCtx::createComponentGraph() { state->createComponentGraph(this); }
void ProgramCtx::strongSafetyCheck() { state->strongSafetyCheck(this); }
void ProgramCtx::createEvalGraph() { state->createEvalGraph(this); }
void ProgramCtx::setupProgramCtx() { state->setupProgramCtx(this); }
void ProgramCtx::evaluate() { state->evaluate(this); }
void ProgramCtx::postProcess() { state->postProcess(this); }

#if 0
PluginAtomPtr
PluginContainer::getAtom(const std::string& name) const
{
  PluginAtomMap::const_iterator pa = pluginAtoms.find(name);

  if (pa == pluginAtoms.end())
    return PluginAtomPtr();
    
  return pa->second;
}

#endif

void ProgramCtx::addPluginAtom(PluginAtomPtr atom)
{
  assert(!!atom);
  assert(!!_registry);
  const std::string& predicate = atom->getPredicate();
  LOG(PLUGIN,"adding PluginAtom '" << predicate << "'");
  if( pluginAtoms.find(predicate) == pluginAtoms.end() )
  {
    atom->setRegistry(_registry);
    pluginAtoms[predicate] = atom;
  }
  else
  {
    LOG(WARNING,"External atom " << predicate << " is already loaded (skipping)");
  }
}

// call processOptions for each loaded plugin
// (this is supposed to remove "recognized" options from pluginOptions)
void ProgramCtx::processPluginOptions(
    std::list<const char*>& pluginOptions)
{
  BOOST_FOREACH(PluginInterfacePtr plugin, pluginContainer()->getPlugins())
  {
    LOG(DBG,"processing options for plugin " << plugin->getPluginName());
    LOG(DBG,"currently have " << printrange(pluginOptions));
	  plugin->processOptions(pluginOptions, *this);
  }
}

void ProgramCtx::addPluginAtomsFromPluginContainer()
{
  assert(!!pluginContainer());
  assert(!!registry());

  BOOST_FOREACH(PluginInterfacePtr plugin, pluginContainer()->getPlugins())
  {
    LOG(DBG,"adding plugin atoms from plugin " << plugin->getPluginName());
    // always freshly create! (pluginatoms are linked to a registry,
    // so when using multiple registries, you have to create multiple pluginatoms)
    BOOST_FOREACH(PluginAtomPtr pap, plugin->createAtoms(*this))
    {
      assert(!!pap);
      pap->setRegistry(registry());
      const std::string& pred = pap->getPredicate();
      LOG(DBG,"  got plugin atom " << pred);
      if( pluginAtoms.count(pred) != 0 )
      {
        LOG(WARNING,"warning: predicate '" << pred << "' already present in PluginAtomMap (skipping)");
      }
      else
      {
        pluginAtoms[pred] = pap;
      }
    }
  }
}

// associate plugins in container to external atoms in registry
void ProgramCtx::associateExtAtomsWithPluginAtoms(
    const Tuple& idb, bool failOnUnknownAtom)
{
  assert(!!_registry);
  DBGLOG_SCOPE(DBG,"aEAwPA",false);
  DBGLOG(DBG,"= associateExtAtomsWithPluginAtoms");

  Tuple eatoms;

  // associate all rules
  for(Tuple::const_iterator it = idb.begin();
      it != idb.end(); ++it)
  {
    assert(it->isRule());
    // skip those without external atoms
    if( !it->doesRuleContainExtatoms() )
      continue;

    // associate all literals in rule body
    const Rule& rule = _registry->rules.getByID(*it);

    // get external atoms (recursively)
    _registry->getExternalAtomsInTuple(rule.body, eatoms);
  }

  // now associate
  for(Tuple::const_iterator it = eatoms.begin();
      it != eatoms.end(); ++it)
  {
    assert(it->isExternalAtom());

    const ExternalAtom& eatom = _registry->eatoms.getByID(*it);
    const std::string& predicate = _registry->getTermStringByID(eatom.predicate);
    // lookup pluginAtom to this eatom predicate
    PluginAtomMap::iterator itpa = pluginAtoms.find(predicate);
    if( itpa != pluginAtoms.end() )
    {
      assert(!!itpa->second);
      // we store this as a POD pointer!
      eatom.pluginAtom = itpa->second.get();
    }
    else
    {
      DBGLOG(DBG,"did not find plugin atom for predicate '" << predicate << "'");
      if( failOnUnknownAtom )
      {
        throw FatalError("did not find plugin atom "
            " for predicate '" + predicate + "'");
      }
    }
  }
}

// setup this ProgramCtx (using setupProgramCtx() for of all plugins)
void ProgramCtx::setupByPlugins()
{
  BOOST_FOREACH(PluginInterfacePtr plugin, pluginContainer()->getPlugins())
  {
    LOG(DBG,"setting up program ctx for plugin " << plugin->getPluginName());
	  plugin->setupProgramCtx(*this);
  }
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
