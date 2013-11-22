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

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/PluginInterface.h"

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
 * @brief Collects and administrates all available plugins.
 *
 * The PluginContainer loads and manages dynamically loaded and internal plugins.
 * It is not aware of the configuration or usage of plugins or plugin atoms in a
 * ProgramCtx.
 *
 * Important: memory allocation policy:
 * * PluginInterface objects are passed by pointer from the extern "C" plugin
 *   import function, they are wrapped in a non-deleting smart pointer by the
 *   PluginContainer and must be deallocated by the library itself.
 * * PluginAtom objects are created by PluginInterface::getAtoms and
 *   then owned by a smart pointer in the PluginContainer. These smart pointers
 *   must contain a "deleter" compiled into the library.
 */
class DLVHEX_EXPORT PluginContainer
{
private:
  /// copy ctor (must not be used, would duplicate library unloads)
  PluginContainer(const PluginContainer&);

public:
  /// ctor
  PluginContainer();

  /// dtor
  // unloads shared libraries (if shared_ptr reference counts are ok)
  ~PluginContainer();

  //
  // loading and accessing
  //

	// search for plugins in searchpath and open those that are plugins
	// may be called multiple times with different paths
	// paths may be separated by ":" just like LD_LIBRARY_PATH
	void loadPlugins(const std::string& searchpath="");

  // add a PluginInterface to the container
  // the smart pointer will not be reconfigured, so if you need to use a
  // custom "deleter", do it before you call this method
  void addInternalPlugin(PluginInterfacePtr plugin);

  // get container with plugins loaded so far
  const std::vector<PluginInterfacePtr>& getPlugins() const
  { return pluginInterfaces; }

  //
  // batch operations on all plugins
  //

	// call printUsage for each loaded plugin
	void printUsage(std::ostream& o);

public:
  struct LoadedPlugin;
  typedef boost::shared_ptr<LoadedPlugin> LoadedPluginPtr;
  typedef std::vector<LoadedPluginPtr> LoadedPluginVector;
  typedef std::vector<PluginInterfacePtr> PluginInterfaceVector;
  
private:
  // add loaded plugin (do not extract plugin atoms)
  void addInternalPlugin(LoadedPluginPtr lplugin);

	/// current search path
	std::string searchPath;

  // loaded plugins
  LoadedPluginVector plugins;

  // loaded plugins (interface ptrs)
  PluginInterfaceVector pluginInterfaces;
};
typedef boost::shared_ptr<PluginContainer> PluginContainerPtr;

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_PLUGINCONTAINER_H */


// Local Variables:
// mode: C++
// End:
