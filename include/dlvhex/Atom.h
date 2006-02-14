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


/**
 * @brief Abstract base class for all objects that are part of a program and
 * dynamically created.
 */
class ProgramObject
{
protected:
    ProgramObject() {}
};


/**
 * @brief Container for all elements of a program.
 */
class ProgramRepository
{
public:

    /**
     * @brief Get (unique) instance of the static repository class.
     */
    static ProgramRepository* Instance();

    /**
     * @brief Register a program element.
     *
     * By registering a program object here, it is assured that the object will
     * be destroyed at pogram termination.
     */
    void
    record(ProgramObject*);

protected:

    ProgramRepository()
    { }

    ~ProgramRepository();

private:

    std::vector<ProgramObject*> objects;

    static ProgramRepository* _instance;
};




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
    getArgument(const unsigned index) const;

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
    virtual bool
    unifiesWith(const Atom&) const;

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

    virtual bool
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
     * Clone function. this function returns a pointer to a newly created Atom
     * or derived class (hence virtual). Needed for copy constructors of classes that
     * use dynamic Atom objects (like LITERAL).
     */
//    virtual Atom*
//    clone();
    
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

    bool isStrongNegated;

    bool isAlwaysFO;
};


/**
 * This operator should only be used for dumping the output; it uses
 * the first-order notation.
 */
std::ostream&
operator<< (std::ostream&, const Atom&);

typedef boost::shared_ptr<Atom> AtomPtr;



/**
 * @brief Ordered set of ground atoms.
 */
//typedef std::set<Atom> GAtomSet;

//typedef Atom GAtom;


/**
 * This operator should only be used for dumping the output.
 */
//std::ostream&
//operator<< (std::ostream& out, const GAtomSet& groundatom);


//
// temp solution:
// implementing GAtomSet functions globally here instead of
// a dedicated class like interpretation
// we will see what turns out to be more practical
//


/*
void
printGAtomSet(const GAtomSet& g,
              std::ostream& stream,
              const bool ho);
*/            



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
    {
        type = BUILTIN;

        builtin = bp.builtin;
    }

/*
    Atom*
    clone()
    {
        return new BuiltinPredicate(*this);
    }
    */

    BuiltinPredicate(std::string b)
        : builtin(b)
    {
        type = BUILTIN;
    }

    /**
     * @brief Prints the atom.
     */
    virtual std::ostream&
    print(std::ostream& stream, const bool ho) const
    {
        return stream << builtin;
    }


    std::string builtin;
};


#endif /* _ATOM_H */

