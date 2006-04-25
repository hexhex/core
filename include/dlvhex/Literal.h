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
#include "dlvhex/Repository.h"


/**
 * @brief Literal class.
 *
 * A literal is the constituting part of a rule body. It can be an atom or a 
 * weakly negated Atom. The atom of a literal can both be
 * an ordinary as well as an external atom.
 */
class Literal : public ProgramObject
{
public:

    /// Ctor
    Literal();


    /// Dtor
    ~Literal();


    /**
     * @brief Construct a literal from an atom, possibly weakly negated.
     */
    Literal(const AtomPtr, bool naf = false);


    /**
     * @brief Construct a literal from a builtin predicate.
     */
    //Literal(const BuiltinPredicate&, bool naf = false);


    /**
     * @brief Construct a literal from an external atom, possibly weakly negated.
     */
    //Literal(const ExternalAtom&, bool naf = false);


    /**
     * @brief Assignment operator.
     */
    Literal&
    operator=(const Literal&);


    /**
     * @brief returns a pointer to the atom of the literal.
     */
    const AtomPtr
    getAtom() const;


    /**
     * @brief returns true if the literal's atom is weakly negated, otherwise false.
     */
    bool
    isNAF() const;


    /**
     * @brief Test for equality.
     *
     * Two Literals are equal, if they contain the same atom and the same
     * type of negation.
     */
    bool
    operator== (const Literal& lit2) const;

    bool
    operator!= (const Literal& lit2) const;

    /**
     * @brief Serializes the literal.
     *
     * If the second argument is set to true, the literal is serialized in higher
     * order syntax, i.e., with its predicate symbol as an argument and a generic
     * new predicate symbol according to the number of its arguments.
     */
    std::ostream&
    print(std::ostream&, const bool) const;

private:

    const AtomPtr atom;

    bool isWeaklyNegated;
    
};


#endif /* _LITERAL_H */

