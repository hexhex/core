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
     * @brief accepts a visitor.
     */
    virtual void
    accept(BaseVisitor&) const;

    friend std::ostream&
    operator<<(std::ostream&, const Literal&);

private:

    const AtomPtr atom;

    bool isWeaklyNegated;
    
};

/**
 * A rule body is a conjunction of literals.
 */
typedef std::vector<Literal*> RuleBody_t;


std::ostream&
operator<<(std::ostream&, const Literal&);


#endif /* _LITERAL_H */

