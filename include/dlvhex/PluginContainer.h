/* -*- C++ -*- */

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
