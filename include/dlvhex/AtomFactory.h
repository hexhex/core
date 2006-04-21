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
#include "dlvhex/AtomSet.h"

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
    AtomPtr
    insert(Atom*);

protected:

    AtomFactory()
    { }

    ~AtomFactory();

private:

    AtomSet::atomset_t atoms;

    static AtomFactory* _instance;
};




#endif /* _ATOMFACTORY_H */
