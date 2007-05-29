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

#ifndef _PLUGINCONTAINER_H
#define _PLUGINCONTAINER_H

#include <string>

#include "dlvhex/PluginInterface.h"


/**
 * @brief Collects and administrates all available plugins.
 */
class PluginContainer
{
public:
    /**
     * @brief Singleton instance handle.
     */
    static PluginContainer* Instance();

    /**
    * @brief Loads a library and accesses its plugin-interface.
    */
    PluginInterface*
    importPlugin(std::string filename);

    /**
    * @brief returns a plugin-atom object corresponding to a name.
    */
    PluginAtom*
    getAtom(std::string name);

    /**
    * @brief returns the plugin interface that hosts the specified atom.
    */
//    PluginInterface*
//    getInterface(std::string name);

protected:

    /**
     * @brief Ctor (protected to ensure singleton)
     */
    PluginContainer() { }

    /**
     * @brief Dtor.
     */
    ~PluginContainer();

private:

    /**
    * @brief Associative map of external atoms provided by plugins.
    */
    PluginInterface::AtomFunctionMap pluginAtoms;

//    typedef std::map<std::string, PluginInterface*> FunctionInterfaceMap;

//    FunctionInterfaceMap interfaces;
    /**
     * @brief Singleton instance.
     */
    //static PluginContainer* _instance;
};


#endif /* _PLUGINCONTAINER_H */


// Local Variables:
// mode: C++
// End:
