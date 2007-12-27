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
 * @file   DLVresultFlexLexer.h
 * @author Thomas Krennwallner
 * @date   Wed Jun 14 16:43:59 2006
 * 
 * @brief  
 * 
 * 
 */

#if !defined(_DLVHEX_DLVRESULTFLEXLEXER)
#define _DLVHEX_DLVRESULTFLEXLEXER

#include "dlvhex/PlatformDefinitions.h"

#include "DLVresultParser.hpp"


// put FlexLexer.h into its own include guards or yyFlexLexer gets
// redefined
#ifndef __FLEX_LEXER_H
#undef yyFlexLexer
#define yyFlexLexer dlvFlexLexer
#include <FlexLexer.h>
#endif

DLVHEX_NAMESPACE_BEGIN
class ParserDriver;
DLVHEX_NAMESPACE_END


/**
 * @brief Use a refined yyFlexLexer.
 *
 */
class DLVHEX_EXPORT DLVresultFlexLexer : public yyFlexLexer
{
public:
    explicit DLVresultFlexLexer(DLVHEX_NAMESPACE ParserDriver* d) : lexdrv(d) { }
    virtual ~DLVresultFlexLexer() { }
    DLVHEX_NAMESPACE ParserDriver* lexdrv;
    yy::DLVresultParser::location_type* lexloc;
    yy::DLVresultParser::semantic_type* lexval;
    int yylex(); // implemented in DLVresultScanner.lpp
};

#endif /* _DLVHEX_DLVRESULTFLEXLEXER */


// Local Variables:
// mode: C++
// End:
