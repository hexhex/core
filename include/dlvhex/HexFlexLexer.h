/* -*- C++ -*- */

/**
 * @file   HexFlexLexer.h
 * @author Thomas Krennwallner
 * @date   Wed Jun 14 16:43:59 2006
 * 
 * @brief  
 * 
 * 
 */

#ifndef _HEXFLEXLEXER
#define _HEXFLEXLEXER

#include "dlvhex/HexParser.hpp"

// put FlexLexer.h into its own include guards or yyFlexLexer gets
// redefined
#ifndef __FLEX_LEXER_H
#undef yyFlexLexer
#define yyFlexLexer hexFlexLexer
#include <FlexLexer.h>
#endif

class ParserDriver;
    
/**
 * @brief Use a refined yyFlexLexer.
 *
 */
class HexFlexLexer : public yyFlexLexer
{
public:
    HexFlexLexer(ParserDriver* d) : lexdrv(d) { }
    virtual ~HexFlexLexer() { }
    ParserDriver* lexdrv;
    yy::HexParser::location_type* lexloc;
    yy::HexParser::semantic_type* lexval;
    int yylex(); // implemented in HexScanner.lpp
};


#endif
