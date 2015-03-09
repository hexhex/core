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
 * @file ConditionalLiteralPlugin.cpp
 * @author Christoph Redl
 *
 * @brief Support for conditional literals in rule bodies.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

//#define BOOST_SPIRIT_DEBUG

#include "dlvhex2/ConditionalLiteralPlugin.h"
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

ConditionalLiteralPlugin::CtxData::CtxData():
	enabled(false)
{
}

ConditionalLiteralPlugin::ConditionalLiteralPlugin():
	PluginInterface()
{
	setNameVersion("dlvhex-ConditionalLiteralPlugin[internal]", 2, 0, 0);
}

ConditionalLiteralPlugin::~ConditionalLiteralPlugin()
{
}

// output help message for this plugin
void ConditionalLiteralPlugin::printUsage(std::ostream& o) const
{
  //    123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
	o << "     --conditinal-enable[=true,false]" << std::endl
          << "                      Enable conditional literals (default is enabled)." << std::endl;
}

// accepted options: --choice-enable
//
// processes options for this plugin, and removes recognized options from pluginOptions
// (do not free the pointers, the const char* directly come from argv)
void ConditionalLiteralPlugin::processOptions(
		std::list<const char*>& pluginOptions,
		ProgramCtx& ctx)
{
	ConditionalLiteralPlugin::CtxData& ctxdata = ctx.getPluginData<ConditionalLiteralPlugin>();
	ctxdata.enabled = true;

	typedef std::list<const char*>::iterator Iterator;
	Iterator it;
	WARNING("create (or reuse, maybe from potassco?) cmdline option processing facility")
	it = pluginOptions.begin();
	while( it != pluginOptions.end() )
	{
		bool processed = false;
		const std::string str(*it);
		if( boost::starts_with(str, "--conditional-enable" ) )
		{
			std::string m = str.substr(std::string("--conditional-enable").length());
			if (m == "" || m == "=true"){
				ctxdata.enabled = true;
			}else if (m == "=false"){
				ctxdata.enabled = false;
			}else{
				std::stringstream ss;
				ss << "Unknown --conditional-enable option: " << m;
				throw PluginError(ss.str());
			}
			processed = true;
		}

		if( processed )
		{
			// return value of erase: element after it, maybe end()
			DBGLOG(DBG,"ConditionalLiteralPlugin successfully processed option " << str);
			it = pluginOptions.erase(it);
		}
		else
		{
			it++;
		}
	}
}
	
class ConditionalParserModuleSemantics:
	public HexGrammarSemantics
{
public:
	int varnr;
	ConditionalLiteralPlugin::CtxData& ctxdata;

public:
	ConditionalParserModuleSemantics(ProgramCtx& ctx):
		HexGrammarSemantics(ctx),
		varnr(0),
		ctxdata(ctx.getPluginData<ConditionalLiteralPlugin>())
	{
	}

	// use SemanticActionBase to redirect semantic action call into globally
	// specializable sem<T> struct space
	struct conditionalLieral:
		SemanticActionBase<ConditionalParserModuleSemantics, ID, conditionalLieral>
	{
		conditionalLieral(ConditionalParserModuleSemantics& mgr):
			conditionalLieral::base_type(mgr)
		{
		}
	};
};

template<>
struct sem<ConditionalParserModuleSemantics::conditionalLieral>
{
  void operator()(
    ConditionalParserModuleSemantics& mgr,
		const boost::fusion::vector2<
		  	dlvhex::ID,
			boost::optional<boost::optional<std::vector<dlvhex::ID> > >
		>& source,
    ID& target)
  {
	RegistryPtr reg = mgr.ctx.registry();

	ID derivedAtomID = boost::fusion::at_c<0>(source);

	Rule condRule(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR);

	// count instances of the conditional part
	DBGLOG(DBG, "Creating aggregate #count{ ... : condition(...) }");
	AggregateAtom cnt1(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_AGGREGATE);
	cnt1.tuple[0] = reg->getAuxiliaryVariableSymbol('l', ID::termFromInteger(mgr.varnr++));
	cnt1.tuple[1] = ID::termFromBuiltin(ID::TERM_BUILTIN_EQ);
	cnt1.tuple[2] = ID::termFromBuiltin(ID::TERM_BUILTIN_AGGCOUNT);
	cnt1.tuple[3] = ID_FAIL;
	cnt1.tuple[4] = ID_FAIL;
	ID cnt1ID;
	std::set<ID> vars1;
	cnt1.variables.insert(cnt1.variables.end(), vars1.begin(), vars1.end());
	if (!!boost::fusion::at_c<1>(source) && !!boost::fusion::at_c<1>(source).get()) cnt1.literals = boost::fusion::at_c<1>(source).get().get();
	BOOST_FOREACH (ID b, cnt1.literals) reg->getVariablesInID(b, vars1);
	cnt1ID = reg->aatoms.storeAndGetID(cnt1);
	DBGLOG(DBG, "Result: " << printToString<RawPrinter>(cnt1ID, reg));
	condRule.body.push_back(ID::posLiteralFromAtom(cnt1ID));

	// count instances of the derived atom
	DBGLOG(DBG, "Creating aggregate #count{ ... : derived(...), condition(...) }");
	AggregateAtom cnt2(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_AGGREGATE);
	cnt2.tuple[0] = reg->getAuxiliaryVariableSymbol('l', ID::termFromInteger(mgr.varnr++));
	cnt2.tuple[1] = ID::termFromBuiltin(ID::TERM_BUILTIN_EQ);
	cnt2.tuple[2] = ID::termFromBuiltin(ID::TERM_BUILTIN_AGGCOUNT);
	cnt2.tuple[3] = ID_FAIL;
	cnt2.tuple[4] = ID_FAIL;
	ID cnt2ID;
	std::set<ID> vars2;
	cnt2.variables.insert(cnt1.variables.end(), vars2.begin(), vars2.end());
	if (!!boost::fusion::at_c<1>(source) && !!boost::fusion::at_c<1>(source).get()) cnt2.literals = boost::fusion::at_c<1>(source).get().get();
	cnt2.literals.push_back(derivedAtomID);
	BOOST_FOREACH (ID b, cnt2.literals) reg->getVariablesInID(b, vars2);
	cnt2ID = reg->aatoms.storeAndGetID(cnt2);
	DBGLOG(DBG, "Result: " << printToString<RawPrinter>(cnt2ID, reg));
	condRule.body.push_back(ID::posLiteralFromAtom(cnt2ID));

	// compare the two numbers
	BuiltinAtom bi(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_BUILTIN);
	bi.tuple.push_back(ID::termFromBuiltin(ID::TERM_BUILTIN_ADD));
	bi.tuple.push_back(cnt1.tuple[0]);
	bi.tuple.push_back(cnt2.tuple[0]);
	bi.tuple.push_back(reg->getAuxiliaryVariableSymbol('l', ID::termFromInteger(mgr.varnr++)));

//	target = reg->storeRule(r);
  }
};

