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
 * @file   PrintVisitor.h
 * @author Thomas Krennwallner
 * @date   Mon Oct 23 18:16:28 2006
 * 
 * @brief Provides various pretty-printers using the visitor pattern.
 * 
 * 
 */


#if !defined(_DLVHEX_PRINTVISITOR_H)
#define _DLVHEX_PRINTVISITOR_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/BaseVisitor.h"

#include <iosfwd>

DLVHEX_NAMESPACE_BEGIN

/**
 * @brief Base print visitor.
 *
 * Implements the common printing methods. For instance, if a Rule
 * object calls visitRule(this), visitRule will iterate through the
 * components of the Rule and outputs it to #stream.
 */
class DLVHEX_EXPORT PrintVisitor : public BaseVisitor
{
protected:
  std::ostream& stream;

public:

  explicit
  PrintVisitor(std::ostream&);

  /**
   * returns the stream of the visitor.
   */
  std::ostream&
  getStream();

  /// iterate over all rules and calls Rule::accept with this visitor
  virtual void
  visit(Program* const);

  /// outputs the Rule in
  /// 'a_1 v ... v a_k :- b_1, ..., b_m, not b_{m+1}, ... not b_n.'
  /// form, i.e., it calls accept(*this) on each a_i and b_j.
  virtual void
  visit(Rule* const);

  /// outputs the AtomSet in '{ a_1, ..., a_n }' form
  virtual void
  visit(AtomSet* const);

  /// outputs the Literal either as Atom 'a' or as 'not a'
  virtual void
  visit(Literal* const);

  /// outputs the Atom in 'p(t_1,...,t_n)' form
  virtual void
  visit(Atom* const);

  /// outputs the BuiltinPredicate in 't_1 COMP t_2' form
  virtual void
  visit(BuiltinPredicate* const);

  /// outputs the AggregateAtom in
  /// 't_l COMP { t_1,...,t_n : b_1,...,b_m } COMP t_r'
  /// form
  virtual void
  visit(AggregateAtom* const);

};


/**
 * @brief Prints all elements of a Program in its "raw"
 * representation, i.e., as HEX program
 */
class DLVHEX_EXPORT RawPrintVisitor : public PrintVisitor
{
public:

  explicit
  RawPrintVisitor(std::ostream&);

  /// calls PrintVisitor::visitRule and appends a newline
  virtual void
  visit(Rule* const);

  /// outputs a WeakConstraint in
  /// ':~ b_1, ..., b_m, not b_{m+1}, not b_n. [w:l]'
  /// form and appends a newline
  virtual void
  visit(WeakConstraint* const);

  /// outputs an ExternalAtom in
  /// '&f[i_1,...,i_n](o_1,...,o_m)'
  /// form
  virtual void
  visit(ExternalAtom* const);

};


/**
 * @brief Prints all elements of a Program suitable for sending to
 * DLV.
 */
class DLVHEX_EXPORT DLVPrintVisitor : public PrintVisitor
{
public:

  explicit
  DLVPrintVisitor(std::ostream&);

  /// outputs an AtomSet in
  /// 'a_1. \n a_2. \n ... \n a_n.\n'
  /// form, i.e. this method is suitable for printing an EDB.
  virtual void
  visit(AtomSet* const);

  /// calls PrintVisitor::visitRule and appends a newline
  virtual void
  visit(Rule* const);

  /// calls DLVPrintVisitor::visitRule, i.e. it will output a
  /// constraint of form ':- b_1, ..., b_m, not b_{m+1}, not b_n.\n'
  virtual void
  visit(WeakConstraint* const);

  /// outputs an ExternalAtom in its replacement form, i.e. as Atom
  /// 'repl(i_1,...,i_n,o_1,...,o_m)'
  virtual void
  visit(ExternalAtom* const);

};


/**
 * @brief Prints all atoms in HO-mode.
 */
class DLVHEX_EXPORT HOPrintVisitor : public DLVPrintVisitor
{
public:

  explicit
  HOPrintVisitor(std::ostream&);

  /// outputs an Atom in higher order mode, i.e. in
  /// 'a_{n+1}(p,t_1,...t_n)' form except for pure first order atoms.
  virtual void
  visit(Atom* const);

};

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_PRINTVISITOR_H */


// Local Variables:
// mode: C++
// End:
