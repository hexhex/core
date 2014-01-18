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
 * @file ManualEvalHeuristicsPlugin.cpp
 * @author Peter Schueller
 *
 * @brief Plugin for specifying evaluation units in HEX input.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/config_values.h"

#include "dlvhex2/ManualEvalHeuristicsPlugin.h"
#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Printhelpers.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/HexParser.h"
#include "dlvhex2/HexParserModule.h"
#include "dlvhex2/HexGrammar.h"

#include "dlvhex2/EvalHeuristicBase.h"
#include "dlvhex2/EvalHeuristicShared.h"
#include "dlvhex2/EvalGraphBuilder.h"

DLVHEX_NAMESPACE_BEGIN

namespace spirit = boost::spirit;
namespace qi = boost::spirit::qi;

ManualEvalHeuristicsPlugin::CtxData::CtxData():
	enabled(false),
	lastUserRuleID(ID_FAIL),
	currentUnit(0)
{
}

ManualEvalHeuristicsPlugin::ManualEvalHeuristicsPlugin():
	PluginInterface()
{
	setNameVersion("dlvhex-manualevalheuristicsplugin[internal]", 2, 0, 0);
}

ManualEvalHeuristicsPlugin::~ManualEvalHeuristicsPlugin()
{
}

// output help message for this plugin
void ManualEvalHeuristicsPlugin::printUsage(std::ostream& o) const
{
  //    123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
	o << "     --manualevalheuristics-enable" << std::endl <<
       "                  Enable parsing and processing of '#evalunit(...).' instructions." << std::endl;
}

// accepted options: --manualevalheuristics-enable
//
// processes options for this plugin, and removes recognized options from pluginOptions
// (do not free the pointers, the const char* directly come from argv)
//
// configures custom evaluation heuristics
void ManualEvalHeuristicsPlugin::processOptions(
		std::list<const char*>& pluginOptions,
		ProgramCtx& ctx)
{
	ManualEvalHeuristicsPlugin::CtxData& ctxdata = ctx.getPluginData<ManualEvalHeuristicsPlugin>();

	typedef std::list<const char*>::iterator Iterator;
	Iterator it;
	#warning create (or reuse, maybe from potassco?) cmdline option processing facility
	it = pluginOptions.begin();
	while( it != pluginOptions.end() )
	{
		bool processed = false;
		const std::string str(*it);
		if( str == "--manualevalheuristics-enable" )
		{
			ctxdata.enabled = true;
			processed = true;
		}

		if( processed )
		{
			// return value of erase: element after it, maybe end()
			DBGLOG(DBG,"ManualEvalHeuristicsPlugin successfully processed option " << str);
			it = pluginOptions.erase(it);
		}
		else
		{
			it++;
		}
	}

	// register eval heuristics
	if( ctxdata.enabled )
	{
		// directly uses data from ctxdata
		ctx.config.setOption(CFG_EVAL_HEURISTIC, Eval_FromHEXSourcecode);
	}
}
	
class ManualEvalHeuristicsParserModuleSemantics:
	public HexGrammarSemantics
{
public:
	ManualEvalHeuristicsPlugin::CtxData& ctxdata;

public:
	ManualEvalHeuristicsParserModuleSemantics(ProgramCtx& ctx):
		HexGrammarSemantics(ctx),
		ctxdata(ctx.getPluginData<ManualEvalHeuristicsPlugin>())
	{
	}

	// use SemanticActionBase to redirect semantic action call into globally
	// specializable sem<T> struct space
	struct evalUnit:
		SemanticActionBase<ManualEvalHeuristicsParserModuleSemantics, ID, evalUnit>
	{
		evalUnit(ManualEvalHeuristicsParserModuleSemantics& mgr):
			evalUnit::base_type(mgr)
		{
		}
	};
};

// create semantic handler for above semantic action
// (needs to be in globally specializable struct space)
template<>
struct sem<ManualEvalHeuristicsParserModuleSemantics::evalUnit>
{
  void operator()(
    ManualEvalHeuristicsParserModuleSemantics& mgr,
    const uint32_t& unit, // source is not used
    ID&) // the target is not used
  {
		// get largest rule id from registry
		RuleTable::AddressIterator it, it_end;
		boost::tie(it, it_end) = mgr.ctx.registry()->rules.getAllByAddress();
		ID maxruleid;
		if( it != it_end )
		{
			it_end--;
			maxruleid = ID(it->kind, it_end-it); 
			LOG(INFO,"when encountering #evalunit(...). found largest rule id " << maxruleid <<
					" corresponding to rule '" << printToString<RawPrinter>(maxruleid, mgr.ctx.registry()));
		}
		else
		{
			// otherwise maxruleid stays ID_FAIL
			LOG(INFO,"when encountering #evalunit(...). saw no previous rules");
		}
		mgr.ctxdata.instructions.push_back(std::make_pair(maxruleid, unit));
		mgr.ctxdata.currentUnit = unit;
  }
};

namespace
{

template<typename Iterator, typename Skipper>
struct ManualEvalHeuristicsParserModuleGrammarBase:
	// we derive from the original hex grammar
	// -> we can reuse its rules
	public HexGrammarBase<Iterator, Skipper>
{
	typedef HexGrammarBase<Iterator, Skipper> Base;

	ManualEvalHeuristicsParserModuleSemantics& sem;

	ManualEvalHeuristicsParserModuleGrammarBase(ManualEvalHeuristicsParserModuleSemantics& sem):
		Base(sem),
		sem(sem)
	{
		typedef ManualEvalHeuristicsParserModuleSemantics Sem;
		evalUnit =
		       	(
			qi::lit("#evalunit") >> qi::lit("(") >> qi::ulong_ >> qi::lit(")") >> qi::lit(".") > qi::eps
			) [ Sem::evalUnit(sem) ];

		#ifdef BOOST_SPIRIT_DEBUG
		BOOST_SPIRIT_DEBUG_NODE(evalUnit);
		#endif
	}

	qi::rule<Iterator, ID(), Skipper> evalUnit;
};

struct ManualEvalHeuristicsParserModuleGrammar:
  ManualEvalHeuristicsParserModuleGrammarBase<HexParserIterator, HexParserSkipper>,
	// required for interface
  // note: HexParserModuleGrammar =
	//       boost::spirit::qi::grammar<HexParserIterator, HexParserSkipper>
	HexParserModuleGrammar
{
	typedef ManualEvalHeuristicsParserModuleGrammarBase<HexParserIterator, HexParserSkipper> GrammarBase;
  typedef HexParserModuleGrammar QiBase;

  ManualEvalHeuristicsParserModuleGrammar(ManualEvalHeuristicsParserModuleSemantics& sem):
    GrammarBase(sem),
    QiBase(GrammarBase::evalUnit)
  {
  }
};
typedef boost::shared_ptr<ManualEvalHeuristicsParserModuleGrammar>
	ManualEvalHeuristicsParserModuleGrammarPtr;

class ManualEvalHeuristicsParserModule:
	public HexParserModule
{
public:
	// the semantics manager is stored/owned by this module!
	ManualEvalHeuristicsParserModuleSemantics sem;
	// we also keep a shared ptr to the grammar module here
	ManualEvalHeuristicsParserModuleGrammarPtr grammarModule;

	ManualEvalHeuristicsParserModule(ProgramCtx& ctx):
		HexParserModule(TOPLEVEL),
		sem(ctx)
	{
		LOG(INFO,"constructed ManualEvalHeuristicsParserModule");
	}

	virtual HexParserModuleGrammarPtr createGrammarModule()
	{
		assert(!grammarModule && "for simplicity (storing only one grammarModule pointer) we currently assume this will be called only once .. should be no problem to extend");
		grammarModule.reset(new ManualEvalHeuristicsParserModuleGrammar(sem));
		LOG(INFO,"created ManualEvalHeuristicsParserModuleGrammar");
		return grammarModule;
	}
};

} // anonymous namespace

// create parser modules that extend and the basic hex grammar
// this parser also stores the query information into the plugin
std::vector<HexParserModulePtr>
ManualEvalHeuristicsPlugin::createParserModules(ProgramCtx& ctx)
{
	DBGLOG(DBG,"ManualEvalHeuristicsPlugin::createParserModules()");
	std::vector<HexParserModulePtr> ret;

	ManualEvalHeuristicsPlugin::CtxData& ctxdata = ctx.getPluginData<ManualEvalHeuristicsPlugin>();
	if( ctxdata.enabled )
	{
		ret.push_back(HexParserModulePtr(
					new ManualEvalHeuristicsParserModule(ctx)));
	}

	return ret;
}

namespace
{
class ManualEvalHeuristicsPluginRewriter:
       	public PluginRewriter {
public:
	ManualEvalHeuristicsPlugin::CtxData& ctxdata;

public:
	ManualEvalHeuristicsPluginRewriter(ProgramCtx& ctx):
		ctxdata(ctx.getPluginData<ManualEvalHeuristicsPlugin>()) {
	}

	virtual ~ManualEvalHeuristicsPluginRewriter() {}

	virtual void rewrite(ProgramCtx& ctx) {
		// we do not rewrite, but we gather information from the parsed program

		RuleTable::AddressIterator it, it_end;
		boost::tie(it, it_end) = ctx.registry()->rules.getAllByAddress();
		if( it != it_end )
		{
			it_end--;
			ctxdata.lastUserRuleID = ID(it_end->kind, it_end-it);
		}
		else
			ctxdata.lastUserRuleID = ID_FAIL;
		LOG(INFO,"ManualEvalHeuristicsPluginRewriter got lastUserRuleID=" << ctxdata.lastUserRuleID);
	}
};
}

PluginRewriterPtr
ManualEvalHeuristicsPlugin::createRewriter(ProgramCtx& ctx)
{
	ManualEvalHeuristicsPlugin::CtxData& ctxdata = ctx.getPluginData<ManualEvalHeuristicsPlugin>();
	if( ctxdata.enabled )
		return PluginRewriterPtr(new ManualEvalHeuristicsPluginRewriter(ctx));
	else
		return PluginRewriterPtr();
}

DLVHEX_NAMESPACE_END

// this would be the code to use this plugin as a "real" plugin in a .so file
// but we directly use it in dlvhex.cpp
#if 0
ManualEvalHeuristicsPlugin theManualEvalHeuristicsPlugin;

// return plain C type s.t. all compilers and linkers will like this code
extern "C"
void * PLUGINIMPORTFUNCTION()
{
	return reinterpret_cast<void*>(& DLVHEX_NAMESPACE theManualEvalHeuristicsPlugin);
}

#endif

// Local Variables:
// mode: C++
// End:
