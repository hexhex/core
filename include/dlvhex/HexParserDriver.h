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


#ifndef _HEXPARSERDRIVER_H
#define _HEXPARSERDRIVER_H

#include <iostream>
#include <string>

#include "dlvhex/ParserDriver.h"
#include "dlvhex/Program.h"
#include "dlvhex/Error.h"

// put FlexLexer.h into its own include guards or yyFlexLexer gets
// redefined
#ifndef __FLEX_LEXER_H
#undef yyFlexLexer
#define yyFlexLexer hexFlexLexer
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
class HexFlexLexer : public yyFlexLexer
{
public:
    HexFlexLexer(ParserDriver* d) : lexdrv(d) { }
    virtual ~HexFlexLexer() { }
    ParserDriver* lexdrv;
    yy::location* lexloc;
    YYSTYPE* lexval;
    int yylex(); // implemented in HexScanner.lpp
};


class HexParserDriver : public ParserDriver
{
public:
    HexParserDriver();

    virtual
    ~HexParserDriver();

    void
    parse(std::istream& is,
          Program& program,
          AtomSet& EDB) throw (SyntaxError);

    void
    parse(std::string filename,
          Program& program,
          AtomSet& EDB) throw (SyntaxError);

    HexFlexLexer*
    getLexer();

    std::string
    getInputFilename() const;

private:

    /// lexer object which scans the stream
    HexFlexLexer* lexer;

};



#endif // _HEXPARSERDRIVER_H

