/* -*- C++ -*- */

/**
 * @file Rule.cpp
 * @author Roman Schindlauer
 * @date Thu Jun 30 12:39:40 2005
 *
 * @brief Rule class.
 *
 *
 *
 */


#include <iostream>

#include "dlvhex/Rule.h"
#include "dlvhex/ExternalAtom.h"


 
Rule::Rule(const Atom& head, const std::vector<Literal>& b)
    : head(head)
{
    ///todo: initialize body?
    body = b;
}

/*
Rule::Rule(const Atom &h)
{
    head = h;
    body.clear();
}
*/

/*
bool Rule::hasBody() const
{
    return (body.size() > 0);
}
*/

const Atom*
Rule::getHead() const
{
    return &head;
}

const std::vector<Literal>*
Rule::getBody() const
{
    return &body;
}


/*
ostream& operator<< (ostream& out, const Rule& rule)
{
    out << *rule.getHead();

    if (rule.hasBody())
    {
        out << " :- ";
            
        for (std::vector<Literal>::const_iterator l = (rule.getBody())->begin(); l != (rule.getBody())->end(); l++)
        {
            if (l != (rule.getBody())->begin())
                out << ", ";
            
            out << (*l);
        }
    }

    out << ".";

    return out;
}
*/


Program::Program()
{
}

Program::Program(Rules& r)
{
    setRules(r);
}


void
Program::setRules(const Rules& r)
{
    for (Rules::const_iterator ri = r.begin();
         ri != r.end();
         ++ri)
    {
        addRule(*ri);
    }
}


void
Program::addRule(const Rule& r)
{
    rules.push_back(r);

    for (std::vector<Literal>::const_iterator li = r.getBody()->begin();
         li != r.getBody()->end();
         ++li)
    {
        if ((*li).getAtom()->getType() == Atom::EXTERNAL)
            externalAtoms.push_back(*(ExternalAtom*)(*li).getAtom());
    }
}


void
Program::setExternalAtoms(std::vector<ExternalAtom>& ex)
{
    externalAtoms = ex;
}


const Rules&
Program::getRules() const
{
    return rules;
}


const std::vector<ExternalAtom>&
Program::getExternalAtoms() const
{
    return externalAtoms;
}
