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
 * 02110-1301 USA.
 */


/**
 * @file   PluginInterface.cpp
 * @author Roman Schindlauer
 * @date   Mon Oct 17 14:37:07 CEST 2005
 * 
 * @brief Definition of Classes PluginAtom, PluginRewriter,
 * and PluginInterface.
 * 
 *      
 */     

#include "dlvhex/PluginInterface.h"

DLVHEX_NAMESPACE_BEGIN

PluginAtom::Query::Query(const AtomSet& i,
                         const Tuple& in,
                         const Tuple& pat)
    : interpretation(i),
      input(in),
      pattern(pat)
{
}


const AtomSet&
PluginAtom::Query::getInterpretation() const
{
    return interpretation;
}


const Tuple&
PluginAtom::Query::getInputTuple() const
{
    return input;
}


const Tuple&
PluginAtom::Query::getPatternTuple() const
{
    return pattern;
}

        
PluginAtom::Answer::Answer()
    : output(new std::vector<Tuple>)
{
}


void
PluginAtom::Answer::addTuple(const Tuple& out)
{
    output->push_back(out);
}


void
PluginAtom::Answer::addTuples(const std::vector<Tuple>& out)
{
    output->insert(output->end(), out.begin(), out.end());
}

void
PluginAtom::Answer::setTuples(const std::vector<Tuple>& out)
{
    *output = out;
}

boost::shared_ptr<std::vector<Tuple> >
PluginAtom::Answer::getTuples() const
{
    return output;
}


void
PluginAtom::addInputPredicate()
{
	// throw error if last input term was tuple
	if (inputType.size() > 0)
		if (inputType.back() == TUPLE)
			throw GeneralError("Tuple inputs must be specified last in input list");

    inputType.push_back(PREDICATE);
}


void
PluginAtom::addInputConstant()
{
	// throw error if last input term was tuple
	if (inputType.size() > 0)
		if (inputType.back() == TUPLE)
			throw GeneralError("Tuple inputs must be specified last in input list");

    inputType.push_back(CONSTANT);
}


void
PluginAtom::addInputTuple()
{
    inputType.push_back(TUPLE);
}


bool
PluginAtom::checkInputArity(const unsigned arity) const
{
	if (inputType.back() == TUPLE)
		return true;

    return (inputType.size() == arity);
}


void
PluginAtom::setOutputArity(const unsigned arity)
{
    outputSize = arity;
}


bool
PluginAtom::checkOutputArity(const unsigned arity) const
{
    return (arity == outputSize);
}


const std::vector<PluginAtom::InputType>&
PluginAtom::getInputTypes() const
{
  return inputType;
}


PluginAtom::InputType
PluginAtom::getInputType(const unsigned index) const
{
	if (index >= inputType.size())
	{
		assert(inputType.back() == TUPLE);
		return inputType.back();
	}

    return inputType[index];
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
