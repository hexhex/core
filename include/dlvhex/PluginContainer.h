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
 * @file   PluginContainer.h
 * @author Roman Schindlauer
 * @author Peter Schueller
 * @date   Thu Sep 1 17:21:53 2005
 * 
 * @brief  Container class for plugins.
 * 
 * 
 */

#if !defined(_DLVHEX_PLUGINCONTAINER_H)
#define _DLVHEX_PLUGINCONTAINER_H

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/fwd.hpp"
#include "dlvhex/PluginInterface.h"

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
 * @brief Collects and administrates all available plugins.
 */
class DLVHEX_EXPORT PluginContainer
{
public:
  /// ctor
  PluginContainer(RegistryPtr registry);

  /// copy ctor
  PluginContainer(const PluginContainer&);

  /// dtor
  ~PluginContainer();

  //
  // loading and accessing
  //

	// search for plugins in searchpath and open those that are plugins
	// may be called multiple times with different paths
	// paths may be separated by ":" just like LD_LIBRARY_PATH
	void loadPlugins(const std::string& searchpath="");

  // add a PluginInterface to the container
  void addInternalPlugin(PluginInterfacePtr plugin);

  // add a PluginAtom to the container
  void addInternalPluginAtom(PluginAtomPtr atom);

  // get container with plugins loaded so far
  const std::vector<PluginInterfacePtr>& getPlugins() const;

  /**
   * @brief returns a plugin-atom object corresponding to a name.
   */
  PluginAtomPtr getAtom(const std::string& name) const;

  RegistryPtr getRegistry() const
    { return registry; }

  //
  // batch operations on all plugins
  //

	// call printUsage for each loaded plugin
	void printUsage(std::ostream& o);

	// call processOptions for each loaded plugin
	// (this is supposed to remove "recognized" options from pluginOptions)
	void processOptions(std::list<const char*>& pluginOptions);

  // associate plugins in container to external atoms in given rules
  void associateExtAtomsWithPluginAtoms(
      const std::vector<ID>& idb, bool failOnUnknownAtom=true);

private:
  // one plugincontainer can only be used with one registry,
  // as all the plugin atoms have an association with a registry
  RegistryPtr registry;

	/// current search path
	std::string searchPath;

  // loaded plugins
  std::vector<PluginInterfacePtr> plugins;

  /**
   * @brief Associative map of external atoms provided by plugins.
   */
  PluginAtomMap pluginAtoms;
};
typedef boost::shared_ptr<PluginContainer> PluginContainerPtr;

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_PLUGINCONTAINER_H */


// Local Variables:
// mode: C++
// End:
