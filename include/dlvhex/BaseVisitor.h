/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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


#ifndef _BASEVISITOR_H
#define _BASEVISITOR_H

//
// forward declarations
//
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
class BaseVisitor
{
public:
  virtual
  ~BaseVisitor()
  { }

  // a set of atoms

  virtual void
  visitAtomSet(const AtomSet*) = 0;

  // different types of rules

  virtual void
  visitRule(const Rule*) = 0;

  virtual void
  visitWeakConstraint(const WeakConstraint*) = 0;

  // a literal

  virtual void
  visitLiteral(const Literal*) = 0;

  // different types of atoms

  virtual void
  visitAtom(const Atom*) = 0;

  virtual void
  visitExternalAtom(const ExternalAtom*) = 0;

  virtual void
  visitBuiltinPredicate(const BuiltinPredicate*) = 0;

  virtual void
  visitAggregateAtom(const AggregateAtom*) = 0;

};


#endif /* _BASEVISITOR_H */


// Local Variables:
// mode: C++
// End:
