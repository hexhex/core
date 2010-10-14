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

struct RuleAdornmentBase:
  private ostream_printable<RuleAdornmentBase>
{
  virtual RuleAdornmentBase* clone() const = 0;
  virtual std::ostream& print(std::ostream& o) const = 0;
};

struct WeakRuleAdornment:
  public RuleAdornmentBase
{
  // TODO level and weight

  virtual RuleAdornmentBase* clone() const;
  virtual std::ostream& print(std::ostream& o) const;
};

struct Rule:
  private ostream_printable<Rule>
{
  // the kind part of the ID of this rule
  IDKind kind;

  // the IDs of ordinary atoms in the head of this rule
  Tuple head;

  // the IDs of literals in the body of this rule
  Tuple body;

  // additional information (e.g., weak constraint weights, something else for the future?)
  std::auto_ptr<RuleAdornmentBase> adornment;

  Rule(IDKind kind):
    kind(kind), head(), body(), adornment() {}
  Rule(IDKind kind, const Tuple& head, const Tuple& body):
    kind(kind), head(head), body(body), adornment() {}
  std::ostream& print(std::ostream& o) const
    { o << "Rule(" << printvector(head) << " <- " << printvector(body);
      if( adornment.get() != 0 ) o << " " << (*adornment);
      return o << ")"; }
};

DLVHEX_NAMESPACE_END

#endif // RULE_HPP_INCLUDED__12102010
