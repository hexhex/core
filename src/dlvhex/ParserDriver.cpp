/* -*- C++ -*- */

/**
 * @file   ParserDriver.cpp
 * @author Roman Schindlauer
 * @date   Wed Mar 22 14:38:53 CET 2006
 * 
 * @brief  C++ interface to the bison parser.
 * 
 * 
 */

#include <iostream>
#include <sstream>

#include "dlvhex/ParserDriver.h"

//#include "dlvhex/location.hh"



ParserDriver::ParserDriver()
{
}


/*
void
ParserDriver::syncStream()
{
    // sync and clear stream s.t. consecutive reading on the stream
    // works. Otherwise we would need to create a dedicated iostream for
    // each Racer command.

    is.sync();
    is.clear();
}
*/


void
ParserDriver::error(const yy::location& l,
                    const std::string& m) throw (SyntaxError)
{
    //syncStream();
//    std::stringstream s;
//    s << "Parsing error at " << l << ": " << m;
    throw SyntaxError(m, l.begin.line);
}


void
ParserDriver::error(const std::string& m) throw (SyntaxError)
{
    //syncStream();
//    throw SyntaxError("", 0, "Parsing error: " + m);
    throw SyntaxError(m);
}

