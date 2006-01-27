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


 
Rule::Rule(const std::vector<Atom>& head,
          const std::vector<Literal>& b)
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

const std::vector<Atom>&
Rule::getHead() const
{
    return head;
}

const std::vector<Literal>&
Rule::getBody() const
{
    return body;
}


bool
Rule::operator== (const Rule& rule2) const
{
    if (head.size() != rule2.getHead().size())
        return 0;

    if (body.size() != rule2.getBody().size())
        return 0;

    for (std::vector<Atom>::const_iterator hl = rule2.getHead().begin();
         hl != rule2.getHead().end();
         hl++)
    {
        if (find(head.begin(), head.end(), *hl) == head.end())
            return 0;
    }

    for (std::vector<Literal>::const_iterator hl = rule2.getBody().begin();
         hl != rule2.getBody().end();
         hl++)
    {
        if (find(body.begin(), body.end(), *hl) == body.end())
            return 0;
    }

    return 1;
}


std::ostream&
operator<< (std::ostream& out, const Rule& rule)
{
    for (std::vector<Atom>::const_iterator hl = rule.getHead().begin();
         hl != rule.getHead().end();
         hl++)
    {
        if (hl != rule.getHead().begin())
            out << " v ";
        
        hl->print(out, 0);
    }

    out << " :- ";
            
    for (std::vector<Literal>::const_iterator l = rule.getBody().begin();
         l != rule.getBody().end();
         l++)
    {
        if (l != rule.getBody().begin())
            out << ", ";
            
        l->print(out, 0);
    }

    out << ".";

    return out;
}


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

    //
    // store the rule's external atoms also separately
    //
    for (std::vector<Literal>::const_iterator li = r.getBody().begin();
         li != r.getBody().end();
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


ExternalAtom*
Program::findExternalAtom(const std::string name, const Tuple& params)
{
    for (std::vector<ExternalAtom>::iterator exi = externalAtoms.begin();
        exi != externalAtoms.end();
        ++exi)
    {
        if ((exi->getFunctionName() == name) &&
            (exi->getInputTerms() == params))
        {
            return &(*exi);
        }
        else
        {
            return NULL;
        }
    }
}


void
Program::dump(std::ostream& out) const
{
    //
    // dump is only for debugging and verbose. We don't want to dump the
    // higher-order rewriting
    //
    bool higherOrder = 0;

    for (Rules::const_iterator r = rules.begin();
         r != rules.end();
         r++)
    {
        for (std::vector<Atom>::const_iterator hl = r->getHead().begin();
                hl != r->getHead().end();
                hl++)
        {
            if (hl != r->getHead().begin())
                out << " v ";
            
            hl->print(out, higherOrder);
        }

        if (r->getBody().size() > 0)
            out << " :- ";
            
        for (std::vector<Literal>::const_iterator l = r->getBody().begin();
                l != r->getBody().end();
                l++)
        {
            if (l != r->getBody().begin())
                out << ", ";
            
            l->print(out, higherOrder);
        }

        out << "." << std::endl;
    }

    if (externalAtoms.size() > 0)
    {
//        out << std::endl;

        out << "External Atoms: " << std::endl;

        for (std::vector<ExternalAtom>::const_iterator exi = getExternalAtoms().begin();
            exi != getExternalAtoms().end();
            ++exi)
        {
            out << *exi << std::endl;
        }
    }
}
