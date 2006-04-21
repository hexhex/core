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
    : lexer(new HexFlexLexer(this))
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
    return  "a";
}

void
HexParserDriver::parse(std::istream& is,
                       Program &program,
                       AtomSet& EDB) throw (SyntaxError)
{
    yy::HexParser parser(this, program, EDB);
    parser.set_debug_level(false);
    lexer->switch_streams(&is, &std::cerr);
    parser.parse();
    //syncStream();
} 


void
HexParserDriver::parse(std::string filename,
                       Program &program,
                       AtomSet& EDB) throw (SyntaxError)
{
    std::ifstream ifs;

    ifs.open(filename.c_str());

    if (!ifs.is_open())
    {
        exit(0);
    }

    yy::HexParser parser(this, program, EDB);
    parser.set_debug_level(false);
    lexer->switch_streams(&ifs, &std::cerr);

    try
    {
        parser.parse();
    }
    catch  (SyntaxError& e)
    {
        e.file = filename;
        throw e;
    }

    //syncStream();

    ifs.close();
} 
