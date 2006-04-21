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


Literal::Literal()
{ }


Literal::~Literal()
{
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


std::ostream&
Literal::print(std::ostream& stream, const bool ho) const
{
    if (isNAF())
        stream << "not ";
        
    getAtom()->print(stream, ho);
    
    return stream;
}

