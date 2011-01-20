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
 * @file   PluginContainer.cpp
 * @author Roman Schindlauer
 * @author Thomas Krennwallner
 * @author Peter Schueller
 * @date   Thu Sep 1 17:25:55 2005
 * 
 * @brief  Container class for plugins.
 * 
 * 
 */

#include "dlvhex/PluginContainer.h"
#include "dlvhex/Configuration.hpp"
#include "dlvhex/Error.h"
#include "dlvhex/Logger.hpp"
#include "dlvhex/PluginInterface.h"
#include "dlvhex/Registry.hpp"

#include <ltdl.h>

#include <sys/types.h>
#include <dirent.h>
#include <pwd.h>

#include <boost/foreach.hpp>

#include <iostream>
#include <sstream>
#include <set>

DLVHEX_NAMESPACE_BEGIN

namespace
{

struct PluginCandidate
{
  lt_dlhandle handle;
  PluginInterfacePtr plugin;
  
  PluginCandidate(lt_dlhandle handle, PluginInterfacePtr plugin):
    handle(handle), plugin(plugin) {}
};
typedef std::vector<PluginCandidate> CandidateVector;

typedef PluginInterface* (*t_import)();

int
findplugins(const char* filename, lt_ptr data)
{
  std::vector<std::string>* pluginlist = reinterpret_cast<std::vector<std::string>*>(data);

  std::string fn(filename);
  std::string::size_type base = fn.find_last_of("/");

  // if basename starts with 'libdlvhex', then we should have a plugin here
  /// @todo we could lt_dlopen the file here, to check if it is really
  /// a plugin, for now we exclude loading of libdlvhexbase as it is
  /// not a plugin and caused duplicate instantiations of the Term tables
  if (fn.substr(base).find("/libdlvhex") == 0 &&
      fn.substr(base).find("/libdlvhexbase") == std::string::npos)
    {
      pluginlist->push_back(fn);
    }

  return 0;
}

void findPluginLibraryCandidates(const std::string& searchpath, std::vector<std::string>& libcandidates)
{
  if (lt_dlinit())
    {
      throw GeneralError("Could not initialize libltdl");
    }
  
  //
  // now look into the user's home, and into the global plugin directory
  //
  
  LOG(PLUGIN,"findPluginLibraryCandidates with searchpath='" << searchpath << "'");
  if (lt_dlsetsearchpath(searchpath.c_str()))
    {
      throw GeneralError("Could not set libltdl search path: " + searchpath);
    }

  // search the directory search paths for plugins and setup pluginList
  lt_dlforeachfile(NULL, findplugins, reinterpret_cast<void*>(&libcandidates));
}

void loadCandidates(const std::vector<std::string>& libnames, CandidateVector& plugins)
{
  BOOST_FOREACH(const std::string& lib, libnames)
  {
    LOG(PLUGIN,"loading Plugin Library: '" << lib << "'");
    lt_dlhandle dlHandle = lt_dlopenext(lib.c_str());

    // do while false for breaking out easily
    do
    {
      if( dlHandle == NULL )
      {
        LOG(WARNING,"Selected library '" << lib << "' for opening but cannot open: '" << lt_dlerror() << "' (skipping)");
        break;
      }

      t_import getplugin = reinterpret_cast<t_import>(lt_dlsym(dlHandle, PLUGINIMPORTFUNCTIONSTRING));
      if( getplugin == NULL )
      {
        LOG(INFO,"Library '" << lib << "' selected for opening but no import function '" << PLUGINIMPORTFUNCTIONSTRING << "' (skipping)");
        break;
      }

      // get it!
      DBGLOG(DBG,"now calling plugin import function for " << lib);
      PluginInterfacePtr plugin(getplugin());
      DBGLOG(DBG,"plugin import function returned " << printptr(plugin));

      plugins.push_back(PluginCandidate(dlHandle, plugin));
    }
    while(false);
  }
}

void selectPluginCandidates(std::vector<PluginInterfacePtr>& plugins, CandidateVector& candidates)
{
  // remove those with duplicate names
  std::set<std::string> names;
  BOOST_FOREACH(PluginInterfacePtr plugin, plugins)
  {
    names.insert(plugin->getPluginName());
  }
  DBGLOG(DBG,"selectPluginCandidates: already loaded: " << printset(names));

  CandidateVector::iterator it = candidates.begin();
  while(it != candidates.end())
  {
    const std::string& pname = it->plugin->getPluginName();
    if( names.find(pname) != names.end() )
    {
      // warn, unload, remove, restart loop

      // warn
      LOG(WARNING,"already loaded a plugin with name " << pname << " (skipping)");
      // free plugininterface
      assert(it->plugin.use_count() == 1);
      it->plugin.reset();
      // unload lib
      if( 0 != lt_dlclose(it->handle) )
      {
        LOG(WARNING,"failed unloading plugin library " << pname);
      }
      // remove
      candidates.erase(it);
      // restart
      it = candidates.begin();
    }
    else
    {
      // check next
      ++it;
    }
  }
}

} // anonymous namespace

PluginContainer::PluginContainer(const PluginContainer& pc):
  registry(pc.registry),
  searchPath(pc.searchPath),
  plugins(pc.plugins),
  pluginAtoms(pc.pluginAtoms)
{
}


PluginContainer::PluginContainer(RegistryPtr registry):
  registry(registry)
{
  assert(registry && "PluginContainer needs registry!");
}

PluginContainer::~PluginContainer()
{
}

// search for plugins in searchpath and open those that are plugins
// may be called multiple times with different paths
// paths may be separated by ":" just like LD_LIBRARY_PATH
void PluginContainer::loadPlugins(const std::string& search)
{
  LOG_SCOPE(PLUGIN,"loadPlugins",false);

  // find candidates
  std::vector<std::string> libcandidates;
  findPluginLibraryCandidates(search, libcandidates);

  // TODO probably preselect using library names and already loaded plugins

  // load candidates
  CandidateVector plugincandidates;
  loadCandidates(libcandidates, plugincandidates);

  // TODO probably select/unload using PluginInterface and already loaded plugins
  selectPluginCandidates(plugins, plugincandidates);

  // add new plugins to list of loaded plugins
  BOOST_FOREACH(const PluginCandidate& cand, plugincandidates)
  {
    addInternalPlugin(cand.plugin);
    // (automatically adds atoms)
    // discard cand.handle
  }

  // add to existing search path
  if( !searchPath.empty() )
    searchPath += ":";
  searchPath += search;
}

// add a PluginInterface to the container
void PluginContainer::addInternalPlugin(PluginInterfacePtr plugin)
{
  LOG(PLUGIN,"adding PluginInterface '" << plugin->getPluginName() << "'");

  PluginAtomMap pa;
  plugin->getAtoms(pa);
  
  for(PluginAtomMap::const_iterator it = pa.begin();
      it != pa.end(); ++it)
  {
    // simply use "addInternal" method
    addInternalPluginAtom(it->second);
  }

  plugins.push_back(plugin);
}

// add a PluginAtom statically linked into this program to the container
// (for testsuite and statically linked applications using dlvhex lib API)
void PluginContainer::addInternalPluginAtom(PluginAtomPtr atom)
{
  assert(!!atom);
  const std::string& predicate = atom->getPredicate();
  LOG(PLUGIN,"adding PluginAtom '" << predicate << "'");
  if( pluginAtoms.find(predicate) == pluginAtoms.end() )
  {
    atom->setRegistry(registry);
    pluginAtoms[predicate] = atom;
  }
  else
  {
    LOG(WARNING,"External atom " << predicate << " is already loaded (skipping)");
  }
}

PluginAtomPtr
PluginContainer::getAtom(const std::string& name) const
{
  PluginAtomMap::const_iterator pa = pluginAtoms.find(name);

  if (pa == pluginAtoms.end())
    return PluginAtomPtr();
    
  return pa->second;
}

// call printUsage for each loaded plugin
void PluginContainer::printUsage(
    std::ostream& o)
{
  BOOST_FOREACH(PluginInterfacePtr plugin, plugins)
  {
    o << "Plugin help for " << plugin->getPluginName() << ":" << std::endl;
	  plugin->printUsage(o);
  }
}

// call processOptions for each loaded plugin
// (this is supposed to remove "recognized" options from pluginOptions)
void PluginContainer::processOptions(
    std::list<const char*>& pluginOptions)
{
  BOOST_FOREACH(PluginInterfacePtr plugin, plugins)
  {
    LOG(DBG,"processing options for plugin " << plugin->getPluginName());
    LOG(DBG,"currently have " << printrange(pluginOptions));
	  plugin->processOptions(pluginOptions);
  }
}

// associate plugins in container to external atoms in registry
void PluginContainer::associateExtAtomsWithPluginAtoms(
    const std::vector<ID>& idb, bool failOnUnknownAtom)
{
  // associate all rules
  for(std::vector<ID>::const_iterator it = idb.begin();
      it != idb.end(); ++it)
  {
    assert(it->isRule());
    // skip those without external atoms
    if( !it->doesRuleContainExtatoms() )
      continue;

    // associate all literals in rule body
    const Rule& rule = registry->rules.getByID(*it);
    for(Tuple::const_iterator itl = rule.body.begin();
        itl != rule.body.end(); ++itl)
    {
      assert(itl->isLiteral());
      // skip literals that are not external atoms
      #warning aggregates may have external atoms inside!
      if( !itl->isExternalAtom() )
        continue;

      const ExternalAtom& eatom = registry->eatoms.getByID(*itl);
      const std::string& predicate = registry->getTermStringByID(eatom.predicate);
      // lookup pluginAtom to this eatom predicate
      PluginAtomMap::iterator itpa = pluginAtoms.find(predicate);
      if( itpa != pluginAtoms.end() )
      {
        eatom.pluginAtom = itpa->second;
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
}

// call all setupProgramCtx methods of all plugins
void PluginContainer::setupProgramCtx(ProgramCtx& ctx)
{
  BOOST_FOREACH(PluginInterfacePtr plugin, plugins)
  {
    LOG(DBG,"setting up program ctx for plugin " << plugin->getPluginName());
	  plugin->setupProgramCtx(ctx);
  }
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
