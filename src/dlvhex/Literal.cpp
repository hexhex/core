/* -*- C++ -*- */

/**
 * @file   Literal.cpp
 * @author Roman Schindlauer
 * @date   Sun Sep 4 12:52:05 2005
 * 
 * @brief  Literal class.
 * 
 * 
 */

#include "dlvhex/Literal.h"
#include "dlvhex/BaseVisitor.h"
#include "dlvhex/PrintVisitor.h"

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


void
Literal::accept(BaseVisitor& v) const
{
  v.visitLiteral(this);
}


std::ostream&
operator<<(std::ostream& o, const Literal& l)
{
  RawPrintVisitor rpv(o);
  l.accept(rpv);
  return o;
}
