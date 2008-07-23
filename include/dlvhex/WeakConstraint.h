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
 * @file WeakConstraint.h
 * @author Roman Schindlauer
 * @author Thomas Krennwallner
 * @date Thu Jun 30 12:39:40 2005
 *
 * @brief WeakConstraint class.
 *
 */


#if !defined(_DLVHEX_WEAKCONSTRAINT_H)
#define _DLVHEX_WEAKCONSTRAINT_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/BaseRule.h"

DLVHEX_NAMESPACE_BEGIN


/**
 * @brief A weak constraint is a rule with empty head and weight/level values.
 */
class DLVHEX_EXPORT WeakConstraint : public BaseRule
{
 private:

  // weak constraint body
  BodyPtr weakbody;

  // weak constraint head
  HeadPtr weakhead;

  /// wc weight
  Term weight;
  /// wc level
  Term level;


  int
  compare(const BaseRule&) const;

public:

  /**
   * @brief See constructor of Rule.
   *
   * The third parameter is the weight, the fourth is the level of the weak
   * constraint.
   */
  WeakConstraint(const BodyPtr&, const Term&, const Term&);

  /**
   * @brief accepts a visitor.
   *
   * According to the visitor pattern, accept simply calls the respective
   * visitor with the weak constraint itself as parameter.
   *
   * \sa http://en.wikipedia.org/wiki/Visitor_pattern
   */
  virtual void
  accept(BaseVisitor* const);


  /**
   * Returns the weight of the WC.
   */
  const Term&
  getWeight() const
  {
    return weight;
  }


  /**
   * Returns the level of the WC.
   */
  const Term&
  getLevel() const
  {
    return level;
  }


  void
  setHead(const HeadPtr&);


  void
  setBody(const BodyPtr&);


  const HeadPtr&
  head() const;


  HeadPtr&
  head();

  const BodyPtr&
  body() const;

  BodyPtr&
  body();

};

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_WEAKCONSTRAINT_H */

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
