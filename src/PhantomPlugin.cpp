/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Thomas Krennwallner
 * Copyright (C) 2009, 2010, 2011 Peter Schüller
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
 * @file PhantomPlugin.cpp
 * @author Christoph Redl
 *
 * @brief Provides dummy implementations of external predicates
 *        which are never evaluated. This is useful in combination
 *        with special model generators.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/PhantomPlugin.h"
#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Printhelpers.h"
#include "dlvhex2/Logger.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

DLVHEX_NAMESPACE_BEGIN

PhantomPlugin::PhantomPlugin():
	PluginInterface()
{
	setNameVersion("dlvhex-phantomplugin[internal]", 2, 0, 0);
}

PhantomPlugin::~PhantomPlugin()
{
}

std::vector<PluginAtomPtr> PhantomPlugin::createAtoms(ProgramCtx& ctx) const{
	std::vector<PluginAtomPtr> ret;

	if (ctx.config.getOption("Repair")){
		std::vector<PluginAtom::InputType> it;

		// This defines dummy external dlC and dlR atoms (without implementation) for repair answer set computation
		// @TODO: update predicate names, types of input parameters and output arities (if necessary)
		it.clear();
		it.push_back(PluginAtom::CONSTANT);
		it.push_back(PluginAtom::PREDICATE);
		it.push_back(PluginAtom::PREDICATE);
		it.push_back(PluginAtom::PREDICATE);
		it.push_back(PluginAtom::PREDICATE);
		it.push_back(PluginAtom::CONSTANT);
		// Arguments of PhantomPluginAtom constructor: predicate name, monotonicity, vector of parameter types, output arity
		// (do not use dlC and dlR as predicate names to avoid a clash with the real dlplugin)
		ret.push_back(PluginAtomPtr(new PhantomPluginAtom("repairDLC", false, it, 1)));

		it.clear();
		it.push_back(PluginAtom::CONSTANT);
		it.push_back(PluginAtom::PREDICATE);
		it.push_back(PluginAtom::PREDICATE);
		it.push_back(PluginAtom::PREDICATE);
		it.push_back(PluginAtom::PREDICATE);
		it.push_back(PluginAtom::CONSTANT);
		ret.push_back(PluginAtomPtr(new PhantomPluginAtom("repairDLR", false, it, 2)));
	}

	return ret;
}

DLVHEX_NAMESPACE_END

// this would be the code to use this plugin as a "real" plugin in a .so file
// but we directly use it in dlvhex.cpp
#if 0
PhantomPlugin thePhantomPlugin;

// return plain C type s.t. all compilers and linkers will like this code
extern "C"
void * PLUGINIMPORTFUNCTION()
{
	return reinterpret_cast<void*>(& DLVHEX_NAMESPACE thePhantomPlugin);
}

#endif
/* vim: set noet sw=2 ts=2 tw=80: */

// Local Variables:
// mode: C++
// End:
