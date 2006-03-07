/* -*- C++ -*- */

/**
 * @file Registry.cpp
 * @author Roman Schindlauer
 * @date Tue Feb 14 15:47:48 CET 2006
 *
 * @brief Registry class.
 *
 */

#include "dlvhex/Registry.h"
//#include "dlvhex/Atom.h"
//#include "dlvhex/Repository.h"


//
// initialize static variable:
//
Registry* Registry::_instance = 0;


Registry*
Registry::Instance()
{
   if (_instance == 0)
    {
        _instance = new Registry;
    }

    return _instance;
}


AtomPtr
Registry::storeFact(Atom* a)
{
    AtomPtr ap(a);

    AtomFactory::Instance()->insert(ap);

    return ap;
}


ProgramObjectPtr
Registry::storeObject(ProgramObject* po)
{
    ProgramObjectPtr pop(po);

    Repository::Instance()->insert(pop);

    return pop;
}

