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
 * @file WeakConstraint.cpp
 * @author Roman Schindlauer
 * @author Thomas Krennwallner
 * @date Thu Jun 30 12:39:40 2005
 *
 * @brief WeakConstraint class.
 *
 *
 *
 */


#include "dlvhex/WeakConstraint.h"
#include "dlvhex/Atom.h"
#include "dlvhex/Error.h"

#include <iostream>
#include <sstream>
#include <set>

#include <boost/iterator/indirect_iterator.hpp>

DLVHEX_NAMESPACE_BEGIN



WeakConstraint::WeakConstraint(const BodyPtr& b, const Term& w, const Term& l)
  : weakbody(b),
    weakhead(), // weak head should be always and forever the NULL pointer
    weight(w),
    level(l)
{
  if (l.getInt() < 1)
    {
      throw SyntaxError("level must be > 0");
    }
}


const BodyPtr&
WeakConstraint::body() const
{
  return weakbody;
}


BodyPtr&
WeakConstraint::body()
{
  return weakbody;
}


HeadPtr&
WeakConstraint::head()
{
  return weakhead; // this could be dangerous
}


const HeadPtr&
WeakConstraint::head() const
{
  return weakhead;
}


void
WeakConstraint::setBody(const BodyPtr& b)
{
  this->weakbody = b;
}


void
WeakConstraint::setHead(const HeadPtr&)
{
  // there is nothing for you in here
  // ignore the request, the default head is the NULL pointer
}


int
WeakConstraint::compare(const BaseRule& rule2) const
{
  const std::type_info& type1 = typeid(*this);
  const std::type_info& type2 = typeid(rule2);

  if (type1 == type2)
    {
      const WeakConstraint& wc2 = static_cast<const WeakConstraint&>(rule2);

      int w = weight.getInt() - wc2.weight.getInt();

      if (w) return w;

      int l = level.getInt() - wc2.level.getInt();	  

      if (l) return l;

      return lexicographical_compare_3way(boost::make_indirect_iterator(body()->begin()),
					  boost::make_indirect_iterator(body()->end()),
					  boost::make_indirect_iterator(rule2.body()->begin()),
					  boost::make_indirect_iterator(rule2.body()->end()));
    }

  return type1.before(type2) ? -1 : 1;
}


void
WeakConstraint::accept(BaseVisitor* const v)
{
  v->visit(this);
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
