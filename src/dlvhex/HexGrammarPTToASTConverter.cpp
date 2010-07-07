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
 * @file   HexGrammarPTToASTConverter.cpp
 * @author Peter Schüller
 * @date   Wed Jul  8 14:30:08 CEST 2009
 * 
 * @brief  Converter: parse tree from HexGrammar to HEX AST
 */

#include "dlvhex/HexGrammarPTToASTConverter.h"

#include "dlvhex/Registry.h"
#include "dlvhex/globals.h"
#include "dlvhex/SpiritDebugging.h"
#include <boost/algorithm/string/trim.hpp>

#include <sstream>
#include <cassert>

DLVHEX_NAMESPACE_BEGIN

void HexGrammarPTToASTConverter::convertPTToAST(
    node_t& node, Program& program, AtomSet& edb)
{
  // node is from "root" rule
  assert(node.value.id() == HexGrammar::Root);
  for(node_t::tree_iterator it = node.children.begin();
      it != node.children.end(); ++it)
    if( it->value.id() == HexGrammar::Clause )
      createASTFromClause(*it, program, edb);
}

// optionally assert whether node comes from certain rule
// descend into tree at node, until one child with value is found
// return this as string
// assert if multiple children with values are found
std::string HexGrammarPTToASTConverter::createStringFromNode(
    node_t& node, HexGrammar::RuleTags verifyRuleTag)
{
  // verify the tag
  assert(verifyRuleTag == HexGrammar::None || node.value.id() == verifyRuleTag);
  // debug output
  //printSpiritPT(std::cerr, node);
  // descend as long as there is only one child and the node has no value
  node_t& at = node;
  while( (at.children.size() == 1) && (at.value.begin() == at.value.end()) )
    at = at.children[0];
  // if we find one child which has a value, we return it
  if( at.value.begin() != at.value.end() )
  {
    std::string ret(at.value.begin(), at.value.end());
    boost::trim(ret);
    //std::cerr << "createStringFromNode returns '" << ret << "'" << std::endl;
    return ret;
  }
  // if we find multiple children which have a value, this is an error
  assert(false && "found multiple value children in createStringFromNode");
}

Term HexGrammarPTToASTConverter::createTerm_Helper(
    node_t& node, HexGrammar::RuleTags verify)
{
  assert(node.value.id() == verify);
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

void HexGrammarPTToASTConverter::createASTFromClause(
    node_t& node, Program& program, AtomSet& edb)
{
  // node is from "clause" rule
  assert(node.children.size() == 1);
  node_t& child = node.children[0];
  switch(child.value.id().to_long())
  {
  case HexGrammar::Maxint:
    //printSpiritPT(std::cerr, child, "maxint>>");
    Globals::Instance()->maxint = "#maxint=" + createStringFromNode(
        child.children[2], HexGrammar::Number) + ".";
    break;
  case HexGrammar::Namespace:
    {
      std::string prefix = createStringFromNode(child.children[2]);
      if( prefix[0] == '"' ) prefix = prefix.substr(1, prefix.length()-2);
      std::string ns = createStringFromNode(child.children[4]);
      if( ns[0] == '"' ) ns = ns.substr(1, ns.length()-2);
      Term::getNameSpaces().push_back(std::make_pair(ns,prefix));
    }
    break;
  case HexGrammar::Rule:
    {
      //printSpiritPT(std::cerr, child, "rule>>");
      RuleHead_t head = createRuleHeadFromDisj(child.children[0]);
      RuleBody_t body;
      if( child.children.size() == 4 )
        // nonempty body
        body = createRuleBodyFromBody(child.children[2]);

      if( body.empty() && head.size() == 1 )
      {
        // atom -> edb
        AtomPtr at = *head.begin();
        if( !at->isGround() )
          throw SyntaxError("fact not safe!");
        edb.insert(at);
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
  case HexGrammar::Constraint:
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
  case HexGrammar::WeakConstraint:
    {
      Term leftTerm(1);
      Term rightTerm(1);
      if( child.children.size() > 6 )
      {
        // there is some weight
        unsigned offset = 0;
        if( !child.children[4].children.empty() )
        {
          // found first weight
          leftTerm = createTermFromIdentVarNumber(child.children[4]);
          offset = 1;
        }
        if( !child.children[5+offset].children.empty() )
        {
          // found second weight
          rightTerm = createTermFromIdentVarNumber(child.children[5+offset]);
        }
      }
      WeakConstraint* c = new WeakConstraint(
        createRuleBodyFromBody(child.children[1]),
        leftTerm, rightTerm);
      program.addWeakConstraint(c);
    }
    break;
  default:
    assert(false && "encountered unknown node in createASTFromClause!");
  }
}

RuleHead_t HexGrammarPTToASTConverter::createRuleHeadFromDisj(node_t& node)
{
  assert(node.value.id() == HexGrammar::Disj);
  RuleHead_t head;
  for(node_t::tree_iterator it = node.children.begin();
      it != node.children.end(); ++it)
    head.insert(createAtomFromUserPred(*it));
  return head;
}

RuleBody_t HexGrammarPTToASTConverter::createRuleBodyFromBody(node_t& node)
{
  assert(node.value.id() == HexGrammar::Body);
  RuleBody_t body;
  for(node_t::tree_iterator it = node.children.begin();
      it != node.children.end(); ++it)
    body.insert(createLiteralFromLiteral(*it));
  return body;
}

Literal* HexGrammarPTToASTConverter::createLiteralFromLiteral(node_t& node)
{
  assert(node.value.id() == HexGrammar::Literal);
  Literal* lit = 0;
  if( node.children[0].value.id() == HexGrammar::BuiltinPred )
  {
    lit = new Literal(createBuiltinPredFromBuiltinPred(node.children[0]));
  }
  else
  {
    bool naf = node.children[0].value.id() == HexGrammar::Naf;
    int offset = naf?1:0;
    switch(node.children[offset].value.id().to_long())
    {
    case HexGrammar::UserPred:
      lit = new Literal(createAtomFromUserPred(node.children[offset]), naf);
      break;
    case HexGrammar::ExtAtom:
      lit = new Literal(createExtAtomFromExtAtom(node.children[offset]), naf);
      break;
    case HexGrammar::Aggregate:
      lit = new Literal(createAggregateFromAggregate(node.children[offset]), naf);
      break;
    default:
      assert(false && "encountered unknown node in createLiteralFromLiteral!");
    }
  }
  assert(lit);
  Registry::Instance()->storeObject(lit);
  return lit;
}

AtomPtr HexGrammarPTToASTConverter::createAtomFromUserPred(node_t& node)
{
  assert(node.value.id() == HexGrammar::UserPred);
  node_t prednode = node.children[0];
  bool neg = (prednode.children[0].value.id() == HexGrammar::Neg);
  int offset = neg?1:0;
  switch(prednode.value.id().to_long())
  {
  case HexGrammar::UserPredClassical:
    return AtomPtr(new Atom(
          createStringFromNode(prednode.children[0+offset]),
          createTupleFromTerms(prednode.children[2+offset]), neg));
  case HexGrammar::UserPredTuple:
    return AtomPtr(new Atom(
          createTupleFromTerms(prednode.children[1])));
  case HexGrammar::UserPredAtom:
    return AtomPtr(new Atom(
          createStringFromNode(prednode.children[0+offset]), neg));
  default:
    assert(false && "encountered unknown node in createAtomFromUserPred!");
    return AtomPtr(); // keep the compiler happy
  }
}

AtomPtr HexGrammarPTToASTConverter::createBuiltinPredFromBuiltinPred(node_t& node)
{
  assert(node.value.id() == HexGrammar::BuiltinPred);
  node_t& child = node.children[0];
  switch(child.value.id().to_long())
  {
  case HexGrammar::BuiltinTertopPrefix:
    return AtomPtr(new BuiltinPredicate(
          createTermFromTerm(child.children[2]),
          createTermFromTerm(child.children[4]),
          createTermFromTerm(child.children[6]),
          createStringFromNode(child.children[0])));
  case HexGrammar::BuiltinTertopInfix:
    return AtomPtr(new BuiltinPredicate(
          createTermFromTerm(child.children[2]),
          createTermFromTerm(child.children[4]),
          createTermFromTerm(child.children[0]),
          createStringFromNode(child.children[3])));
  case HexGrammar::BuiltinBinopPrefix:
    return AtomPtr(new BuiltinPredicate(
          createTermFromTerm(child.children[2]),
          createTermFromTerm(child.children[4]),
          createStringFromNode(child.children[0])));
  case HexGrammar::BuiltinBinopInfix:
    return AtomPtr(new BuiltinPredicate(
          createTermFromTerm(child.children[0]),
          createTermFromTerm(child.children[2]),
          createStringFromNode(child.children[1])));
  case HexGrammar::BuiltinOther:
    if( child.children.size() == 6 )
      // #succ/2
      return AtomPtr(new BuiltinPredicate(
        createTermFromTerm(child.children[2]),
        createTermFromTerm(child.children[4]),
        createStringFromNode(child.children[0])));
    else
      // #int/1
      return AtomPtr(new BuiltinPredicate(
        createTermFromTerm(child.children[2]),
        createStringFromNode(child.children[0])));

  default:
    assert(false && "encountered unknown node in createBuiltinPredFromBuiltinPred!");
    return AtomPtr(); // keep the compiler happy
  }
}

AtomPtr HexGrammarPTToASTConverter::createExtAtomFromExtAtom(node_t& node)
{
  //printSpiritPT(std::cerr, node, ">>");
  assert(node.value.id() == HexGrammar::ExtAtom);
  Tuple inputs;
  Tuple outputs;
  if( node.children.size() > 2 )
  {
    // either input or output
    unsigned offset = 0;
    if( node.children[2].value.id() == HexGrammar::ExtInputs )
    {
      // input
      offset = 1;
      if( node.children[2].children.size() > 2 )
      {
        // there is at least one term
        //printSpiritPT(std::cerr, node.children[2].children[1], ">>ii");
        inputs = createTupleFromTerms(node.children[2].children[1]);
      }
    }
    // check for output
    if( node.children.size() > (2+offset) )
    {
      if( node.children[2+offset].value.id() == HexGrammar::ExtOutputs )
      {
        // output
        if( node.children[2+offset].children.size() > 2 )
        {
          // there is at least one term
          //printSpiritPT(std::cerr, node.children[2+offset].children[1], ">>oo");
          outputs = createTupleFromTerms(node.children[2+offset].children[1]);
        }
      }
    }
  }
  ExternalAtom* extat = new ExternalAtom(
    createStringFromNode(node.children[1]),
    outputs, inputs, node.value.value().pos.line);

  return AtomPtr(extat);
}

AggregateAtomPtr HexGrammarPTToASTConverter::createAggregateFromAggregate(
    node_t& node)
{
  assert(node.value.id() == HexGrammar::Aggregate);

  AggregateAtomPtr agg;
  Term leftTerm;
  std::string leftComp;
  Term rightTerm;
  std::string rightComp;

  node_t& child = node.children[0];
  if( child.value.id() == HexGrammar::AggregateRel )
  {
    // binary relation between aggregate and term
    if( child.children[0].value.id() == HexGrammar::Term )
    {
      leftTerm = createTermFromTerm(child.children[0]);
      leftComp = createStringFromNode(child.children[1]);
      agg = createAggregateFromAggregatePred(child.children[2]);
    }
    else
    {
      agg = createAggregateFromAggregatePred(child.children[0]);
      rightComp = createStringFromNode(child.children[1]);
      rightTerm = createTermFromTerm(child.children[2]);
    }
  }
  else
  {
    // aggregate is in (ternary) range between terms
    assert(child.value.id() == HexGrammar::AggregateRange);
    leftTerm = createTermFromTerm(child.children[0]);
    leftComp = createStringFromNode(child.children[1]);
    agg = createAggregateFromAggregatePred(child.children[2]);
    rightComp = createStringFromNode(child.children[3]);
    rightTerm = createTermFromTerm(child.children[4]);
  }
  agg->setComp(leftComp, rightComp);
  agg->setLeftTerm(leftTerm);
  agg->setRightTerm(rightTerm);
  return agg;
}

AggregateAtomPtr HexGrammarPTToASTConverter::createAggregateFromAggregatePred(
    node_t& node)
{
  assert(node.value.id() == HexGrammar::AggregatePred);
  AggregateAtomPtr agg(new AggregateAtom(
        createStringFromNode(node.children[0]),
        createTupleFromTerms(node.children[2]),
        createRuleBodyFromBody(node.children[4])));
  return agg;
}

Tuple HexGrammarPTToASTConverter::createTupleFromTerms(node_t& node)
{
  assert(node.value.id() == HexGrammar::Terms);
  Tuple t;
  for(node_t::tree_iterator it = node.children.begin();
      it != node.children.end(); ++it)
    t.push_back(createTermFromTerm(*it));
  return t;
}

Term HexGrammarPTToASTConverter::createTermFromIdentVarNumber(node_t& node)
{
  return createTerm_Helper(node, HexGrammar::IdentVarNumber);
}

Term HexGrammarPTToASTConverter::createTermFromTerm(node_t& node)
{
  return createTerm_Helper(node, HexGrammar::Term);
}

DLVHEX_NAMESPACE_END

// vim: set expandtab:

// Local Variables:
// mode: C++
// End:
