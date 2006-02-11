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


void
AtomFactory::insert(AtomPtr& ap)
{
//    AtomPtr ap(new Atom(a));

    //
    // insert the atom pointer.
    // if the Atom that is referenced by ap (see definiton of atoms)
    // is already in the set, then the
    // returned iterator points to this atom's AtomPtr.
    // if not, then the ap is added to the set.
    // 
    std::pair<std::set<AtomPtr, AtomCompare>::iterator, bool> p = atoms.insert(ap);

    ap = *(p.first);
}


AtomFactory::~AtomFactory()
{
}


