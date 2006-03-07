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



Rule::Rule(const RuleHead_t& head,
           const RuleBody_t& body,
           std::string file,
           unsigned line)
    : head(head),
      body(body),
      programFile(file),
      programLine(line)
{
    externalAtoms.clear();

    //
    // store the rule's external atoms separately
    //
    for (RuleBody_t::const_iterator bi = body.begin();
        bi != body.end();
        ++bi)
    {
        if ((*bi)->getAtom()->getType() == Atom::EXTERNAL)
            externalAtoms.push_back((ExternalAtom*)(*bi)->getAtom());
    }

//    std::cout << " rule has extatoms: " << externalAtoms.size() << std::endl;
}


const RuleHead_t&
Rule::getHead() const
{
    return head;
}


const RuleBody_t&
Rule::getBody() const
{
    return body;
}


std::string
Rule::getFile() const
{
    return programFile;
}


unsigned
Rule::getLine() const
{
    return programLine;
}


const std::vector<ExternalAtom*>&
Rule::getExternalAtoms() const
{
//    std::cout << "getting extatoms of rule " << this <<  std::endl;
//    std::cout << " size: " << externalAtoms.size() << std::endl;
    return externalAtoms;
}


bool
Rule::operator== (const Rule& rule2) const
{
    /// @todo: stdlib algorithms here?

    if (head.size() != rule2.getHead().size())
        return 0;

    if (body.size() != rule2.getBody().size())
        return 0;

    for (RuleHead_t::const_iterator hi = head.begin();
         hi != head.end();
         ++hi)
    {
        for (RuleHead_t::const_iterator hi2 = rule2.getHead().begin();
            hi2 != rule2.getHead().end();
            ++hi2)
        {
            if (**hi != **hi2)
                return 0;
        }
    }

    for (RuleBody_t::const_iterator bi = body.begin();
         bi != body.end();
         ++bi)
    {
        for (RuleBody_t::const_iterator bi2 = rule2.getBody().begin();
            bi2 != rule2.getBody().end();
            ++bi2)
        {
            if (**bi != **bi2)
                return 0;
        }
    }

    return 1;
}


std::ostream&
operator<< (std::ostream& out, const Rule& rule)
{
    for (RuleHead_t::const_iterator hl = rule.getHead().begin();
         hl != rule.getHead().end();
         ++hl)
    {
        if (hl != rule.getHead().begin())
            out << " v ";
        
        (*hl)->print(out, 0);
    }

    out << " :- ";
            
    for (RuleBody_t::const_iterator l = rule.getBody().begin();
         l != rule.getBody().end();
         ++l)
    {
        if (l != rule.getBody().begin())
            out << ", ";
            
        (*l)->print(out, 0);
    }

    out << ".";

    return out;
}




