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
 * @file AggregateAtom.cpp
 * @author Roman Schindlauer
 * @date Wed Oct 18 16:04:54 CEST 2006
 *
 * @brief Aggregate Atom class.
 *
 *
 */


#include "dlvhex/AggregateAtom.h"
#include "dlvhex/BaseVisitor.h"
#include "dlvhex/Error.h"

#include <boost/mem_fn.hpp>

DLVHEX_NAMESPACE_BEGIN

AggregateAtom::AggregateAtom(const Term& aggtype,
                             const Tuple& vars,
                             const BodyPtr& conj)
    : body(conj),
      aggVars(vars),
      type(aggtype),
      cmpLeft(""),
      cmpRight("")
{ }


void
AggregateAtom::setComp(const std::string& compLeft,
                       const std::string& compRight)
{
    this->cmpLeft = compLeft;
    this->cmpRight = compRight;
}


void
AggregateAtom::setLeftTerm(const Term& left)
{
    this->left = left;

    ///@todo fix safety
    //
    // we add the comparees as arguments - this is without effect except when we
    // check for safety - then we treat them like normal arguments, such that a
    // rule like:
    //   p(W) :- #min{S : c(S)} = W.
    // is safe. If the W in the body aggregate wouldn't be treated like an
    // atom's argument, this rule would of course be unsafe.
    //
}


void
AggregateAtom::setRightTerm(const Term& right)
{
    this->right = right;
}


bool
AggregateAtom::unifiesWith(const BaseAtom&) const
{
  ///@todo not implemented properly
  return false;
  //
  // an aggregate depends on the atoms in its conjunction
  //
}


void
AggregateAtom::accept(BaseVisitor* const v)
{
  v->visit(this);
}


DLVHEX_NAMESPACE_END


// Local Variables:
// mode: C++
// End:
