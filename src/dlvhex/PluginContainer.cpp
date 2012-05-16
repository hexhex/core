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
 * @date   Thu Sep 1 17:25:55 2005
 * 
 * @brief  Container class for plugins.
 * 
 * 
 */

#include "dlvhex/globals.h"
#include "dlvhex/Error.h"
#include "dlvhex/PluginInterface.h"
#include "dlvhex/PluginContainer.h"

#include <ltdl.h>

#include <sys/types.h>
#include <dirent.h>
#include <pwd.h>

#include <iostream>
#include <sstream>
#include <set>

DLVHEX_NAMESPACE_BEGIN

typedef PluginInterface* (*t_import)();

PluginContainer* PluginContainer::theContainer = 0;


PluginContainer*
PluginContainer::instance(const std::string& optionPath)
{
  if (PluginContainer::theContainer == 0)
    {
      PluginContainer::theContainer = new PluginContainer(optionPath);
    }

  return PluginContainer::theContainer;
}


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


PluginContainer::PluginContainer(const PluginContainer& pc)
  : pluginList(pc.pluginList),
    pluginAtoms(pc.pluginAtoms)
{ }


PluginContainer::PluginContainer(const std::string& optionPath)
{
  if (lt_dlinit())
    {
      throw GeneralError("Could not initialize libltdl");
    }
  
  //
  // now look into the user's home, and into the global plugin directory
  //
  std::stringstream searchpath;
  
  const char* homedir = ::getpwuid(::geteuid())->pw_dir;
  
  searchpath << optionPath << ':'
	     << homedir << "/" USER_PLUGIN_DIR << ':'
	     << SYS_PLUGIN_DIR;
  
  if (Globals::Instance()->doVerbose(Globals::PLUGIN_LOADING))
    {
      Globals::Instance()->getVerboseStream() << "Plugin Search Path: \"" 
					      << searchpath.str() << "\"" 
					      << std::endl;
    }
  
  if (lt_dlsetsearchpath(searchpath.str().c_str()))
    {
      throw GeneralError("Could not set libltdl search path: " + searchpath.str());
    }

  // search the directory search paths for plugins and setup pluginList
  lt_dlforeachfile(NULL, findplugins, &this->pluginList);
}


PluginContainer::~PluginContainer()
{
  ///@todo this does not work, we have to include specific unloading functions in the plugins
  /* if (lt_dlexit())
    {
      std::cerr << "lt_dlexit() failed" << std::endl;
      } */
}


std::vector<PluginInterface*>
PluginContainer::importPlugins()
{
  std::set<std::string> names;

  ///@todo this is not that good
  std::vector<PluginInterface*> plugins;

  for (std::vector<std::string>::const_iterator it = pluginList.begin();
       it != pluginList.end(); ++it)
    {
      if (Globals::Instance()->doVerbose(Globals::PLUGIN_LOADING))
	{
	  Globals::Instance()->getVerboseStream() << "Loading Plugin Library: \"" 
						  << *it 
						  << "\"" 
						  << std::endl;
	}

      lt_dlhandle dlHandle = lt_dlopenext(it->c_str());

      ///@todo if we cannot open the plugin, we bail out. maybe we
      ///should gracefully resuscitate ourselves
      if (dlHandle == NULL)
	{
    std::cerr << "Warning: Cannot open library " + *it + ": " + lt_dlerror() << std::endl;
    continue;
	}
      
      t_import getplugin = (t_import) lt_dlsym(dlHandle, PLUGINIMPORTFUNCTIONSTRING);
      
      if (getplugin != NULL)
	{
	  PluginInterface::AtomFunctionMap pa;
	  
	  PluginInterface* plugin = getplugin();

	  const std::string& pname = plugin->getPluginName();

	  if (names.find(pname) != names.end())
	    {
	      ///@todo is this a warning, or a proper (installation) error?
	      std::cerr << "Warning: Already loaded a plugin with name "
			<< pname
			<< ", ignoring " 
			<< *it
			<< std::endl;
	    }
	  else
	    {
	      names.insert(pname);
	      plugins.push_back(plugin);
	  
	      plugin->getAtoms(pa);
	  
	      for(PluginInterface::AtomFunctionMap::const_iterator it = pa.begin();
		  it != pa.end();
		  ++it)
		{
		  // first come, first serve
		  if (pluginAtoms.find(it->first) == pluginAtoms.end())
		    {
		      pluginAtoms[it->first] = it->second;
		    }
		  else
		    {
		      ///@todo is this a warning, or a proper (installation) error?
		      std::cerr << "Warning: the external atom " << it->first
				<< " is already loaded." << std::endl;
		    }
		}
	    }
	}
      else
        {
          if (Globals::Instance()->doVerbose(Globals::PLUGIN_LOADING))
          {
            Globals::Instance()->getVerboseStream() <<
              "Found no symbol \"" << PLUGINIMPORTFUNCTIONSTRING << "\" "
              "in Library: \"" << *it << "\"" << std::endl;
          }
	}
    }

  return plugins;
}


boost::shared_ptr<PluginAtom>
PluginContainer::getAtom(const std::string& name) const
{
  PluginInterface::AtomFunctionMap::const_iterator pa = pluginAtoms.find(name);

  if (pa == pluginAtoms.end())
    {
      return boost::shared_ptr<PluginAtom>();
    }
    
  return pa->second;
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
