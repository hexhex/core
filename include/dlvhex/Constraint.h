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
 * @file Constraint.h
 * @author Roman Schindlauer
 * @author Thomas Krennwallner
 * @date Thu Jun 30 12:39:40 2005
 *
 * @brief Constraint class.
 *
 */


#if !defined(_DLVHEX_CONSTRAINT_H)
#define _DLVHEX_CONSTRAINT_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/BaseRule.h"

DLVHEX_NAMESPACE_BEGIN


/**
 * @brief A weak constraint is a rule with empty head and weight/level values.
 */
class DLVHEX_EXPORT Constraint : public BaseRule
{
 protected:

  // constraint body
  BodyPtr constraintbody;

  int
  compare(const BaseRule&) const;

  
 public:

  /**
   * @brief See constructor of Rule.
   */
  Constraint(const BodyPtr&);

  virtual
  ~Constraint();

  /**
   * @brief Returns the rule's head.
   */
  const HeadPtr&
  head() const;

  /**
   * @brief Returns the rule's head.
   */
  HeadPtr&
  head();


  /**
   * @brief Returns the rule's body.
   */
  const BodyPtr&
  body() const;

  /**
   * @brief Returns the rule's body.
   */
  BodyPtr&
  body();

  /**
   * @brief Replaces the rule's head by the specified one.
   */
  void
  setHead(const HeadPtr&);

  /**
   * @brief Replaces the rule's body by the specified one.
   */
  void
  setBody(const BodyPtr&);


  /**
   * @brief accepts a visitor.
   *
   * According to the visitor pattern, accept simply calls the respective
   * visitor with the weak constraint itself as parameter.
   *
   * \sa http://en.wikipedia.org/wiki/Visitor_pattern
   */
  void
  accept(BaseVisitor* const);

};

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_WEAKCONSTRAINT_H */

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
