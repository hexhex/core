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
//    std::cout << "Parsing error at " << l << ": " << m << std::endl;

    //
    // the actual line of the error seems to be l.end.line!
    //
    throw SyntaxError(m, l.end.line);
}


void
ParserDriver::error(const std::string& m) throw (SyntaxError)
{
    //syncStream();
//    throw SyntaxError("", 0, "Parsing error: " + m);
    throw SyntaxError(m);
}

