/* -*- C++ -*- */

/**
 * @file   ParserDriver.h
 * @author Roman Schindlauer
 * @date   Wed Mar 22 14:39:32 CET 2006
 * 
 * @brief  C++ interface to the bison parser.
 * 
 * 
 */


#ifndef _DLVRESULTPARSERDRIVER_H
#define _DLVRESULTPARSERDRIVER_H

#include <iostream>
#include <string>

#include "dlvhex/ParserDriver.h"
#include "dlvhex/AtomSet.h"
#include "dlvhex/Error.h"

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
  
union YYSTYPE;

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


class DLVresultParserDriver : public ParserDriver
{
public:
    DLVresultParserDriver();

    virtual
    ~DLVresultParserDriver();

    void
    parse(std::istream& is,
          std::vector<AtomSet>& result) throw (SyntaxError);

    DLVresultFlexLexer*
    getLexer();

private:

    /// lexer object which scans the stream
    DLVresultFlexLexer* lexer;
};

#endif // _DLVRESULTPARSERDRIVER_H

