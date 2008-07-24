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
 * @file   Literal.h
 * @author Roman Schindlauer
 * @author Thomas Krennwallner
 * @date   Sun Sep 4 12:39:40 2005
 * 
 * @brief  Literal class.
 * 
 * 
 */

#if !defined(_DLVHEX_LITERAL_H)
#define _DLVHEX_LITERAL_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/BaseLiteral.h"
#include "dlvhex/NegationTraits.h"

DLVHEX_NAMESPACE_BEGIN

/**
 * @brief Literal class.
 *
 * A literal is the constituting part of a rule body. It can contain an
 * atom or a weakly negated Atom. The atom of a literal can both be an
 * ordinary as well as an external atom.
 */
template<class T>
class DLVHEX_EXPORT Literal : public BaseLiteral
{
 protected:

  /**
   * Atom of the Literal.
   */
  AtomPtr atom;

  /**
   * No default constructed literals.
   */
  Literal();

 public:

  /**
   * @brief Construct a literal containing the specified atom,
   * possibly weakly negated.
   *
   * \sa AtomPtr
   */
  explicit
  Literal(const AtomPtr& at);

  explicit
  Literal(const Literal<T>&);


  /**
   * @brief Assignment operator.
   */
  Literal<T>&
  operator=(const Literal<T>& lit2);

  /**
   * @brief Generalised assignment operator.
   */
  template<typename U>
  Literal<T>&
  operator=(const Literal<U>& lit2);

  /**
   * @return a pointer to the atom of the literal.
   *
   * \sa AtomPtr
   */
  const AtomPtr&
  getAtom() const;

  /**
   * @return a pointer to the atom of the literal.
   *
   * \sa AtomPtr
   */
  AtomPtr&
  getAtom();


  bool
  unifiesWith(const BaseLiteral&) const;


  int
  compare(const BaseLiteral&) const;


  /**
   * @brief Accepts a visitor.
   *
   * According to the visitor pattern, accept simply calls the respective
   * visitor with the Literal itself as parameter.
   *
   * \sa http://en.wikipedia.org/wiki/Visitor_pattern
   */
  void
  accept(BaseVisitor* const v);


};


DLVHEX_NAMESPACE_END

#include "dlvhex/Literal.tcc"

#endif /* _DLVHEX_LITERAL_H */

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
