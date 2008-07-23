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
 * @file BaseRule.h
 * @author Roman Schindlauer
 * @author Thomas Krennwallner
 * @date Mon Jul 14 11:20:24 2008
 *
 * @brief Base Rule class.
 *
 */


#if !defined(_DLVHEX_BASERULE_H)
#define _DLVHEX_BASERULE_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/Head.h"
#include "dlvhex/Body.h"

DLVHEX_NAMESPACE_BEGIN



/**
 * @brief Class for representing a rule object.
 */
class DLVHEX_EXPORT BaseRule : public ProgramNode
{
 public:

  /**
   * Destructor.
   */
  virtual
  ~BaseRule()
  { }

  /**
   * @brief Returns the rule's head.
   */
  virtual HeadPtr&
  head() = 0;

  /**
   * @brief Returns the rule's head - const version.
   */
  virtual const HeadPtr&
  head() const = 0;

  /**
   * @brief Returns the rule's body.
   */
  virtual BodyPtr&
  body() = 0;

  /**
   * @brief Returns the rule's body - const version.
   */
  virtual const BodyPtr&
  body() const = 0;

  /**
   * @brief Replaces the rule's head by the specified one.
   */
  virtual void
  setHead(const HeadPtr&) = 0;

  /**
   * @brief Replaces the rule's body by the specified one.
   */
  virtual void
  setBody(const BodyPtr&) = 0;

  /// the comparison template method
  virtual int
  compare(const BaseRule&) const = 0;


  /**
   * @brief Test for equality.
   */
  bool
  operator== (const BaseRule& rule2) const
  {
    return compare(rule2) == 0;
  }


  /**
   * @brief Test for inequality.
   */
  bool
  operator!= (const BaseRule& rule2) const
  {
    return compare(rule2) != 0;
  }


  /**
   * @brief Less-than comparison.
   */
  bool
  operator< (const BaseRule& rule2) const
  {
    return compare(rule2) < 0;
  }

  /**
   * @brief accepts a visitor.
   *
   * According to the visitor pattern, accept simply calls the respective
   * visitor with the Rule itself as parameter.
   *
   * \sa http://en.wikipedia.org/wiki/Visitor_pattern
   */
  virtual void
  accept(BaseVisitor* const);

};


/**
 * Direct serialization of a Rule using a RawPrintVisitor.
 *
 * Should be used for debugging or verbose only.
 */
std::ostream&
operator<< (std::ostream& out, const BaseRule& rule);


/// managed Rule pointer
typedef boost::shared_ptr<BaseRule> RulePtr;

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_BASERULE_H */

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
