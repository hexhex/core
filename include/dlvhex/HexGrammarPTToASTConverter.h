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
 * @file   HexGrammarPTToASTConverter.h
 * @author Peter Schüller
 * @date   Wed Jul  8 14:30:08 CEST 2009
 * 
 * @brief  Converter: parse tree from HexGrammar to HEX AST
 */

#ifndef DLVHEX_HEX_GRAMMAR_PT_TO_AST_CONVERTER_H_INCLUDED
#define DLVHEX_HEX_GRAMMAR_PT_TO_AST_CONVERTER_H_INCLUDED

#include "dlvhex/PlatformDefinitions.h"

// AST
#include "dlvhex/ID.hpp"

// boost::spirit parsing
#include "dlvhex/HexGrammar.h"
#include "dlvhex/SpiritFilePositionNode.h"

#include <boost/spirit/iterator/position_iterator.hpp>

DLVHEX_NAMESPACE_BEGIN

class ProgramCtx;

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
  HexGrammarPTToASTConverter(ProgramCtx& ctx): ctx(ctx) { }

  // convert the root node of the spirit parse tree (from HexSpiritGrammar)
  // to an idb and idb in ProgramCtx, using registry of ProgramCtx
  void convertPTToAST(node_t& node);

private:
  ProgramCtx& ctx;

  //
  // general helpers
  //

  // verify type of node
  // follow tree until a single content node is found
  // return its content as a string
  // TODO: do not return but create result in ref arg
  std::string createStringFromNode(node_t& node,
      HexGrammar::RuleTags verifyRuleTag = HexGrammar::None);

  // use createStringFromNode to get data
  // create correct term type and return id (anonymous vs numeric vs string)
  ID createTerm_Helper(node_t& node, HexGrammar::RuleTags verify);

  //
  // converters for specific rules
  //

  // rule "clause", put result into ctx.edb and ctx.idb
  void createASTFromClause(node_t& node);
  // rule "disj"
  // TODO: do not return but create result in ref arg
  Tuple createRuleHeadFromDisj(node_t& node);
  // rule "body"
  // TODO: do not return but create result in ref arg
  Tuple createRuleBodyFromBody(node_t& node);

  // rule "literal"
  ID createLiteralFromLiteral(node_t& node);
  // rule "user_pred"
  ID createAtomFromUserPred(node_t& node);
  // rule "builtin_pred"
  ID createBuiltinPredFromBuiltinPred(node_t& node);
  // rule "external_atom"
  ID createExtAtomFromExtAtom(node_t& node);
  // rule "external_atom"
  ID createExtAtomFromModAtom(node_t& node);

  // rule "aggregate"
  ID createAggregateFromAggregate(node_t& node);
  // rule "aggregate_pred"
  ID createAggregateFromAggregatePred(node_t& node);

  // rule "terms"
  // TODO: do not return but create result in ref arg
  Tuple createTupleFromTerms(node_t& node);
  // rule "ident_or_var_or_number"
  ID createTermFromIdentVarNumber(node_t& node);
  // rule "ident_or_var"
  ID createTermFromIdentVar(node_t& node);
  // rule "term"
  ID createTermFromTerm(node_t& node);
};

DLVHEX_NAMESPACE_END

#endif // DLVHEX_HEX_GRAMMAR_PT_TO_AST_CONVERTER_H_INCLUDED

// vim: set expandtab:

// Local Variables:
// mode: C++
// End:
