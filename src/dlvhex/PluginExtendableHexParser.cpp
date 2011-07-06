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
 * @file   HexParser.cpp
 * @author Peter Schüller
 * 
 * @brief  HEX parser implementation
 */

#ifdef BOOST_SPIRIT_DEBUG
# define BOOST_SPIRIT_DEBUG_OUT std::cerr
# define BOOST_SPIRIT_DEBUG_FLAGS BOOST_SPIRIT_DEBUG_FLAGS_MAX
# include <boost/spirit/debug.hpp>
#endif
#include "dlvhex/PluginExtendableHexParser.hpp"

#include "dlvhex/ProgramCtx.h"
#include "dlvhex/Registry.hpp"
#include "dlvhex/Printer.hpp"
#include "dlvhex/Logger.hpp"
#include "dlvhex/HexParser.hpp"
#include "dlvhex/HexGrammar.h"
#include "dlvhex/HexGrammarPTToASTConverter.h"
#include "dlvhex/SpiritDebugging.h"

DLVHEX_NAMESPACE_BEGIN

namespace
{

struct ExtendableHexGrammarBase:
	public HexGrammarBase
{
	typedef HexGrammarBase Base;

	enum QueryTags { MinTag = Base::MaxTag,
		OtherClauseParsers,
		OtherBodyPredicateParsers,
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

    definition(ExtendableHexGrammarBase const& self);

    boost::spirit::rule<S, c, tag<OtherClauseParsers> > otherClause;
    boost::spirit::rule<S, c, tag<OtherBodyPredicateParsers> > otherBodyPredicate;
	};
};

struct ExtendableHexGrammar:
  public boost::spirit::grammar<ExtendableHexGrammar>,
  public ExtendableHexGrammarBase
{
};

template<typename ScannerT>
ExtendableHexGrammarBase::definition<ScannerT>::definition(ExtendableHexGrammarBase const& self):
	Base(self)
{
  namespace sp = boost::spirit;
  using sp::ch_p;

  // shortcut for sp::discard_node_d()
  const sp::node_parser_gen<sp::discard_node_op> rm =
    sp::node_parser_gen<sp::discard_node_op>();

  // base case: match nothing, extend grammar by nothing
	otherClause = sp::nothing_p;
	otherBodyPredicate = sp::nothing_p;

	Base::clause =
    Base::maxint | Base::namespace_ | Base::rule_ | Base::constraint | Base::wconstraint | otherClause;
	Base::body_pred =
    Base::user_pred | Base::external_atom | Base::aggregate | otherBodyPredicate;

#   ifdef BOOST_SPIRIT_DEBUG
    BOOST_SPIRIT_DEBUG_NODE(otherClause);
    BOOST_SPIRIT_DEBUG_NODE(otherBodyPredicate);
#   endif
}

} // anonymous namespace

struct PluginExtendableHexParser::Impl
{
};

PluginExtendableHexParser::PluginExtendableHexParser():
	pimpl(new Impl)
{
}

PluginExtendableHexParser::~PluginExtendableHexParser()
{
}

// register parser module
void PluginExtendableHexParser::addModule(
    boost::shared_ptr<PluginExtendableHexParser::ClauseParserModule> module)
{
  #warning todo implement
  throw std::runtime_error("foo");
}

void PluginExtendableHexParser::addModule(
    boost::shared_ptr<PluginExtendableHexParser::BodyPredicateParserModule> module)
{
  #warning todo implement
  throw std::runtime_error("foo");
}

// virtual
void PluginExtendableHexParser::parse(InputProviderPtr in, ProgramCtx& ctx)
{
  assert(!!in);
  assert(!!ctx.registry());

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
  ExtendableHexGrammar grammar;
  typedef HexGrammarPTToASTConverter Converter;

  Converter::iterator_t it_begin(input.c_str(), input.c_str()+input.size());
  Converter::iterator_t it_end;

  // parse ast
	//LOG(DBG,"parsing with special parser");
  boost::spirit::tree_parse_info<Converter::iterator_t, Converter::factory_t> info =
    boost::spirit::pt_parse<
			Converter::factory_t, Converter::iterator_t, ExtendableHexGrammar>(
        it_begin, it_end, grammar, boost::spirit::space_p);

  // successful parse?
  if( !info.full )
	{
		if( Logger::Instance().shallPrint(Logger::DBG) )
		{
			LOG(DBG,"ExtendableHexParser partial parse tree");
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
}

DLVHEX_NAMESPACE_END