namespace
{

template<typename Iterator, typename Skipper>
struct ConditionalParserModuleGrammarBase:
	// we derive from the original hex grammar
	// -> we can reuse its rules
	public HexGrammarBase<Iterator, Skipper>
{
	typedef HexGrammarBase<Iterator, Skipper> Base;

	ConditionalParserModuleSemantics& sem;

	ConditionalParserModuleGrammarBase(ConditionalParserModuleSemantics& sem):
		Base(sem),
		sem(sem)
	{
		typedef ConditionalParserModuleSemantics Sem;

		conditionalLieral
			= (
					Base::classicalAtom >> -(qi::lit(':') >> (Base::bodyLiteral % qi::lit(';'))) > qi::eps
				) [ Sem::conditionalLieral(sem) ];

		#ifdef BOOST_SPIRIT_DEBUG
		BOOST_SPIRIT_DEBUG_NODE(conditionalLieral);
		#endif
	}

	qi::rule<Iterator, ID(), Skipper> conditionalLieral;
};

struct ConditionalParserModuleGrammar:
  ConditionalParserModuleGrammarBase<HexParserIterator, HexParserSkipper>,
	// required for interface
  // note: HexParserModuleGrammar =
	//       boost::spirit::qi::grammar<HexParserIterator, HexParserSkipper>
	HexParserModuleGrammar
{
	typedef ConditionalParserModuleGrammarBase<HexParserIterator, HexParserSkipper> GrammarBase;
  typedef HexParserModuleGrammar QiBase;

  ConditionalParserModuleGrammar(ConditionalParserModuleSemantics& sem):
    GrammarBase(sem),
    QiBase(GrammarBase::conditionalLieral)
  {
  }
};
typedef boost::shared_ptr<ConditionalParserModuleGrammar>
	ConditionalParserModuleGrammarPtr;

// moduletype = HexParserModule::BODYATOM
template<enum HexParserModule::Type moduletype>
class ConditionalParserModule:
	public HexParserModule
{
public:
	// the semantics manager is stored/owned by this module!
	ConditionalParserModuleSemantics sem;
	// we also keep a shared ptr to the grammar module here
	ConditionalParserModuleGrammarPtr grammarModule;

	ConditionalParserModule(ProgramCtx& ctx):
		HexParserModule(moduletype),
		sem(ctx)
	{
		LOG(INFO,"constructed ConditionalParserModule");
	}

	virtual HexParserModuleGrammarPtr createGrammarModule()
	{
		assert(!grammarModule && "for simplicity (storing only one grammarModule pointer) we currently assume this will be called only once .. should be no problem to extend");
		grammarModule.reset(new ConditionalParserModuleGrammar(sem));
		LOG(INFO,"created ConditionalParserModuleGrammar");
		return grammarModule;
	}
};

} // anonymous namespace

// create parser modules that extend and the basic hex grammar
// this parser also stores the query information into the plugin
std::vector<HexParserModulePtr>
ConditionalLiteralPlugin::createParserModules(ProgramCtx& ctx)
{
	DBGLOG(DBG,"ConditionalLiteralPlugin::createParserModules()");
	std::vector<HexParserModulePtr> ret;

	ConditionalLiteralPlugin::CtxData& ctxdata = ctx.getPluginData<ConditionalLiteralPlugin>();
	if( ctxdata.enabled )
	{
		ret.push_back(HexParserModulePtr(
					new ConditionalParserModule<HexParserModule::BODYATOM>(ctx)));
	}

	return ret;
}

std::vector<PluginAtomPtr> ConditionalLiteralPlugin::createAtoms(ProgramCtx& ctx) const{
	std::vector<PluginAtomPtr> ret;
	// we don't have external atoms, only a parer plugin and a rewriter
	return ret;
}

void ConditionalLiteralPlugin::setupProgramCtx(ProgramCtx& ctx)
{
	ConditionalLiteralPlugin::CtxData& ctxdata = ctx.getPluginData<ConditionalLiteralPlugin>();
	if( !ctxdata.enabled )
		return;

	RegistryPtr reg = ctx.registry();
}

DLVHEX_NAMESPACE_END

// this would be the code to use this plugin as a "real" plugin in a .so file
// but we directly use it in dlvhex.cpp
#if 0
ConditionalLiteralPlugin theConditionalLiteralPlugin;

// return plain C type s.t. all compilers and linkers will like this code
extern "C"
void * PLUGINIMPORTFUNCTION()
{
	return reinterpret_cast<void*>(& DLVHEX_NAMESPACE theConditionalLiteralPlugin);
}

#endif
/* vim: set noet sw=2 ts=2 tw=80: */

// Local Variables:
// mode: C++
// End:
