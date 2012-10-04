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
 * @file   Rule.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Rule: store rules (not facts!), constraints, weak constraints
 */

#ifndef RULE_HPP_INCLUDED__12102010
#define RULE_HPP_INCLUDED__12102010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/ID.h"

DLVHEX_NAMESPACE_BEGIN

struct Rule:
  private ostream_printable<Rule>
{
  // the kind part of the ID of this rule
  IDKind kind;

  // the IDs of ordinary atoms in the head of this rule
  Tuple head;

  // the IDs of literals in the body of this rule
  Tuple body;

  // only for lparse weight rules (not to be confused with weak constraints!)
  Tuple bodyWeightVector;
  ID bound;

  // additional information for weak constraints (ID_FAIL if unused)
  ID weight;
  ID level;

  Rule(IDKind kind):
    kind(kind), head(), body(), bound(ID_FAIL), weight(ID_FAIL), level(ID_FAIL)
      { assert(ID(kind,0).isRule()); }
  Rule(IDKind kind, const Tuple& head, const Tuple& body):
    kind(kind), head(head), body(body), bound(ID_FAIL), weight(ID_FAIL), level(ID_FAIL)
      { assert(ID(kind,0).isRule()); }
  Rule(IDKind kind, const Tuple& head, const Tuple& body, ID weight, ID level):
    kind(kind), head(head), body(body), bound(ID_FAIL), weight(weight), level(level)
      { assert(ID(kind,0).isRule()); }
  Rule(IDKind kind, ID weight, ID level):
    kind(kind), head(), body(), bound(ID_FAIL), weight(weight), level(level)
      { assert(ID(kind,0).isRule()); }
  Rule(IDKind kind, const Tuple& head, const Tuple& body, const Tuple& bodyWeightVector, ID bound):
    kind(kind), head(head), body(body), bodyWeightVector(bodyWeightVector), bound(bound), weight(ID_FAIL), level(ID_FAIL)
      { assert(ID(kind,0).isWeightRule()); assert(body.size() == bodyWeightVector.size()); }
  inline bool isEAGuessingRule() const
    { return head.size() == 2 && head[0].isExternalAuxiliary() && head[1].isExternalAuxiliary(); }
  inline bool isEAAuxInputRule() const
    { return head.size() == 1 && head[0].isExternalInputAuxiliary(); }
  std::ostream& print(std::ostream& o) const
    { o << "Rule(" << printvector(head) << " <- " << printvector(body);
      if( weight != ID_FAIL || level != ID_FAIL )
        o << " [" << weight << ":" << level << "]";
      if ( ID(kind,0).isWeightRule() )
        o << "; " << printvector(bodyWeightVector) << " >= " << bound.address;
      return o << ")"; }
};

DLVHEX_NAMESPACE_END

#endif // RULE_HPP_INCLUDED__12102010
