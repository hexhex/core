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
 * @file Atom.h
 * @author Roman Schindlauer
 * @author Thomas Krennwallner
 * @date Mon Sep  5 16:57:17 CEST 2005
 *
 * @brief Atom base class.
 *
 *
 */

#if !defined(_DLVHEX_ATOM_H)
#define _DLVHEX_ATOM_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/BaseAtom.h"
#include "dlvhex/NegationTraits.h"
#include "dlvhex/Term.h"

#include <boost/shared_ptr.hpp>


DLVHEX_NAMESPACE_BEGIN


/**
 * @brief An Atom has a predicate and (if not propositional) an
 * optional list of arguments.
 *
 * \ingroup dlvhextypes
 *
 * An Atom corresponds to a logical atom.
 *
 */
template<typename T>
class DLVHEX_EXPORT Atom : public BaseAtom
{
 protected:
  /**
   * Arguments of the atom.
   *
   * The predicate is considered as an argument itself, being the first term
   * in the tuple.
   */
  Tuple arguments;

  /**
   * Default constructor is protected, we do not want to create void atoms.
   */
  Atom();

  int
  compare(const BaseAtom&) const;

 public:

  /**
   * Copy constructor from positive/negative atoms.
   */
  template<typename U>
  Atom(const Atom<U>&);

  /**
   * Copy constructor from base atoms.
   */
  Atom(const BaseAtom&);

  /// assigment operator for converting positive/negative to negative/positive atoms
  template<typename U>
  Atom<T>&
  operator= (const Atom<U>&);

  /**
   * Constructs an atom from a string.
   *
   * This can be:
   * - A propositional atom, like \b lightOn
   * - A first order atom, like \b p(X) or \b q(a,b,Z)
   * .
   * If the atom is propositional, it must be ground, i.e., the string must
   * not denote a single variable.
   */
  explicit
  Atom(const std::string&);

  /**
   * Constructs an atom from a predicate string and a tuple.
   * The string denotes the predicate symbol of the atom, the tuple its
   * arguments.
   * The third argument indicates if the atom is strongly negated.
   * The tuple can also be empty, then the atom is propositional and consists
   * only of the predicate identifier (which must not be a variable then).
   */
  Atom(const Term&, const Tuple&);

  /**
   * Constructs an atom from a list of arguments.
   *
   * This is another way of constructing an atom, reflecting the notion of
   * higher-order syntax, where the predicate is just a Term inside the
   * argument list.  The first element of the tuple is considered to be the
   * predicate - this is important for the usage of Atom::getPredicate() and
   * Atom::getArguments().  The second argument indicates if the atom is strongly
   * negated. The tuple must not be empty or contain a single variable term.
   */
  explicit
  Atom(const Tuple&);

  /**
   * Returns the predicate of the atom.
   *
   * If the atom was constructed as a propositional atom, then the entire atom
   * is returned.
   */
  virtual const Term&
  getPredicate() const;

		
  /**
   * @brief Sets first argument (corresponding to the predicate) of an atom.
   */
  void
  setPredicate(const Term& term2);


  /**
   * Returns the arguments of an atom.
   *
   * If the atom is propositional, an empty Tuple is returned.
   */
  virtual const Tuple&
  getArguments() const;

  /**
   * @brief sets arguments of an atom.
   */
  virtual void
  setArguments(const Tuple& nargs);

  /**
   * @brief Returns the specified argument term.
   *
   * The arguments of an n-ary atom are numbered from 1 to n. An index of 0
   * returns the predicate symbol of the atom.
   */
  const Term&
  operator[] (unsigned i) const;

  Term&
  operator[] (unsigned i);
	
  /**
   * Returns the arity of an atom (number of arguments).
   *
   * For traditional atoms this works as expected:
   * - \b p(q)  has arity 1
   * - \b a	 has arity 0
   * .
   * For atoms that were constructed from tuple-syntax, the arity is one less
   * than the original tuple's arity, since the first term of the tuple is
   * regarded as the atom's predicate:
   * - \b (X,Y) has arity 1 (seen as <b>X(Y)</b>)
   * .
   */
  virtual unsigned
  getArity() const;
	
  /**
   * @brief Tests for unification with another atom.
   *
   * Two atoms unify if they have the same arity and all of their arguments
   * (including the predicate symbols) unify pairwise.
   *
   * @see Term::unifiesWith()
   */
  virtual bool
  unifiesWith(const BaseAtom&) const;

  bool
  isGround() const;


  /**
   * @brief Accepts a visitor.
   *
   * A visitor is a common design pattern to implement context-specific
   * operations outside the class. We use visitors for serialization of
   * objects. This function calls the visitAtom() method of the specified
   * visitor, passing itself as parameter. The visitor serializes the atom
   * according to its (the visitor's) type (plain text, XML, etc.)
   */
  virtual void
  accept(BaseVisitor* const);

};


DLVHEX_NAMESPACE_END

#include "dlvhex/Atom.tcc"

#endif /* _DLVHEX_ATOM_H */

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
