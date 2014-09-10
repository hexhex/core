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
 * @file PythonPlugin.cpp
 * @author Christoph Redl
 *
 * @brief Supports Python-implemented plugins.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_PYTHON

#include <Python.h>

//#define BOOST_SPIRIT_DEBUG

#include "dlvhex2/PythonPlugin.h"
#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Printhelpers.h"
#include "dlvhex2/PredicateMask.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/HexParser.h"
#include "dlvhex2/HexParserModule.h"
#include "dlvhex2/HexGrammar.h"
#include "dlvhex2/ExternalLearningHelper.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

DLVHEX_NAMESPACE_BEGIN

PythonPlugin::CtxData::CtxData()
{
}

PythonPlugin::PythonPlugin():
	PluginInterface()
{
	setNameVersion("dlvhex-pythonplugin[internal]", 2, 0, 0);
}

PythonPlugin::~PythonPlugin()
{
}

// output help message for this plugin
void PythonPlugin::printUsage(std::ostream& o) const
{
  //    123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
	o << "     --pluginscript=[PATH]    Add Python script \"PATH\" as new plugin." << std::endl;
}

// accepted options: --pluginscript=[PATH]
//
// processes options for this plugin, and removes recognized options from pluginOptions
// (do not free the pointers, the const char* directly come from argv)
void PythonPlugin::processOptions(
		std::list<const char*>& pluginOptions,
		ProgramCtx& ctx)
{
	PythonPlugin::CtxData& ctxdata = ctx.getPluginData<PythonPlugin>();

	typedef std::list<const char*>::iterator Iterator;
	Iterator it;
	WARNING("create (or reuse, maybe from potassco?) cmdline option processing facility")
	it = pluginOptions.begin();
	while( it != pluginOptions.end() )
	{
		bool processed = false;
		const std::string str(*it);
		if( boost::starts_with(str, "--pluginscript=") )
		{
			ctxdata.pythonScripts.push_back(str.substr(std::string("--pluginscript=").length()));
			processed = true;
		}

		if( processed )
		{
			// return value of erase: element after it, maybe end()
			DBGLOG(DBG,"PythonPlugin successfully processed option " << str);
			it = pluginOptions.erase(it);
		}
		else
		{
			it++;
		}
	}
}

namespace
{

typedef PythonPlugin::CtxData CtxData;

class PythonRewriter:
	public PluginRewriter
{
private:
	PythonPlugin::CtxData& ctxdata;
public:
	PythonRewriter(PythonPlugin::CtxData& ctxdata) : ctxdata(ctxdata) {}
	virtual ~PythonRewriter() {}

	virtual void rewrite(ProgramCtx& ctx);
};

void PythonRewriter::rewrite(ProgramCtx& ctx)
{
}

} // anonymous namespace

// rewrite program
PluginRewriterPtr PythonPlugin::createRewriter(ProgramCtx& ctx)
{
	PythonPlugin::CtxData& ctxdata = ctx.getPluginData<PythonPlugin>();
	return PluginRewriterPtr(new PythonRewriter(ctxdata));
}

// register auxiliary printer for strong negation auxiliaries
void PythonPlugin::setupProgramCtx(ProgramCtx& ctx)
{
	PythonPlugin::CtxData& ctxdata = ctx.getPluginData<PythonPlugin>();
	RegistryPtr reg = ctx.registry();
}

namespace{

class PythonAtom : public PluginAtom
{
	private:
		PyObject *pModule;
		PyObject *pFunc;

	public:
		PythonAtom(ProgramCtx& ctx, PyObject *pModule, std::string functionName)
			: PluginAtom(functionName.c_str(), false), pModule(pModule)
		{
			addInputPredicate();
			addInputPredicate();
			addInputConstant();

			setOutputArity(1);

			pFunc = PyObject_GetAttrString(pModule, functionName.c_str());
		}

		virtual ~PythonAtom(){
			Py_XDECREF(pFunc);
		}

		virtual void
		retrieve(const Query& query, Answer& answer) throw (PluginError)
		{
			PyObject *pArgs, *pValue;
			PyObject_CallObject(pFunc, pArgs);

/*
			Py_SetProgramName("python_plugin");
			Py_Initialize();
			PyRun_SimpleString("from time import time,ctime\n"
				     "print 'Today is',ctime(time())\n");
			Py_Finalize();
*/

		}
};

}

std::vector<PluginAtomPtr> PythonPlugin::createAtoms(ProgramCtx& ctx) const{
	std::vector<PluginAtomPtr> ret;

	// we have to do the program rewriting already here because it creates some side information that we need
	PythonPlugin::CtxData& ctxdata = ctx.getPluginData<PythonPlugin>();

	BOOST_FOREACH (std::string script, ctxdata.pythonScripts){

		PyObject *pModule, *pName, *pDict, *pFunc;
		PyObject *pArgs, *pValue;

		Py_Initialize();

		DBGLOG(DBG, "PythonPlugin: Loading script \"" << script << "\"");
		pName = PyString_FromString(script.c_str());
		pModule = PyImport_Import(pName);

		Py_DECREF(pName);

		if (pModule != NULL) {
			DBGLOG(DBG, "PythonPlugin: Loading atoms from script \"" << script << "\"");
			pFunc = PyObject_GetAttrString(pModule, "register");

			/* pFunc is a new reference */
			if (pFunc && PyCallable_Check(pFunc)) {
				pArgs = PyTuple_New(0);
/*
				for (int i = 0; i < 2; ++i) {
					pValue = PyInt_FromLong(2);
					PyTuple_SetItem(pArgs, i, pValue);
				}
*/
				pValue = PyObject_CallObject(pFunc, pArgs);
				Py_DECREF(pArgs);
				if (pValue != NULL) {
					// return smart pointer with deleter (i.e., delete code compiled into this plugin)
					ret.push_back(PluginAtomPtr(new PythonAtom(ctx, pModule, PyString_AsString(pValue)), PluginPtrDeleter<PluginAtom>()));

					std::cout << PyString_AsString(pValue) << std::endl;
					Py_DECREF(pValue);
				}else{
					Py_DECREF(pFunc);
					PyErr_Print();
				}
			}else{
				if (PyErr_Occurred()) PyErr_Print();
			}
			Py_XDECREF(pFunc);
		}else{
		        PyErr_Print();
		}
	}

	return ret;
}

DLVHEX_NAMESPACE_END

#endif

// this would be the code to use this plugin as a "real" plugin in a .so file
// but we directly use it in dlvhex.cpp
#if 0
PythonPlugin thePythonPlugin;

// return plain C type s.t. all compilers and linkers will like this code
extern "C"
void * PLUGINIMPORTFUNCTION()
{
	return reinterpret_cast<void*>(& DLVHEX_NAMESPACE thePythonPlugin);
}

#endif
/* vim: set noet sw=2 ts=2 tw=80: */

// Local Variables:
// mode: C++
// End:
