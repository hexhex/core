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

#undef CWDEBUG
#ifdef CWDEBUG
# define _GNU_SOURCE
# include <libcwd/sys.h>
# include <libcwd/debug.h>
#endif

#undef BOOST_SPIRIT_DEBUG

#include "dlvhex/DLVresultParserDriver.h"
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

DLVresultParserDriver::DLVresultParserDriver() : pMode(AUTO)
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

//
// useful phoenix operator debugging stuff:
// -> just add [ handle_dbg("message") ] to any rule/parser to get information
//
#if 1
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

	#if 1
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
	#else
  void operator()(
			const boost::fusion::vector2<
				boost::fusion::vector2<
					boost::optional<char>,
					std::string>,
				boost::optional<PTuple> >& attr,
			qi::unused_type, qi::unused_type) const
  {
		// alias for fusion input
		bool strong_neg = !!fusion::at_c<0>(fusion::at_c<0>(attr));
		const std::string& predicate = fusion::at_c<1>(fusion::at_c<0>(attr));
		bool propositional = !fusion::at_c<1>(attr);
		PTuple const* ptuple = 0;
		if( !propositional )
			ptuple = &fusion::at_c<1>(attr).get();
	#endif

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

#undef DUMMYPARSE

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
			//| lexeme[char_("a-z") >> *char_("a-zA-Z0-9_")];
			| lexeme[ascii::lower > *(ascii::alnum|char_('_'))];
		groundterm
			= int_
#ifndef DUMMYPARSE
      [ handle_int() ]
#endif
			| ident
#ifndef DUMMYPARSE
      [ handle_ident() ]
#endif
      ;
		fact
			// char_ synthesizes a char attribute!
			//= ( (-char_('-') >> ident) > -params )
			= ( -char_('-') > ident > -params )
#ifndef DUMMYPARSE
      [ handle_fact(state) ]
#endif
      ;
		params
			%= '(' > groundterm > *(',' > groundterm) > ')';
		answerset
			= (lit('{') >> '}') [ handle_new_answerset(state) ]
			| (lit('{') [ handle_new_answerset(state) ] > fact > *(',' > fact) > '}');
		answersets
			// end_p enforces a "full" match (in case of success) even with trailing newlines
			= *answerset > (qi::eol | qi::eoi);

    #ifdef BOOST_SPIRIT_DEBUG
		BOOST_SPIRIT_DEBUG_NODE(answersets);
		BOOST_SPIRIT_DEBUG_NODE(answerset);
		BOOST_SPIRIT_DEBUG_NODE(fact);
		BOOST_SPIRIT_DEBUG_NODE(groundterm);
		BOOST_SPIRIT_DEBUG_NODE(ident);
    #endif
	}

	/*
	void setParserState(ParserState* ps)
	{
		state = ps;
	}
	*/

	qi::rule<Iterator, ascii::space_type>                 answersets, answerset, fact;
#ifndef DUMMYPARSE
	qi::rule<Iterator, dlvhex::Term*(), ascii::space_type> groundterm;
	qi::rule<Iterator, std::string(), ascii::space_type>  ident;
	qi::rule<Iterator, PTuple(), ascii::space_type>  params;
#else
	qi::rule<Iterator, ascii::space_type>                 groundterm, ident, params;
#endif

	ParserState& state;
};


#if 0
// converts the parse tree from boost::spirit to a DLV result
class DLVResultGrammarPTToResultConverter
{
public:
  typedef boost::spirit::node_val_data_factory<> factory_t;

  typedef const char* iterator_t;

  // node type for spirit PT
  typedef boost::spirit::tree_match<iterator_t, factory_t>::node_t node_t;

public:
  DLVResultGrammarPTToResultConverter(DLVresultParserDriver::ParseMode pMode);

  // convert the root node of the spirit parse tree (from HexSpiritGrammar)
  // to a program and edb
  void appendPTToResult(node_t& node, std::vector<AtomSet>& result);

private:
  //
  // general helpers
  //

  // verify type of node
  // follow tree until a single content node is found
  // return its content as a string
  std::string createStringFromNode(node_t& node,
      DLVResultGrammar::RuleTags verifyRuleTag = DLVResultGrammar::None);

  // use createStringFromNode to get data
  // create correct term type (numeric vs string)
  Term createTerm(node_t& node);

  // interpret atoms as FO or HO atoms
  DLVresultParserDriver::ParseMode pMode;

  void appendFactFromPropFact(node_t& node, AtomSet& result);
  void appendFactFromNonpropFact(node_t& node, AtomSet& result);
};

DLVResultGrammarPTToResultConverter::DLVResultGrammarPTToResultConverter(DLVresultParserDriver::ParseMode mode) : pMode(mode)
{
}

void DLVResultGrammarPTToResultConverter::appendPTToResult(
    node_t& node, std::vector<AtomSet>& result)
{
  assert(node.value.id() == DLVResultGrammar::Root);
  for(node_t::tree_iterator it = node.children.begin();
      it != node.children.end(); ++it)
  {
    node_t& at = *it;
    //printSpiritPT(std::cerr, at, "node");
    // skip empty lines and end marker
    if( at.value.id() != DLVResultGrammar::AnswerSet )
      continue;

    // add empty answer set
    result.push_back(AtomSet());

    // convert facts in answer set and append to the answer set we just added
    node_t::tree_iterator itf = at.children.begin();
    // skip first item (opening bracket)
    // we cannot "rm[]" this bracket or we will loose empty answer sets)
    if( itf != at.children.end() )
      ++itf;
    // continue converting with second child, until end (final bracket is rm[]'d)
    for(; itf != at.children.end(); ++itf)
    {
      assert(!itf->children.empty());
      node_t& atf = itf->children[0];
      switch(atf.value.id().to_long())
      {
      case DLVResultGrammar::PropFact:
        // nonempty answer set
        //printSpiritPT(std::cerr, atf, "pf");
        appendFactFromPropFact(atf, result.back());
        break;
      case DLVResultGrammar::NonpropFact:
        //printSpiritPT(std::cerr, atf, "npf");
        appendFactFromNonpropFact(atf, result.back());
        break;
      default:
        assert(false);
      }
    }
  }
}

// verify type of node
// follow tree until a single content node is found
// return its content as a string
std::string DLVResultGrammarPTToResultConverter::createStringFromNode(
    node_t& node, DLVResultGrammar::RuleTags verifyRuleTag)
{
  // verify the tag
  assert(verifyRuleTag == DLVResultGrammar::None || node.value.id() == verifyRuleTag);
  // debug output
  //printSpiritPT(std::cerr, node);
  // descend as long as there is only one child and the node has no value
  node_t* at = &node;
  while( (at->children.size() == 1) && (at->value.begin() == at->value.end()) )
    at = &(at->children[0]);
  // if we find one child which has a value, we return it
  if( at->value.begin() != at->value.end() )
  {
    std::string ret(at->value.begin(), at->value.end());
    boost::trim(ret);
    //std::cerr << "createStringFromNode returns '" << ret << "'" << std::endl;
    return ret;
  }
  // if we find multiple children which have a value, this is an error
  assert(false && "found multiple value children in createStringFromNode");
}

// use createStringFromNode to get data
// create correct term type (anonymous vs numeric vs string)
Term DLVResultGrammarPTToResultConverter::createTerm(node_t& node)
{
  std::string str = createStringFromNode(node);
  switch(node.children[0].value.id().to_long())
  {
  case DLVResultGrammar::Ident:
    return Term(str);
  case DLVResultGrammar::Number:
    {
      std::stringstream strstr;
      unsigned num;
      strstr << str;
      strstr >> num;
      return Term(num);
    }
  default:
    assert(false);
    return Term(); // keep the compiler happy
  }
}

void DLVResultGrammarPTToResultConverter::appendFactFromPropFact(
    node_t& node, AtomSet& result)
{
  assert(node.value.id() == DLVResultGrammar::PropFact);

  // negation
  unsigned offset = 0;
  bool neg = false;
  if( node.children[0].value.id() == DLVResultGrammar::Neg )
  {
    offset = 1;
    neg = true;
  }

  // in case that we parse in HO-mode, prop. facts must not occur!
  // we run in HO-mode iff
  //    either 1. the mode is AUTO and this instance runs in HO mode
  //    or 2. HO mode is explicitly requested
  assert(!(pMode == DLVresultParserDriver::AUTO && Globals::Instance()->getOption("NoPredicate")) || (pMode == DLVresultParserDriver::HO));

  AtomPtr atom(new Atom(
      createStringFromNode(node.children[offset], DLVResultGrammar::Ident),
      neg));
  result.insert(atom);
}

void DLVResultGrammarPTToResultConverter::appendFactFromNonpropFact(
    node_t& node, AtomSet& result)
{
  assert(node.value.id() == DLVResultGrammar::NonpropFact);

  // negation
  unsigned offset = 0;
  bool neg = false;
  if( node.children[0].value.id() == DLVResultGrammar::Neg )
  {
    offset = 1;
    neg = true;
  }

  // content
  Tuple terms;
  for(unsigned i = 2+offset; i != (node.children.size()-1); ++i)
    terms.push_back(createTerm(node.children[i]));

  AtomPtr atom;
  // Determine the actual parse mode:
  //   if higher-order mode is explicitly requested (pType == HO), or the mode is AUTO and the current instance runs in HO mode,
  //       we will just take the arguments of the atom (and drop it's predicate), i.e. "a_i(p, ...)" is transformed into "p(...)"
  //   otherwise we interpret it as first-order atom and take it as it is (including the predicate name)
  if( (pMode == DLVresultParserDriver::AUTO && Globals::Instance()->getOption("NoPredicate")) || (pMode == DLVresultParserDriver::HO) )
  {
    // HO --> drop the predicate name
    atom = AtomPtr(new Atom(
      terms,
      neg));
  }
  else
  {
    // FO --> insert the predicate name
    atom = AtomPtr(new Atom(
      createStringFromNode(node.children[offset], DLVResultGrammar::Ident),
      terms,
      neg));
  }
  result.insert(atom);
}
#endif

void
DLVresultParserDriver::parse(std::istream& is,
                             std::vector<AtomSet>& result) throw (SyntaxError)
{
#if 0
	typedef std::istreambuf_iterator<char> base_iterator_type;
	typedef spirit::multi_pass<base_iterator_type> forward_iterator_type;

	// iterate over stream input
	base_iterator_type in_begin(is);
	 
	// convert input iterator to forward iterator, usable by spirit parser
	forward_iterator_type fwd_begin =
		    spirit::make_default_multi_pass(in_begin);
	forward_iterator_type fwd_end;
	#else
	std::ostringstream buf;
	buf << is.rdbuf();
	const std::string& input = buf.str();

	typedef std::string::const_iterator forward_iterator_type;
	// convert input iterator to forward iterator, usable by spirit parser
	forward_iterator_type fwd_begin = input.begin();
	forward_iterator_type fwd_end = input.end();
	#endif

	// TODO: dump linewise
	/*
  if( Globals::Instance()->doVerbose(Globals::DUMP_OUTPUT) )
  {
    Globals::Instance()->getVerboseStream() <<
      "Got Result:\n===\n" << input << "\n===" << std::endl;
  }
	*/

	DLVHEX_BENCHMARK_REGISTER(sid,"qi::phrase_parse");
	DLVHEX_BENCHMARK_START(sid);

	//   if higher-order mode is explicitly requested (pType == HO), or the mode is AUTO and the current instance runs in HO mode,
	//       we will just take the arguments of the atom (and drop it's predicate), i.e. "a_i(p, ...)" is transformed into "p(...)"
	//   otherwise we interpret it as first-order atom and take it as it is (including the predicate name)
	bool dropPredicates =
		(pMode == DLVresultParserDriver::AUTO &&
		 Globals::Instance()->getOption("NoPredicate"))
		||
		(pMode == DLVresultParserDriver::HO);
	ParserState state(result, dropPredicates);

	// @todo: this would not be multithreading safe (multiple answer sets parsed at once) but we instantiate the grammar only once for performance reasons
	//grammar.setParserState(&state);
	//DLVHEX_BENCHMARK
  DLVResultGrammar<forward_iterator_type> grammar(state);
	bool r = qi::phrase_parse(fwd_begin, fwd_end, grammar, ascii::space);

	DLVHEX_BENCHMARK_STOP(sid);

	if (!r || fwd_begin != fwd_end)
		throw SyntaxError("Could not parse complete DLV output!");
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
