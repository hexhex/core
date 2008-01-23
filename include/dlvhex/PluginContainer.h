/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


/**
 * @file   PluginContainer.h
 * @author Roman Schindlauer
 * @date   Thu Sep 1 17:21:53 2005
 * 
 * @brief  Container class for plugins.
 * 
 * 
 */

#if !defined(_DLVHEX_PLUGINCONTAINER_H)
#define _DLVHEX_PLUGINCONTAINER_H

#include "dlvhex/PlatformDefinitions.h"

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
 protected:
  /// ctor
  explicit
  PluginContainer(const std::string& path);

  /// copy ctor
  PluginContainer(const PluginContainer&);

  /// dtor
  ~PluginContainer();

public:

  /// get the PluginContainer singleton instance
  static PluginContainer*
  instance(const std::string&);

  /**
   * @brief Loads a library and accesses its plugin-interface.
   */
  std::vector<PluginInterface*>
  importPlugins();

  /**
   * @brief returns a plugin-atom object corresponding to a name.
   */
  boost::shared_ptr<PluginAtom>
  getAtom(const std::string& name) const;


private:

  /// singleton instance
  static PluginContainer* theContainer;

  /// list of plugins
  std::vector<std::string> pluginList;

  /**
   * @brief Associative map of external atoms provided by plugins.
   */
  PluginInterface::AtomFunctionMap pluginAtoms;

};


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_PLUGINCONTAINER_H */


// Local Variables:
// mode: C++
// End:
