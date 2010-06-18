/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
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
 * @file   Repository.h
 * @author Roman Schindlauer
 * @date   Tue Mar  7 17:06:12 CET 2006
 * 
 * @brief  Singleton class for storing all kinds of objects created from the
 * input program.
 * 
 * 
 */

#if !defined(_DLVHEX_REPOSITORY_H)
#define _DLVHEX_REPOSITORY_H

#include "dlvhex/PlatformDefinitions.h"

#include <vector>

#include <boost/shared_ptr.hpp>


DLVHEX_NAMESPACE_BEGIN

//
// forward declaration
//
class BaseVisitor;


/**
 * @brief Abstract base class for all objects that are part of a
 * program and dynamically created.
 *
 * This class does not implement an methods. It is just used as a common base
 * class for internal storage structures.
 */
class DLVHEX_EXPORT ProgramObject
{
protected:
    ProgramObject() {}
public:
    virtual
    ~ProgramObject();

    /// The accept method is part of the visitor pattern and used to
    /// double dispatch the correct type of the child, i.e. if someone
    /// calls accept on a subclass Atom with BaseVisitor v,
    /// Atom::accept() will call v.visitAtom(this) and v can decide
    /// what to do. This is useful in situation where we have an Atom*
    /// pointer to a ExternalAtom object and want to pretty print the
    /// ExternalAtom in its different representations (say in raw,
    /// first order or higher order mode). For each representation
    /// form we implement the corresponding concrete Visitor class.
    virtual void
    accept(BaseVisitor&) const = 0;
};


typedef boost::shared_ptr<ProgramObject> ProgramObjectPtr;



/**
 * @brief Container for all elements of a program.
 */
class DLVHEX_EXPORT Repository
{
public:

    /**
     * @brief Get (unique) instance of the static repository class.
     */
    static Repository* Instance();

    /**
     * @brief Register a program element.
     *
     * By registering a program object here, it is assured that the object will
     * be destroyed at pogram termination.
     */
    void
    insert(ProgramObjectPtr);

protected:

    Repository()
    { }

    ~Repository();

private:

    std::vector<ProgramObjectPtr> objects;

    static Repository* _instance;
};


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_REPOSITORY_H */


// Local Variables:
// mode: C++
// End:
