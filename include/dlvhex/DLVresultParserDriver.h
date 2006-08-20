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

// some forward declaration
class DLVresultFlexLexer;

class DLVresultParserDriver : public ParserDriver
{
public:
    DLVresultParserDriver();

    virtual
    ~DLVresultParserDriver();

    void
    parse(std::istream& is,
          std::vector<AtomSet>&,
          std::string&) throw (SyntaxError);

    DLVresultFlexLexer*
    getLexer();

private:

    /// lexer object which scans the stream
    DLVresultFlexLexer* lexer;
};

#endif // _DLVRESULTPARSERDRIVER_H

