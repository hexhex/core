/* -*- C++ -*- */

/**
 * @file   PrintVisitor.h
 * @author Thomas Krennwallner
 * @date   Mon Oct 23 18:16:28 2006
 * 
 * @brief  
 * 
 * 
 */


#ifndef _PRINTVISITOR_H
#define _PRINTVISITOR_H

#include "dlvhex/BaseVisitor.h"

#include <iosfwd>


/**
 * @brief Base print visitor.
 *
 * Implements common print-visiting methods.
 */
class PrintVisitor : public BaseVisitor
{
protected:
  std::ostream& stream;

public:
  PrintVisitor(std::ostream&);

  virtual void
  visitRule(const Rule*);

  virtual void
  visitAtomSet(const AtomSet*);

  /**
   * @brief Serializes the literal.
   */
  virtual void
  visitLiteral(const Literal*);

  virtual void
  visitAtom(const Atom*);

  virtual void
  visitBuiltinPredicate(const BuiltinPredicate*);

  /**
   * @brief Prints the aggregate.
   * The aggregate is printed in its normal syntax to be understood by
   * dlv.
   */
  virtual void
  visitAggregateAtom(const AggregateAtom*);

};


/**
 * @brief Prints all elements of a Program as-is.
 */
class RawPrintVisitor : public PrintVisitor
{
public:
  RawPrintVisitor(std::ostream&);

  virtual void
  visitWeakConstraint(const WeakConstraint*);

  virtual void
  visitExternalAtom(const ExternalAtom*);

};


/**
 * @brief Prints all elements of a Program suitable for sending to
 * DLV.
 */
class DLVPrintVisitor : public PrintVisitor
{
public:
  DLVPrintVisitor(std::ostream&);

  virtual void
  visitAtomSet(const AtomSet*);

  virtual void
  visitRule(const Rule*);

  virtual void
  visitWeakConstraint(const WeakConstraint*);

  /**
   * @brief Serialize the external atom.
   */
  virtual void
  visitExternalAtom(const ExternalAtom*);

};


/**
 * @brief Prints all atoms in HO-mode.
 */
class HOPrintVisitor : public DLVPrintVisitor
{
public:
  HOPrintVisitor(std::ostream&);

  virtual void
  visitAtom(const Atom*);

};


#endif /* _PRINTVISITOR_H */
