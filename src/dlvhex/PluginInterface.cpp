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
    inputType.push_back(PREDICATE);
}


void
PluginAtom::addInputConstant()
{
    inputType.push_back(CONSTANT);
}


unsigned
PluginAtom::getInputArity() const
{
    return inputType.size();
}


void
PluginAtom::setOutputArity(const unsigned arity)
{
    outputSize = arity;
}


unsigned
PluginAtom::getOutputArity() const
{
    return outputSize;
}


PluginAtom::InputType
PluginAtom::getInputType(const unsigned index) const
{
    assert(index < inputType.size());

    return inputType[index];
}


// Local Variables:
// mode: C++
// End:
