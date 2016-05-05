/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2016 Peter Schüller
 * Copyright (C) 2011-2016 Christoph Redl
 * Copyright (C) 2015-2016 Tobias Kaminski
 * Copyright (C) 2015-2016 Antonius Weinzierl
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
 * @file   Configuration.cpp
 * @author Roman Schindlauer, Peter Schueller
 * @date   Sat Nov  5 15:26:18 CET 2005
 *
 * @brief  configuration container (previously global variables)
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#include "dlvhex2/Configuration.h"

#include <iostream>
#include <stdexcept>
#include <cassert>

DLVHEX_NAMESPACE_BEGIN

Configuration::Configuration()
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
    verboseLevel[PLUGIN_LOADING] = 4;

    //
    // intermediate model generation
    //
    verboseLevel[COMPONENT_EVALUATION] = 4;
    verboseLevel[MODEL_GENERATOR] = 4;
    verboseLevel[GRAPH_PROCESSOR] = 4;
    verboseLevel[DUMP_OUTPUT] = 4;

    //
    // time benchmarking
    //
    verboseLevel[PROFILING] = 8;
}


unsigned
Configuration::getOption(const std::string& option) const
{
    if( optionMap.find(option) == optionMap.end() )
        throw std::runtime_error("requested non-existing/unset option '"+option+"'");
    return optionMap.at(option);
}


bool
Configuration::doVerbose(verboseAction_t va)
{
    //
    // bitwise and
    //
    return (this->getOption("Verbose") & verboseLevel[va]) != 0;
}


void
Configuration::setOption(const std::string& option, unsigned value)
{
    optionMap[option] = value;
}


void
Configuration::addFilter(const std::string& f)
{
    optionFilter.push_back(f);
}

void
Configuration::addExplanationAtom(const std::string& e)
{
    optionFilter.push_back(e);
}

const std::vector<std::string>&
Configuration::getFilters() const
{
    return optionFilter;
}

const std::vector<std::string>&
Configuration::getExplanationAtoms() const
{
    return optionExplanation;
}

const std::string&
Configuration::getStringOption(
const std::string& key) const
{
    std::map<std::string, std::string>::const_iterator it =
        stringOptionMap.find(key);
    assert(it != stringOptionMap.end());
    return it->second;
}


void Configuration::setStringOption(
const std::string& key, const std::string& value)
{
    stringOptionMap[key] = value;
}


DLVHEX_NAMESPACE_END


// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
