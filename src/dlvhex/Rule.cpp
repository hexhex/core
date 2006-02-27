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


 
/*
RuleBody::RuleBody()
//    : LogicalObject()
{
}


void
RuleBody::add(Literal* lit)
{
    body.push_back(lit);
}


size_t
RuleBody::size()
{
    return body.size();
}


bool
RuleBody::exists(const Literal* l)
{
    return (find(body.begin(), body.end(), l) != body.end());
}


RuleHead::RuleHead()
//    : LogicalObject()
{
}


void
RuleHead::add(Atom* at)
{
    head.push_back(at);
}


size_t
RuleHead::size()
{
    return head.size();
}


bool
RuleHead::exists(const Atom* a)
{
    return (find(head.begin(), head.end(), a) != head.end());
}
*/


Rule::Rule(const RuleHead_t& head,
           const RuleBody_t& body,
           std::string file,
           unsigned line)
    : head(head),
      body(body),
      programFile(file),
      programLine(line)
{
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

}


//const std::vector<Atom>&
const RuleHead_t&
Rule::getHead() const
{
    return head;
}

//const std::vector<Literal>&
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






Program::Program()
//    : LogicalObject()
{
}


/*
Program::Program(Rules& r)
{
    setRules(r);
}
*/


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
            if ((*bi)->getAtom()->getType() == Atom::EXTERNAL)
                externalAtoms.push_back((ExternalAtom*)(*bi)->getAtom());
        }
    }
}


bool
Program::exists(const Rule* r)
{
    return (find(rules.begin(), rules.end(), r) != rules.end());
}



/*
void
Program::setExternalAtoms(std::vector<ExternalAtom>& ex)
{
    externalAtoms = ex;
}
*/


/*
const Rules&
Program::getRules() const
{
    return rules;
}
*/


const std::vector<ExternalAtom*>&
Program::getExternalAtoms() const
{
    return externalAtoms;
}


/*
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
*/


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

    /*
    if (externalAtoms.size() > 0)
    {
        out << "External Atoms: " << std::endl;

        for (std::vector<ExternalAtom>::const_iterator exi = getExternalAtoms().begin();
            exi != getExternalAtoms().end();
            ++exi)
        {
            out << *exi << std::endl;
        }
    }
    */
}
