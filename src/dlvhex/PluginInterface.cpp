/* -*- C++ -*- */

/**
 * @file   PluginAtom.cpp
 * @author Roman Schindlauer
 * @date   Mon Oct 17 14:37:07 CEST 2005
 * 
 * @brief Definition of Classes PluginAtom, PluginRewriter,
 * and PluginInterface.
 * 
 *      
 */     

#include "dlvhex/PluginInterface.h"


PluginAtom::Query::Query(const Interpretation& i,
                         const Tuple& in,
                         const Tuple& pat)
    : interpretation(i),
      input(in),
      pattern(pat)
{
}


const Interpretation&
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
{
}


void
PluginAtom::Answer::addTuple(const Tuple& out)
{
    output.push_back(out);
}


void
PluginAtom::Answer::addTuples(const std::vector<Tuple>& out)
{
    output.insert(output.end(), out.begin(), out.end());
}

void
PluginAtom::Answer::setTuples(const std::vector<Tuple>& out)
{
    output = out;
}

const std::vector<Tuple>*
PluginAtom::Answer::getTuples() const
{
    return &output;
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
PluginAtom::setOutputArity(unsigned arity)
{
    outputSize = arity;
}


unsigned
PluginAtom::getOutputArity() const
{
    return outputSize;
}


PluginAtom::InputType
PluginAtom::getInputType(unsigned index)
{
    assert(index < inputType.size());

    return inputType[index];
}
