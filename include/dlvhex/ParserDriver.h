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

//
// this is included from src/, not include/ !
// the makefile has to ensure that this file is created before, by bison
//
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

