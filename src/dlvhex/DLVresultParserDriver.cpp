/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * Copyright (C) 2009 Peter Schüller
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
 * @file   DLVresultParserDriver.cpp
 * @author Roman Schindlauer, Peter Schüller
 * @date   Wed Mar 22 14:38:53 CET 2006
 * 
 * @brief  DLV result parser using boost::spirit
 * 
 * 
 */

// use this for developing the parser
#undef CWDEBUG
#ifdef CWDEBUG
# define _GNU_SOURCE
# include <libcwd/sys.h>
# include <libcwd/debug.h>
#endif

#include "dlvhex/DLVresultParserDriver.h"

// use this for debugging parser progress (XML style)
#undef BOOST_SPIRIT_DEBUG

// activate benchmarking if activated by configure option --enable-debug
#ifdef HAVE_CONFIG_H
#  include "config.h"
#  ifdef DLVHEX_DEBUG
#    define DLVHEX_BENCHMARK
#  endif
#endif

#include "dlvhex/Benchmarking.h"
#include "dlvhex/globals.h"

#include <boost/optional.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/support_multi_pass.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <sstream>
#include <iostream>

namespace spirit = boost::spirit;
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace fusion = boost::fusion;
namespace phoenix = boost::phoenix;

DLVHEX_NAMESPACE_BEGIN

DLVresultParserDriver::DLVresultParserDriver() : pMode(FirstOrder)
{
}

DLVresultParserDriver::DLVresultParserDriver(ParseMode mode)
{
	setParseMode(mode);
}

DLVresultParserDriver::~DLVresultParserDriver()
{
}

void
DLVresultParserDriver::setParseMode(ParseMode mode)
{
	pMode = mode;
}

struct ParserState
{
	std::vector<AtomSet>& result;
	bool dropPredicates; 

  ParserState(std::vector<AtomSet>& result, bool dropPredicates):
		result(result), dropPredicates(dropPredicates) {}
};

#if 0
//
// useful phoenix operator debugging stuff:
// -> just add [ handle_dbg("message") ] to any rule/parser to get information
//
template<typename Attrib>
void houtput(Attrib const& a)
{
#ifdef CWDEBUG
  std::cerr << "XXX handling attribute " << libcwd::type_info_of(a).demangled_name() << std::endl;
#endif
};

template<>
void houtput(std::vector<char> const& a)
{
  std::cerr << "XXX got string  attribute '" << std::string(a.begin(), a.end()) << "'" << std::endl;
}

template<typename Content>
void houtput(boost::optional<Content> const& a)
{
  if( !a )
  {
    std::cerr << "XXX optional (unset):" << std::endl;
  }
  else
  {
    std::cerr << "XXX optional:" << std::endl;
  }
  houtput(a.get());
}

struct handle_dbg
{
  handle_dbg(std::string s): s(s) {}
  std::string s;
  template<typename Attrib>
  void operator()(Attrib& a, qi::unused_type, qi::unused_type) const
  {
    std::cerr << "DBG=" << s << std::endl;
    houtput(a);
  }
};
#endif

struct handle_int
{
  template <typename Context>
  void operator()(int i, Context& ctx, qi::unused_type) const
  {
    dlvhex::Term*& ruleAttr = fusion::at_c<0>(ctx.attributes);

    ruleAttr = new dlvhex::Term(i);
    //std::cerr << "created int " << i << std::endl;
  }
};

struct handle_ident
{
  template <typename Context>
  void operator()(const std::string& s, Context& ctx, qi::unused_type) const
  {
    dlvhex::Term*& ruleAttr = fusion::at_c<0>(ctx.attributes);

    ruleAttr = new dlvhex::Term(s, false);
    //std::cerr << "created ident '" << s << "'" << std::endl;
  }
};

struct handle_new_answerset
{
  handle_new_answerset(ParserState& state): state(state) {}

  void operator()(qi::unused_type, qi::unused_type, qi::unused_type) const
  {
		state.result.push_back(AtomSet());
		//std::cerr << "created new atomset" << std::endl;
  }

  ParserState& state;
};

struct handle_fact
{
  handle_fact(ParserState& state): state(state) {}

  void operator()(
			boost::fusion::vector3<
				boost::optional<char>,
				std::string,
				boost::optional<PTuple> >& attr,
			qi::unused_type, qi::unused_type) const
  {
		// alias for fusion input
		bool strong_neg = !!fusion::at_c<0>(attr);
		const std::string& predicate = fusion::at_c<1>(attr);
		bool propositional = !fusion::at_c<2>(attr);
		PTuple* ptuple = 0;
		if( !propositional )
			ptuple = &fusion::at_c<2>(attr).get();

		// alias for state
		AtomSet& atomSet = state.result.back();
		bool dropPredicates = state.dropPredicates;

		// we cannot have propositional atoms in higher order mode
		assert(!(dropPredicates && propositional));

		if( propositional )
		{
			atomSet.insert(AtomPtr(new Atom(predicate, strong_neg)));
		}
		else
		{
			if( dropPredicates )
			{
				atomSet.insert(AtomPtr(new Atom(*ptuple, strong_neg)));
			}
			else
			{
				atomSet.insert(AtomPtr(new Atom(predicate, *ptuple, strong_neg)));
			}
			BOOST_FOREACH(Term* pterm, *ptuple)
			{
				delete pterm;
			}
		}
  }

  ParserState& state;
};

// "The Grammar"
template<typename Iterator>
struct DLVResultGrammar:
  public qi::grammar<Iterator, ascii::space_type>
{
	DLVResultGrammar(ParserState& state):
		DLVResultGrammar::base_type(answersets), state(state)
	{
    using spirit::int_;
    using spirit::_val;
    using spirit::_1;
    using qi::lexeme;
    using qi::char_;
    using qi::omit;
    using qi::lit;

		ident
			= lexeme[char_('"') > *(char_ - '"') > char_('"')]
      // @todo test performance
			//| lexeme[char_("a-z") >> *char_("a-zA-Z0-9_")];
			| lexeme[ascii::lower > *(ascii::alnum|char_('_'))];
		groundterm
			= int_ [ handle_int() ]
			| ident [ handle_ident() ]
      ;
		fact
			// char_ synthesizes a char attribute!
			= ( -char_('-') > ident > -params ) [ handle_fact(state) ] ;
		params
			%= '(' > groundterm > *(',' > groundterm) > ')';
		answerset
			= (lit('{') >> '}') [ handle_new_answerset(state) ]
			| (lit('{') [ handle_new_answerset(state) ] > fact > *(',' > fact) > '}');
    /// @todo: do not throw away weak answer set information but store it
    costline
      = lit("Cost") > +(ascii::alnum|char_("[]<>():"));
		answersets
			// end_p enforces a "full" match (in case of success) even with trailing newlines
			= *(
            (-lit("Best model:") >> answerset)
            |
            costline
         ) > (qi::eol | qi::eoi);

    #ifdef BOOST_SPIRIT_DEBUG
		BOOST_SPIRIT_DEBUG_NODE(answersets);
		BOOST_SPIRIT_DEBUG_NODE(answerset);
		BOOST_SPIRIT_DEBUG_NODE(costline);
		BOOST_SPIRIT_DEBUG_NODE(fact);
		BOOST_SPIRIT_DEBUG_NODE(groundterm);
		BOOST_SPIRIT_DEBUG_NODE(ident);
    #endif
	}

	qi::rule<Iterator, ascii::space_type>                  answersets, answerset, fact, costline;
	qi::rule<Iterator, dlvhex::Term*(), ascii::space_type> groundterm;
	qi::rule<Iterator, std::string(), ascii::space_type>   ident;
	qi::rule<Iterator, PTuple(), ascii::space_type>        params;

	ParserState& state;
};

void
DLVresultParserDriver::parse(std::istream& is,
                             std::vector<AtomSet>& result) throw (SyntaxError)
{
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"DLVresultParserDriver::parse");

  // @todo: read each line to the next endl, then parse, then get exactly one answer set per parse, maybe reuse the grammar (state can be avoided by using attributes, AtomSetPtr and AtomPtr or just use a new AST with integers instead)

	std::ostringstream buf;
	buf << is.rdbuf();
	const std::string& input = buf.str();

	typedef std::string::const_iterator forward_iterator_type;
	// convert input iterator to forward iterator, usable by spirit parser
	forward_iterator_type fwd_begin = input.begin();
	forward_iterator_type fwd_end = input.end();

	// @todo dump linewise before each line parse (see above todo)
  if( Globals::Instance()->doVerbose(Globals::DUMP_OUTPUT) )
  {
    Globals::Instance()->getVerboseStream() <<
      "Got Result:\n===\n" << input << "\n===" << std::endl;
  }

	bool dropPredicates =
		(pMode == DLVresultParserDriver::HO);
	ParserState state(result, dropPredicates);

  DLVResultGrammar<forward_iterator_type> grammar(state);
  try
  {
    bool r = qi::phrase_parse(fwd_begin, fwd_end, grammar, ascii::space);

    // @todo: add better error message with position iterator 
    if (!r || fwd_begin != fwd_end)
      throw SyntaxError("Could not parse complete DLV output!");
  } catch(const qi::expectation_failure<forward_iterator_type>& e)
  {
    throw SyntaxError("Could not parse DLV output! (expectation failure)");
  }
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
