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
 * @file   Rule.hpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Rule: store rules (not facts!), constraints, weak constraints
 */

#ifndef ORDINARYATOM_HPP_INCLUDED__12102010
#define ORDINARYATOM_HPP_INCLUDED__12102010

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/Logger.hpp"
#include "dlvhex/ID.hpp"

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

  // additional information for weak constraints (ID_FAIL if unused)
  ID weight;
  ID level;

  Rule(IDKind kind):
    kind(kind), head(), body(), weight(ID_FAIL), level(ID_FAIL)
      { assert(ID(kind,0).isRule()); }
  Rule(IDKind kind, const Tuple& head, const Tuple& body):
    kind(kind), head(head), body(body), weight(ID_FAIL), level(ID_FAIL)
      { assert(ID(kind,0).isRule()); }
  Rule(IDKind kind, const Tuple& head, const Tuple& body, ID weight, ID level):
    kind(kind), head(head), body(body), weight(weight), level(level)
      { assert(ID(kind,0).isRule()); }
  Rule(IDKind kind, ID weight, ID level):
    kind(kind), head(), body(), weight(weight), level(level)
      { assert(ID(kind,0).isRule()); }
  std::ostream& print(std::ostream& o) const
    { o << "Rule(" << printvector(head) << " <- " << printvector(body);
      if( weight != ID_FAIL || level != ID_FAIL )
        o << " [" << weight << ":" << level << "]";
      return o << ")"; }
};

DLVHEX_NAMESPACE_END

#endif // RULE_HPP_INCLUDED__12102010
