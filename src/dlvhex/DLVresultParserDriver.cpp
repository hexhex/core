/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
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

#include "dlvhex/DLVresultParserDriver.h"
#include "dlvhex/SpiritDebugging.h"
#include "dlvhex/globals.h"

#include <boost/spirit/core.hpp>
#include <boost/spirit/utility/regex.hpp>
#include <boost/spirit/tree/parse_tree.hpp>

#include <sstream>
#include <iostream>

DLVHEX_NAMESPACE_BEGIN

DLVresultParserDriver::DLVresultParserDriver()
{
}

DLVresultParserDriver::~DLVresultParserDriver()
{
}

// "The Grammar"
struct DLVResultGrammar:
  public boost::spirit::grammar<DLVResultGrammar>
{
  enum RuleTags {
    None = 0, Root, AnswerSet, PropFact, NonpropFact, Groundterm, Neg, Ident, Number };

  // S = ScannerT
  template<typename S>
  struct definition
  {
    // shortcut
    typedef boost::spirit::parser_context<> c;
    template<int Tag> struct tag: public boost::spirit::parser_tag<Tag> {};

    definition(DLVResultGrammar const& self);
    boost::spirit::rule< S, c, tag<Root> > const& start() const { return root; }

    boost::spirit::rule<S, c, tag<Root> >        root;
    boost::spirit::rule<S, c, tag<AnswerSet> >   answerset;
    boost::spirit::rule<S, c, tag<PropFact> >    prop_fact;
    boost::spirit::rule<S, c, tag<NonpropFact> > nonprop_fact;
    boost::spirit::rule<S>                       fact;
    boost::spirit::rule<S, c, tag<Groundterm> >  groundterm;
    boost::spirit::rule<S, c, tag<Neg> >         neg;
    boost::spirit::rule<S, c, tag<Ident> >       ident;
    boost::spirit::rule<S, c, tag<Number> >      number;
  };
};

template<typename ScannerT>
DLVResultGrammar::definition<ScannerT>::definition(DLVResultGrammar const&)
{
  namespace sp = boost::spirit;

  // shortcut for sp::discard_node_d()
  const sp::node_parser_gen<sp::discard_node_op> rm =
    sp::node_parser_gen<sp::discard_node_op>();

  ident
    = sp::regex_p("[a-z][a-zA-Z0-9_]*")
    | sp::regex_p("\"[^\"]*\"");
  number
    = sp::regex_p("[0-9]+");
  groundterm
    = ident | number;
  neg
    = sp::ch_p('-');
  prop_fact
    = !neg >> ident;
  nonprop_fact
    = !neg >> ident >> '(' >> groundterm >> *(rm[sp::ch_p(',')] >> groundterm) >> ')';
  fact
    = nonprop_fact | prop_fact;
  answerset
    = '{' >> !fact >> *(rm[sp::ch_p(',')] >> fact) >> rm[sp::ch_p('}')];
  root
    // end_p enforces a "full" match (in case of success) even with trailing newlines
    = *answerset >> !sp::end_p;
}

// converts the parse tree from boost::spirit to a DLV result
class DLVResultGrammarPTToResultConverter
{
public:
  typedef boost::spirit::node_val_data_factory<> factory_t;

  typedef const char* iterator_t;

  // node type for spirit PT
  typedef boost::spirit::tree_match<iterator_t, factory_t>::node_t node_t;

public:
  DLVResultGrammarPTToResultConverter();

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

  void appendFactFromPropFact(node_t& node, AtomSet& result);
  void appendFactFromNonpropFact(node_t& node, AtomSet& result);
};

DLVResultGrammarPTToResultConverter::DLVResultGrammarPTToResultConverter()
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
    printSpiritPT(std::cerr, at, "node");
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

  assert(!Globals::Instance()->getOption("NoPredicate"));
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
  if( Globals::Instance()->getOption("NoPredicate") )
  {
    atom = AtomPtr(new Atom(
      terms,
      neg));
  }
  else
  {
    atom = AtomPtr(new Atom(
      createStringFromNode(node.children[offset], DLVResultGrammar::Ident),
      terms,
      neg));
  }
  result.insert(atom);
}

void
DLVresultParserDriver::parse(std::istream& is,
                             std::vector<AtomSet>& result) throw (SyntaxError)
{
  // put whole input from stream into a string
  std::ostringstream buf;
  buf << is.rdbuf();
  std::string input = buf.str();

  DLVResultGrammar grammar;
  typedef DLVResultGrammarPTToResultConverter Converter;

  Converter::iterator_t it_begin = input.c_str();
  Converter::iterator_t it_end = input.c_str()+input.size();

  // parse ast
  boost::spirit::tree_parse_info<Converter::iterator_t, Converter::factory_t> info =
    boost::spirit::pt_parse<Converter::factory_t>(
        it_begin, it_end, grammar, boost::spirit::space_p);
  //printSpiritPT(std::cerr, *info.trees.begin());

  // successful parse?
  if( !info.full )
    throw SyntaxError("Could not parse complete DLV output!");

  // if this is not ok, there is some bug and the following code will be incomplete
  assert(info.trees.size() == 1);

  // create dlvhex AST from spirit parser tree
  Converter converter;
  converter.appendPTToResult(*info.trees.begin(), result);
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
