/* -*- C++ -*- */

/**
 * @file Registry.h
 * @author Roman Schindlauer
 * @date Tue Feb 14 15:47:48 CET 2006
 *
 * @brief Registry class.
 *
 */


#ifndef _REGISTRY_H
#define _REGISTRY_H


#include "dlvhex/Atom.h"
#include "dlvhex/AtomFactory.h"
#include "dlvhex/Repository.h"


/**
 * The Registry class is a sort of mediator that inserts objects into factory
 * classes.
 */
class Registry
{
public:

    /**
     * @brief Get (unique) instance of the static registry class.
     */
    static Registry*
    Instance();

    /**
     * @brief Stores an Atom.
     *
     * Using boost::shared_ptr, the ownership over a is transferred to the
     * shared pointer. The pointer Atom* must not be deleted after this call.
     * This method is supposed to be used for storing ground atoms.
     * The Atom is stored in the singleton instance of AtomFactory, which
     * maintains a set of Atoms, taking care of uniqueness of its members and
     * therefore optimal memory management. 
     */
    AtomPtr
    storeFact(Atom*);

    /**
     * @brief Stores a ProgramObject.
     *
     * Using boost::shared_ptr, the ownership over a is transferred to the
     * shared pointer. The pointer ProgramObject* must not be deleted after this call.
     * This method is supposed to be used for storing non-ground Objects from
     * the input program. The Objects are stored in the singleton instance of
     * Repository.
     */
    ProgramObjectPtr
    storeObject(ProgramObject*);

protected:

    Registry()
    { };

private:

    static Registry* _instance;
};

#endif /* _REGISTRY_H */
