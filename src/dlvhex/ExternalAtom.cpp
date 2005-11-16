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


ExternalAtom::ExternalAtom(std::string name,
                           const Tuple& params,
                           const Tuple& input,
                           unsigned line)
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

        throw generalError(errorstr.str());
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

        throw generalError(errorstr.str());
    }
    

    if ((*pluginAtom).getOutputArity() != getArity())
    {
        errorstr << "Line " << line << ": "
                 << "Arity mismatch in function " << functionName;

        throw generalError(errorstr.str());
    }
    
    //
    // at the moment, we don't allow variable input arguments:
    //
    for (Tuple::const_iterator il = inputList.begin();
         il != inputList.end();
         il++)
    {
        if (il->isVariable())
        {
            errorstr << "Line " << line << ": "
                     << "Variable input arguments not allowed";

            throw generalError(errorstr.str());
        }
    }

    //
    // if we got here, the syntax is fine!
    //

    std::stringstream ss;
    
    ss << functionName << "_" << uniqueNumber;
    
    replacementName = ss.str();

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


void
ExternalAtom::getInputTerms(Tuple &it) const
{
    it = inputList;
}


void
ExternalAtom::evaluate(const Interpretation& i,
                            const Tuple& inputParms,
                            GAtomSet& result) const
{
    std::string fnc(getFunctionName());

    Interpretation inputSet;

    GAtomSet factlist;
    
    for (int s = 0; s < inputList.size(); s++)
    {
        const Term* inputTerm = &inputList[s];

        switch(pluginAtom->getInputType(s))
        {
        case PluginAtom::CONSTANT:
            break;

        case PluginAtom::PREDICATE:

            //
            // collect all facts from interpretation that we need for the input
            // of the external atom
            //
            factlist.clear();

            //
            // at this point, of course everything needs to be ground!
            //
            assert(!inputTerm->isVariable());

            i.matchPredicate(inputTerm->getString(), factlist);

            inputSet.add(factlist);
    
            break;

        default:

            assert(0);
            break;
        }
    }


    std::vector<Tuple> extoutput;
    
    try
    {
        pluginAtom->retrieve(inputSet, inputParms, extoutput);
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
    
    for (std::vector<Tuple>::const_iterator s = extoutput.begin();
         s != extoutput.end();
         s++)
    {
        //std::cout << *s << std::endl;
        result.insert(GAtom(getReplacementName(), *s));
    }
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

