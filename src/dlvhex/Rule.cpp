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
 * @file Rule.cpp
 * @author Roman Schindlauer
 * @author Thomas Krennwallner
 * @date Thu Jun 30 12:39:40 2005
 *
 * @brief Rule class.
 *
 *
 *
 */


#include "dlvhex/Rule.h"
#include "dlvhex/ExternalAtom.h"
#include "dlvhex/Registry.h"
#include "dlvhex/PrintVisitor.h"
#include "dlvhex/Error.h"

#include <iostream>
#include <sstream>

#include <boost/iterator/indirect_iterator.hpp>

DLVHEX_NAMESPACE_BEGIN

Rule::Rule(const HeadPtr& h, const BodyPtr& b)
  : rulehead(h),
    rulebody(b)
{ }


Rule::~Rule()
{ }


const HeadPtr&
Rule::head() const
{
  return this->rulehead;
}


HeadPtr&
Rule::head()
{
  return this->rulehead;
}


const BodyPtr&
Rule::body() const
{
  return this->rulebody;
}


BodyPtr&
Rule::body()
{
  return this->rulebody;
}


void
Rule::setHead(const HeadPtr& h)
{
  this->rulehead = h;
}


void
Rule::setBody(const BodyPtr& b)
{
  this->rulebody = b;
}


int
Rule::compare(const BaseRule& rule2) const
{
  const std::type_info& type1 = typeid(*this);
  const std::type_info& type2 = typeid(rule2);

  if (type1 == type2)
    {
      int h = lexicographical_compare_3way(boost::make_indirect_iterator(this->rulehead->begin()),
					   boost::make_indirect_iterator(this->rulehead->end()),
					   boost::make_indirect_iterator(rule2.head()->begin()),
					   boost::make_indirect_iterator(rule2.head()->end()));

      // if h == 0 then compare the body, otw. return h
      return h != 0 ? h :
	lexicographical_compare_3way(boost::make_indirect_iterator(this->rulebody->begin()),
				     boost::make_indirect_iterator(this->rulebody->end()),
				     boost::make_indirect_iterator(rule2.body()->begin()),
				     boost::make_indirect_iterator(rule2.body()->end()));
    }

  return type1.before(type2) ? -1 : 1;
}


void
Rule::accept(BaseVisitor* const v)
{
  v->visit(this);
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
