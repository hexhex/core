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

//#define BOOST_SPIRIT_DEBUG
#ifdef BOOST_SPIRIT_DEBUG
# define BOOST_SPIRIT_DEBUG_OUT std::cerr
# define BOOST_SPIRIT_DEBUG_FLAGS BOOST_SPIRIT_DEBUG_FLAGS_MAX
# include <boost/spirit/debug.hpp>
#endif
#include "dlvhex/QueryPlugin.hpp"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/Registry.hpp"
#include "dlvhex/HexParser.hpp"
#include "dlvhex/HexGrammar.h"
#include "dlvhex/HexGrammarPTToASTConverter.h"
#include "dlvhex/SpiritDebugging.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

DLVHEX_NAMESPACE_BEGIN

struct HexQueryGrammarBase:
	public HexGrammarBase
{
	typedef HexGrammarBase Base;

	enum QueryTags { MinTag = Base::MaxTag,
		Query,
    MaxTag // this must stay last for extendability!
	};

  // S = ScannerT
  template<typename S>
  struct definition:
		public HexGrammarBase::definition<S>
  {
		typedef typename HexGrammarBase::definition<S> Base;

    typedef boost::spirit::parser_context<> c;
    template<int Tag> struct tag: public boost::spirit::parser_tag<Tag> {};

    definition(HexQueryGrammarBase const& self);
    boost::spirit::rule<S, c, tag<Query> > query;
	};
};

struct HexQueryGrammar:
  public boost::spirit::grammar<HexQueryGrammar>,
  public HexQueryGrammarBase
{
};

template<typename ScannerT>
HexQueryGrammarBase::definition<ScannerT>::definition(HexQueryGrammarBase const& self):
	Base(self)
{
  namespace sp = boost::spirit;
  using sp::ch_p;

  // shortcut for sp::discard_node_d()
  const sp::node_parser_gen<sp::discard_node_op> rm =
    sp::node_parser_gen<sp::discard_node_op>();

	query = Base::body >> rm[ch_p('?')];
	Base::clause = Base::maxint | Base::namespace_ | Base::rule_ | query | Base::constraint | Base::wconstraint;

#   ifdef BOOST_SPIRIT_DEBUG
    BOOST_SPIRIT_DEBUG_NODE(query);
#   endif
}

class HexQueryGrammarPTToASTConverter:
	public HexGrammarPTToASTConverter
{
public:
	typedef HexGrammarPTToASTConverter Base;

public:
	HexQueryGrammarPTToASTConverter(ProgramCtx& ctx):
		Base(ctx) { }
	virtual ~HexQueryGrammarPTToASTConverter() {}
	virtual void createASTFromClause(node_t& node);
};

void HexQueryGrammarPTToASTConverter::createASTFromClause(node_t& node)
{
  // node is from "clause" rule
  assert(node.children.size() == 1);
  node_t& child = node.children[0];
  if( Logger::Instance().shallPrint(Logger::DBG) )
  {
    LOG(DBG,"QueryGrammarPTToAstConverter::createASTFromClause:");
		printSpiritPT(Logger::Instance().stream(), child, "cAFC");
  }

	if( child.value.id().to_long() == HexQueryGrammar::Query )
	{
		QueryPlugin::CtxData& ctxdata =
			ctx.getPluginData<QueryPlugin>();

		if( !ctxdata.query.empty() )
		{
			LOG(WARNING,"got more than one query, ignoring all but the first one!");
			return;
		}

		LOG(DBG,"child.children.size() = " << child.children.size());
		assert(child.children.size() == 1);
		// query body
		ctxdata.query = createRuleBodyFromBody(child.children[0]);

		// get variables/check groundness
		std::set<ID> vars;
		ctx.registry()->getVariablesInTuple(ctxdata.query, vars);
		ctxdata.ground = vars.empty();
		LOG(INFO,"got " << (ctxdata.ground?"ground":"nonground") << " query!");

		// safety of the query is implicitly checked by checking safety
		// of the transformed rules
		#warning we should check query safety explicitly to get better error messages
	}
	else
	{
		Base::createASTFromClause(node);
	}
}

class HexQueryParser:
	public HexParser
{
public:
	HexQueryParser() {}
	virtual ~HexQueryParser() {}
	virtual void parse(InputProviderPtr in, ProgramCtx& ctx);
};

void HexQueryParser::parse(InputProviderPtr in, ProgramCtx& ctx)
{
	#warning this is copied from HexParser.cpp, we should find some more generic way to extend parsers/grammars
  assert(!!in);
  assert(!!ctx.registry());

	QueryPlugin::CtxData& ctxdata =
		ctx.getPluginData<QueryPlugin>();
	assert(ctxdata.enabled && "we should not be here if plugin is not enabled");
	assert(ctxdata.query.empty() &&
			"we do not want to parse if we already have parsed some query");

  if( ctx.edb == 0 )
  {
    // create empty interpretation using this context's registry
    ctx.edb.reset(new Interpretation(ctx.registry()));
  }

  // put whole input from stream into a string
  // (an alternative would be the boost::spirit::multi_pass iterator
  // but this can be done later when the parser is updated to Spirit V2)
  #warning TODO incrementally read and parse this stream
  std::ostringstream buf;
  buf << in->getAsStream().rdbuf();
  std::string input = buf.str();

	//LOG(DBG,"=== parsing input" << std::endl << input << std::endl << "===");
  HexQueryGrammar grammar;
  typedef HexQueryGrammarPTToASTConverter Converter;

  Converter::iterator_t it_begin(input.c_str(), input.c_str()+input.size());
  Converter::iterator_t it_end;

  // parse ast
	//LOG(DBG,"parsing with special parser");
  boost::spirit::tree_parse_info<Converter::iterator_t, Converter::factory_t> info =
    boost::spirit::pt_parse<
			Converter::factory_t, Converter::iterator_t, HexQueryGrammar>(
        it_begin, it_end, grammar, boost::spirit::space_p);

  // successful parse?
  if( !info.full )
	{
		if( Logger::Instance().shallPrint(Logger::DBG) )
		{
			LOG(DBG,"HexQueryParser partial parse tree");
			if( info.trees.empty() )
			{
				LOG(DBG,"(empty)");
			}
			else
			{
				printSpiritPT(Logger::Instance().stream(), *info.trees.begin(), "ppt");
			}
		}
    throw SyntaxError("Could not parse complete input!",
        info.stop.get_position().line, "TODO");
	}

  // if this is not ok, there is some bug and the following code will be incomplete
  assert(info.trees.size() == 1);

  // create dlvhex AST from spirit parser tree
  Converter converter(ctx);
  converter.convertPTToAST(*info.trees.begin());

	if( ctxdata.query.empty() )
		throw FatalError("query mode enabled, but got no query!");
}

QueryPlugin::CtxData::CtxData():
	enabled(false),
	mode(DEFAULT),
	ground(false),
	query()
{
}

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
void QueryPlugin::processOptions(
		std::list<const char*>& pluginOptions,
		ProgramCtx& ctx)
{
	QueryPlugin::CtxData& ctxdata = ctx.getPluginData<QueryPlugin>();

	typedef std::list<const char*>::iterator Iterator;
	Iterator it;
	#warning create (or reuse, maybe from potassco?) cmdline option processing facility
	it = pluginOptions.begin();
	while( it != pluginOptions.end() )
	{
		bool processed = false;
		const std::string str(*it);
		if( str == "--query-enable" )
		{
			ctxdata.enabled = true;
			processed = true;
		}
		else if( str == "--query-brave" )
		{
			ctxdata.mode = CtxData::BRAVE;
			processed = true;
		}
		else if( str == "--query-cautious" )
		{
			ctxdata.mode = CtxData::CAUTIOUS;
			processed = true;
		}

		if( processed )
		{
			// return value of erase: element after it, maybe end()
			DBGLOG(DBG,"QueryPlugin successfully processed option " << str);
			it = pluginOptions.erase(it);
		}
		else
		{
			it++;
		}
	}

	// some checks
	if( ctxdata.mode != CtxData::DEFAULT )
	{
		if( !ctxdata.enabled )
		{
			LOG(WARNING,"querying mode selected, but plugin not enabled "
					"(automatically enabling)");
			ctxdata.enabled = true;
		}
	}
	if( ctxdata.enabled && ctxdata.mode == CtxData::DEFAULT )
		throw FatalError("querying plugin enabled but no querying mode selected");
}

// create custom parser that extends and uses the basic hex parser for parsing queries
// this parser also stores the query information into the plugin
HexParserPtr QueryPlugin::createParser(ProgramCtx& ctx)
{
	QueryPlugin::CtxData& ctxdata = ctx.getPluginData<QueryPlugin>();
	if( !ctxdata.enabled )
		return HexParserPtr();

	return HexParserPtr(new HexQueryParser);
}

namespace
{

class QueryAdderRewriter:
	public PluginRewriter
{
public:
	QueryAdderRewriter() {}
	virtual ~QueryAdderRewriter() {}

  virtual void rewrite(ProgramCtx& ctx);
};

void QueryAdderRewriter::rewrite(ProgramCtx& ctx)
{
	QueryPlugin::CtxData& ctxdata = ctx.getPluginData<QueryPlugin>();
	assert(ctxdata.enabled && "this rewriter should only be used "
			"if the plugin is enabled");

	// convert query
	if( ctxdata.mode == CtxData::BRAVE && ctxdata.ground )
	{
		// from query a_1,...,a_j,not a_{j+1},...,not a_n
		// create constraints
		// :- not a_i. for 1 <= i <= j
		// :- a_i. for j+1 <= i <= n
		// then all answer sets are positive witnesses of the ground query

		throw std::runtime_error("TODO qbg");
	}
	else if( ctxdata.mode == CtxData::CAUTIOUS && ctxdata.ground )
	{
		// from query a_1,...,a_j,not a_{j+1},...,not a_n
		// create constraint
		// :- a_1,...,a_j,not a_{j+1},...,not a_n.
		// then all answer sets are negative witnesses of the ground query

		throw std::runtime_error("TODO qcg");
	}
	else if( ctxdata.mode == CtxData::BRAVE && !ctxdata.ground )
	{
		// from query a_1,...,a_j,not a_{j+1},...,not a_n
		// with variables X_1,...,X_k
		// create rules
		// aux[q0](X_1,...,X_k) :- a_1,...,a_j,not a_{j+1},...,not a_n.
		// aux[q1] :- aux(Q)(X_1,...,X_k).
		// create constraint
		// :- not aux[q1].
		// then all answer sets are positive witnesses of the nonground query
		// and facts aux[q0] in the respective model gives all bravely true substitutions

		throw std::runtime_error("TODO qbn");
	}
	else if( ctxdata.mode == CtxData::CAUTIOUS && !ctxdata.ground )
	{
		// from query a_1,...,a_j,not a_{j+1},...,not a_n
		// with variables X_1,...,X_k
		// create rules
		// aux[q0](X_1,...,X_k) :- a_1,...,a_j,not a_{j+1},...,not a_n.
		// then intersect all answer sets,
		// facts aux[q0] in the resulting model gives all cautiously true substitutions

		throw std::runtime_error("TODO qcn");
	}
	else
	{
		assert("this case should never happen");
	}
}

} // anonymous namespace

// rewrite program by adding auxiliary query rules
PluginRewriterPtr QueryPlugin::createRewriter(ProgramCtx& ctx)
{
	QueryPlugin::CtxData& ctxdata = ctx.getPluginData<QueryPlugin>();
	if( !ctxdata.enabled )
		return PluginRewriterPtr();

	return PluginRewriterPtr(new QueryAdderRewriter);
}

// change model callback and register final callback
void QueryPlugin::setupProgramCtx(ProgramCtx& ctx)
{
	QueryPlugin::CtxData& ctxdata = ctx.getPluginData<QueryPlugin>();
	if( !ctxdata.enabled )
		return;

	throw std::runtime_error("TODO implement");
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
