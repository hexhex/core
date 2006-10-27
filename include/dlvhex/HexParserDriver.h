/* -*- C++ -*- */

/**
 * @file   HexParserDriver.h
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

// some forward declaration 
class HexFlexLexer;

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
          AtomSet& EDB);

    HexFlexLexer*
    getLexer();

    std::string
    getInputFilename() const;

    void
    setOrigin(const std::string&);

private:

    /// lexer object which scans the stream
    HexFlexLexer* lexer;

    std::string source;

};



#endif // _HEXPARSERDRIVER_H

