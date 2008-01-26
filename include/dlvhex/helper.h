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
 * @file   helper.h
 * @author Roman Schindlauer
 * @date   Thu Nov 24 23:59:33 CET 2005
 * 
 * @brief  Declaration of helper functions.
 * 
 * 
 */

#if !defined(_DLVHEX_HELPER_H)
#define _DLVHEX_HELPER_H

#include "dlvhex/PlatformDefinitions.h"

#include <string>
#include <vector>

DLVHEX_NAMESPACE_BEGIN

/**
 * The helper functions are kept in a namespace in order to keep global stuff
 * together.
 */
namespace helper
{
    /**
     * Explodes a string by a specified delimiter string.
     */
    std::vector<std::string>
    stringExplode(const std::string&, const std::string&);
    

    /**
     * Escapes quotes in a string (e.g. to use it for shell commands).
     */
    void
    escapeQuotes(std::string &str);
}

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_HELPER_H */


// Local Variables:
// mode: C++
// End:
