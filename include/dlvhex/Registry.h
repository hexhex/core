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
     * @brief Dispatches an atom to a factory.
     */
    AtomPtr
    dispatch(Atom* a);

protected:

    Registry()
    { };

private:

    static Registry* _instance;
};

#endif /* _REGISTRY_H */
