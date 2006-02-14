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
Registry::dispatch(Atom* a)
{
    AtomPtr ap(a);

    AtomFactory::Instance()->insert(ap);

    return ap;
}

