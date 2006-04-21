/* -*- C++ -*- */

/**
 * @file   ExternalAtom.cpp
 * @author Roman Schindlauer
 * @date   Wed Sep 21 19:45:11 CEST 2005
 * 
 * @brief  External class atom.
 * 
 * 
 */

#include <assert.h>
#include <string>
#include <iostream>
#include <sstream>

#include "dlvhex/PluginContainer.h"
#include "dlvhex/PluginInterface.h"
#include "dlvhex/ExternalAtom.h"
#include "dlvhex/Error.h"
#include "dlvhex/Registry.h"


ExternalAtom::ExternalAtom()
{
}



ExternalAtom::ExternalAtom(const ExternalAtom& extatom)
    : Atom(extatom.arguments),
      inputList(extatom.inputList),
      functionName(extatom.functionName),
      auxPredicate(extatom.auxPredicate),
      replacementName(extatom.replacementName),
      line(extatom.line),
      pluginAtom(extatom.pluginAtom)
{
//    type = extatom.type;
}



ExternalAtom::ExternalAtom(const std::string name,
                           const Tuple& params,
                           const Tuple& input,
                           const unsigned line)
    : Atom(params),
      inputList(input),
      functionName(name),
      line(line)
{
    //
    // set parent class type
    //

    std::ostringstream errorstr;

    //
    // first, get respective PluginAtom object
    //
    pluginAtom = PluginContainer::Instance()->getAtom(functionName);
    
    if (pluginAtom == 0)
    {
        throwSourceError("function " + functionName + " unknown");
    }

    //
    // is the desired arity equal to the parsed arity?
    //
    if ((*pluginAtom).getInputArity() != inputList.size())
    {
        throwSourceError("arity mismatch in function " + functionName);
    }
    
    if ((*pluginAtom).getOutputArity() != getArity())
    {
        throwSourceError("arity mismatch in function " + functionName);
    }
    
    //
    // make replacement name
    //
    std::stringstream ss;
    ss << functionName << "_" << uniqueNumber;
    replacementName = ss.str();

    //
    // remember this artificial atom name, we need to remove those later, they
    // shouldn't be in the actual result
    //
    Term::auxnames.insert(replacementName);

    //
    // build input list
    //
    bool inputIsGround(1);

    for (unsigned s = 0; s < inputList.size(); s++)
    {
        const Term inputTerm = inputList[s];

        if (inputTerm.isVariable())
        {
            inputIsGround = 0;

            //
            // at the moment, we don't allow variable predicate input arguments:
            //
            if (pluginAtom->getInputType(s) == PluginAtom::PREDICATE)
            {
                errorstr << "Line " << line << ": "
                        << "Variable predicate input arguments not allowed (yet)";

                throw FatalError(errorstr.str());
            }
        }
    }

    //
    // build auxiliary predicate
    //
    auxPredicate.clear();

    if (!inputIsGround)
    {
        //
        // also produce auxiliary predicate name, we need this for the
        // nonground input list
        //
        ss << "_aux";
        auxPredicate = ss.str();
        Term::auxnames.insert(auxPredicate);
    }

    
    //
    // if we got here, the syntax is fine!
    //

    uniqueNumber++;

}


void
ExternalAtom::throwSourceError(std::string msg) const
{
   throw SyntaxError(msg, line);
}


std::string
ExternalAtom::getAuxPredicate() const
{
    return auxPredicate;
}


std::string
ExternalAtom::getBasePredicate() const
{
    return basePredicate;
}


std::string
ExternalAtom::getFunctionName() const
{
    return functionName;
}


std::string
ExternalAtom::getReplacementName() const
{
    return replacementName;
}


Tuple
ExternalAtom::getArguments() const
{
    //
    // simply return a copy of arguments
    //
    return Tuple(arguments);
}


bool
ExternalAtom::pureGroundInput() const
{
    for (Tuple::const_iterator ti = inputList.begin();
         ti != inputList.end();
         ++ti)
    {
        if ((*ti).isVariable())
            return 0;
    }

    return 1;
}


const Tuple&
ExternalAtom::getInputTerms() const
{
    return inputList;
}


PluginAtom::InputType
ExternalAtom::getInputType(unsigned idx) const
{
    return pluginAtom->getInputType(idx);
}


void
ExternalAtom::groundInputList(const AtomSet& i,
                              std::vector<Tuple>& inputArguments) const
{
    //
    // first, we assume that we only take the original input list
    //
    inputArguments.push_back(inputList);

    //
    // did we create an auxiliary predicate before?
    //
    if (!auxPredicate.empty())
    {
        //
        // now that we know there are variable input arguments
        // (otherwise there wouldn't be such a dependency), we can start
        // over with the input list again and construct it from the
        // result of the auxiliary rules
        //
        inputArguments.clear();

        AtomSet arglist;

        //
        // get all the facts from i that match the auxiliary head atom
        // the arguments of those facts will be our input lists!
        //
        i.matchPredicate(auxPredicate, arglist);

        for (AtomSet::const_iterator argi = arglist.begin();
                argi != arglist.end();
                ++argi)
        {
            inputArguments.push_back((*argi).getArguments());
        }
    }
}



void
ExternalAtom::evaluate(const AtomSet& i,
                       AtomSet& result) const
{
    std::vector<Tuple> inputArguments;

    groundInputList(i, inputArguments);

    std::string fnc(getFunctionName());

    //
    // evaluate external atom for each input tuple we have now
    //
    for (std::vector<Tuple>::const_iterator inputi = inputArguments.begin();
            inputi != inputArguments.end();
            ++inputi)
    {
        AtomSet inputSet;

        //
        // extract input set from i according to the input parameters
        //
        for (unsigned s = 0; s < (*inputi).size(); s++)
        {
            const Term* inputTerm = &(*inputi)[s];

            //
            // at this point, the entire input list must be ground!
            //
            assert(!inputTerm->isVariable());

            switch(pluginAtom->getInputType(s))
            {
            case PluginAtom::CONSTANT:

                //
                // nothing to do, the constant will be passed directly to the plugin
                //

                break;

            case PluginAtom::PREDICATE:

                //
                // collect all facts from interpretation that we need for the input
                // of the external atom
                //


                /// @todo: since matchpredicate doesn't neet the output list, do we
                // need that factlist here?
                i.matchPredicate(inputTerm->getString(), inputSet);

                break;

            default:

                assert(0);

                break;
            }
        }

        //
        // build a query object:
        // - interpretation
        // - input list
        // - actual arguments of the external atom (maybe it is partly ground,
        // then the plugin can be more efficient)
        //
        PluginAtom::Query query(inputSet, *inputi, arguments);

        PluginAtom::Answer answer;
        
        try
        {
            pluginAtom->retrieve(query, answer);
        }
        catch (PluginError& e)
        {
            std::ostringstream atomstr;

            atomstr << functionName << "[" << 
                    inputList << "](" << arguments << ")" <<
                    " in line " << line;

            e.setContext(atomstr.str());

            throw e;
        }
        

        //
        // build result with the replacement name for each answer tuple
        //
        for (std::vector<Tuple>::const_iterator s = (*answer.getTuples()).begin();
             s != (*answer.getTuples()).end();
             ++s)
        {
            //
            // the replacement atom contains both the input and the output list!
            // (*inputi must be ground here, since it comes from
            // groundInputList(i, inputArguments))
            //
            Tuple resultTuple(*inputi);

            //
            // add output list
            //
            resultTuple.insert(resultTuple.end(), (*s).begin(), (*s).end());

            AtomPtr ap(new Atom(getReplacementName(), resultTuple));

            result.insert(ap);
        }
    }
}



bool
ExternalAtom::unifiesWith(const AtomPtr atom) const
{
    return 0;
}


bool
ExternalAtom::operator== (const Atom& atom2) const
{
    // TODO: how does this work - inheriting an operator?
    // and what is the right behaviour here?
    return 0;
}

std::ostream&
ExternalAtom::print(std::ostream& out, const bool ho) const
{
    //
    // the replacement atom contains both the input and the output list
    //

    Tuple replacementArgs(inputList);

    replacementArgs.insert(replacementArgs.end(), arguments.begin(), arguments.end());

    Atom tmp(getReplacementName(), replacementArgs);

    tmp.print(out, ho);

    return out;
}   


unsigned ExternalAtom::uniqueNumber = 0;


unsigned
ExternalAtom::getLine() const
{
    return line;
}
