/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with dlvhex; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */


/**
 * @file   AtomFactory.h
 * @author Roman Schindlauer
 * @date   Sat Feb  4 19:09:02 CET 2006
 * 
 * @brief  Singleton class for storing atoms.
 * 
 * 
 */

#if !defined(_DLVHEX_ATOMFACTORY_H)
#define _DLVHEX_ATOMFACTORY_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/Atom.h"
#include "dlvhex/AtomSet.h"


DLVHEX_NAMESPACE_BEGIN


/**
 * @brief The Factory stores all (ground) atoms that emerge in the course of
 * solving the program.
 */
class DLVHEX_EXPORT AtomFactory
{
public:

    /**
     * @brief Get (unique) instance of the static factory class.
     */
    static AtomFactory* Instance();

    /**
     * @brief Inserts an Atom into the Factory.
     *
     * 
     */
    AtomPtr
    insert(Atom*);

    /*
     * @brief Clears the Factory.
     */
    void
    reset();

protected:

    AtomFactory()
    { }

    ~AtomFactory();

private:

    AtomSet::atomset_t atoms;

    static AtomFactory* _instance;
};


DLVHEX_NAMESPACE_END

#endif /* _ATOMFACTORY_H */


// Local Variables:
// mode: C++
// End:
