/* -*- C++ -*- */

/**
 * @file   AtomFactory.cpp
 * @author Roman Schindlauer
 * @date   Sat Feb  4 19:09:02 CET 2006
 * 
 * @brief  Singleton class for storing atoms.
 * 
 * 
 */


#include "dlvhex/AtomFactory.h"


//
// initialize static variable:
//
AtomFactory* AtomFactory::_instance = 0;


AtomFactory*
AtomFactory::Instance()
{
    if (_instance == 0)
    {
        _instance = new AtomFactory;
    }

    return _instance;
}


struct NullDeleter
{
    void operator() (Atom*) {} // don't delete managed Atom object
};



AtomPtr
AtomFactory::insert(Atom* ap)
{
    AtomPtr a(ap, NullDeleter());

    AtomSet::atomset_t::const_iterator it = atoms.find(a);

    if (it == atoms.end())
    {
        AtomPtr x(ap);
        atoms.insert(x);
        return x;
    }

    delete ap;

    return *it;

}


AtomFactory::~AtomFactory()
{
}


