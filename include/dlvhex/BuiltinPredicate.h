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
 * @file BuiltinPredicate.h
 * @author Thomas Krennwallner
 * @date Sun Jul 13 14:03:57 CEST 2008
 *
 * @brief Atom base class.
 *
 *
 */

#if !defined(_DLVHEX_BUILTINPREDICATE_H)
#define _DLVHEX_BUILTINPREDICATE_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/BaseAtom.h"


DLVHEX_NAMESPACE_BEGIN

/**
 * @brief Builtin predicate.
 *
 * This class represents atoms for builtin-predicates of dlv. For now, we just
 * pass the string on to the ASP solver and do not process it in any other way.
 *
 * The terms of a builtin are stored as atom arguments, the operator as
 * predicate.
 */
class DLVHEX_EXPORT BuiltinPredicate : public BaseAtom
{
 protected:

  Term builtin;

  Tuple args;

  

  /**
   * Default constructor is protected, we do not want to create void atoms.
   */
  BuiltinPredicate();

  /**
   * @return 0 if atom2 is equal to *this, -1 if atom2 is smaller, and
   * 1 if atom2 is greater than *this
   */
  int
  compare(const BaseAtom& atom2) const;

 public:
  BuiltinPredicate(const Term& l, const Term& b, const Term& r);

  /**
   * @return the predicate of the atom.
   */
  const Term&
  getPredicate() const;
	
  /**
   * @brief 
   */
  void
  setPredicate(const Term&);

  /**
   * @return
   */
  const Tuple&
  getArguments() const;

  /**
   * @brief
   */
  void
  setArguments(const Tuple& nargs);

  /**
   * @return
   */
  const Term&
  operator[] (unsigned i) const;

  Term&
  operator[] (unsigned i);
	
  /**
   * @return 
   */
  unsigned
  getArity() const;
	
  /**
   * @return
   */
  bool
  unifiesWith(const BaseAtom&) const;

  /**
   * @return
   */
  bool
  isGround() const;


  /**
   * @brief accepts a visitor.
   */
  void
  accept(BaseVisitor* const);

};


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_BUILTINPREDICATE_H */

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
