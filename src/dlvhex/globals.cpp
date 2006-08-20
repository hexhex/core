/* -*- C++ -*- */

/**
 * @file   globals.cpp
 * @author Roman Schindlauer
 * @date   Sat Nov  5 15:26:18 CET 2005
 * 
 * @brief  Global variable definitions.
 * 
 */

#include <iostream>
#include "dlvhex/globals.h"


Globals*
Globals::_instance = 0;


Globals::Globals()
{
}

Globals*
Globals::Instance()
{
    if (_instance == 0)
        _instance = new Globals;

    return _instance;
}


unsigned
Globals::getOption(std::string option)
{
    return optionMap[option];
}


void
Globals::setOption(std::string option, unsigned value)
{
    optionMap[option] = value;
}


std::ostream&
Globals::getVerboseStream()
{
    return std::cerr;
}

/*
namespace global
{
    bool optionNoPredicate = true;

    bool optionSilent = false;

    bool optionVerbose = false;

    bool optionStrongSafety = true;

    std::string maxint = "";

    std::vector<std::string> Messages;    

    std::string lpfilename;
}
*/
