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

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/HexGrammarPTToASTConverter.h"

#include "dlvhex/ProgramCtx.h"
//#include "dlvhex/globals.h"
#include "dlvhex/SpiritDebugging.h"

#include <boost/algorithm/string/trim.hpp>
#include <boost/foreach.hpp>

#include <sstream>
#include <cassert>

DLVHEX_NAMESPACE_BEGIN

void HexGrammarPTToASTConverter::convertPTToAST(
    node_t& node)
{
  // node is from "root" rule
  assert(node.value.id() == HexGrammar::Root);
  for(node_t::tree_iterator it = node.children.begin();it != node.children.end(); ++it){
    if( it->value.id() == HexGrammar::Clause ) createASTFromClause(*it);
    if( it->value.id() == HexGrammar::ModHeader ) doModuleHeader(*it);
  }

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
  node_t* at = &node;
  while( (at->children.size() == 1) && (at->value.begin() == at->value.end()) )
    at = &at->children[0];
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

ID HexGrammarPTToASTConverter::createTerm_Helper(
    node_t& node, HexGrammar::RuleTags verify)
{
  assert(node.value.id() == verify);
  std::string s = createStringFromNode(node);
  assert(!s.empty());
  if( s == "_" )
  {
    // anonymous variable
    ID id = ctx.registry->terms.getIDByString("_");
    if( id == ID_FAIL )
    {
      Term term(ID::MAINKIND_TERM | ID::SUBKIND_TERM_VARIABLE |
        ID::PROPERTY_VAR_ANONYMOUS, "_");
      id = ctx.registry->terms.storeAndGetID(term);
    }
    return id;
  }
  else if( s.find_first_not_of("0123456789") == std::string::npos )
  {
    // integer term
    std::stringstream st;
    st <<  s;
    ID id(ID::MAINKIND_TERM | ID::SUBKIND_TERM_INTEGER, 0);
    st >> id.address;
    return id;
  }
  else
  {
    // string, variable, or constant term
    if( s[0] == '"' )
      LOG("warning: we should expand the namespace of s='" << s << "' here!");
    ID id = ctx.registry->terms.getIDByString(s);
    if( id == ID_FAIL )
    {
      Term term(ID::MAINKIND_TERM, s);
      if( s[0] == '"' || islower(s[0]) )
        term.kind |= ID::SUBKIND_TERM_CONSTANT;
      else
        term.kind |= ID::SUBKIND_TERM_VARIABLE;
      id = ctx.registry->terms.storeAndGetID(term);
    }
    return id;
  }
}

namespace
{
  void markExternalPropertyIfExternalBody(Rule& r)
  {
    // determine external atom property
    for(Tuple::const_iterator itt = r.body.begin(); itt != r.body.end(); ++itt)
    {
      if( itt->isExternalAtom() )
      {
        r.kind |= ID::PROPERTY_RULE_EXTATOMS;
        return;
      } else 
      if( itt->isModuleAtom() )
      {
        r.kind |= ID::PROPERTY_RULE_MODATOMS;
        return;
      }
    }
  }
}

void HexGrammarPTToASTConverter::doModuleHeader(node_t& node)
{
  // node is from "mod_header" rule
  assert(node.children.size() == 9);
  LOG("got module header: " << createStringFromNode(node.children[2], HexGrammar::Ident) << "'");
}

void HexGrammarPTToASTConverter::createASTFromClause(
    node_t& node)
{
  // node is from "clause" rule
  assert(node.children.size() == 1);
  node_t& child = node.children[0];
  switch(child.value.id().to_long())
  {
  case HexGrammar::Maxint:
    {
      std::stringstream ss;
      ss << createStringFromNode(
        child.children[2], HexGrammar::Number);
      ss >> ctx.maxint;
      //printSpiritPT(std::cerr, child, "maxint>>");
    }
    break;
  case HexGrammar::Namespace:
    {
      std::string prefix = createStringFromNode(child.children[2]);
      if( prefix[0] == '"' ) prefix = prefix.substr(1, prefix.length()-2);
      std::string ns = createStringFromNode(child.children[4]);
      if( ns[0] == '"' ) ns = ns.substr(1, ns.length()-2);
      bool success;
      success = (ctx.registry->namespaces.insert(
             NamespaceTable::value_type(ns, prefix) )).second;
      if( !success )
        throw SyntaxError("error adding namespace '"+ns+"'/'"+prefix+"'");
    }
    break;
  case HexGrammar::Rule:
    {
      //printSpiritPT(std::cerr, child, "rule>>");
      Tuple head = createRuleHeadFromDisj(child.children[0]);
      Tuple body;
      if( child.children.size() == 4 )
        // nonempty body
        body = createRuleBodyFromBody(child.children[2]);

      if( body.empty() && head.size() == 1 )
      {
        // atom -> put into edb

        // TODO: this case should not go here, but be made faster by already detecting it in the grammar
        ID id = *head.begin();
        if( !id.isOrdinaryGroundAtom() )
          throw SyntaxError("fact '"+ctx.registry->ogatoms.getByID(id).text+"' not safe!");
        ctx.edb->setFact(id.address);
        LOG("added fact with id " << id << " to edb");
      }
      else
      {
        // rule -> put into idb

        //TODO: store file position in rule (it was stored for diagnostics)
        // node.value.value().pos.file, node.value.value().pos.line);
        Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_REGULAR, head, body);
        markExternalPropertyIfExternalBody(r);
        ID id = ctx.registry->rules.storeAndGetID(r);
        ctx.idb.push_back(id);
        LOG("added rule " << r << " with id " << id << " to idb");
      }
    }
    break;
  case HexGrammar::Constraint:
    {
      //TODO: store file position in rule (it was stored for diagnostics)
      // node.value.value().pos.file, node.value.value().pos.line);

      Rule r(ID::MAINKIND_RULE | ID::SUBKIND_RULE_CONSTRAINT);
      r.body = createRuleBodyFromBody(child.children[1]);
      markExternalPropertyIfExternalBody(r);
      ID id = ctx.registry->rules.storeAndGetID(r);
      ctx.idb.push_back(id);
      LOG("added constraint " << r << " with id " << id << " to idb");
    }
    break;
  case HexGrammar::WeakConstraint:
    {
      //TODO: store file position in rule (it was stored for diagnostics)
      // node.value.value().pos.file, node.value.value().pos.line);

      Rule r(
        ID::MAINKIND_RULE | ID::SUBKIND_RULE_WEAKCONSTRAINT,
        ID::termFromInteger(1),
        ID::termFromInteger(1));
      if( child.children.size() > 6 )
      {
        // there is some weight
        unsigned offset = 0;
        if( !child.children[4].children.empty() )
        {
          // found first weight
          r.weight = createTermFromIdentVarNumber(child.children[4]);
          offset = 1;
        }
        if( !child.children[5+offset].children.empty() )
        {
          // found second weight
          r.level = createTermFromIdentVarNumber(child.children[5+offset]);
        }
      }

      r.body = createRuleBodyFromBody(child.children[1]);
      markExternalPropertyIfExternalBody(r);
      ID id = ctx.registry->rules.storeAndGetID(r);
      ctx.idb.push_back(id);
      LOG("added weakconstraint " << r << " with id " << id << " to idb");
    }
    break;
  default:
    assert(false && "encountered unknown node in createASTFromClause!");
  }
}

