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
 * @file Registry.cpp
 * @author Roman Schindlauer
 * @date Tue Feb 14 15:47:48 CET 2006
 *
 * @brief Registry class.
 *
 */

#include "dlvhex/Registry.h"

DLVHEX_NAMESPACE_BEGIN

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
Registry::storeAtom(Atom* a)
{
//    AtomPtr ap(a);

    return AtomFactory::Instance()->insert(a);

//    return ap;
}


ProgramObjectPtr
Registry::storeObject(ProgramObject* po)
{
    ProgramObjectPtr pop(po);

    Repository::Instance()->insert(pop);

    return pop;
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
