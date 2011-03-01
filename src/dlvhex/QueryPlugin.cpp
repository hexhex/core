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
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/HexParser.hpp"
#include "dlvhex/HexGrammar.h"
#include "dlvhex/HexGrammarPTToASTConverter.h"
#include "dlvhex/SpiritDebugging.h"

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

namespace
{

using dlvhex::QueryPlugin;

struct HexQueryGrammar:
	public dlvhex::HexGrammar
{
	enum QueryTags { Query };

  // S = ScannerT
  template<typename S>
  struct definition:
		public HexGrammar::definition<S>
  {
		typedef typename HexGrammar::definition<S> Base;

    typedef boost::spirit::parser_context<> c;
    template<int Tag> struct tag: public boost::spirit::parser_tag<Tag> {};

    definition(HexQueryGrammar const& self);

    boost::spirit::rule< S, c, tag<Root> > const& start() const { return Base::root; }

    boost::spirit::rule<S, c, tag<Query> > query;
	};
};

template<typename ScannerT>
HexQueryGrammar::definition<ScannerT>::definition(HexQueryGrammar const& gr):
	Base(gr)
{
  //namespace sp = boost::spirit;
  //using sp::str_p;
  //using sp::ch_p;

  // shortcut for sp::discard_node_d()
  //const sp::node_parser_gen<sp::discard_node_op> rm =
  //  sp::node_parser_gen<sp::discard_node_op>();

	query = Base::body >> '?';
	Base::clause = Base::maxint | Base::namespace_ | Base::rule_ |
		query | Base::constraint | Base::wconstraint;
}

class HexQueryGrammarPTToASTConverter:
	public dlvhex::HexGrammarPTToASTConverter
{
public:
	typedef dlvhex::HexGrammarPTToASTConverter Base;

public:
	HexQueryGrammarPTToASTConverter(dlvhex::ProgramCtx& ctx):
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

		LOG(DBG,"child.children.size() = " << child.children.size());
		if( child.children.size() == 4 )
			// nonempty body
			ctxdata.query = createRuleBodyFromBody(child.children[2]);
		LOG(ERROR,"todo continue here");
		throw "foo";
	}
	else
	{
		Base::createASTFromClause(node);
	}
}

}

DLVHEX_NAMESPACE_BEGIN

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

  HexQueryGrammar grammar;
  typedef HexQueryGrammarPTToASTConverter Converter;

  Converter::iterator_t it_begin(input.c_str(), input.c_str()+input.size());
  Converter::iterator_t it_end;

  // parse ast
  boost::spirit::tree_parse_info<Converter::iterator_t, Converter::factory_t> info =
    boost::spirit::pt_parse<Converter::factory_t>(
        it_begin, it_end, grammar, boost::spirit::space_p);

  // successful parse?
  if( !info.full )
    throw SyntaxError("Could not parse complete input!",
        info.stop.get_position().line, "TODO");

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
