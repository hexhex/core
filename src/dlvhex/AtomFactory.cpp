/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


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

DLVHEX_NAMESPACE_BEGIN

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


void
AtomFactory::reset()
{
    atoms.clear();
}


AtomFactory::~AtomFactory()
{
}

DLVHEX_NAMESPACE_END


// Local Variables:
// mode: C++
// End:
