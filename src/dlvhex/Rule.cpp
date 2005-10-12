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


 
Rule::Rule(const Atom &head, const std::vector<Literal> &b)
    : head(head)
{
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
