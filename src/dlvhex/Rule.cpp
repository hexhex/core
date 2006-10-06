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
#include <sstream>

#include "dlvhex/Rule.h"
#include "dlvhex/ExternalAtom.h"
#include "dlvhex/Registry.h"



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
        if (typeid(*((*bi)->getAtom())) == typeid(ExternalAtom))
            externalAtoms.push_back(dynamic_cast<ExternalAtom*>((*bi)->getAtom().get()));
    }

//    std::cout << " rule has extatoms: " << externalAtoms.size() << std::endl;
//    std::cout << "rule: " << *this << std::endl;
}


Rule::~Rule()
{ }


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
Rule::output(std::ostream& out) const
{
  if (!getHead().empty()) // output head
    {
      for (RuleHead_t::const_iterator hl = getHead().begin();
	   hl != getHead().end() - 1;
	   ++hl)
	{
	  (*hl)->print(out, 0);
	  out << " v ";
	}

      getHead().back()->print(out, 0);
    }

  if (!getBody().empty()) // output body with leading " :- "
    {
      out << " :- ";
            
      for (RuleBody_t::const_iterator l = getBody().begin();
	   l != getBody().end() - 1;
	   ++l)
	{
	  (*l)->print(out, 0);
	  out << ", ";
	}

      getBody().back()->print(out, 0);
    }

  return out << '.';
}



WeakConstraint::WeakConstraint(const RuleBody_t& b,
                               Term w,
                               Term l,
                               std::string file,
                               unsigned line)
    : Rule(RuleHead_t(), b, file, line),
      weight(w),
      level(l)
{
    if (l < 1)
        throw SyntaxError("level must be > 0");

    static unsigned uniqueid(0);

    std::stringstream wcheadname;

    wcheadname << "wch__" << uniqueid++;
    //wcheadname << "wc_h_";

    Term::registerAuxiliaryName(wcheadname.str());

    std::set<Term> headargs;

    RuleBody_t::const_iterator bodylit(b.begin()), bodyend(b.end());
    while (bodylit != bodyend)
    {
        Tuple args = (*bodylit)->getAtom()->getArguments();

        headargs.insert(args.begin(), args.end());

        ++bodylit;
    }

    Tuple hargs;

    std::set<Term>::const_iterator argit = headargs.begin();
    while (argit != headargs.end())
        hargs.push_back(*argit++);

    hargs.push_back(w);
    hargs.push_back(l);

    Atom* at = new Atom(wcheadname.str(), hargs);
    AtomPtr hatom(Registry::Instance()->storeAtom(at));

    head.push_back(hatom);
}


bool
WeakConstraint::operator== (const WeakConstraint& wc2) const
{
    ///todo implement this correctly!
    //

    return 0;
}


std::ostream&
WeakConstraint::output(std::ostream& out) const
{
  if (!getBody().empty()) // output body with leading ":~ "
    {
      out << ":~ ";
            
      for (RuleBody_t::const_iterator l = getBody().begin();
	   l != getBody().end() - 1;
	   ++l)
	{
	  (*l)->print(out, 0);
	  out << ", ";
	}

      getBody().back()->print(out, 0);
    }

  return out << ". [" << weight << ':' << level << ']';
}
