/* -*- C++ -*- */

/**
 * @file   PluginContainer.cpp
 * @author Roman Schindlauer
 * @date   Thu Sep 1 17:25:55 2005
 * 
 * @brief  Container class for plugins.
 * 
 * 
 */

#include <dlfcn.h>

#include "dlvhex/Error.h"
#include "dlvhex/PluginInterface.h"
#include "dlvhex/PluginContainer.h"
#include "dlvhex/globals.h"


typedef PluginInterface* (*t_import)();


PluginContainer*
PluginContainer::Instance ()
{
    static PluginContainer _instance;

    return &_instance;
}

PluginContainer::~PluginContainer()
{
    // TODO: make this more elegant:

    for(PluginInterface::AtomFunctionMap::const_iterator it = pluginAtoms.begin();
        it != pluginAtoms.end();
        it++)
    {
        delete pluginAtoms[(*it).first];
    }
}

PluginInterface*
PluginContainer::importPlugin(std::string filename)
{
    void* dlHandle = dlopen(filename.c_str(), RTLD_LAZY);

    if (!dlHandle)
    {
        throw FatalError("Cannot open library " + filename + ": " + dlerror());
    }

    t_import getplugin = (t_import) dlsym(dlHandle, PLUGINIMPORTFUNCTIONSTRING);

    if (!getplugin)
    {
//        throw FatalError("Cannot load symbol " PLUGINIMPORTFUNCTIONSTRING);
        return NULL;
    }

    PluginInterface::AtomFunctionMap pa;

    PluginInterface* plugin = getplugin();

    if (!Globals::Instance()->getOption("Silent"))
        std::cout << "opening plugin " << filename << " version "
                  << plugin->getVersionMajor() << "."
                  << plugin->getVersionMinor() << "."
                  << plugin->getVersionMicro() << std::endl;

    plugin->getAtoms(pa);

    for(PluginInterface::AtomFunctionMap::const_iterator it = pa.begin();
        it != pa.end();
        it++)
    {
        // std::cout << (*it).first << " -> " << (*it).second << std::endl;
        
        //
        // TODO: check if this function name already exists!
        //

        if (Globals::Instance()->getOption("Verbose"))
            std::cout << "Registering external atom " << (*it).first << std::endl;

        pluginAtoms[(*it).first] = (*it).second;

//        interfaces[(*it).first] = plugin;
    }

    return plugin;
}


PluginAtom*
PluginContainer::getAtom(std::string name)
{
    PluginInterface::AtomFunctionMap::const_iterator pa(pluginAtoms.find(name));

    if (pa == pluginAtoms.end())
        return 0;
    
    return pa->second;
}

/*
PluginInterface*
PluginContainer::getInterface(std::string name)
{
    FunctionInterfaceMap::const_iterator in(interfaces.find(name));

    if (in == interfaces.end())
        return 0;
    
    return in->second;
}
*/
