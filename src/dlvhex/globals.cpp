/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with dlvhex; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */


/**
 * @file   globals.cpp
 * @author Roman Schindlauer
 * @date   Sat Nov  5 15:26:18 CET 2005
 * 
 * @brief  Global variable definitions.
 * 
 */

#include "dlvhex/globals.h"

#include <iostream>

DLVHEX_NAMESPACE_BEGIN

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
    verboseLevel[GRAPH_PROCESSOR] = 4;

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
Globals::getOption(const std::string& option)
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
Globals::setOption(const std::string& option, unsigned value)
{
    optionMap[option] = value;
}


void
Globals::addFilter(const std::string& f)
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

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
