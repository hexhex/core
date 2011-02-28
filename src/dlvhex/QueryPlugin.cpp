/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Thomas Krennwallner
 * Copyright (C) 2009, 2010, 2011 Peter Schüller
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
 * @file QueryPlugin.cpp
 * @author Peter Schueller
 *
 * @brief Plugin for cautions/brave ground/nonground queries in dlvhex.
 */

#include "dlvhex/QueryPlugin.hpp"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

//#include "dlvhex/ComfortPluginInterface.hpp"
//#include "dlvhex/Term.hpp"
//#include "dlvhex/Registry.hpp"
//
//#include <boost/foreach.hpp>
//
//#include <string>
//#include <sstream>
//#include <iostream>
//
//#include <cstdio>
//#include <cassert>

DLVHEX_NAMESPACE_BEGIN

QueryPlugin::QueryPlugin():
	PluginInterface()
{
	setNameVersion("dlvhex-queryplugin[internal]", 2, 0, 0);
}

QueryPlugin::~QueryPlugin()
{
}

// output help message for this plugin
void QueryPlugin::printUsage(std::ostream& o) const
{
  //    123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
	o << "     --query-enable   Enable this (i.e., the querying) plugin." << std::endl <<
		   "     --query-brave    Do brave reasoning." << std::endl <<
			 "     --query-cautious Do cautious reasoning." << std::endl;
}

// accepted options: --query-enables --query-brave --query-cautious
//
// processes options for this plugin, and removes recognized options from pluginOptions
// (do not free the pointers, the const char* directly come from argv)
void QueryPlugin::processOptions(std::list<const char*>& pluginOptions)
{
	// TODO
}

// create custom parser that extends and uses the basic hex parser for parsing queries
// this parser also stores the query information into the plugin
HexParserPtr QueryPlugin::createParser()
{
	// TODO
	return HexParserPtr();
}

// change model callback and register final callback
void QueryPlugin::setupProgramCtx(ProgramCtx& ctx)
{
	// TODO
}

DLVHEX_NAMESPACE_END

// this would be the code to use this plugin as a "real" plugin in a .so file
// but we directly use it in dlvhex.cpp
#if 0
QueryPlugin theQueryPlugin;

// return plain C type s.t. all compilers and linkers will like this code
extern "C"
void * PLUGINIMPORTFUNCTION()
{
	return reinterpret_cast<void*>(& DLVHEX_NAMESPACE theQueryPlugin);
}
#endif

/* vim: set noet sw=2 ts=2 tw=80: */

// Local Variables:
// mode: C++
// End:
