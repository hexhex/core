/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
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
 * @file   HexParserDriver.cpp
 * @author Roman Schindlauer
 * @date   Wed Mar 22 14:38:53 CET 2006
 * 
 * @brief  C++ interface to the bison parser.
 * 
 * 
 */

#include "dlvhex/HexParser.hpp"
#include "dlvhex/ParserDriver.h"
#include "dlvhex/HexFlexLexer.h"

#include <boost/spirit/core.hpp>
#include <boost/spirit/utility/regex.hpp>
#include <boost/spirit/utility/confix.hpp>
#include <boost/spirit/iterator/multi_pass.hpp>
#include <boost/spirit/iterator/position_iterator.hpp>
// parse_tree is useful for debugging (grammar and ast creation)
#include <boost/spirit/tree/parse_tree.hpp>
// ast will be useful for creating the AST
#include <boost/spirit/tree/ast.hpp>

#include <iostream>
#include <sstream>
#include <fstream>

DLVHEX_NAMESPACE_BEGIN

// boost::spirit Part begin

namespace sp = boost::spirit;

using sp::ch_p;
using sp::str_p;
using sp::space_p;

// private namespace, as this parser part should only be used
// via the HexParserDriver interface
namespace
{

typedef sp::position_iterator<const char*> pos_iterator_t;

template<typename NodeT>
void printSpiritAST(std::ostream& o, NodeT& node, const std::string& indent="");

// see "The Grammar" in the spirit docs
struct HexSpiritGrammar:
  public sp::grammar<HexSpiritGrammar>
{
  template<typename ScannerT>
  struct definition
  {
  public:
    typedef typename sp::rule<ScannerT> rule_t;

  public:
    definition(HexSpiritGrammar const& self);
    rule_t const& start() const { return root; }

  protected:
    rule_t ident;
    rule_t var;
    rule_t number;
    rule_t ident_or_var;
    rule_t ident_or_var_or_number;
    rule_t cons;
    rule_t term;
    rule_t terms; // list of terms
    rule_t literal;
    rule_t external_atom;
    rule_t aggregate_leq_binop;
    rule_t aggregate_geq_binop;
    rule_t aggregate_binop;
    rule_t binop;
    rule_t tertop;
    rule_t aggregate;
    rule_t aggregate_pred;
    rule_t builtin_pred;
    rule_t disj;
		rule_t neg;
    rule_t user_pred;
    rule_t body; // body of a rule or constraint or aggregate
		rule_t clause;
    rule_t root;
  };
};

struct lift_node_op
{
	template <typename MatchT>
	void operator()(MatchT& m) const
	{    
		// if match has one tree with more than one child, replace match by these children
		// (do as if the rule were inlined in the other rule)
		if (m.trees.size() == 1 && m.trees.begin()->children.size() > 1)
		{
			std::swap(m.trees, m.trees.begin()->children);
		}    
	}    
};
const sp::node_parser_gen<lift_node_op> lift_node_d =
    sp::node_parser_gen<lift_node_op>();

// impl of HexSpiritGrammar
template<typename ScannerT>
HexSpiritGrammar::definition<ScannerT>::definition(HexSpiritGrammar const&)
{
	// shortcut for sp::discard_node_d()
	const sp::node_parser_gen<sp::discard_node_op> rm =
		sp::node_parser_gen<sp::discard_node_op>();

  // identifier or string
  ident
		= sp::reduced_node_d[sp::regex_p("[a-z][a-zA-Z0-9_]*")]
		| sp::reduced_node_d[sp::confix_p("\"", *sp::anychar_p, "\"")];
  // variable
  var
		= sp::reduced_node_d[sp::regex_p("[A-Z][a-zA-Z0-9_]*")];
  // nonnegative integer
  number = sp::reduced_node_d[+sp::digit_p];
  ident_or_var = ident | var;
  ident_or_var_or_number = ident | var | number;
  aggregate_leq_binop = str_p("<=") | '<';
  aggregate_geq_binop = str_p(">=") | '>';
  aggregate_binop = aggregate_leq_binop | aggregate_geq_binop | str_p("==") | '=';
  binop = aggregate_binop | str_p("!=") | str_p("<>");
  tertop = ch_p('*') | '+';
  cons = str_p(":-") | str_p("<-");
  // identifiers, variables, numbers, anonymous variables
  term = ident_or_var_or_number | ch_p('_');
  terms = term >> *(rm[ch_p(',')] >> term);
	// strong negation
	neg = ch_p('-')|ch_p('~');
  user_pred
    = ( ( !neg >> ident_or_var >> '(' >> lift_node_d[terms] >> ')')
			| ( '(' >> lift_node_d[terms] >> ')' )
      | ( !neg >> ident_or_var)
      );
  external_atom = ch_p('&') >> ident >> !('[' >> lift_node_d[!terms] >> ']') >> !('(' >> lift_node_d[!terms] >> ')');
  aggregate_pred
    = (str_p("#any")|"#avg"|"#count"|"#max"|"#min"|"#sum"|"#times")
    >> '{' >> lift_node_d[terms] >> ':' >> lift_node_d[body] >> '}';
  aggregate
    = (term >> aggregate_binop >> aggregate_pred)
    | (aggregate_pred >> aggregate_binop >> term)
    | (term >> aggregate_leq_binop >> aggregate_pred >> aggregate_leq_binop >> term)
    | (term >> aggregate_geq_binop >> aggregate_pred >> aggregate_geq_binop >> term);
  builtin_pred
    = (term >> '=' >> term >> tertop >> term) // TODO: changelog: previously not supported
    | (tertop >> '(' >> term >> rm[ch_p(',')] >> term >> rm[ch_p(',')] >> term >> ')') // TODO: changelog: previously not supported
    | (binop >> '(' >> term >> rm[ch_p(',')] >> term >> ')') // TODO: changelog: previously not supported
    | (term >> binop >> term)
    | (str_p("#int") >> '(' >> term >> ')') // TODO: was user_pred previously, but this is wrong ("#int(X) :- foo(X)." is not allowed)
    | (str_p("#succ") >> '(' >> term >> rm[ch_p(',')] >> term >> ')')  // TODO: changelog: previously not supported
    ;
  literal
    = builtin_pred
    | ( !(str_p("not")|str_p("non")) >> (user_pred | external_atom | aggregate) );
  disj = user_pred >> *(rm[ch_p('v')] >> user_pred);
  body
    = literal >> *(rm[ch_p(',')] >> literal);
	clause
		=   (str_p("#maxint") >> '=' >> number >> rm[ch_p('.')]) // TODO: previously implemented in a wrong way (no "." at the end!)
      // TODO: sp::eol_p should be added, but this does not work (because of skip parser?)
      | (str_p("#namespace") >> '(' >> ident >> rm[ch_p(',')] >> ident >> ')') // TODO: change and add "." at the end?
      | // rule (optional body/condition)
        (disj >> !(cons >> !body) >> rm[ch_p('.')])
      | // constraint
        (cons >> sp::gen_pt_node_d[body] >> rm[ch_p('.')])
      | // weak constraint
        (str_p(":~") >> body >> rm[ch_p('.')] >>
				// optional weight
				 !( rm[ch_p('[')]
					  >> !ident_or_var_or_number
						>> rm[ch_p(':')]
						>> !ident_or_var_or_number
						>> rm[ch_p(']')]
				  ));
  root =
		*( // comment
       rm[sp::comment_p("%")]
		 | clause
		 | sp::eol_p
		 )
     // end_p enforces a "full" match in case of success even with trailing newlines
     >> sp::end_p;
}

// AST debugging

template<typename NodeT>
void printSpiritAST(std::ostream& o, NodeT& node, const std::string& indent)
{
  o << indent << "'" << std::string(node.value.begin(), node.value.end()) << "'" << std::endl;
  if( !node.children.empty() )
  {
    std::string cindent(indent + "  ");
    for(typename NodeT::const_tree_iterator it = node.children.begin(); it != node.children.end(); ++it)
    {
      printSpiritAST(o, *it, cindent);
    }
  }
}

} // anonymous namespace

// boost::spirit Part end

HexParserDriver::HexParserDriver()
    : lexer(new HexFlexLexer(this)),
      source("")
{
}


HexParserDriver::~HexParserDriver()
{
    delete lexer;
}


HexFlexLexer*
HexParserDriver::getLexer()
{
    return lexer;
}


const std::string&
HexParserDriver::getInputFilename() const
{
    return  this->source;
}


void
HexParserDriver::setOrigin(const std::string& org)
{
    this->source = org;
}

void
HexParserDriver::parse(std::istream& is,
                       Program& program,
                       AtomSet& EDB) throw (SyntaxError)
{
  // put input into a string
  std::ostringstream buf;
  buf << is.rdbuf();
  std::string input = buf.str();

  // parse with spirit
  try
  {
    HexSpiritGrammar grammar;

    // while remembering line numbers
    pos_iterator_t it_begin(input.c_str(), input.c_str()+input.size());
    pos_iterator_t it_end;

    // TODO: if debugging, parse with pt_parse and output with printSpiritAST, otherwise use ast_parse directly

    // parse generic tree (no special ast actions are used except lexeme_p)
    sp::tree_parse_info<pos_iterator_t> info =
      sp::ast_parse(it_begin, it_end, grammar, sp::space_p);

    // debug output
    std::cerr << "spirit match: full=" << info.full << ", match=" << info.match << std::endl;
    typedef sp::tree_match<pos_iterator_t>::node_t node_t;
    node_t::children_t::iterator itt;
    for(itt = info.trees.begin(); itt != info.trees.end(); ++itt)
    {
      std::cerr << "got tree from spirit:" << std::endl;
      printSpiritAST(std::cerr, *itt);
    }
  }
  catch(const std::exception& e)
  {
    std::cerr << "caught spirit exception: " << e.what() << std::endl;
    // TODO handle exception better
  }

  // old parser code
  yy::HexParser parser(this, program, EDB);
  parser.set_debug_level(false);
  std::istringstream input_is(input);
  lexer->switch_streams(&input_is, &std::cerr);

  try
    {
      parser.parse();
    }
  catch (SyntaxError& e)
    {
      //
      // is there was an error on the bison part, add the filename and throw
      // again
      //
      e.setFile(this->source);
      throw e;
    }
} 


void
HexParserDriver::parse(const std::string& file,
                       Program &program,
                       AtomSet& EDB) throw (SyntaxError)
{
    this->source = file;

    std::ifstream ifs;

    ifs.open(this->source.c_str());

    if (!ifs.is_open())
      {
        throw SyntaxError("File " + this->source + " not found");
      }

    parse(ifs, program, EDB);

    ifs.close();
} 

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
