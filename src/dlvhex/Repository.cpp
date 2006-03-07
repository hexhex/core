/* -*- C++ -*- */

/**
 * @file Repository.cpp
 * @author Roman Schindlauer
 * @date Tue Mar  7 17:06:12 CET 2006
 *
 * @brief Singleton class for storing all kinds of objects created from the
 * input program.
 *
 *
 */


#include "dlvhex/Repository.h"


//
// initialize static variable:
//
Repository* Repository::_instance = 0;


Repository::~Repository()
{
}


Repository*
Repository::Instance()
{
    if (_instance == 0)
    {
        _instance = new Repository;
    }

    return _instance;
}


void
Repository::insert(ProgramObjectPtr po)
{
    objects.push_back(po);
}


