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
      pluginAtom(extatom.pluginAtom),
      auxPredicate(extatom.auxPredicate)
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
    
    std::stringstream ss;
    
    ss << functionName << "_" << uniqueNumber;
    
    replacementName = ss.str();

    auxPredicate.clear();

    //
    // remember this artificial atom name, we need to remove those later, they
    // shouldn't be in the actual result
    //
    Term::auxnames.insert(replacementName);

    
    //
    // at the moment, we don't allow variable predicate input arguments:
    //
    for (int s = 0; s < inputList.size(); s++)
    {
        const Term inputTerm = inputList[s];

        if (inputTerm.isVariable())
        {
            if (pluginAtom->getInputType(s) == PluginAtom::PREDICATE)
            {
                errorstr << "Line " << line << ": "
                        << "Variable predicate input arguments not allowed (yet)";

                throw FatalError(errorstr.str());
            }

            //
            // also produce auxiliary predicate name, we need this for the
            // nonground input list
            //
            ss << "_aux";
            
            auxPredicate = ss.str();

            Term::auxnames.insert(auxPredicate);
        }
    }

    //
    // if we got here, the syntax is fine!
    //

    uniqueNumber++;
}


std::string
ExternalAtom::getAuxPredicate() const
{
    return auxPredicate;
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
ExternalAtom::evaluate(const AtomSet& i,
                       const Tuple& inputParms,
                       AtomSet& result) const
{
    std::vector<Tuple> inputArguments;

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
        for (int s = 0; s < (*inputi).size(); s++)
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
            AtomPtr ap(new Atom(getReplacementName(), *s));

            //
            // setting the alwaysFirstOrder flag of the Atom ensures that this Atom
            // will never be serialized in higher-order-syntax! since the
            // replacement predicate for external atoms is always first order, the
            // corresponding facts need to be fo, too!
            //
            ap->setAlwaysFO();

            result.insert(ap);
        }

//                std::cout << "result:" << std::endl;
//                printGAtomSet(r, std::cout, 0);
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

/*
Atom*
ExternalAtom::clone()
{
    return new ExternalAtom(*this);
}
*/

unsigned ExternalAtom::uniqueNumber = 0;


unsigned
ExternalAtom::getLine() const
{
    return line;
}
