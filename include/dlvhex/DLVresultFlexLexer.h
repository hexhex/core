/* -*- C++ -*- */

/**
 * @file   DLVresultFlexLexer.h
 * @author Roman Schindlauer
 * @date   Thu Apr 13 14:52:39 CEST 2006
 * 
 * @brief  Derive custom class from Flex's FlexLexer to have a cleaner
 * C++ interface.
 * 
 * 
 */

#ifndef _DLVRESULTFLEXLEXER_H
#define _DLVRESULTFLEXLEXER_H


// put FlexLexer.h into its own include guards or yyFlexLexer gets
// redefined
#ifndef __FLEX_LEXER_H
#undef yyFlexLexer
#define yyFlexLexer dlvFlexLexer
#include <FlexLexer.h>
#endif


//
// some forward declarations
//
namespace yy
{
    class location;
}
  
union YYSTYPE;

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
    yy::location* lexloc;
    YYSTYPE* lexval;
    int yylex(); // implemented in DLVresultScanner.lpp
};

#endif // _DLVRESULTFLEXLEXER_H

