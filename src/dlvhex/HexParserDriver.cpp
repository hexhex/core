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
 * @file   HexParserDriver.cpp
 * @author Roman Schindlauer
 * @date   Wed Mar 22 14:38:53 CET 2006
 * 
 * @brief  C++ interface to the bison parser.
 * 
 * 
 */

#include <iostream>
#include <sstream>
#include <fstream>

#include "dlvhex/HexParser.hpp"
#include "dlvhex/ParserDriver.h"
#include "dlvhex/HexFlexLexer.h"

HexParserDriver::HexParserDriver()
    : lexer(new HexFlexLexer(this)),
      source("")
{
}


HexParserDriver::~HexParserDriver()
{
    delete lexer;
}


HexFlexLexer*
HexParserDriver::getLexer()
{
    return lexer;
}


std::string
HexParserDriver::getInputFilename() const
{
    return  this->source;
}


void
HexParserDriver::setOrigin(const std::string& org)
{
    this->source = org;
}


void
HexParserDriver::parse(std::istream& is,
                       Program &program,
                       AtomSet& EDB) throw (SyntaxError)
{
    yy::HexParser parser(this, program, EDB);
    parser.set_debug_level(false);
    lexer->switch_streams(&is, &std::cerr);

    try
    {
        parser.parse();
    }
    catch (SyntaxError& e)
    {
        //
        // is there was an error on the bison part, add the filename and throw
        // again
        //
        e.setFile(this->source);
        throw e;
    }

    //syncStream();
} 


void
HexParserDriver::parse(std::string file,
                       Program &program,
                       AtomSet& EDB)
{
    this->source = file;

    std::ifstream ifs;

    ifs.open(this->source.c_str());

    if (!ifs.is_open())
    {
        throw GeneralError("File " + this->source + " not found");
    }

    yy::HexParser parser(this, program, EDB);
    parser.set_debug_level(false);
    lexer->switch_streams(&ifs, &std::cerr);

    try
    {
        parser.parse();
    }
    catch (SyntaxError& e)
    {
        //
        // is there was an error on the bison part, add the filename and throw
        // again
        //
        e.setFile(this->source);
        throw e;
    }

    //syncStream();

    ifs.close();
} 
