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
#include <boost/spirit/tree/parse_tree.hpp>

#include <iostream>
#include <sstream>
#include <fstream>
#include <cassert>

DLVHEX_NAMESPACE_BEGIN

// boost::spirit Part begin

namespace sp = boost::spirit;

using sp::rule;
using sp::ch_p;
using sp::str_p;
using sp::space_p;

// private namespace, as this parser part should only be used
// via the HexParserDriver interface
namespace
{

//
// parsing and creating the boost::spirit parse tree (PT)
//

// data we want to store in the parse tree
// (this data is set manually by sp::access_node_d in the grammar)
// TODO: write a custom factory which does this automatically for each node
struct CustomNodeData
{
  // where was the match to this node?
  sp::file_position pos;

  CustomNodeData(): pos() {}
  CustomNodeData& operator=(const CustomNodeData& n)
   { pos = n.pos; return *this; }
};
typedef sp::node_val_data_factory<CustomNodeData> factory_t;

// iterator which remembers file positions (useful for error messages)
typedef sp::position_iterator<const char*> pos_iterator_t;
// node type for spirit PT
typedef sp::tree_match<pos_iterator_t, factory_t>::node_t node_t;

// boost::spirit PT debugging
void printSpiritPT(std::ostream& o, const node_t& node, const std::string& indent="");

// the grammar of hex (see "The Grammar" in the spirit docs)
struct HexSpiritGrammar:
  public sp::grammar<HexSpiritGrammar>
{
  enum RuleTags {
    None = 0, Root,
    Maxint, Namespace, Rule, Constraint, WeakConstraint,
    Body, Number, Disj,
    UserPredClassical, UserPredTuple, UserPredAtom, UserPred,
    Neg, Terms, Term
  };

  // S = ScannerT
  template<typename S>
  struct definition
  {
    // shortcut
    typedef typename sp::rule<S> rule_t;
    typedef sp::parser_context<> c;
    template<int Tag> struct tag: public sp::parser_tag<Tag> {};

    definition(HexSpiritGrammar const& self);
    rule< S, c, tag<Root> > const& start() const { return root; }

    rule_t ident;
    rule_t var;
    rule< S, c, tag<Number> > number;
    rule_t ident_or_var;
    rule_t ident_or_var_or_number;
    rule_t cons;
    rule< S, c, tag<Term> > term;
    rule< S, c, tag<Terms> > terms; // list of terms
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
    rule< S, c, tag<Disj> > disj;
    rule< S, c, tag<Neg> > neg;
    rule< S, c, tag<UserPredClassical> > user_pred_classical;
    rule< S, c, tag<UserPredTuple> > user_pred_tuple;
    rule< S, c, tag<UserPredAtom> > user_pred_atom;
    rule< S, c, tag<UserPred> > user_pred;
    rule< S, c, tag<Body> > body; // body of a rule or constraint or aggregate
    rule< S, c, tag<Maxint> > maxint;
    rule< S, c, tag<Namespace> > namespace_;
    rule< S, c, tag<Rule> > rule_;
    rule< S, c, tag<Constraint> > constraint;
    rule< S, c, tag<WeakConstraint> > wconstraint;
    rule_t clause;
    rule< S, c, tag<Root> > root;
  };
};

void recordFilePosition(node_t& node, const pos_iterator_t& first, const pos_iterator_t& last)
{
  // it is only possible this way, because node_val_data lacks non-const value()
  CustomNodeData data = node.value.value(); // get
  data.pos = first.get_position(); // modify
  node.value.value(data); // set
  (void)last; // keep compiler happy
}

// TODO: use the older one and trim in the create* funtions, or copy correctly working _d into this source file with a different name
// newer boost requires this
#define flatten sp::reduced_node_d
// KBS boost requires this (this does not work correctly in newer boost)
//#define flatten sp::leaf_node_d

// impl of HexSpiritGrammar
template<typename ScannerT>
HexSpiritGrammar::definition<ScannerT>::definition(HexSpiritGrammar const&)
{
  // shortcut for sp::discard_node_d()
  const sp::node_parser_gen<sp::discard_node_op> rm =
    sp::node_parser_gen<sp::discard_node_op>();

  // identifier or string
  ident
    = flatten[sp::regex_p("[a-z][a-zA-Z0-9_]*")]
    | flatten[sp::confix_p("\"", *sp::anychar_p, "\"")];
  // variable
  var
    = flatten[sp::regex_p("[A-Z][a-zA-Z0-9_]*")];
  // nonnegative integer
  number
    = flatten[+sp::digit_p];
  ident_or_var
    = ident | var;
  ident_or_var_or_number
    = ident | var | number;
  aggregate_leq_binop
    = str_p("<=") | '<';
  aggregate_geq_binop
    = str_p(">=") | '>';
  aggregate_binop
    = aggregate_leq_binop | aggregate_geq_binop | str_p("==") | '=';
  binop
    = aggregate_binop | str_p("!=") | str_p("<>");
  tertop
    = ch_p('*') | '+';
  cons
    = str_p(":-") | str_p("<-");
  // identifiers, variables, numbers, anonymous variables
  term
    = ident_or_var_or_number | ch_p('_');
  terms
    = term >> *(rm[ch_p(',')] >> term);
  neg
    = ch_p('-')|ch_p('~');
  user_pred_classical
    = !neg >> ident_or_var >> '(' >> terms >> ')';
  user_pred_tuple
    = '(' >> terms >> ')';
  user_pred_atom
    = !neg >> ident_or_var;
  user_pred
    = user_pred_classical | user_pred_tuple | user_pred_atom;
  external_atom
    = ch_p('&') >> ident >> !('[' >> !terms >> ']') >> !('(' >> !terms >> ')');
  aggregate_pred
    = (str_p("#any")|"#avg"|"#count"|"#max"|"#min"|"#sum"|"#times")
    >> '{' >> terms >> ':' >> body >> '}';
  aggregate
    = (term >> aggregate_binop >> aggregate_pred)
    | (aggregate_pred >> aggregate_binop >> term)
    | (term >> aggregate_leq_binop >> aggregate_pred >> aggregate_leq_binop >> term)
    | (term >> aggregate_geq_binop >> aggregate_pred >> aggregate_geq_binop >> term);
  builtin_pred
    = (term >> '=' >> term >> tertop >> term)
    | (tertop >> '(' >> term >> rm[ch_p(',')] >> term >> rm[ch_p(',')] >> term >> ')')
    | (binop >> '(' >> term >> rm[ch_p(',')] >> term >> ')')
    | (term >> binop >> term)
    | (str_p("#int") >> '(' >> term >> ')')
    | (str_p("#succ") >> '(' >> term >> rm[ch_p(',')] >> term >> ')')
    ;
  literal
    = builtin_pred
    | ( !(str_p("not")|str_p("non")) >> (user_pred | external_atom | aggregate) );
  disj = user_pred >> *(rm[ch_p('v')] >> user_pred);
  body = literal >> *(rm[ch_p(',')] >> literal);
  maxint = str_p("#maxint") >> '=' >> number >> rm[ch_p('.')];
  // TODO: change #namespace to have "." at the end?
  // TODO: sp::eol_p should be added, but this does not work (because of skip parser?)
  namespace_ = str_p("#namespace") >> '(' >> ident >> rm[ch_p(',')] >> ident >> ')';
  // rule (optional body/condition)
  rule_ = disj >> !(cons >> !body) >> rm[ch_p('.')];
  // constraint
  constraint =  (cons >> body >> rm[ch_p('.')]);
  // weak constraint
  wconstraint =
    str_p(":~") >> body >> rm[ch_p('.')]
    // optional weight
    >> !( rm[ch_p('[')]
       >> !ident_or_var_or_number
       >> rm[ch_p(':')]
       >> !ident_or_var_or_number
       >> rm[ch_p(']')]
        );
  clause
    = sp::access_node_d[maxint | namespace_ | rule_ | constraint | wconstraint]
        [&recordFilePosition];
  root
    = *( // comment
         rm[sp::comment_p("%")]
       | clause
       )
       // end_p enforces a "full" match (in case of success) even with trailing newlines
       >> !sp::end_p;
}

void printSpiritPT(std::ostream& o, const node_t& node, const std::string& indent)
{
  o << indent << "'" << std::string(node.value.begin(), node.value.end()) << "'\t\t\t(" << node.value.id().to_long() << ")" << std::endl;
  if( !node.children.empty() )
  {
    std::string cindent(indent + "  ");
    for(node_t::const_tree_iterator it = node.children.begin(); it != node.children.end(); ++it)
    {
      printSpiritPT(o, *it, cindent);
    }
  }
}

//
// creating the dlvhex AST from the boost::spirit PT
//

void createASTFromClause(node_t& node, Program& program, AtomSet& edb);
void createAST(node_t& node, Program& program, AtomSet& edb)
{
  // node is from "root" rule
  assert(node.value.id() == HexSpiritGrammar::Root);
  for(node_t::tree_iterator it = node.children.begin(); it != node.children.end(); ++it)
    createASTFromClause(*it, program, edb);
}

// optionally assert whether node comes from certain rule
// descend into tree at node, until one child with value is found
// return this as string
// assert if multiple children with values are found
std::string createStringFromNode(node_t& node,
    HexSpiritGrammar::RuleTags verifyRuleTag = HexSpiritGrammar::None)
{
  // verify the tag
  assert(verifyRuleTag == HexSpiritGrammar::None || node.value.id() == verifyRuleTag);
  // debug output
  //printSpiritPT(std::cerr, node);
  // descend as long as there is only one child and the node has no value
  node_t& at = node;
  while( (at.children.size() == 1) && (at.value.begin() == at.value.end()) )
    at = at.children[0];
  // if we find one child which has a value, we return it
  if( at.value.begin() != at.value.end() )
    return std::string(at.value.begin(), at.value.end());
  // if we find multiple children which have a value, this is an error
  assert(false && "found multiple value children in createStringFromNode");
}

RuleHead_t createRuleHeadFromDisj(node_t& node);
RuleBody_t createRuleBodyFromBody(node_t& node);
void createASTFromClause(node_t& node, Program& program, AtomSet& edb)
{
  // node is from "clause" rule
  assert(node.children.size() == 1);
  node_t& child = node.children[0];
  //printSpiritPT(std::cerr, child);
  switch(child.value.id().to_long())
  {
  case HexSpiritGrammar::Maxint:
    Globals::Instance()->maxint = "#maxint=" + createStringFromNode(
        child.children[2], HexSpiritGrammar::Number) + ".";
    break;
  case HexSpiritGrammar::Namespace:
    {
      std::string prefix = createStringFromNode(child.children[2]);
      if( prefix[0] == '"' ) prefix = prefix.substr(1, prefix.length()-2);
      std::string ns = createStringFromNode(child.children[3]);
      if( ns[0] == '"' ) ns = ns.substr(1, ns.length()-2);
      Term::namespaces.push_back(std::make_pair(ns,prefix));
    }
    break;
  case HexSpiritGrammar::Rule:
    {
      RuleHead_t head = createRuleHeadFromDisj(child.children[0]);
      RuleBody_t body;
      if( child.children.size() == 3 )
        // nonempty body
        body = createRuleBodyFromBody(child.children[2]);

      if( body.empty() && head.size() == 1 )
      {
        // atom -> edb
        edb.insert(*head.begin());
      }
      else
      {
        // rule -> program
        Rule* r = new Rule(head, body,
            node.value.value().pos.file, node.value.value().pos.line);

        // Storing the rule as a ProgramObject. We don't have to take care
        // of deleting this pointer any more now.
        Registry::Instance()->storeObject(r);
        program.addRule(r);
      }
    }
    break;
  case HexSpiritGrammar::Constraint:
    {
      // empty head
      RuleHead_t head;
      RuleBody_t body = createRuleBodyFromBody(child.children[1]);
      Rule* r = new Rule(head, body,
          node.value.value().pos.file, node.value.value().pos.line);

      // Storing the rule as a ProgramObject. We don't have to take care
      // of deleting this pointer any more now.
      Registry::Instance()->storeObject(r);
      program.addRule(r);
    }
    break;
  case HexSpiritGrammar::WeakConstraint:
    // TODO: weak constraints
    printSpiritPT(std::cerr, child);
    break;
  default:
    assert(false && "encountered unknown node in createASTFromClause!");
  }
}

AtomPtr createAtomFromUserPred(node_t& node);
RuleHead_t createRuleHeadFromDisj(node_t& node)
{
  assert(node.value.id() == HexSpiritGrammar::Disj);
  RuleHead_t head;
  for(node_t::tree_iterator it = node.children.begin(); it != node.children.end(); ++it)
    head.insert(createAtomFromUserPred(*it));
  return head;
}

RuleBody_t createRuleBodyFromBody(node_t& node)
{
  assert(node.value.id() == HexSpiritGrammar::Body);
  RuleBody_t body;
  // TODO
  return body;
}

Tuple createTupleFromTerms(node_t& node);
AtomPtr createAtomFromUserPred(node_t& node)
{
  assert(node.value.id() == HexSpiritGrammar::UserPred);
  node_t prednode = node.children[0];
  bool neg = (prednode.children[0].value.id() == HexSpiritGrammar::Neg);
  int offset = neg?1:0;
  switch(prednode.value.id().to_long())
  {
  case HexSpiritGrammar::UserPredClassical:
    return AtomPtr(new Atom(
          createStringFromNode(prednode.children[0+offset]),
          createTupleFromTerms(prednode.children[2+offset]), neg));
  case HexSpiritGrammar::UserPredTuple:
    return AtomPtr(new Atom(
          createTupleFromTerms(prednode.children[1])));
  case HexSpiritGrammar::UserPredAtom:
    return AtomPtr(new Atom(
          createStringFromNode(prednode.children[0+offset]), neg));
  default:
    assert(false && "encountered unknown node in createAtomFromUserPred!");
    return AtomPtr(); // keep the compiler happy
  }
}

Term createTermFromTerm(node_t& node);

Tuple createTupleFromTerms(node_t& node)
{
  assert(node.value.id() == HexSpiritGrammar::Terms);
  Tuple t;
  for(node_t::tree_iterator it = node.children.begin();
      it != node.children.end(); ++it)
    t.push_back(createTermFromTerm(*it));
  return t;
}

Term createTermFromTerm(node_t& node)
{
  assert(node.value.id() == HexSpiritGrammar::Term);
  std::string s = createStringFromNode(node);
  if( s == "_" )
  {
    // anonymous variable
    return Term();
  }
  else if( s.find_first_not_of("0123456789") == std::string::npos )
  {
    // integer term
    std::stringstream st;
    st <<  s;
    int i;
    st >> i;
    return Term(i);
  }
  else
  {
    // string term
    return Term(s);
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

  // create grammar
  HexSpiritGrammar grammar;

  // remember line numbers
  pos_iterator_t it_begin(input.c_str(), input.c_str()+input.size());
  pos_iterator_t it_end;

  // parse ast
  sp::tree_parse_info<pos_iterator_t, factory_t> info =
    sp::pt_parse<factory_t>(it_begin, it_end, grammar, sp::space_p);

  // debug output
  std::cerr << "spirit match: full=" << info.full << ", match=" << info.match << std::endl;
  node_t::children_t::iterator itt;
  for(itt = info.trees.begin(); itt != info.trees.end(); ++itt)
  {
    std::cerr << "got tree from spirit:" << std::endl;
    printSpiritPT(std::cerr, *itt);
  }

  // successful parse?
  if( !info.full )
    throw SyntaxError("Could not parse complete input!",
        info.stop.get_position().line, this->source);

  // if this is not ok, there is some bug and the following code will be incomplete
  assert(info.trees.size() == 1);

  // create dlvhex AST from spirit (rudimentary) AST
  createAST(*info.trees.begin(), program, EDB);

  return;

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

// vim: set expandtab:
// Local Variables:
// mode: C++
// End:
