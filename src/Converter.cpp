/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * Copyright (C) 2013 Andreas Humenberger
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
 * @file   Converter.cpp
 * @author Andreas Humenberger <e1026602@student.tuwien.ac.at>
 * 
 * @brief  
 */

#include "dlvhex2/Converter.h"
#include "dlvhex2/Printer.h"

DLVHEX_NAMESPACE_BEGIN

ID RuleConverter::convertRule(ID ruleid)
{
  RegistryPtr reg = ctx.registry();
  if( !ruleid.doesRuleContainExtatoms() )
  {
    DBGLOG(DBG,"not converting rule " << ruleid << " (does not contain extatoms)");
    return ruleid;
  }

  // we need to rewrite
  const Rule& rule = reg->rules.getByID(ruleid);
  #ifndef NDEBUG
  {
    std::stringstream s;
    RawPrinter printer(s, reg);
    printer.print(ruleid);
    DBGLOG(DBG,"rewriting rule " << s.str() << " from " << rule <<
        " with id " << ruleid << " to auxiliary predicates");
  }
  #endif

  // copy it
  Rule newrule(rule);
  newrule.kind |= ID::PROPERTY_AUX;
  newrule.body.clear();

  // convert (recursively in aggregates)
  convertBody(rule.body, newrule.body);

  // store as rule
  ID newruleid = reg->storeRule(newrule);
  #ifndef NDEBUG
  {
    std::stringstream s;
    RawPrinter printer(s, reg);
    printer.print(newruleid);
    DBGLOG(DBG,"rewritten rule " << s.str() << " from " << newrule <<
        " got id " << newruleid);
  }
  #endif
  return newruleid;

}

void RuleConverter::convertBody(const Tuple& body, Tuple& convbody)
{
  assert(convbody.empty());
  RegistryPtr reg = ctx.registry();
  for(Tuple::const_iterator itlit = body.begin();
      itlit != body.end(); ++itlit)
  {
    if( itlit->isAggregateAtom() )
    {
      // recursively treat aggregates
      
      // findout if aggregate contains external atoms
      const AggregateAtom& aatom = reg->aatoms.getByID(*itlit);
      AggregateAtom convaatom(aatom);
      convaatom.literals.clear();
      convertBody(aatom.literals, convaatom.literals);
      if( convaatom.literals != aatom.literals )
      {
        // really create new aggregate atom
        convaatom.kind |= ID::PROPERTY_AUX;
        ID newaatomid = reg->aatoms.storeAndGetID(convaatom);
        convbody.push_back(newaatomid);
      }
      else
      {
        // use original aggregate atom
        convbody.push_back(*itlit);
      }
    }
    else if( itlit->isExternalAtom() )
    {
      bool naf = itlit->isNaf();
      const ExternalAtom& eatom = reg->eatoms.getByID(
          ID::atomFromLiteral(*itlit));
      DBGLOG(DBG,"rewriting external atom " << eatom <<
          " literal with id " << *itlit);

      // create replacement atom
      OrdinaryAtom replacement(ID::MAINKIND_ATOM | ID::PROPERTY_AUX | ID::PROPERTY_EXTERNALAUX);
      assert(!!eatom.pluginAtom);
      replacement.tuple.push_back(
          reg->getAuxiliaryConstantSymbol('r',
            eatom.pluginAtom->getPredicateID()));
      if (ctx.config.getOption("IncludeAuxInputInAuxiliaries") && eatom.auxInputPredicate != ID_FAIL){
        replacement.tuple.push_back(eatom.auxInputPredicate);
      }
      replacement.tuple.insert(replacement.tuple.end(),
          eatom.inputs.begin(), eatom.inputs.end());
      replacement.tuple.insert(replacement.tuple.end(),
          eatom.tuple.begin(), eatom.tuple.end());

      // bit trick: replacement is ground so far, by setting one bit we make it nonground
      bool ground = true;
      BOOST_FOREACH(ID term, replacement.tuple)
      {
        if( term.isVariableTerm() )
          ground = false;
      }
      if( !ground )
        replacement.kind |= ID::SUBKIND_ATOM_ORDINARYN;

      ID idreplacement;
      if( ground )
        idreplacement = reg->storeOrdinaryGAtom(replacement);
      else
        idreplacement = reg->storeOrdinaryNAtom(replacement);
      DBGLOG(DBG,"adding replacement atom " << idreplacement << " as literal");
      convbody.push_back(ID::literalFromAtom(idreplacement, naf));
    }
    else
    {
      DBGLOG(DBG,"adding original literal " << *itlit);
      convbody.push_back(*itlit);
    }
  }
}

DLVHEX_NAMESPACE_END
