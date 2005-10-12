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

#include <string>
#include <iostream>
#include <sstream>

#include "dlvhex/PluginContainer.h"
#include "dlvhex/PluginInterface.h"
#include "dlvhex/ExternalAtom.h"
#include "dlvhex/errorHandling.h"


#include <assert.h>

ExternalAtom::ExternalAtom()
{
}

ExternalAtom::ExternalAtom(const ExternalAtom& extatom)
    : Atom(extatom.arguments),
      inputList(extatom.inputList),
      functionName(extatom.functionName),
      replacementName(extatom.replacementName),
      line(extatom.line),
      externalPlugin(extatom.externalPlugin)
{
    type = extatom.type;
}

ExternalAtom::ExternalAtom(std::string name,
                           const Tuple &params,
                           const Tuple &input,
                           unsigned line)
    : Atom(params),
      line(line)
{
    /*
    for (Tuple::const_iterator t = input.begin(); t != input.end(); t++)
    {
        assert(!t->isVariable());
    }
    */
    inputList = input;
    
    functionName = name;
    
    type = external;
    
    std::stringstream ss;
    
    ss << functionName << "_" << uniqueNumber;
    
    replacementName = ss.str();

    uniqueNumber++;

    externalPlugin = PluginContainer::Instance()->getAtom(functionName);
    
    std::ostringstream errorstr;

    if (externalPlugin == 0)
    {
        errorstr << "Function " << functionName <<
                    " in line " << line << " unknown";

        throw generalError(errorstr.str());
    }

    if (!(*externalPlugin).testArities(input.size(), params.size()))
    {
        errorstr << "Arity mismatch in line " << line;

        throw generalError(errorstr.str());
    }

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

void ExternalAtom::evaluate(const Interpretation &i, GAtomSet &result) const
{
    std::string fnc(getFunctionName());

    PluginAtom::FACTSETVECTOR extinput;
    
    GAtomSet factlist;
    
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

        matchPredicate(i, il->getString(), factlist);

        extinput.push_back(factlist);
    }
    

    PluginAtom::TUPLEVECTOR extoutput;
    
    try
    {
        externalPlugin->retrieve(extinput, extoutput);
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
    
    for (PluginAtom::TUPLEVECTOR::const_iterator s = extoutput.begin();
         s != extoutput.end();
         s++)
    {
        std::cout << *s << std::endl;
        result.insert(GAtom(getReplacementName(), *s));
    }
}

std::ostream&
ExternalAtom::print(std::ostream &out, const bool ho) const
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
