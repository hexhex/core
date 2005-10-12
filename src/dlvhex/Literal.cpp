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
    delete atom;
}


Literal::Literal(const Literal &literal2)
    : isWeaklyNegated(literal2.isWeaklyNegated)
{
    atom = literal2.atom->clone();
    
 //   cout << "created literal: atom: " << *atom << endl;
 //   cout << "  is ho: " << atom->isHigherOrder << endl;
}

Literal::Literal(const Atom *at, bool naf)
    : isWeaklyNegated(naf)
{
    atom = new Atom(*at);
}

Literal::Literal(const ExternalAtom *at, bool naf)
    : isWeaklyNegated(naf)
{
    atom = new ExternalAtom(*at);
}

Atom*
Literal::getAtom() const
{
    return atom;
}

bool
Literal::isNAF() const
{
    return isWeaklyNegated;
}

std::ostream&
Literal::print(std::ostream &stream, const bool ho) const
{
    if (isNAF())
        stream << "not ";
        
    getAtom()->print(stream, ho);
    
    return stream;
}

/*
ostream& operator<< (ostream& out, const Literal& literal)
{
    if (literal.isNAF())
        out << "not ";
    
    out << *literal.getAtom();
    
    return out;
}
*/
