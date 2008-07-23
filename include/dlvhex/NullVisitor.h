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
 * @file   NullVisitor.h
 * @author Thomas Krennwallner
 * @date   Mon Jul 21 12:48:44 2008
 * 
 * @brief  A no-op visitors.
 * 
 * 
 */


#if !defined(_DLVHEX_NULLVISITOR_H)
#define _DLVHEX_NULLVISITOR_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/BaseVisitor.h"

DLVHEX_NAMESPACE_BEGIN


/**
 * @brief does nothing.
 */
class DLVHEX_EXPORT NullVisitor : public BaseVisitor
{
public:
  // visit different types of queries

  virtual void
  visit(Query<Brave>* const)
  { }

  virtual void
  visit(Query<Cautious>* const)
  { }

  // visit different types of rules

  virtual void
  visit(Rule* const)
  { }

  virtual void
  visit(WeakConstraint* const)
  { }

  virtual void
  visit(Constraint* const)
  { }

  // visit positive and negative literals

  virtual void
  visit(Literal<Positive>* const)
  { }

  virtual void
  visit(Literal<Negative>* const)
  { }

  // visit different types of atoms

  virtual void
  visit(Atom<Positive>* const)
  { }

  virtual void
  visit(Atom<Negative>* const)
  { }

  virtual void
  visit(ExternalAtom* const)
  { }

  virtual void
  visit(BuiltinPredicate* const)
  { }

  virtual void
  visit(AggregateAtom* const)
  { }

};

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_NULLVISITOR_H */


// Local Variables:
// mode: C++
// End:
