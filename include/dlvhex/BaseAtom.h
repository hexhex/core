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
 * @file BaseAtom.h
 * @author Thomas Krennwallner
 * @date Sun Jul 13 14:03:57 CEST 2008
 *
 * @brief Atom base class.
 *
 *
 */

#if !defined(_DLVHEX_BASEATOM_H)
#define _DLVHEX_BASEATOM_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/Term.h"

#include <boost/shared_ptr.hpp>


DLVHEX_NAMESPACE_BEGIN

// forward declaration
class BaseAtom;

/**
 * Atom Pointer.
 *
 * \ingroup dlvhextypes
 *
 * We use the shared pointer type from the boost-library instead of Atom* to
 * handle pointers to atoms. These shared pointers maintain a reference count
 * and automatically delete the object when the last reference disappears. This
 * is convenient in our framework, since The same atom object might be
 * referenced in many different modules.
 *
 * Instead of creating an Atom in the classical way,
 *
 * \code
 * Atom* a = new Atom("foo");
 * \endcode
 *
 * we use this:
 *
 * \code
 * AtomPtr a(new Atom("foo"));
 * \endcode
 *
 * or this:
 *
 * \code
 * AtomPtr a;
 * ...
 * a = AtomPtr(new Atom("foo"));
 * \endcode
 */
typedef boost::shared_ptr<BaseAtom> AtomPtr;



/**
 * @brief The base class for all atom types.
 *
 * \ingroup dlvhextypes
 *
 */
class DLVHEX_EXPORT BaseAtom : public ProgramNode
{
 public:

  /**
   * Destructor.
   */
  virtual
  ~BaseAtom()
  { }

  /**
   * @return the predicate of the atom.
   *
   * If the atom was constructed as a propositional atom, then the entire atom
   * is returned.
   */
  virtual const Term&
  getPredicate() const = 0;
	
  /**
   * @brief Sets first argument (corresponding to the predicate) of an atom.
   */
  virtual void
  setPredicate(const Term& term2) = 0;

  /**
   * @return the arguments of an atom.
   *
   * @todo maybe return a std::pair<Tuple::iterator,Tuple::iterator>?
   * If the atom is propositional, an empty Tuple is returned.
   */
  virtual const Tuple&
  getArguments() const = 0;

  /**
   * @brief sets arguments of an atom.
   */
  virtual void
  setArguments(const Tuple& nargs) = 0;

  /**
   * @return the specified argument term.
   *
   * The arguments of an n-ary atom are numbered from 1 to n. An index of 0
   * returns the predicate symbol of the atom.
   */
  virtual const Term&
  operator[] (unsigned i) const = 0;

  virtual Term&
  operator[] (unsigned i) = 0;
	
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
  getArity() const = 0;
	
  /**
   * @brief Tests for unification with another atom.
   *
   * Two atoms unify if they have the same arity and all of their arguments
   * (including the predicate symbols) unify pairwise.
   *
   * @see Term::unifiesWith()
   */
  virtual bool
  unifiesWith(const BaseAtom&) const = 0;

 
  /**
   * @return true if the atom is ground, false otw.
   */
  virtual bool
  isGround() const = 0;


  /**
   * @return 0 if atom2 is equal to *this, -1 if atom2 is smaller, and
   * 1 if atom2 is greater than *this
   */
  virtual int
  compare(const BaseAtom& atom2) const = 0;

  /**
   * @brief Tests for equality.
   *
   * Two Atoms are equal, if they have the same arity and list of arguments
   * (including the predicate). Two variable arguments are equal in this context, if
   * their strings are equal.
   * Two atoms of different class (e.g., ExternalAtom and Atom) are always inequal.
   */
  bool
  operator== (const BaseAtom& atom2) const
  {
    return compare(atom2) == 0;
  }

  /**
   * Tests for inequality.
   *
   * Negation of Atom::operator==.
   */
  bool
  operator!= (const BaseAtom& atom2) const
  {
    return compare(atom2) != 0;
  }

  /**
   * Comparison operator.
   *
   * A comparison operator is needed for collecting Atom-objects in a
   * std::set. First, the predicates are compared via Term::compare(). If the
   * result of the term-comparison is negative, 1 is returned, 0 otherwise.
   * are equal, the arities are compared: smaller arity yields a "smaller"
   * atom (Having different arities with the same predicate can happen for
   * atoms with variable predicates!). If arities are equal as well, the
   * atoms' arguments are compared, from left to right, via Term::compare().
   */
  bool
  operator< (const BaseAtom& atom2) const
  {
    return compare(atom2) < 0;
  }
  
};


/**
 * This operator should be used only for debugging and verbosity purposes.
 *
 * It uses the first-order notation. Proper serialization of an Atom happens
 * through the PrintVisitor class.
 */
std::ostream&
operator<< (std::ostream& out, const BaseAtom&);



DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_BASEATOM_H */

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
