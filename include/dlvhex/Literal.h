/* -*- C++ -*- */

/**
 * @file   Literal.h
 * @author Roman Schindlauer
 * @date   Sun Sep 4 12:39:40 2005
 * 
 * @brief  Literal class.
 * 
 * 
 */

#ifndef _LITERAL_H
#define _LITERAL_H

#include "dlvhex/Atom.h"
#include "dlvhex/ExternalAtom.h"

class Literal
{
public:
    /// Ctor
    Literal();

    /// Dtor
    ~Literal();

    /**
     * @brief Copy constructor.
     */
    Literal(const Literal &literal2);

    Literal(const Atom *atom, bool naf = false);

    Literal(const ExternalAtom *atom, bool naf = false);

    Atom*
    getAtom() const;

    bool
    isNAF() const;

    std::ostream&
    print(std::ostream &stream, const bool ho) const;

private:

    Atom *atom;

    bool isWeaklyNegated;
    
};

//ostream& operator<< (ostream& out, const Literal& literal);

#endif /* _LITERAL_H */
