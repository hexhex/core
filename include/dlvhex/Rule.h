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
 * @file Rule.h
 * @author Roman Schindlauer
 * @author Thomas Krennwallner
 * @date Thu Jun 30 12:39:40 2005
 *
 * @brief Rule class.
 *
 */


#if !defined(_DLVHEX_RULE_H)
#define _DLVHEX_RULE_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/BaseRule.h"


DLVHEX_NAMESPACE_BEGIN


/**
 * @brief Class for representing a rule object.
 */
class DLVHEX_EXPORT Rule : public BaseRule
{
 protected:

  /**
   * Rule head.
   */
  HeadPtr rulehead;

  /**
   * Rule body.
   */
  BodyPtr rulebody;
  
 public:

  /**
   * @brief Constructs a rule from a head and a body.
   *
   * Third argument is the file name and fourth the line number this rule
   * appeared in. Both can be ommitted.
   */
  Rule(const HeadPtr&, const BodyPtr&);

  /**
   * Destructor.
   */
  virtual
  ~Rule();

  /**
   * @return the rule head.
   */
  const HeadPtr&
  head() const;

  /**
   * @return the rule head.
   */
  HeadPtr&
  head();


  /**
   * @return the rule body.
   */
  const BodyPtr&
  body() const;

  /**
   * @return the rule body.
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
   * visitor with the Rule itself as parameter.
   *
   * \sa http://en.wikipedia.org/wiki/Visitor_pattern
   */
  void
  accept(BaseVisitor* const);

  int
  compare(const BaseRule&) const;



};


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_RULE_H */

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
