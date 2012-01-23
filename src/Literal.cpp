/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
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
 * @file   Literal.cpp
 * @author Roman Schindlauer
 * @date   Sun Sep 4 12:52:05 2005
 * 
 * @brief  Literal class.
 * 
 * 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/Literal.h"
#include "dlvhex2/BaseVisitor.h"
#include "dlvhex2/PrintVisitor.h"

DLVHEX_NAMESPACE_BEGIN

Literal::Literal()
{ }


Literal::~Literal()
{
}


Literal&
Literal::operator=(const Literal& lit2)
{
    this->isWeaklyNegated = lit2.isWeaklyNegated;

    const_cast<AtomPtr&>(this->atom) = lit2.atom;

    return *this;
}

Literal::Literal(const AtomPtr at, bool naf)
    : atom(at),
      isWeaklyNegated(naf)
{
}


const AtomPtr
Literal::getAtom() const
{
    return atom;
}


bool
Literal::isNAF() const
{
    return isWeaklyNegated;
}

bool
Literal::isHigherOrder() const
{
    return atom->isHigherOrder();
}


bool
Literal::operator== (const Literal& lit2) const
{
    if (!(*atom == *(lit2.getAtom())))
        return 0;

    if (isWeaklyNegated != lit2.isNAF())
        return 0;

    return 1;
}


bool
Literal::operator!= (const Literal& lit2) const
{
    return !(*this == lit2);
}


bool
Literal::operator< (const Literal& lit2) const
{
	if (!this->isNAF() && lit2.isNAF())
		return 1;
	if (this->isNAF() && !lit2.isNAF())
		return 0;

	return (this->getAtom() < lit2.getAtom());
}


void
Literal::accept(BaseVisitor& v) const
{
  v.visit(this);
}


std::ostream&
operator<<(std::ostream& o, const Literal& l)
{
  RawPrintVisitor rpv(o);
  const_cast<Literal*>(&l)->accept(rpv);
  return o;
}


bool
operator< (const RuleBody_t& body1, const RuleBody_t& body2)
{
  return std::lexicographical_compare(body1.begin(), body1.end(),
				      body2.begin(), body2.end());
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
