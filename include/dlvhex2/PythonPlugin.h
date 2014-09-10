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
 * @file PythonPlugin.h
 * @author Christoph Redl
 *
 * @brief Supports Python-implemented plugins.
 */

#ifndef PYTHON_PLUGIN__HPP_INCLUDED
#define PYTHON_PLUGIN__HPP_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/PluginInterface.h"

#include <string>
#include <vector>

#ifdef HAVE_PYTHON

#include <Python.h>

DLVHEX_NAMESPACE_BEGIN

class PythonPlugin:
  public PluginInterface
{
public:
	// stored in ProgramCtx, accessed using getPluginData<PythonPlugin>()
	class CtxData:
	public PluginData
	{
		public:

		PyObject *pModule;
		std::vector<std::string> pythonScripts;

		CtxData();
		virtual ~CtxData();
	};

public:
	PythonPlugin();
	virtual ~PythonPlugin();

	// output help message for this plugin
	virtual void printUsage(std::ostream& o) const;

	// accepted options: --aggregate-enable
	//
	// processes options for this plugin, and removes recognized options from pluginOptions
	// (do not free the pointers, the const char* directly come from argv)
	virtual void processOptions(std::list<const char*>& pluginOptions, ProgramCtx&);

	// rewrite program: rewrite aggregate atoms to external atoms
	virtual PluginRewriterPtr createRewriter(ProgramCtx&);

	// register model callback which transforms all auxn(p,t1,...,tn) back to p(t1,...,tn)
	virtual void setupProgramCtx(ProgramCtx&);

	virtual std::vector<PluginAtomPtr> createAtoms(ProgramCtx&) const;

	// no atoms!
};

DLVHEX_NAMESPACE_END

#endif

#endif

