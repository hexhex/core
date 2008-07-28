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
 * @file   BaseLiteral.h
 * @author Roman Schindlauer
 * @author Thomas Krennwallner
 * @date   Sun Sep 4 12:39:40 2005
 * 
 * @brief  Base Literal class.
 * 
 * 
 */

#if !defined(_DLVHEX_BASELITERAL_H)
#define _DLVHEX_BASELITERAL_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/ProgramNode.h"
#include "dlvhex/BaseAtom.h"

DLVHEX_NAMESPACE_BEGIN

/**
 * @brief Base Literal class.
 *
 * A literal is the constituting part of a rule body. The type of the
 * literal determines the negation type.
 */
class DLVHEX_EXPORT BaseLiteral : public ProgramNode
{
 public:

  /**
   * Destructor.
   */
  virtual
  ~BaseLiteral();


  /**
   * @return a pointer to the atom of the literal.
   *
   * \sa AtomPtr
   */
  virtual const AtomPtr&
  getAtom() const = 0;

  /**
   * @return a pointer to the atom of the literal.
   *
   * \sa AtomPtr
   */
  virtual AtomPtr&
  getAtom() = 0;


  virtual bool
  unifiesWith(const BaseLiteral&) const = 0;

  virtual int
  compare(const BaseLiteral&) const = 0;


  /**
   * @brief Test for equality.
   *
   * Two Literals are equal, if they contain the same atom and neither
   * or both are weakly negated.
   */
  inline bool
  operator== (const BaseLiteral& lit2) const
  {
    return compare(lit2) == 0;
  }


  /**
   * Inequality test.
   *
   * \sa Literal::operator==
   */
  inline bool
  operator!= (const BaseLiteral& lit2) const
  {
    return compare(lit2) != 0;
  }


  /**
   * Less-than operator.
   *
   * A Literal is "smaller" than another, if the first is not weakly negated
   * but the second is. If none or both are weakly negated, then their atoms
   * are compared.
   *
   * \sa Atom::operator<
   */
  inline bool
  operator< (const BaseLiteral& lit2) const
  {
    return compare(lit2) < 0;
  }
  

  /**
   * @brief Accepts a visitor.
   *
   * According to the visitor pattern, accept simply calls the respective
   * visitor with the Literal itself as parameter.
   *
   * \sa http://en.wikipedia.org/wiki/Visitor_pattern
   */
  virtual void
  accept(BaseVisitor* const) = 0;


};



/**
 * Direct serialization of a Literal using a RawPrintVisitor.
 *
 * Should be used for debugging or verbose only.
 */
std::ostream&
operator<<(std::ostream&, const BaseLiteral&);


/**
 * A shared pointer to a base literal
 */
typedef boost::shared_ptr<BaseLiteral> LiteralPtr;


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_BASELITERAL_H */

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
