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
 * @file   ParserDriver.h
 * @author Roman Schindlauer
 * @date   Wed Mar 22 14:39:32 CET 2006
 * 
 * @brief  C++ interface to the bison parser.
 * 
 * 
 */


#if !defined(_DLVHEX_PARSERDRIVER_H)
#define _DLVHEX_PARSERDRIVER_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/Error.h"

#include <string>

// forward declaration (so we don't need to include bison-generated files here)
namespace yy
{
  class location;
}

DLVHEX_NAMESPACE_BEGIN

/**
 * @brief Conducting the whole scanning and parsing of Hex programs.
 */
class DLVHEX_EXPORT ParserDriver
{
protected:
    ParserDriver();

public:

    // Error handling.
    void
    error(const yy::location& l, const std::string& m) throw (SyntaxError);

    void
    error(const std::string& m) throw (SyntaxError);
};

DLVHEX_NAMESPACE_END

#endif // _DLVHEX_PARSERDRIVER_H


// Local Variables:
// mode: C++
// End:
