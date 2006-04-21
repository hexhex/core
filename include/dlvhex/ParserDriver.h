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


#ifndef _PARSERDRIVER_H
#define _PARSERDRIVER_H

#include <iostream>
#include <string>
#include <vector>

#include "dlvhex/location.hh"

#include "dlvhex/Error.h"

/**
 * @brief Conducting the whole scanning and parsing of Hex programs.
 */
class ParserDriver
{
protected:
    ParserDriver();

    /// parse this stream
//    std::istream is;

    //void
    //syncStream();

public:

    // Error handling.
    void
    error(const yy::location& l, const std::string& m) throw (SyntaxError);

    void
    error(const std::string& m) throw (SyntaxError);
};


#endif // _PARSERDRIVER_H

