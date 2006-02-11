/* -*- C++ -*- */

/**
 * @file   AtomFactory.h
 * @author Roman Schindlauer
 * @date   Sat Feb  4 19:09:02 CET 2006
 * 
 * @brief  Singleton class for storing atoms.
 * 
 * 
 */

#ifndef _ATOMFACTORY_H
#define _ATOMFACTORY_H

#include <vector>

#include "dlvhex/Atom.h"


/**
 * @brief The Factory stores all (ground) atoms that emerge in the course of
 * solving the program.
 */
class AtomFactory
{
public:

    /**
     * @brief Get (unique) instance of the static factory class.
     */
    static AtomFactory* Instance();

    /**
     * @brief Inserts an Atom into the Factory.
     *
     * 
     */
    void
    insert(AtomPtr&);

protected:

    AtomFactory()
    { }

    ~AtomFactory();

private:

    /**
     * @brief Custom compare operator.
     *
     * In order to treat the internal atom storage as a set of Atoms instead of
     * a set of AtomPtr, we define a custom compare operator that dereferences
     * the AtomPtrs.
     */
    struct AtomCompare
    {
        bool 
        operator() (const AtomPtr& a, const AtomPtr& b)
        {
            return *a < *b;
        }
    };

    /**
     * @brief Internal atom storage.
     *
     * The atom storage is a set of AtomPtrs, using std::set with a custom
     * compare operator that dereferences the AtomPtrs. This ensures that not
     * the pointers are uniquely inserted, but the Atoms themselves
     * (std::set::insert() uses the compare operator for determining element
     * existence).
     */
    std::set<AtomPtr, AtomCompare> atoms;

    static AtomFactory* _instance;
};




#endif /* _ATOMFACTORY_H */
