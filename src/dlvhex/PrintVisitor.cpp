/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with dlvhex; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */


/**
 * @file   PrintVisitor.cpp
 * @author Thomas Krennwallner
 * @date   Mon Oct 23 19:03:41 2006
 * 
 * @brief  
 * 
 * 
 */


#include "dlvhex/PrintVisitor.h"
#include "dlvhex/Program.h"
#include "dlvhex/AtomSet.h"
#include "dlvhex/Rule.h"
#include "dlvhex/Constraint.h"
#include "dlvhex/WeakConstraint.h"
#include "dlvhex/Literal.h"
#include "dlvhex/Atom.h"
#include "dlvhex/BuiltinPredicate.h"
#include "dlvhex/ExternalAtom.h"
#include "dlvhex/AggregateAtom.h"
#include "dlvhex/PredefinedNames.h"

#include <iostream>
#include <sstream>
#include <boost/iterator/indirect_iterator.hpp>

DLVHEX_NAMESPACE_BEGIN


PrintVisitor&
operator<< (PrintVisitor& v, const Program& p)
{
  std::for_each(boost::make_indirect_iterator(p.begin()),
		boost::make_indirect_iterator(p.end()),
		std::bind2nd(std::mem_fun_ref(&BaseRule::accept), &v));
  return v;
}


PrintVisitor&
operator<< (PrintVisitor& v, const AtomSet& as)
{
  AtomSet& atoms = const_cast<AtomSet&>(as);
  std::ostream& stream = v.getStream();

  stream << '{';

  if (!atoms.empty())
    {
      AtomSet::const_iterator lastelem = --atoms.end();

      for (AtomSet::const_iterator a = atoms.begin();
	   a != lastelem; ++a)
	{
	  (*a)->accept(&v);
	  stream << ", ";
	}

      (*lastelem)->accept(&v);
    }

  stream << '}';

  return v;
}


PrintVisitor::PrintVisitor(std::ostream& s)
  : stream(s)
{ }


std::ostream&
PrintVisitor::getStream()
{
  return stream;
}


void
PrintVisitor::visit(Rule* const r)
{
  const HeadPtr& head = r->head();
  const Head::size_type hs = head->size();

  if (hs > 0) // output disjunctive head
    {
      Head::size_type n = 0;
      for (Head::const_iterator hl = head->begin(); hl != head->end(); ++hl)
	{
	  (*hl)->accept(this);
	  if (++n < hs) stream << " v ";
	}
    }

  const BodyPtr& body = r->body();
  const Body::size_type bs = body->size();

  // only append a space if both head and body is non-empty
  if (hs > 0 && bs > 0)
    {
      stream << ' ';
    }

  if (bs > 0) // output body with leading ":- "
    {
      stream << ":- ";
      
      Body::size_type n = 0;
      for (Body::const_iterator l = body->begin(); l != body->end(); ++l)
	{
	  (*l)->accept(this);
	  if (++n < bs) stream << ", ";
	}
    }

  if (hs + bs) stream << '.';
}



void
PrintVisitor::visit(Constraint* const r)
{
  const BodyPtr& body = r->body();
  const Body::size_type bs = body->size();

  if (bs > 0) // output body with leading ":- "
    {
      stream << ":- ";
      
      Body::size_type n = 0;
      for (Body::const_iterator l = body->begin(); l != body->end(); ++l)
	{
	  (*l)->accept(this);
	  if (++n < bs) stream << ", ";
	}

      stream << '.';
    }
}


void
PrintVisitor::visit(Literal<Positive>* const l)
{
  l->getAtom()->accept(this);
}


void
PrintVisitor::visit(Literal<Negative>* const l)
{
  stream << "not ";
  l->getAtom()->accept(this);
}


void
PrintVisitor::visit(Atom<Positive>* const a)
{
  stream << a->getPredicate();

  if (a->getArity() > 0)
    {
      stream << '(';

      const Tuple& arguments = a->getArguments();

      std::copy(arguments.begin(), --arguments.end(),
		std::ostream_iterator<Term>(stream, ","));

      stream << arguments.back()
	     << ')';
    }
}


void
PrintVisitor::visit(Atom<Negative>* const a)
{
  stream << '-' << a->getPredicate();

  if (a->getArity() > 0)
    {
      stream << '(';

      const Tuple& arguments = a->getArguments();

      std::copy(arguments.begin(), --arguments.end(),
		std::ostream_iterator<Term>(stream, ","));

      stream << arguments.back()
	     << ')';
    }
}


void
PrintVisitor::visit(BuiltinPredicate* const bp)
{
  stream << bp[1] << bp->getPredicate() << bp[2];
}


void
PrintVisitor::visit(AggregateAtom* const aa)
{
  if (!aa->getCmpLeft().empty())
    {
      stream << aa->getLeft() << ' ' << aa->getCmpLeft() << ' ';
    }

  stream << aa->getType() << '{'
	 << aa->getVars() << " : ";

  const BodyPtr& body = aa->getBody();
  const Body::size_type bs = body->size();

  if (bs > 0)
    {
      Body::size_type n = 0;
      for (Body::const_iterator l = body->begin(); l != body->end(); ++l)
	{
	  (*l)->accept(this);
	  if (++n < bs) stream << ", ";
	}
    }

  stream << '}';

  if (!aa->getCmpRight().empty())
    {
      stream << ' ' << aa->getCmpRight() << ' ' << aa->getRight();
    }
}


void
PrintVisitor::visit(Query<Brave>* const)
{ 
  ///@todo nothing in here yet
}


void
PrintVisitor::visit(Query<Cautious>* const)
{
  ///@todo nothing in here yet

}




RawPrintVisitor::RawPrintVisitor(std::ostream& s)
: PrintVisitor(s)
{ }


void
RawPrintVisitor::visit(Rule* const r)
{
  PrintVisitor::visit(r);
  stream << std::endl;
}

void
RawPrintVisitor::visit(WeakConstraint* const wc)
{
  const BodyPtr& body = wc->body();
  const Body::size_type bs = body->size();

  if (bs > 0) // output body with leading ":~ "
    {
      stream << ":~ ";

      Body::size_type n = 0;
      for (Body::const_iterator l = body->begin(); l != body->end(); ++l)
	{
	  (*l)->accept(this);
	  if (++n < bs) stream << ", ";
	}
      
      stream << ". [" << wc->getWeight() << ':' << wc->getLevel() << ']';
    }

  stream << std::endl;
}


void
RawPrintVisitor::visit(ExternalAtom* const ea)
{
  stream << '&' << ea->getPredicate() << '[';

  const Tuple& inputList = ea->getInputList();

  if (!inputList.empty())
    {
      std::copy(inputList.begin(), --inputList.end(),
		std::ostream_iterator<Term>(stream, ","));
      stream << inputList.back();
    }

  stream << "](";

  const Tuple& arguments = ea->getArguments();

  if (!arguments.empty())
    {
      std::copy(arguments.begin(), --arguments.end(),
		std::ostream_iterator<Term>(stream, ","));
      stream << arguments.back();
    }

  stream << ')';
}


DLVPrintVisitor::DLVPrintVisitor(std::ostream& s)
  : PrintVisitor(s),
    wcCounter(0)
{ }


void
DLVPrintVisitor::visit(AtomSet* const as)
{
  for (AtomSet::const_iterator a = as->begin();
       a != as->end(); ++a)
    {
      (*a)->accept(this);
      stream << '.' << std::endl;
    }
}


void
DLVPrintVisitor::visit(Rule* const r)
{
  PrintVisitor::visit(r);
  stream << std::endl;
}


void
DLVPrintVisitor::visit(WeakConstraint* const wc)
{
  std::set<Term> headargs;

  const BodyPtr& weakbody = wc->body();

  for (Body::const_iterator bodylit = weakbody->begin();
       bodylit != weakbody->end();
       ++bodylit)
    {
      const Tuple& args = (*bodylit)->getAtom()->getArguments();
      headargs.insert(args.begin(), args.end());
    }

  Tuple hargs;
  hargs.insert(hargs.end(), headargs.begin(), headargs.end());
  hargs.push_back(wc->getWeight());
  hargs.push_back(wc->getLevel());
  
  std::stringstream wcheadname;

  wcheadname << PredefinedNames::WEAKHEAD << this->wcCounter++;

  ///@todo use __ as aux symbol
  Term::registerAuxiliaryName(wcheadname.str());
      
  Term pred(wcheadname.str());

  AtomPtr hatom(new Atom<Positive>(pred, hargs));

  HeadPtr weakhead(new Head(1, hatom));

  Rule tmp(weakhead, wc->body());
  tmp.accept(this);
}


void
DLVPrintVisitor::visit(ExternalAtom* const ea)
{
  //
  // the replacement atom contains both the input and the output list
  //
  
  const Tuple& inputList = ea->getInputList();

  Tuple replacementArgs(inputList);

  const Tuple& arguments = ea->getArguments();

  replacementArgs.insert(replacementArgs.end(), arguments.begin(), arguments.end());

  Atom<Positive> tmp(ea->getReplacementName(), replacementArgs);

  tmp.accept(this);
}


HOPrintVisitor::HOPrintVisitor(std::ostream& s)
: DLVPrintVisitor(s)
{ }


void
HOPrintVisitor::visit(Atom<Positive>* const a)
{
  ///@todo add "a_" as macro

  unsigned n = a->getArity();

  // output a_n
  stream << "a_" << n
	 << '(';

  // output predicate and arguments - 1
  for (unsigned i = 0; i < n; i++)
    {
      stream << a[i] << ',';
    }

  // output last argument
  stream << a[n]
	 << ')';
}

void
HOPrintVisitor::visit(Atom<Negative>* const a)
{
  ///@todo add "a_" as macro

  unsigned n = a->getArity();

  // output -a_n
  stream << "-a_" << n
	 << '(';

  // output predicate and arguments - 1
  for (unsigned i = 0; i < n; i++)
    {
      stream << a[i] << ',';
    }

  // output last argument
  stream << a[n]
	 << ')';
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
