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
 * @file Registry.h
 * @author Roman Schindlauer
 * @date Tue Feb 14 15:47:48 CET 2006
 *
 * @brief Registry class.
 *
 */


#if !defined(_DLVHEX_REGISTRY_H)
#define _DLVHEX_REGISTRY_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/Atom.h"
#include "dlvhex/Repository.h"


DLVHEX_NAMESPACE_BEGIN

/**
 * The Registry class is a sort of mediator that inserts objects into factory
 * classes.
 */
class DLVHEX_EXPORT Registry
{
public:

    /**
     * @brief Get (unique) instance of the static registry class.
     */
    static Registry*
    Instance();

    /**
     * @brief Stores a ProgramObject.
	 *
	 * \todo do we need this at all?
     *
     * Using boost::shared_ptr, the ownership over a is transferred to the
     * shared pointer. The pointer ProgramObject* must not be deleted after this call.
     * This method is supposed to be used for storing non-ground Objects from
     * the input program. The Objects are stored in the singleton instance of
     * Repository.
     */
    ProgramObjectPtr
    storeObject(ProgramObject*);

protected:

    Registry()
    { };

private:

    static Registry* _instance;
};

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_REGISTRY_H */


// Local Variables:
// mode: C++
// End:
