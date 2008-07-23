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
 * @file Constraint.cpp
 * @author Roman Schindlauer
 * @author Thomas Krennwallner
 * @date Thu Jun 30 12:39:40 2005
 *
 * @brief Constraint class.
 *
 *
 *
 */


#include "dlvhex/Constraint.h"

#include <iostream>
#include <sstream>
#include <typeinfo>


DLVHEX_NAMESPACE_BEGIN



Constraint::Constraint(const BodyPtr& b)
  : constraintbody(b)
{ }


const HeadPtr&
Constraint::head() const
{
  ///@todo hm, what should we put here?
}

HeadPtr&
Constraint::head()
{ 
  ///@todo hm, what should we put here?
}


const BodyPtr&
Constraint::body() const
{
  return this->constraintbody;
}

BodyPtr&
Constraint::body()
{
  return this->constraintbody;
}



void
Constraint::setHead(const HeadPtr&)
{
  // there is nothing for you in here
}


void
Constraint::setBody(const BodyPtr& b)
{
  this->constraintbody = b;
}


int
Constraint::compare(const BaseRule& rule2) const
{
  const std::type_info& type1 = typeid(*this);
  const std::type_info& type2 = typeid(rule2);

  if (type1 == type2)
    {
      unsigned s1 = constraintbody->size();
      unsigned s2 = rule2.body()->size();

      if (s1 == s2)
	{
	  std::pair<Body::const_iterator, Body::const_iterator> res =
	    std::mismatch(constraintbody->begin(), constraintbody->end(),
			  rule2.body()->begin());

	  // did we find a mismatch?
	  return
	    res.first == constraintbody->end() ? 0 :
	    (*res.first)->compare(**res.second);
	}

      return s1 - s2;
    }

  return type1.before(type2) ? -1 : 1;
}


void
Constraint::accept(BaseVisitor* const v)
{
  v->visit(this);
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
