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
 * @file   PrintVisitor.h
 * @author Thomas Krennwallner
 * @date   Mon Oct 23 18:16:28 2006
 * 
 * @brief Provides various pretty-printers using the visitor pattern.
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
 * Implements the common printing methods. For instance, if a Rule
 * object calls visitRule(this), visitRule will iterate through the
 * components of the Rule and outputs it to #stream.
 */
class PrintVisitor : public BaseVisitor
{
protected:
  std::ostream& stream;

public:
  PrintVisitor(std::ostream&);

  /**
   * returns the stream of the visitor.
   */
  std::ostream&
  getStream();

  /// outputs the Rule in
  /// 'a_1 v ... v a_k :- b_1, ..., b_m, not b_{m+1}, ... not b_n.'
  /// form, i.e. it calls accept(*this) on each a_i and b_j.
  virtual void
  visitRule(const Rule*);

  /// outputs the AtomSet in '{ a_1, ..., a_n }' form
  virtual void
  visitAtomSet(const AtomSet*);

  /// outputs the Literal either as Atom 'a' or as 'not a'
  virtual void
  visitLiteral(const Literal*);

  /// outputs the Atom in 'p(t_1,...,t_n)' form
  virtual void
  visitAtom(const Atom*);

  /// outputs the BuiltinPredicate in 't_1 COMP t_2' form
  virtual void
  visitBuiltinPredicate(const BuiltinPredicate*);

  /// outputs the AggregateAtom in
  /// 't_l COMP { t_1,...,t_n : b_1,...,b_m } COMP t_r'
  /// form
  virtual void
  visitAggregateAtom(const AggregateAtom*);

};


/**
 * @brief Prints all elements of a Program in its "raw"
 * representation, i.e. as HEX program
 */
class RawPrintVisitor : public PrintVisitor
{
public:
  RawPrintVisitor(std::ostream&);

  /// outputs a WeakConstraint in
  /// ':~ b_1, ..., b_m, not b_{m+1}, not b_n. [w:l]'
  /// form
  virtual void
  visitWeakConstraint(const WeakConstraint*);

  /// outputs an ExternalAtom in
  /// '&f[i_1,...,i_n](o_1,...,o_m)'
  /// form
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

  /// outputs an AtomSet in
  /// 'a_1. \n a_2. \n ... \n a_n.\n'
  /// form, i.e. this method is suitable for printing an EDB.
  virtual void
  visitAtomSet(const AtomSet*);

  /// calls PrintVisitor::visitRule and appends a newline
  virtual void
  visitRule(const Rule*);

  /// calls DLVPrintVisitor::visitRule, i.e. it will output a
  /// constraint of form ':- b_1, ..., b_m, not b_{m+1}, not b_n.\n'
  virtual void
  visitWeakConstraint(const WeakConstraint*);

  /// outputs an ExternalAtom in its replacement form, i.e. as Atom
  /// 'repl(i_1,...,i_n,o_1,...,o_m)'
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

  /// outputs an Atom in higher order mode, i.e. in
  /// 'a_i(p,t_1,...t_n)' form except for pure first order atoms.
  virtual void
  visitAtom(const Atom*);

};


#endif /* _PRINTVISITOR_H */


// Local Variables:
// mode: C++
// End:
