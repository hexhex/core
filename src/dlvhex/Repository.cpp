/* -*- C++ -*- */

/**
 * @file   Repository.cpp
 * @author Roman Schindlauer
 * @date   Sat Feb  4 19:09:02 CET 2006
 * 
 * @brief  Singleton class for storing logical objects.
 * 
 * 
 */


#include "dlvhex/Repository.h"


//
// initialize static variable:
//
Repository* Repository::_instance = 0;


Repository*
Repository::Instance()
{
    if (_instance == 0)
    {
        _instance = new Repository;
    }

    return _instance;
}


/*
Atom*
Repository::makeAtom(std::string atom)
{
    Atom* a = new Atom(atom);

    atoms.push_back(a);

    return a;
}
*/

void
Repository::addAtom(Atom* atom)
{
    atoms.push_back(atom);
}


Repository::~Repository()
{
    for (std::vector<Atom*>::iterator ai = atoms.begin();
         ai != atoms.end();
         ++ai)
    {
        delete *ai;
    }
}
