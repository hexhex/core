/* -*- C++ -*- */

/**
 * @file Program.cpp
 * @author Roman Schindlauer
 * @date Tue Mar  7 16:51:41 CET 2006
 *
 * @brief Program class.
 *
 *
 *
 */


#include <iostream>

#include "dlvhex/Program.h"



Program::Program()
{
}


void
Program::addRule(const Rule* r)
{
    if (!exists(r))
    {
        rules.push_back(r);

        //
        // store the rule's external atoms also separately
        //
        for (RuleBody_t::const_iterator bi = r->getBody().begin();
            bi != r->getBody().end();
            ++bi)
        {
            if (typeid(*(*bi)->getAtom()) == typeid(ExternalAtom))
                externalAtoms.push_back((ExternalAtom*)(*bi)->getAtom());
        }
    }
}


bool
Program::exists(const Rule* r)
{
    return (find(rules.begin(), rules.end(), r) != rules.end());
}


const std::vector<ExternalAtom*>&
Program::getExternalAtoms() const
{
    return externalAtoms;
}


void
Program::dump(std::ostream& out) const
{
    //
    // dump is only for debugging and verbose. We don't want to dump the
    // higher-order rewriting
    //
    bool higherOrder = 0;

    for (const_iterator r = begin();
         r != end();
         ++r)
    {
        for (RuleHead_t::const_iterator hl = (*r)->getHead().begin();
                hl != (*r)->getHead().end();
                ++hl)
        {
            if (hl != (*r)->getHead().begin())
                out << " v ";
            
            (*hl)->print(out, higherOrder);
        }

        if ((*r)->getBody().size() > 0)
            out << " :- ";
            
        for (RuleBody_t::const_iterator l = (*r)->getBody().begin();
                l != (*r)->getBody().end();
                ++l)
        {
            if (l != (*r)->getBody().begin())
                out << ", ";
            
            (*l)->print(out, higherOrder);
        }

        out << "." << std::endl;
    }
}
