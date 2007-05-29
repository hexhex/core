/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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
#include "dlvhex/globals.h"
#include "dlvhex/BaseVisitor.h"

AggregateAtom::AggregateAtom(const std::string& aggtype,
                             const Tuple& vars,
                             const RuleBody_t& conj)
    : body(conj),
      aggVars(vars),
      type(aggtype),
      cmpLeft(""),
      cmpRight("")
{
    //
    // in higher-rder mode we cannot have aggregates, because then they would
    // almost certainly be recursive, because of our atom-rewriting!
    //
    if (Globals::Instance()->getOption("NoPredicate"))
        throw SyntaxError("Aggregates only allowed in FO-mode (use --firstorder)");

    this->arguments.push_back(Term(""));

//    for (Tuple::const_iterator t = vars.begin(); t != vars.end(); ++t)
//        aggVars.push_back(*t);
}


void
AggregateAtom::setComp(const std::string compLeft,
                       const std::string compRight)
{
    this->cmpLeft = compLeft;
    this->cmpRight = compRight;
}


void
AggregateAtom::setLeftTerm(const Term& left)
{
    this->left = left;

    //
    // we add the comparees as arguments - this is without effect except when we
    // check for safety - then we treat them like normal arguments, such that a
    // rule like:
    //   p(W) :- #min{S : c(S)} = W.
    // is safe. If the W in the body aggregate wouldn't be treated like an
    // atom's argument, this rule would of course be unsafe.
    //
    this->arguments.push_back(left);
}


void
AggregateAtom::setRightTerm(const Term& right)
{
    this->right = right;

    //
    // see comment above
    //
    this->arguments.push_back(right);
}


bool
AggregateAtom::unifiesWith(const AtomPtr atom) const
{
    //
    // an aggregate depends on the atoms in its conjunction
    //
    for (RuleBody_t::const_iterator l = this->body.begin();
            l != this->body.end();
            ++l)
    {
        if (atom->unifiesWith((*l)->getAtom()))
            return true;
    }

    return false;
}


void
AggregateAtom::accept(BaseVisitor& v) const
{
  v.visitAggregateAtom(this);
}





// Local Variables:
// mode: C++
// End:
