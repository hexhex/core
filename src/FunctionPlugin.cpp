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
 * @file FunctionPlugin.cpp
 * @author Christoph Redl
 *
 * @brief Support for function symbol handling via external atoms
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

//#define BOOST_SPIRIT_DEBUG

#include "dlvhex2/FunctionPlugin.h"
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
#include "dlvhex2/LiberalSafetyChecker.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

DLVHEX_NAMESPACE_BEGIN

namespace spirit = boost::spirit;
namespace qi = boost::spirit::qi;

FunctionPlugin::CtxData::CtxData():
	maxArity(1)
{
}

FunctionPlugin::FunctionPlugin():
	PluginInterface()
{
	setNameVersion("dlvhex-functionplugin[internal]", 2, 0, 0);
}

FunctionPlugin::~FunctionPlugin()
{
}

// output help message for this plugin
void FunctionPlugin::printUsage(std::ostream& o) const
{
  //    123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
	o << "     --function-maxarity=<N> Maximum number of output terms in functionDecompose." << std::endl;
}

// accepted options: --exists-enable
//
// processes options for this plugin, and removes recognized options from pluginOptions
// (do not free the pointers, the const char* directly come from argv)
void FunctionPlugin::processOptions(
		std::list<const char*>& pluginOptions,
		ProgramCtx& ctx)
{
	FunctionPlugin::CtxData& ctxdata = ctx.getPluginData<FunctionPlugin>();

	typedef std::list<const char*>::iterator Iterator;
	Iterator it;
	#warning create (or reuse, maybe from potassco?) cmdline option processing facility
	it = pluginOptions.begin();
	while( it != pluginOptions.end() )
	{
		bool processed = false;
		const std::string str(*it);
		if( boost::starts_with(str, "--function-maxarity=") )
		{
			ctxdata.maxArity = boost::lexical_cast<int>(str.substr(std::string("--function-maxarity=").length()));
			processed = true;
		}

		if( processed )
		{
			// return value of erase: element after it, maybe end()
			DBGLOG(DBG,"FunctionPlugin successfully processed option " << str);
			it = pluginOptions.erase(it);
		}
		else
		{
			it++;
		}
	}
}

class FunctionComposeAtom : public PluginAtom
{
	public:

		FunctionComposeAtom() : PluginAtom("functionCompose", true)
		{
			prop.functional = true;

			addInputTuple();

			setOutputArity(1);
		}

		virtual void
		retrieve(const Query& query, Answer& answer) throw (PluginError)
		{
			Registry &registry = *getRegistry();

			Term t(ID::MAINKIND_TERM | ID::SUBKIND_TERM_NESTED, query.input, getRegistry());
			ID tid = registry.terms.getIDByString(t.symbol);
			if (tid == ID_FAIL) tid = registry.terms.storeAndGetID(t);
			Tuple tuple;
			tuple.push_back(tid);
			answer.get().push_back(tuple);
		}
};

class FunctionDecomposeAtom : public PluginAtom
{
	private:
		int arity;
		std::string getName(std::string f, int ar){
			std::stringstream ss;
			ss << f << ar;
			return ss.str();
		}

	public:
		FunctionDecomposeAtom(int arity) : PluginAtom(getName("functionDecompose", arity), true), arity(arity)
		{
			prop.functional = true;
			for (int i = 0; i <= arity; ++i){
				prop.wellorderingStrlen.insert(std::pair<int, int>(0, i));
			}

			addInputConstant();

			setOutputArity(arity + 1);
		}

		virtual void
		retrieve(const Query& query, Answer& answer) throw (PluginError)
		{
			Registry &registry = *getRegistry();

			const Term& t = registry.terms.getByID(query.input[0]);
			if (t.isNestedTerm() && t.arguments.size() == arity + 1){
				Tuple tuple;
				BOOST_FOREACH (ID id, t.arguments){
					tuple.push_back(id);
				}
				answer.get().push_back(tuple);
			}
		}
};

class IsFunctionTermAtom : public PluginAtom
{
	public:
		IsFunctionTermAtom() : PluginAtom("isFunctionTerm", true)
		{
			prop.functional = true;

			addInputConstant();

			setOutputArity(0);
		}

		virtual void
		retrieve(const Query& query, Answer& answer) throw (PluginError)
		{
			Registry &registry = *getRegistry();

			const Term& t = registry.terms.getByID(query.input[0]);
			if (t.isNestedTerm()){
				Tuple tuple;
				answer.get().push_back(tuple);
			}
		}
};

class GetArityAtom : public PluginAtom
{
	public:
		GetArityAtom() : PluginAtom("getArity", true)
		{
			prop.functional = true;

			addInputConstant();

			setOutputArity(1);
		}

		virtual void
		retrieve(const Query& query, Answer& answer) throw (PluginError)
		{
			Registry &registry = *getRegistry();

			const Term& t = registry.terms.getByID(query.input[0]);
			if (t.isNestedTerm()){
				Tuple tuple;
				tuple.push_back(ID::termFromInteger(t.arguments.size() - 1));
				answer.get().push_back(tuple);
			}
		}
};

class FunctionDecomposeGeneralAtom : public PluginAtom
{
	public:
		FunctionDecomposeGeneralAtom() : PluginAtom("functionDecompose", true)
		{
			prop.functional = true;
			prop.wellorderingStrlen.insert(std::pair<int, int>(0, 0));

			addInputConstant();
			addInputConstant();

			setOutputArity(1);
		}

		virtual void
		retrieve(const Query& query, Answer& answer) throw (PluginError)
		{
			Registry &registry = *getRegistry();

			const Term& t = registry.terms.getByID(query.input[0]);
			if (t.isNestedTerm()){
				if (!query.input[1].isIntegerTerm() || query.input[1].address >= t.arguments.size()) throw PluginError("Argument position out of bounds");
				Tuple tuple;
				tuple.push_back(t.arguments[query.input[1].address]);
				answer.get().push_back(tuple);
			}
		}
};

std::vector<PluginAtomPtr> FunctionPlugin::createAtoms(ProgramCtx& ctx) const{
	std::vector<PluginAtomPtr> ret;

	// we have to do the program rewriting already here because it creates some side information that we need
	FunctionPlugin::CtxData& ctxdata = ctx.getPluginData<FunctionPlugin>();

	// return smart pointer with deleter (i.e., delete code compiled into this plugin)
	ret.push_back(PluginAtomPtr(new FunctionComposeAtom(), PluginPtrDeleter<PluginAtom>()));
	DBGLOG(DBG, "Adding functional atom with an input arity of up to " << ctxdata.maxArity);
	for (int i = 0; i <= ctxdata.maxArity; ++i){
		ret.push_back(PluginAtomPtr(new FunctionDecomposeAtom(i), PluginPtrDeleter<PluginAtom>()));
	}
	ret.push_back(PluginAtomPtr(new IsFunctionTermAtom(), PluginPtrDeleter<PluginAtom>()));
	ret.push_back(PluginAtomPtr(new GetArityAtom(), PluginPtrDeleter<PluginAtom>()));
	ret.push_back(PluginAtomPtr(new FunctionDecomposeGeneralAtom(), PluginPtrDeleter<PluginAtom>()));

	return ret;
}

void FunctionPlugin::setupProgramCtx(ProgramCtx& ctx)
{
	FunctionPlugin::CtxData& ctxdata = ctx.getPluginData<FunctionPlugin>();
	RegistryPtr reg = ctx.registry();
}

DLVHEX_NAMESPACE_END

// this would be the code to use this plugin as a "real" plugin in a .so file
// but we directly use it in dlvhex.cpp
#if 0
FunctionPlugin theFunctionPlugin;

// return plain C type s.t. all compilers and linkers will like this code
extern "C"
void * PLUGINIMPORTFUNCTION()
{
	return reinterpret_cast<void*>(& DLVHEX_NAMESPACE theFunctionPlugin);
}

#endif
/* vim: set noet sw=2 ts=2 tw=80: */

// Local Variables:
// mode: C++
// End:
