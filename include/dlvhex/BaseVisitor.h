/* -*- C++ -*- */

/**
 * @file   BaseVisitor.h
 * @author Thomas Krennwallner
 * @date   Mon Oct 23 18:16:28 2006
 * 
 * @brief  The Base for Visitors.
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
