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
#include <fstream>

#include "dlvhex/HexParser.hpp"
#include "dlvhex/ParserDriver.h"


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
