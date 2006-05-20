/* -*- C++ -*- */

/**
 * @file Atom.h
 * @author Roman Schindlauer
 * @date Mon Sep  5 16:57:17 CEST 2005
 *
 * @brief Atom base class.
 *
 *
 */

#ifndef _ATOM_H
#define _ATOM_H


#include <list>
#include <set>

#include "boost/shared_ptr.hpp"
#include "dlvhex/Term.h"
#include "dlvhex/Repository.h"



class Atom;

typedef boost::shared_ptr<Atom> AtomPtr;

/**
 * @brief An Atom has a predicate and (if not propositional) an optional list of arguments.
 *
 */
class Atom : public ProgramObject
{
public:

    /**
     * Type of the atom.
     */
    typedef enum { INTERNAL, EXTERNAL, BUILTIN } Type;

    /**
     * Default constructor.
     */
    Atom();

    /**
     * Destructor.
     */
    virtual
    ~Atom();

    /**
     * Copy constructor.
     */
    Atom(const Atom&);

    /**
     * Constructs an atom from a string. This can be:
     * - A propositional atom, like 'p'
     * - A first order atom, like 'p(X)' or 'q(a,b,Z)'.
     * The second argument indicates if the atom is strongly negated.
     */
    Atom(const std::string, bool = false);

    /**
     * Constructs an atom from a predicate string and a tuple.
     * The third argument indicates if the atom is strongly negated.
     * The tuple can also be empty, then the atom is propositional and consists
     * only of the predicate identifier.
     */
    Atom(const std::string, const Tuple&, bool = false);

    /**
     * Constructs an atom from a list of arguments. This represents a higher-order
     * atom, where the predicate is just a Term inside the argument list.
     * The first element of 'arg' is considered to be the predicate - this is
     * important for the usage of getPredicate() and getArguments().
     * The second argument indicates if the atom is strongly negated.
     */
    Atom(const Tuple&, bool = false);

    /**
     * Returns the predicate of the atom.
     */
    virtual Term
    getPredicate() const;

    /**
     * Returns the arguments of an atom.
     *
     * The predicate is not included here!
     */
    virtual Tuple
    getArguments() const;

    /**
     * @brief Returns the specified argument term.
     *
     * The arguments of an atom are numbered from 1 to n. An index of 0 returns
     * the predicate symbol of the atom.
     */
    Term
    getArgument(const unsigned index) const;

    /**
     * Returns the arity of an atom (number of arguments).
     *
     * Note that we treat atoms as higher-order atoms:
     * p(q)  has arity 2
     * a     has arity 1
     * (X,Y) has arity 2
     */
    virtual unsigned
    getArity() const;
    
    /**
     * @brief Tests for unification with another atom.
     *
     * Two atoms unify if they have the same arity and all of their arguments
     * (including the predicate symbols) unify pairwise.
     */
    virtual bool
    unifiesWith(const AtomPtr) const;

    /**
     * @brief Tests for equality.
     *
     * Two Atoms are equal, if they have the same arity and list of arguments
     * (including the predicate). Two variable arguments are equal in this context, if
     * their strings are equal.
     * Two atoms of different type are always inequal.
     */
    virtual bool
    operator== (const Atom& atom2) const;

    bool
    operator!= (const Atom& atom2) const;

    int
    operator< (const Atom& atom2) const;
    
    /**
     * @brief Avoids serializing this Atom as a higher-order atom.
     */
    void
    setAlwaysFO();

    /**
     * @brief Prints the atom.
     */
    virtual std::ostream&
    print(std::ostream&, const bool) const;
    
    /**
     * @brief Tests if the atom contains only constant arguments.
     */
    bool
    isGround() const;

    bool
    isStronglyNegated() const;
    
protected:

    Tuple arguments;

    bool isStrongNegated;

    bool isAlwaysFO;
};


/**
 * This operator should only be used for dumping the output; it uses
 * the first-order notation.
 */
std::ostream&
operator<< (std::ostream&, const Atom&);


/**
 * @brief Special atom denoting either true or false.
 *
 * boolAtom does not unify with any other atom and can be used wherever 'true'
 * or 'false' are implicitly expected, e.g., as rule head for constraints.
 */
class boolAtom : public Atom
{
public:

    virtual Term
    getPredicate() const
    {
        return Term("");
    }
            
    virtual bool
    unifiesWith(const AtomPtr) const
    {
        return false;
    }

    virtual bool
    operator== (const Atom&) const
    {
        return false;
    }

    virtual std::ostream&
    print(std::ostream& out, const bool) const
    {
        return out << "";
    }

    int
    operator< (const Atom&) const
    {
        return true;
    }
};


#include <sstream>

/**
 * @brief Builtin Atom.
 *
 * This class represents atoms for builtin-predicates of dlv. For now, we just
 * pass the string on to the ASP solver and do not process it in any other way.
 */
class BuiltinPredicate : public Atom
{
public:

    BuiltinPredicate(const BuiltinPredicate& bp)
        : Atom(bp),
          builtin(bp.builtin)
    {

    }


    BuiltinPredicate(Term& term1, Term& term2, std::string& b)
        : t1(term1),
          t2(term2),
          builtin(b)
    {
    }

    /**
     * @brief Prints the atom.
     */
    virtual std::ostream&
    print(std::ostream& stream, const bool) const
    {
        return stream << t1 << builtin << t2;
    }

    Term t1, t2;

    std::string builtin;
};


#endif /* _ATOM_H */

