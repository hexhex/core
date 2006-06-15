/* -*- C++ -*- */

/**
 * @file   DLVresultFlexLexer.h
 * @author Thomas Krennwallner
 * @date   Wed Jun 14 16:43:59 2006
 * 
 * @brief  
 * 
 * 
 */

#ifndef _DLVRESULTFLEXLEXER
#define _DLVRESULTFLEXLEXER

#include "DLVresultParser.hpp"

// put FlexLexer.h into its own include guards or yyFlexLexer gets
// redefined
#ifndef __FLEX_LEXER_H
#undef yyFlexLexer
#define yyFlexLexer dlvFlexLexer
#include <FlexLexer.h>
#endif

class ParserDriver;

/**
 * @brief Use a refined yyFlexLexer.
 *
 */
class DLVresultFlexLexer : public yyFlexLexer
{
public:
    DLVresultFlexLexer(ParserDriver* d) : lexdrv(d) { }
    virtual ~DLVresultFlexLexer() { }
    ParserDriver* lexdrv;
    yy::DLVresultParser::location_type* lexloc;
    yy::DLVresultParser::semantic_type* lexval;
    int yylex(); // implemented in DLVresultScanner.lpp
};


#endif
