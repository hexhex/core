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


Atom*
AtomFactory::insert(Atom a)
{
    //std::pair<std::set<Atom>::iterator, bool> p = atoms.insert(a);

    
    /// @todo very inefficient, how can we use std::set?

    for (std::vector<Atom*>::iterator ai = atoms.begin();
         ai != atoms.end();
         ++ai)
    {
        if (**ai == a)
            return *ai;
    }

    Atom* ins = new Atom(a);

    atoms.push_back(ins);

//    std::set<Atom>::iterator i = p.first;

//    Atom* u = &(*p.first);

//    std::cout << "created atom: " << (&atoms.back()) << std::endl;

//    std::vector<Atom>::iterator i = 
//    Atom aa = atoms.back();
//    std::cout << "inserted atom into factory: " << aa << std::endl;
    return ins;
}


AtomFactory::~AtomFactory()
{
    for (std::vector<Atom*>::iterator ai = atoms.begin();
         ai != atoms.end();
         ++ai)
    {
        delete *ai;
    }

}


