/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


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


#include "dlvhex/Rule.h"
#include "dlvhex/ExternalAtom.h"
#include "dlvhex/Registry.h"
#include "dlvhex/PrintVisitor.h"

#include <iostream>
#include <sstream>

DLVHEX_NAMESPACE_BEGIN

Rule::Rule(const RuleHead_t& h,
           const RuleBody_t& b,
           const std::string& file,
           unsigned line)
    : head(h),
      body(b),
      programFile(file),
      programLine(line)
{
  setExternalAtoms(b);
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


void
Rule::setHead(const RuleHead_t& h)
{
	head = h;
}


void
Rule::addHead(AtomPtr ap)
{
  head.insert(ap);
}


void
Rule::addBody(Literal* l)
{
  body.insert(l);

  AtomPtr ap = l->getAtom();

  if (typeid(*ap) == typeid(ExternalAtom))
    externalAtoms.push_back(dynamic_cast<ExternalAtom*>(ap.get()));
}


void
Rule::setExternalAtoms(const RuleBody_t& b)
{
  //
  // store the rule's external atoms separately
  //
  for (RuleBody_t::const_iterator bi = b.begin(); bi != b.end(); ++bi)
    {
      if (typeid(*((*bi)->getAtom())) == typeid(ExternalAtom))
	externalAtoms.push_back(dynamic_cast<ExternalAtom*>((*bi)->getAtom().get()));
    }
}


void
Rule::setBody(const RuleBody_t& b)
{
  this->body = b;
  setExternalAtoms(b);
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
    const RuleHead_t& head2 = rule2.getHead();

    if (head.size() != head2.size())
        return 0;

    const RuleBody_t& body2 = rule2.getBody();

    if (body.size() != body2.size())
        return 0;

    return std::equal(head.begin(), head.end(), head2.begin()) &&
      std::equal(body.begin(), body.end(), body2.begin());
}


bool
Rule::operator< (const Rule& rule2) const
{
	if (this->head < rule2.head)
		return 1;
	if (this->head > rule2.head)
		return 0;

	if (this->body < rule2.body)
		return 1;

	return 0;
}


void
Rule::accept(BaseVisitor& v) const
{
  v.visitRule(this);
}


std::ostream&
operator<< (std::ostream& out, const Rule& rule)
{
  RawPrintVisitor rpv(out);
  rule.accept(rpv);
  return out;
}


bool
operator< (const RuleHead_t& head1, const RuleHead_t& head2)
{
	return std::lexicographical_compare(head1.begin(), head1.end(),
	  									head2.begin(), head2.end());
}




WeakConstraint::WeakConstraint(const RuleBody_t& b,
                               const Term& w,
                               const Term& l,
                               const std::string& file,
                               unsigned line)
    : Rule(RuleHead_t(), b, file, line),
      weight(w),
      level(l),
      uniqueID(0)
{
  if (l.getInt() < 1)
    {
      throw SyntaxError("level must be > 0");
    }

  static unsigned uniqueid(0);

  this->uniqueID = uniqueid++;
}




const RuleHead_t&
WeakConstraint::getHead() const
{
  if (Rule::getHead().empty())
    {
      std::set<Term> headargs;

      for (RuleBody_t::const_iterator bodylit = getBody().begin();
	   bodylit != getBody().end();
	   ++bodylit)
	{
	  Tuple args = (*bodylit)->getAtom()->getArguments();
	  
	  headargs.insert(args.begin(), args.end());
	}

      Tuple hargs;

      hargs.insert(hargs.end(), headargs.begin(), headargs.end());

      hargs.push_back(getWeight());
      hargs.push_back(getLevel());
    
      std::stringstream wcheadname;
      wcheadname << "wch__" << this->uniqueID;
      Term::registerAuxiliaryName(wcheadname.str());

      Atom* at = new Atom(wcheadname.str(), hargs);
      AtomPtr hatom(Registry::Instance()->storeAtom(at));

      head.insert(hatom);
    }

  return Rule::getHead();
}


void
WeakConstraint::addBody(Literal* l)
{
  head.clear();
  Rule::addBody(l);
}


void
WeakConstraint::setBody(const RuleBody_t& b)
{
  head.clear();
  Rule::setBody(b);
}

void
WeakConstraint::setHead(const RuleHead_t&)
{
  // there is nothing for you in here
}


void
WeakConstraint::addHead(const AtomPtr)
{
  // there is nothing for you in here
}


bool
WeakConstraint::operator== (const WeakConstraint& /* wc2 */) const
{
    ///@todo implement this correctly!
    return 0;
}


void
WeakConstraint::accept(BaseVisitor& v) const
{
  v.visitWeakConstraint(this);
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
