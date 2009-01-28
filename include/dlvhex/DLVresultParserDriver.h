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
 * @file   DLVresultParserDriver.h
 * @author Roman Schindlauer
 * @date   Wed Mar 22 14:39:32 CET 2006
 * 
 * @brief  C++ interface to the bison parser.
 * 
 * 
 */


#if !defined(_DLVHEX_DLVRESULTPARSERDRIVER_H)
#define _DLVHEX_DLVRESULTPARSERDRIVER_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/ParserDriver.h"
#include "dlvhex/AtomSet.h"
#include "dlvhex/Error.h"

#include <iostream>
#include <string>


// some forward declaration
class DLVresultFlexLexer;


DLVHEX_NAMESPACE_BEGIN

/**
 * @brief Parses DLV answer sets.
 */
class DLVHEX_EXPORT DLVresultParserDriver : public ParserDriver
{
public:
    DLVresultParserDriver();

    virtual
    ~DLVresultParserDriver();

    void
    parse(std::istream& is,
          std::vector<AtomSet>&) throw (SyntaxError);

    DLVresultFlexLexer*
    getLexer();

private:

    /// lexer object which scans the stream
    DLVresultFlexLexer* lexer;
};


DLVHEX_NAMESPACE_END

#endif // _DLVHEX_DLVRESULTPARSERDRIVER_H



// Local Variables:
// mode: C++
// End:
