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
 * @file   BaseVisitor.h
 * @author Thomas Krennwallner
 * @date   Mon Oct 23 18:16:28 2006
 * 
 * @brief  The baseclass for all Visitors.
 * 
 * 
 */


#if !defined(_DLVHEX_BASEVISITOR_H)
#define _DLVHEX_BASEVISITOR_H

#include "dlvhex/PlatformDefinitions.h"

DLVHEX_NAMESPACE_BEGIN

//
// forward declarations
//
class Program;
class AtomSet;
class Rule;
class WeakConstraint;
class Literal;
class Atom;
class ExternalAtom;
class BuiltinPredicate;
class AggregateAtom;


/**
 * @brief The baseclass for all visitors.
 *
 * When calling the accept(BaseVisitor&) method of an object, the
 * object knows its own type and calls the corresponding visiting
 * method of BaseVisitor.
 */
class DLVHEX_EXPORT BaseVisitor
{
public:
  virtual
  ~BaseVisitor()
  { }

  virtual void
  visit(const Program* const) = 0;

  // a set of atoms

  virtual void
  visit(const AtomSet* const) = 0;

  // different types of rules

  virtual void
  visit(const Rule* const) = 0;

  virtual void
  visit(const WeakConstraint* const) = 0;

  // a literal

  virtual void
  visit(const Literal* const) = 0;

  // different types of atoms

  virtual void
  visit(const Atom* const) = 0;

  virtual void
  visit(const ExternalAtom* const) = 0;

  virtual void
  visit(const BuiltinPredicate* const) = 0;

  virtual void
  visit(const AggregateAtom* const) = 0;

};

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_BASEVISITOR_H */


// Local Variables:
// mode: C++
// End:
