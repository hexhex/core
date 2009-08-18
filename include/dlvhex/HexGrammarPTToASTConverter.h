/* dlvhex -- Answer-Set Programming with external interfaces.
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
 * @file   HexGrammarPTToASTConverter.h
 * @author Peter Schüller
 * @date   Wed Jul  8 14:30:08 CEST 2009
 * 
 * @brief  Converter: parse tree from HexGrammar to HEX AST
 */

#ifndef DLVHEX_HEX_GRAMMAR_PT_TO_AST_CONVERTER_H_INCLUDED
#define DLVHEX_HEX_GRAMMAR_PT_TO_AST_CONVERTER_H_INCLUDED

// HEX AST
#include "dlvhex/Program.h"
#include "dlvhex/AtomSet.h"
#include "dlvhex/AggregateAtom.h"
#include "dlvhex/Error.h"

// boost::spirit parsing
#include "dlvhex/HexGrammar.h"
#include "dlvhex/SpiritFilePositionNode.h"

#include <boost/spirit/iterator/position_iterator.hpp>

DLVHEX_NAMESPACE_BEGIN

// converts the parse tree from boost::spirit to the HEX AST
class HexGrammarPTToASTConverter
{
public:
  typedef FilePositionNodeFactory<FilePositionNodeData> factory_t;

  // iterator which remembers file positions (useful for error messages)
  typedef boost::spirit::position_iterator<const char*> iterator_t;

  // node type for spirit PT
  typedef boost::spirit::tree_match<iterator_t, factory_t>::node_t node_t;

public:
  // convert the root node of the spirit parse tree (from HexSpiritGrammar)
  // to a program and edb
  void convertPTToAST(node_t& node, Program& program, AtomSet& edb);

private:
  //
  // general helpers
  //

  // verify type of node
  // follow tree until a single content node is found
  // return its content as a string
  std::string createStringFromNode(node_t& node,
      HexGrammar::RuleTags verifyRuleTag = HexGrammar::None);

  // use createStringFromNode to get data
  // create correct term type (anonymous vs numeric vs string)
  Term createTerm_Helper(node_t& node, HexGrammar::RuleTags verify);

  //
  // converters for specific rules
  //

  // rule "clause"
  void createASTFromClause(node_t& node, Program& program, AtomSet& edb);
  // rule "disj"
  RuleHead_t createRuleHeadFromDisj(node_t& node);
  // rule "body"
  RuleBody_t createRuleBodyFromBody(node_t& node);

  // rule "literal"
  Literal* createLiteralFromLiteral(node_t& node);
  // rule "user_pred"
  AtomPtr createAtomFromUserPred(node_t& node);
  // rule "builtin_pred"
  AtomPtr createBuiltinPredFromBuiltinPred(node_t& node);
  // rule "external_atom"
  AtomPtr createExtAtomFromExtAtom(node_t& node);

  // rule "aggregate"
  AggregateAtomPtr createAggregateFromAggregate(node_t& node);
  // rule "aggregate_pred"
  AggregateAtomPtr createAggregateFromAggregatePred(node_t& node);

  // rule "terms"
  Tuple createTupleFromTerms(node_t& node);
  // rule "ident_or_var_or_number"
  Term createTermFromIdentVarNumber(node_t& node);
  // rule "term"
  Term createTermFromTerm(node_t& node);
};

DLVHEX_NAMESPACE_END

#endif // DLVHEX_HEX_GRAMMAR_PT_TO_AST_CONVERTER_H_INCLUDED

// vim: set expandtab:
