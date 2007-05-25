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
        Globals::Instance()->getVerboseStream() << "opening plugin "
			      << filename << " version "
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

//        if (Globals::Instance()->doVerbose(Globals::???))
//            std::cout << "Registering external atom " << (*it).first << std::endl;

        pluginAtoms[(*it).first] = (*it).second;
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

