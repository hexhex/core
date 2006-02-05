/* -*- C++ -*- */

/**
 * @file   Repository.h
 * @author Roman Schindlauer
 * @date   Sat Feb  4 19:09:02 CET 2006
 * 
 * @brief  Singleton class for storing logical objects.
 * 
 * 
 */

#ifndef _REPOSITORY_H
#define _REPOSITORY_H

#include <vector>

#include "dlvhex/Atom.h"


/**
 * @brief The Repository stores all logic-related objects.
 */
class Repository
{
public:

    /**
     * @brief Get (unique) instance of the static repository class.
     */
    static Repository* Instance();

//    Atom*
//    makeAtom(std::string atom);

    void
    addAtom(Atom*);

    /// Dtor.
    ~Repository();

protected:

    Repository()
    { }

private:

    std::vector<Atom*> atoms;

    static Repository* _instance;
};

#endif /* _REPOSITORY_H */
