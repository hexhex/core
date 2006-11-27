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
#include "dlvhex/PrintVisitor.h"


Rule::Rule(const RuleHead_t& head,
           const RuleBody_t& body,
           const std::string& file,
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
}


void
Rule::setBody(const RuleBody_t& b)
{
	body = b;
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
      level(l)
{
    ///@todo move this stuff to another method in WeakConstraint

    if (l < 1)
        throw SyntaxError("level must be > 0");

    static unsigned uniqueid(0);

    std::stringstream wcheadname;

    wcheadname << "wch__" << uniqueid++;
    //wcheadname << "wc_h_";

    Term::registerAuxiliaryName(wcheadname.str());

    std::set<Term> headargs;

    for (RuleBody_t::const_iterator bodylit = b.begin(); bodylit != b.end(); ++bodylit)
    {
        Tuple args = (*bodylit)->getAtom()->getArguments();

        headargs.insert(args.begin(), args.end());
    }

    Tuple hargs;

    hargs.insert(hargs.end(), headargs.begin(), headargs.end());

    hargs.push_back(w);
    hargs.push_back(l);

    Atom* at = new Atom(wcheadname.str(), hargs);
    AtomPtr hatom(Registry::Instance()->storeAtom(at));

    head.insert(hatom);
}

void
WeakConstraint::addHead(AtomPtr)
{
  ///@todo not allowed?
  assert(false);
}


void
WeakConstraint::addBody(Literal*)
{
  ///@todo not allowed?
  assert(false);
}



bool
WeakConstraint::operator== (const WeakConstraint& wc2) const
{
    ///todo implement this correctly!
    //

    return 0;
}


void
WeakConstraint::accept(BaseVisitor& v) const
{
  v.visitWeakConstraint(this);
}
