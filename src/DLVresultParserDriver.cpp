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
 * @file   DLVResultParser.cpp
 * @author Roman Schindlauer
 * @author Peter Schüller
 * @date   Wed Mar 22 14:38:53 CET 2006
 * 
 * @brief  DLV result parser using boost::spirit
 * 
 * 
 */

// activate benchmarking if activated by configure option --enable-debug
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

// use this for developing the parser
#undef CWDEBUG
#ifdef CWDEBUG
# ifndef _GNU_SOURCE
#  define _GNU_SOURCE
# endif
# include <libcwd/sys.h>
# include <libcwd/debug.h>
#endif

//#include "dlvhex2/DLVResultParser.hpp"
#include "dlvhex2/DLVresultParserDriver.h"

// use this for debugging parser progress (XML style)
#undef BOOST_SPIRIT_DEBUG

#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/Term.h"
#include "dlvhex2/Atoms.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/ProgramCtx.h"

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

DLVResultParser::DLVResultParser(RegistryPtr reg):
  reg(reg),
  pMode(FirstOrder)
{
}

DLVResultParser::DLVResultParser(RegistryPtr reg, ParseMode mode):
  reg(reg),
  pMode(mode)
{
}

DLVResultParser::~DLVResultParser()
{
}

void
DLVResultParser::setParseMode(ParseMode mode)
{
	pMode = mode;
}

struct ParserState
{
  RegistryPtr registry;
  AnswerSet::Ptr current;
  DLVResultParser::AnswerSetAdder adder;
	bool dropPredicates; 

  ParserState(
      RegistryPtr registry,
      DLVResultParser::AnswerSetAdder adder,
      bool dropPredicates):
    registry(registry),
    current(new AnswerSet(registry)),
		adder(adder),
    dropPredicates(dropPredicates) {}
};

#if 1
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
    ID& ruleAttr = fusion::at_c<0>(ctx.attributes);

    ruleAttr = ID::termFromInteger(i);
    //std::cerr << "created int " << i << std::endl;
  }
};

namespace
{
  inline ID getOrRegisterTerm(RegistryPtr registry, const std::string& s)
  {
    ID id = registry->terms.getIDByString(s);
    if( id == ID_FAIL )
      {
        id = registry->preds.getIDByString(s);
        if( id == ID_FAIL )
          {
            Term term(ID::MAINKIND_TERM, s);
            // we can only get strings or constants
            assert(s[0] == '"' || islower(s[0]));
            id = registry->terms.storeAndGetID(term);
          }
      }
    return id;
  }
}

struct handle_ident
{
  handle_ident(ParserState& state): state(state) {}

  template <typename Context>
  void operator()(const std::string& s, Context& ctx, qi::unused_type) const
  {
    ID& ruleAttr = fusion::at_c<0>(ctx.attributes);

    ID id = getOrRegisterTerm(state.registry, s);
    ruleAttr = id;
    //std::cerr << "created ident '" << s << "'" << std::endl;
  }

  ParserState& state;
};

struct handle_finished_answerset
{
  handle_finished_answerset(ParserState& state): state(state) {}

  void operator()(qi::unused_type, qi::unused_type, qi::unused_type) const
  {
    // add current answer set as full answer set
    DBGLOG(DBG,"handling parsed answer set " << *state.current);
    state.adder(state.current);
    // create empty answer set for subsequent parsing
		state.current.reset(new AnswerSet(state.registry));
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
				boost::optional<Tuple> >& attr,
			qi::unused_type, qi::unused_type) const
  {
		// alias for fusion input
		bool strong_neg = !!fusion::at_c<0>(attr);
    assert(!strong_neg);
		const std::string& predicate = fusion::at_c<1>(attr);
    ID predid = getOrRegisterTerm(state.registry, predicate);
    OrdinaryAtom atom(ID::MAINKIND_ATOM);
    atom.tuple.push_back(predid);

    // aux predicates create aux atoms
    if( (predid & ID::PROPERTY_AUX) != 0 )
      atom.kind |= ID::PROPERTY_AUX;

    boost::optional<Tuple>& tup = fusion::at_c<2>(attr);
    if( !!tup )
      atom.tuple.insert(atom.tuple.end(), tup.get().begin(), tup.get().end());

    // TODO lookup by string in registry, then by tuple
    ID id = state.registry->ogatoms.getIDByTuple(atom.tuple);
    if( id == ID_FAIL )
    {
      {
        #warning parsing efficiency problem see HexGrammarPTToASTConverter
        std::stringstream ss;
        RawPrinter printer(ss, state.registry);
        Tuple::const_iterator it = atom.tuple.begin();
        printer.print(*it);
        it++;
        if( it != atom.tuple.end() )
        {
          ss << "(";
          printer.print(*it);
          it++;
          while(it != atom.tuple.end())
          {
            ss << ",";
            printer.print(*it);
            it++;
          }
          ss << ")";
        }
        atom.text = ss.str();
      }
      //DBGLOG(DBG,"storing atom " << atom);
      id = state.registry->ogatoms.storeAndGetID(atom);
    }
    //TODO make more efficient (cache pointer to interpretation or even function object)
    //DBGLOG(DBG,"setting fact " << id);
    state.current->interpretation->setFact(id.address);
  }

  ParserState& state;
};

// "The Grammar"
template<typename Iterator>
struct DLVResultGrammar:
  public qi::grammar<Iterator, ascii::space_type>
{
	DLVResultGrammar(ParserState& state):
		DLVResultGrammar::base_type(dlvline), state(state)
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
			| ident [ handle_ident(state) ]
      ;
		fact
			// char_ synthesizes a char attribute!
			= ( -char_('-') > ident > -params ) [ handle_fact(state) ] ;
		params
			%= '(' > groundterm > *(',' > groundterm) > ')';
		answerset
			= (lit('{') >> '}') [ handle_finished_answerset(state) ]
			| (lit('{') > fact > *(',' > fact) > lit('}') [ handle_finished_answerset(state) ]);
    /// @todo: do not throw away weak answer set information but store it
    costline
      = lit("Cost") > +(ascii::alnum|char_("[]<>():"));
		dlvline
			= (-lit("Best model:") >> answerset)
        |
        costline;

    #ifdef BOOST_SPIRIT_DEBUG
		BOOST_SPIRIT_DEBUG_NODE(dlvline);
		BOOST_SPIRIT_DEBUG_NODE(answerset);
		BOOST_SPIRIT_DEBUG_NODE(costline);
		BOOST_SPIRIT_DEBUG_NODE(fact);
		BOOST_SPIRIT_DEBUG_NODE(groundterm);
		BOOST_SPIRIT_DEBUG_NODE(ident);
    #endif
	}

	qi::rule<Iterator, ascii::space_type>                  dlvline,
                                                         answerset,
                                                         fact,
                                                         costline;
	qi::rule<Iterator, ID(), ascii::space_type>            groundterm;
	qi::rule<Iterator, std::string(), ascii::space_type>   ident;
	qi::rule<Iterator, Tuple(), ascii::space_type>         params;

	ParserState& state;
};

void
DLVResultParser::parse(
    std::istream& is,
    AnswerSetAdder adder) throw (SyntaxError)
{
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"DLVResultParser::parse");

	bool dropPredicates =
		(pMode == DLVResultParser::HO);
	ParserState state(reg, adder, dropPredicates);

  typedef std::string::const_iterator forward_iterator_type;
  DLVResultGrammar<forward_iterator_type> grammar(state);
  unsigned errors = 0;
  do
  {
    // @todo: read each line to the next endl, then parse, then get exactly one answer set per parse, maybe reuse the grammar (state can be avoided by using attributes, AtomSetPtr and AtomPtr or just use a new AST with integers instead)

    // get next input line
    std::string input;
    std::getline(is, input);

    // break silently
    if( input.empty() )
      break;

    DBGLOG(DBG,"obtained " << input.size() << " characters from input stream via getline");
    
    if( is.bad() )
    {
      DBGLOG(DBG,"leaving DLVResultParser loop, stream bits are: "
          "fail " << is.fail() << ", bad " << is.bad() << ", eof " << is.eof());
      break;
    }

    // TODO allocate answer set here, just set bits in parser
    LOG(DBG,"parsing input from DLV: '" << input << "'");

    // convert input iterator to forward iterator, usable by spirit parser
    forward_iterator_type fwd_begin = input.begin();
    forward_iterator_type fwd_end = input.end();

    try
    {
      bool r = qi::phrase_parse(fwd_begin, fwd_end, grammar, ascii::space);

      // @todo: add better error message with position iterator 
      if (!r || fwd_begin != fwd_end)
      {
        LOG(ERROR,"for input '" << input << "': r=" << r << " (begin!=end)=" << (fwd_begin != fwd_end));
        errors++;
      }
    }
    catch(const qi::expectation_failure<forward_iterator_type>& e)
    {
      LOG(ERROR,"for input '" << input << "': could not parse DLV output(expectation failure) " << e.what_);
      errors++;
    }
  }
  while(errors < 20);

  if( errors != 0  )
  {
		LOG(ERROR,"error count for parsing DLV output = " << errors);
    throw SyntaxError("Could not parse complete DLV output! (see error log messages)");
  }
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
