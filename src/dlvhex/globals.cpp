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
	//
	// program analysis
	//
    verboseLevel[DUMP_PARSED_PROGRAM] = 1;
    verboseLevel[DUMP_DEPENDENCY_GRAPH] = 1;
    verboseLevel[SAFETY_ANALYSIS] = 1;

	//
	// plugin processing
	//
    verboseLevel[DUMP_CONVERTED_PROGRAM] = 2;
    verboseLevel[DUMP_REWRITTEN_PROGRAM] = 2;
    verboseLevel[DUMP_OPTIMIZED_PROGRAM] = 2;

	//
	// intermediate model generation
	//
    verboseLevel[COMPONENT_EVALUATION] = 4;
    verboseLevel[MODEL_GENERATOR] = 4;

	//
	// time benchmarking
	//
    verboseLevel[PROFILING] = 8;
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
	//
	// bitwise and
	//
    return (this->getOption("Verbose") & verboseLevel[va]);
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

