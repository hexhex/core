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
#include "dlvhex/errorHandling.h"


ExternalAtom::ExternalAtom()
{
}


ExternalAtom::ExternalAtom(const ExternalAtom& extatom)
    : Atom(extatom.arguments),
      inputList(extatom.inputList),
      functionName(extatom.functionName),
      replacementName(extatom.replacementName),
      line(extatom.line),
      pluginAtom(extatom.pluginAtom)
{
    type = extatom.type;
}


ExternalAtom::ExternalAtom(const std::string name,
                           const Tuple& params,
                           const Tuple& input,
                           const unsigned line)
    : functionName(name),
      Atom(params),
      line(line),
      inputList(input)
{
    //
    // set parent class type
    //
    type = EXTERNAL;

    std::ostringstream errorstr;

    //
    // first, get respective PluginAtom object
    //
    pluginAtom = PluginContainer::Instance()->getAtom(functionName);
    
    if (pluginAtom == 0)
    {
        errorstr << "Line " << line << ": "
                 << "Function " << functionName << " unknown";

        throw FatalError(errorstr.str());
    }

    //
    // is the desired arity equal to the parsed arity?
    //
//    std::cout << "out: " << (*pluginAtom).getOutputArity() << std::endl;
//    std::cout << "here: " << getArity() << std::endl;
    if ((*pluginAtom).getInputArity() != inputList.size())
    {
        errorstr << "Line " << line << ": "
                 << "Arity mismatch in function " << functionName;

        throw FatalError(errorstr.str());
    }
    

    if ((*pluginAtom).getOutputArity() != getArity())
    {
        errorstr << "Line " << line << ": "
                 << "Arity mismatch in function " << functionName;

        throw FatalError(errorstr.str());
    }
    
    
    //
    // at the moment, we don't allow variable predicate input arguments:
    //
    for (int s = 0; s < inputList.size(); s++)
    {
        const Term inputTerm = inputList[s];

        if ((inputTerm.isVariable()) &&
            (pluginAtom->getInputType(s) == PluginAtom::PREDICATE))
        {
            errorstr << "Line " << line << ": "
                     << "Variable predicate input arguments not allowed";

            throw FatalError(errorstr.str());
        }
    }
    

    //
    // if we got here, the syntax is fine!
    //

    std::stringstream ss;
    
    ss << functionName << "_" << uniqueNumber;
    
    replacementName = ss.str();

    Term::auxnames.insert(replacementName);

    uniqueNumber++;
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
ExternalAtom::evaluate(const Interpretation& i,
                       const Tuple& inputParms,
                       GAtomSet& result) const
{
    std::string fnc(getFunctionName());

    Interpretation inputSet;

    GAtomSet factlist;
    
    //
    // collect parameters and input set
    //
    for (int s = 0; s < inputParms.size(); s++)
    {
        const Term* inputTerm = &inputParms[s];

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
            factlist.clear();

            i.matchPredicate(inputTerm->getString(), factlist);

            inputSet.add(factlist);
    
            break;

        default:

            assert(0);

            break;
        }
    }

    PluginAtom::Query query(inputSet, inputParms, arguments);

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
    

    for (std::vector<Tuple>::const_iterator s = (*answer.getTuples()).begin();
         s != (*answer.getTuples()).end();
         ++s)
    {
        //
        // construct the GAtom with the third parameter 'true' - this will set
        // the alwaysFirstOrder flag of the GAtom and ensure, that this GAtom
        // will never be serialized in higher-order-syntax! since the
        // replacement predicate for external atoms is always first order, the
        // corresponding facts need to be fo, too!
        //
        result.insert(GAtom(getReplacementName(), *s, 1));
    }
}



bool
ExternalAtom::unifiesWith(const Atom& atom) const
{
//    std::cout << " trying to unify external!" << std::endl;
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
    out << getReplacementName();

    if (getArity() > 0)
    {
        out << "(";
        
        for (unsigned i = 0; i < getArity(); i++)
        {
            out << getArgument(i);
            
            if (i < getArity() - 1)
                out << ",";
            
        }
        
        out << ")";
    }
    return out;
}   


Atom*
ExternalAtom::clone()
{
    return new ExternalAtom(*this);
}

unsigned ExternalAtom::uniqueNumber = 0;


unsigned
ExternalAtom::getLine() const
{
    return line;
}
