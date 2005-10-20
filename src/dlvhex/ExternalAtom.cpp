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
      line(line)
{
    type = EXTERNAL;

    //
    // first, get respective PluginAtom object
    //
    pluginAtom = PluginContainer::Instance()->getAtom(functionName);
    
    std::ostringstream errorstr;

    if (pluginAtom == 0)
    {
        errorstr << "Function " << functionName <<
                    " in line " << line << " unknown";

        throw generalError(errorstr.str());
    }

    //
    // is the desired arity equal to the parsed arity?
    //
    if ((*pluginAtom).getOutputArity() != getArity())
    {
        errorstr << "Arity mismatch in line " << line;

        throw generalError(errorstr.str());
    }

    inputList = input;
    
    std::stringstream ss;
    
    ss << functionName << "_" << uniqueNumber;
    
    replacementName = ss.str();

    uniqueNumber++;

}


std::string ExternalAtom::getFunctionName() const
{
    return functionName;
}


std::string ExternalAtom::getReplacementName() const
{
    return replacementName;
}


void
ExternalAtom::getInputTerms(Tuple &it) const
{
    it = inputList;
}


void ExternalAtom::evaluate(const Interpretation& i,
                            const Tuple& inputParms,
                            GAtomSet& result) const
{
    std::string fnc(getFunctionName());

    Interpretation inputSet;

    GAtomSet factlist;
    
    for (int s = 0; s < inputList.size(); s++)
    {
        switch(pluginAtom->getInputType(s))
        {
        case PluginAtom::CONSTANT:
            break;

        case PluginAtom::PREDICATE:

            //
            // collect all facts from interpretation that we need for the input
            // of the external atom
            //
            for (Tuple::const_iterator il = inputList.begin();
                il != inputList.end();
                il++)
            {
                factlist.clear();

                assert(!il->isVariable());

                i.matchPredicate(il->getString(), factlist);

                inputSet.add(factlist);
                //extinput.push_back(factlist);
            }
    
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


Atom* ExternalAtom::clone()
{
    return new ExternalAtom(*this);
}

unsigned ExternalAtom::uniqueNumber = 0;

