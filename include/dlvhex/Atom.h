/* -*- C++ -*- */

/**
 * @file Atom.h
 * @author Roman Schindlauer
 * @date Mon Sep  5 16:57:17 CEST 2005
 *
 * @brief Atom base class and GroundAtom class.
 *
 *
 */

#ifndef _ATOM_H
#define _ATOM_H


#include <list>
#include <set>

#include "dlvhex/Term.h"

/**
 * @brief An Atom has a predicate and (if not propositional) an optional list of arguments.
 */
class Atom
{
public:

    /**
     * Type of the atom.
     */
    enum Type { INTERNAL, EXTERNAL };

    /**
     * Default constructor.
     */
    Atom();

    /**
     * Destructor.
     */
    virtual ~Atom();

    /**
     * Copy constructor.
     */
    Atom(const Atom &atom2);

    /**
     * Constructs an atom from a string. This can be:
     * - A propositional atom, like 'p'
     * - A first order atom, like 'p(X)' or 'q(a,b,Z)'.
     * An atom constructed this way is always a first-order atom.
     */
    Atom(std::string atom);

    /**
     * Constructs an atom from a predicate string and a tuple. An atom
     * constructed this way is always a first-order atom (an assertion fails,
     * if pred is not a constant).
     */
    Atom(std::string pred, Tuple arg);

    /**
     * Constructs an atom from a list of arguments. This represents a higher-order
     * atom, where the predicate is just a Term inside the argument list.
     * The first element of 'arg' is considered to be the predicate - this is
     * important for the usage of getPredicate() and getArguments().
     */
    Atom(Tuple arg);

    /**
     * Returns the predicate of the atom.
     */
    Term
    getPredicate() const;

    /**
     * Returns the arguments of an atom.
     */
    Tuple
    getArguments() const;

    /**
     * @brief Returns the specified argument term.
     *
     * The arguments of an atom are numbered from 1 to n. An index of 0 returns
     * the predicate symbol of the atom.
     */
    Term
    getArgument(unsigned index) const;

    /**
     * Returns the arity of an atom (number of arguments).
     */
    unsigned
    getArity() const;
    
    /**
     * @brief Tests for unification with another atom.
     *
     * Two atoms unify if they have the same arity and all of their arguments
     * (including the predicate symbols) unify pairwise.
     */
    bool
    unifiesWith(const Atom &atom2) const;

    /**
     * @brief Prints the atom.
     */
    virtual std::ostream&
    print(std::ostream &stream, const bool ho) const;

    /**
     * Clone function. this function returns a pointer to a newly created Atom
     * or derived class (hence virtual). Needed for copy constructors of classes that
     * use dynamic Atom objects (like LITERAL).
     */
    virtual Atom*
    clone();
    
    /**
     * @brief Returns the type (internal - external) of the atom.
     */
    virtual Type
    getType() const;

    /**
     * @brief Tests if the atom contains only constant arguments.
     */
    bool
    isGround() const;
    
protected:

    Type type;

    Tuple arguments;

};

/**
 * This operator should only be used for dumping the output; it uses
 * the first-order notation.
 */
std::ostream&
operator<< (std::ostream &out, const Atom &atom);



/**
 * @brief A GroundAtom is an Atom without any variable arguments.
 */
class GAtom : public Atom
{
public:

    /**
     * @brief Default constructor.
     */
    GAtom();

    GAtom(const Atom &);

    GAtom(std::string);
	
    GAtom(std::string, Term);

    GAtom(std::string, Tuple);

    GAtom(Tuple);

    bool
    operator== (const GAtom &gatom2) const;

    int
    operator< (const GAtom &gatom2) const;
};



/**
 * @brief Set (list) of ground atoms.
 */
//typedef std::list<GAtom> GAtomList;


/**
 * @brief Ordered set of ground atoms.
 */
typedef std::set<GAtom> GAtomSet;


/**
 * This operator should only be used for dumping the output.
 */
std::ostream&
operator<< (std::ostream &out, const GAtomSet &groundatom);


//
// temp solution:
// implementing GAtomSet functions globally here instead of
// a dedicated class like interpretation
// we will see what turns out to be more practical
//

//typedef GAtomSet Interpretation;

/*
void
matchPredicate(const GAtomSet &g,
               const std::string pred,
               GAtomSet &atomset);
*/

void
printGAtomSet(const GAtomSet &g,
              std::ostream& stream,
              const bool ho);
              
#endif /* _ATOM_H */
