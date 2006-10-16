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
//    std::cout << "sizeof: " << sizeof(verboseAction_t) << std::endl;
//    verboseLevel.reserve(sizeof(verboseAction_t));

    verboseLevel[DUMP_REWRITTEN_PROGRAM] = 2;

    verboseLevel[COMPONENT_EVALUATION] = 3;
    verboseLevel[MODEL_GENERATOR] = 3;
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

bool
Globals::doVerbose(verboseAction_t va)
{
    return (this->getOption("Verbose") >= verboseLevel[va]);
}


void
Globals::setOption(std::string option, unsigned value)
{
    optionMap[option] = value;
}


void
Globals::addFilter(std::string& f)
{
    optionFilter.push_back(f);
}


const std::vector<std::string>&
Globals::getFilters() const
{
    return optionFilter;
}


std::ostream&
Globals::getVerboseStream() const
{
    return std::cerr;
}