Tuple HexGrammarPTToASTConverter::createRuleHeadFromDisj(node_t& node)
{
  assert(node.value.id() == HexGrammar::Disj);
  Tuple head;
  for(node_t::tree_iterator it = node.children.begin();
      it != node.children.end(); ++it)
    head.push_back(createAtomFromUserPred(*it));
  return head;
}

Tuple HexGrammarPTToASTConverter::createRuleBodyFromBody(node_t& node)
{
  assert(node.value.id() == HexGrammar::Body);
  Tuple body;
  for(node_t::tree_iterator it = node.children.begin();
      it != node.children.end(); ++it)
    body.push_back(createLiteralFromLiteral(*it));
  return body;
}

ID HexGrammarPTToASTConverter::createLiteralFromLiteral(node_t& node)
{
  assert(node.value.id() == HexGrammar::Literal);
  bool naf = node.children[0].value.id() == HexGrammar::Naf;
  int offset = naf?1:0;
  switch(node.children[offset].value.id().to_long())
  {
  case HexGrammar::BuiltinPred:
    return ID::literalFromAtom(createBuiltinPredFromBuiltinPred(node.children[0]), naf);
  case HexGrammar::UserPred:
    return ID::literalFromAtom(createAtomFromUserPred(node.children[offset]), naf);
  case HexGrammar::ExtAtom:
    return ID::literalFromAtom(createExtAtomFromExtAtom(node.children[offset]), naf);
  case HexGrammar::ModAtom:
    return ID::literalFromAtom(createModAtomFromModAtom(node.children[offset]), naf);
  case HexGrammar::Aggregate:
    return ID::literalFromAtom(createAggregateFromAggregate(node.children[offset]), naf);
  default:
    assert(false && "encountered unknown node in createLiteralFromLiteral!");
    return ID_FAIL;
  }
}

ID HexGrammarPTToASTConverter::createAtomFromUserPred(node_t& node)
{
  // we should do it that way:
  // lookup first by string (somehow normalized, best normalized to printing format)
  // if not found, create tuple and lookup by tuple [best would be if normalization does not allow this to happen]
  // if not found, create from tuple and string
  // TODO normalize tuple "(a,b,c)" atoms to classial ones "a(b,c)"
  // TODO normalize by removing spaces 
  // we will currently do it this way:
  // interpret tuple, lookup by tuple, store if new, otherwise reuse ID

  assert(node.value.id() == HexGrammar::UserPred);
  node_t& prednode = node.children[0];
  OrdinaryAtom atom(ID::MAINKIND_ATOM);
  bool neg = (prednode.children[0].value.id() == HexGrammar::Neg);
  int offset = neg?1:0;
	if( neg )
		throw std::runtime_error("strong negation currently not implemented!");
  //if( atom.neg )
  //  atom.kind |= ID::PROPERTY_NEGATIVE;
  switch(prednode.value.id().to_long())
  {
  case HexGrammar::UserPredClassical:
    {
      // <foo> ( <bar>, <baz>, ... )
      atom.tuple.push_back(createTermFromIdentVar(prednode.children[0+offset]));
      // =append
      Tuple t = createTupleFromTerms(prednode.children[2+offset]);
      atom.tuple.insert(atom.tuple.end(), t.begin(), t.end());
    }
    break;
  case HexGrammar::UserPredTuple:
    // ( <foo>, <bar>, <baz>, ... )
    atom.tuple = createTupleFromTerms(prednode.children[1]);
    break;
  case HexGrammar::UserPredAtom:
    // <foo>
    atom.tuple.push_back(createTermFromIdentVar(prednode.children[0+offset]));
    break;
  default:
    assert(false && "encountered unknown node in createAtomFromUserPred!");
  }

  // groundness
  LOG("checking groundness of tuple " << printrange(atom.tuple));
  IDKind kind = 0;
  BOOST_FOREACH(const ID& id, atom.tuple)
  {
    kind |= id.kind;
    // make this sure to make the groundness check work
    assert((id.kind & ID::SUBKIND_MASK) != ID::SUBKIND_TERM_BUILTIN);
  }
  const bool ground = !(kind & ID::SUBKIND_TERM_VARIABLE);
  OrdinaryAtomTable* tbl;
  if( ground )
  {
    atom.kind |= ID::SUBKIND_ATOM_ORDINARYG;
    tbl = &ctx.registry->ogatoms;
  }
  else
  {
    atom.kind |= ID::SUBKIND_ATOM_ORDINARYN;
    tbl = &ctx.registry->onatoms;
  }

  // lookup if we already know this one
  //LOG("looking up neg " << atom.neg << " tuple " << printvector(atom.tuple));
  LOG("looking up tuple " << printvector(atom.tuple));
  // TODO perhaps pass only ref to atom and let the key extractor do its magic
  {
    ID id = tbl->getIDByTuple(atom.tuple);
    if( id != ID_FAIL )
      return id;
  }

  // atom.text
  // TODO this must be done more efficiently!
  // TODO but we need to do the following in any case: normalizing tuple "(a,b,c)" atoms to classial ones "a(b,c)" (or to the kind of atoms that have to be parsed most likely, or the kind of atoms that should be printed?) 
  #warning parsing efficiency problem
  // ideal way:
  // get begin iterator of first child, end iterator of last child
  // text should be between those
  //
  // our way:
  // print it from the IDs in atom.tuple

  std::stringstream ss;
  RawPrinter printer(ss, ctx.registry);
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

  LOG("got atom text '" << atom.text << "'");
  ID id = tbl->storeAndGetID(atom);
  LOG("stored atom " << atom << " which got id " << id);
  return id;
}

ID HexGrammarPTToASTConverter::createBuiltinPredFromBuiltinPred(node_t& node)
{
  assert(node.value.id() == HexGrammar::BuiltinPred);
  node_t& child = node.children[0];

  BuiltinAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_BUILTIN);
  switch(child.value.id().to_long())
  {
  case HexGrammar::BuiltinTertopPrefix:
    atom.tuple.push_back(ID::termFromBuiltinString(
          createStringFromNode(child.children[0])));
    atom.tuple.push_back(createTermFromTerm(child.children[2]));
    atom.tuple.push_back(createTermFromTerm(child.children[4]));
    atom.tuple.push_back(createTermFromTerm(child.children[6]));
    break;
  case HexGrammar::BuiltinTertopInfix:
    atom.tuple.push_back(ID::termFromBuiltinString(
          createStringFromNode(child.children[3])));
    atom.tuple.push_back(createTermFromTerm(child.children[2]));
    atom.tuple.push_back(createTermFromTerm(child.children[4]));
    atom.tuple.push_back(createTermFromTerm(child.children[0]));
    break;
  case HexGrammar::BuiltinBinopPrefix:
    atom.tuple.push_back(ID::termFromBuiltinString(
          createStringFromNode(child.children[0])));
    atom.tuple.push_back(createTermFromTerm(child.children[2]));
    atom.tuple.push_back(createTermFromTerm(child.children[4]));
    break;
  case HexGrammar::BuiltinBinopInfix:
    atom.tuple.push_back(ID::termFromBuiltinString(
          createStringFromNode(child.children[1])));
    atom.tuple.push_back(createTermFromTerm(child.children[0]));
    atom.tuple.push_back(createTermFromTerm(child.children[2]));
    break;
  case HexGrammar::BuiltinOther:
    if( child.children.size() == 6 )
    {
      // #succ/2
      atom.tuple.push_back(ID::termFromBuiltinString(
            createStringFromNode(child.children[0])));
      atom.tuple.push_back(createTermFromTerm(child.children[2]));
      atom.tuple.push_back(createTermFromTerm(child.children[4]));
    }
    else
    {
      // #int/1
      atom.tuple.push_back(ID::termFromBuiltinString(
            createStringFromNode(child.children[0])));
      atom.tuple.push_back(createTermFromTerm(child.children[2]));
    }
    break;

  default:
    assert(false && "encountered unknown node in createBuiltinPredFromBuiltinPred!");
    return ID_FAIL; // keep the compiler happy
  }

  LOG("storing builtin atom " << atom);
  ID id = ctx.registry->batoms.storeAndGetID(atom);
  LOG("stored builtin atom " << atom << " which got id " << id);
  return id;
}

ID HexGrammarPTToASTConverter::createExtAtomFromExtAtom(node_t& node)
{
  //printSpiritPT(std::cerr, node, ">>");
  assert(node.value.id() == HexGrammar::ExtAtom);
  ExternalAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_EXTERNAL);
  atom.predicate = createTerm_Helper(node.children[1], HexGrammar::Ident);
  Tuple& inputs = atom.inputs;
  Tuple& outputs = atom.tuple;
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

  LOG("storing external atom " << atom);
  ID id = ctx.registry->eatoms.storeAndGetID(atom);
  LOG("stored external atom " << atom << " which got id " << id);
  return id;
}

ID HexGrammarPTToASTConverter::createModAtomFromModAtom(node_t& node)
{
  std::cout << "create module atom: " << createStringFromNode(node.children[1], HexGrammar::Ident) <<std::endl;
  //printSpiritPT(std::cerr, node, ">>");
  assert(node.value.id() == HexGrammar::ModAtom);
  ModuleAtom atom(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_MODULE);
  atom.predicate = createTerm_Helper(node.children[1], HexGrammar::Ident);
  Tuple& inputs = atom.inputs;
  Tuple& outputs = atom.tuple;
  if( node.children.size() > 2 )
  {
    // either input or output
    unsigned offset = 0;
    if( node.children[2].value.id() == HexGrammar::ModInputs )
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
    offset = offset + 1;
    assert(node.children[2+offset].value.id() == HexGrammar::Ident);
    atom.outputpredicate = createTerm_Helper(node.children[2+offset], HexGrammar::Ident);
    offset = offset + 1;
    // check for output
    if( node.children.size() > (2+offset) )
    {
      if( node.children[2+offset].value.id() == HexGrammar::ModOutput )
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

  LOG("storing module atom " << atom);
  ID id = ctx.registry->matoms.storeAndGetID(atom);
  LOG("stored module atom " << atom << " which got id " << id);
  return id;
}

ID HexGrammarPTToASTConverter::createAggregateFromAggregate(node_t& node)
{
  assert(false);
  #if 0
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
  #endif
}

ID HexGrammarPTToASTConverter::createAggregateFromAggregatePred(
    node_t& node)
{
  assert(false);
  #if 0
  assert(node.value.id() == HexGrammar::AggregatePred);
  AggregateAtomPtr agg(new AggregateAtom(
        createStringFromNode(node.children[0]),
        createTupleFromTerms(node.children[2]),
        createRuleBodyFromBody(node.children[4])));
  return agg;
  #endif
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

ID HexGrammarPTToASTConverter::createTermFromIdentVar(node_t& node)
{
  return createTerm_Helper(node, HexGrammar::IdentVar);
}

ID HexGrammarPTToASTConverter::createTermFromIdentVarNumber(node_t& node)
{
  return createTerm_Helper(node, HexGrammar::IdentVarNumber);
}

ID HexGrammarPTToASTConverter::createTermFromTerm(node_t& node)
{
  return createTerm_Helper(node, HexGrammar::Term);
}

DLVHEX_NAMESPACE_END

// vim: set expandtab:

// Local Variables:
// mode: C++
// End:
