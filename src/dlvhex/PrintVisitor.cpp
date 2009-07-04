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
#include "dlvhex/Literal.h"
#include "dlvhex/Atom.h"
#include "dlvhex/ExternalAtom.h"
#include "dlvhex/AggregateAtom.h"

#include <iostream>
#include <iterator>
#include <boost/functional.hpp>

DLVHEX_NAMESPACE_BEGIN

PrintVisitor::PrintVisitor(std::ostream& s)
: stream(s)
{ }


std::ostream&
PrintVisitor::getStream()
{
  return stream;
}


void
PrintVisitor::visit(Program* const p)
{
  std::for_each(p->begin(), p->end(), boost::bind2nd(std::mem_fun(&Rule::accept), *this));
}


void
PrintVisitor::visit(AtomSet* const as)
{
  stream << '{';

  if (!as->empty())
    {
      for (AtomSet::atomset_t::const_iterator a = as->atoms.begin(); a != --as->atoms.end(); ++a)
	{
	  (*a)->accept(*this);
	  stream << ", ";
	}

      (*(--as->atoms.end()))->accept(*this);
    }

  stream << '}';
}


void
PrintVisitor::visit(Rule* const r)
{
  const RuleHead_t& head = r->getHead();

  if (!head.empty()) // output head
    {
      unsigned n(0);
      for (RuleHead_t::const_iterator hl = head.begin(); hl != head.end(); ++hl)
	{
	  (*hl)->accept(*this);
	  if (++n < head.size())
	    stream << " v ";
	}
    }

  const RuleBody_t& body = r->getBody();

  if (!body.empty()) // output body with leading " :- "
    {
      stream << " :- ";
      
      unsigned n(0);
      for (RuleBody_t::const_iterator l = body.begin(); l != body.end(); ++l)
	{
	  (*l)->accept(*this);
	  if (++n < body.size())
	    stream << ", ";
	}
    }

  stream << '.';
}


void
PrintVisitor::visit(Literal* const l)
{
  if (l->isNAF())
    {
      stream << "not ";
    }

  l->getAtom()->accept(*this);
}


void
PrintVisitor::visit(Atom* const a)
{
  if (a->isStronglyNegated())
    {
      stream << '-';
    }

  stream << a->getArgument(0);

  if (a->getArity() > 0)
    {
      stream << '(';

      const Tuple& arguments = a->getArguments();

      std::copy(arguments.begin(), --arguments.end(),
		std::ostream_iterator<Term>(stream, ","));
      stream << arguments.back() << ')';
    }
}


void
PrintVisitor::visit(BuiltinPredicate* const bp)
{
	if( bp->getArity() == 2 )
		stream << bp->getArgument(1) << bp->getArgument(0) << bp->getArgument(2);
	else
		stream << bp->getArgument(3) << "=" << bp->getArgument(1) << bp->getArgument(0) << bp->getArgument(2);
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

  const RuleBody_t& body = aa->getBody();

  if (!body.empty())
    {
      unsigned n(0);
      for (RuleBody_t::const_iterator l = body.begin(); l != body.end(); ++l)
	{
	  (*l)->accept(*this);
	  if (++n < body.size())
	    stream << ", ";
	}
    }

  stream << '}';

  if (!aa->getCmpRight().empty())
    {
      stream << ' ' << aa->getCmpRight() << ' ' << aa->getRight();
    }
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
  const RuleBody_t& body = wc->getBody();

  if (!body.empty()) // output body with leading ":~ "
    {
      stream << ":~ ";

      unsigned n(0);
      for (RuleBody_t::const_iterator l = body.begin(); l != body.end(); ++l)
	{
	  (*l)->accept(*this);
	  if (++n < body.size())
	    stream << ", ";
	}
      
      stream << ". [" << wc->getWeight() << ':' << wc->getLevel() << ']';
    }

  stream << std::endl;
}


void
RawPrintVisitor::visit(ExternalAtom* const ea)
{
  stream << '&' << ea->getFunctionName() << '[';

  const Tuple& inputList = ea->getInputTerms();

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
: PrintVisitor(s)
{ }


void
DLVPrintVisitor::visit(AtomSet* const as)
{
  for (AtomSet::atomset_t::const_iterator a = as->atoms.begin(); a != as->atoms.end(); ++a)
    {
      (*a)->accept(*this);
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
  visit(static_cast<Rule* const>(wc));
}


void
DLVPrintVisitor::visit(ExternalAtom* const ea)
{
  //
  // the replacement atom contains both the input and the output list
  //
  
  const Tuple& inputList = ea->getInputTerms();

  Tuple replacementArgs(inputList);

  const Tuple& arguments = ea->getArguments();

  replacementArgs.insert(replacementArgs.end(), arguments.begin(), arguments.end());

  Atom tmp(ea->getReplacementName(), replacementArgs);

  tmp.accept(*this);
}


HOPrintVisitor::HOPrintVisitor(std::ostream& s)
: DLVPrintVisitor(s)
{ }


void
HOPrintVisitor::visit(Atom* const a)
{
  if (a->getAlwaysFO()) // FO mode is always raw
    {
      RawPrintVisitor v(stream);
      a->accept(v);
    }
  else // print HO representation
    {
      if (a->isStronglyNegated())
	{
	  stream << '-';
	}

      // output a_n
      stream << "a_" << a->getArity() << '(';

      // output predicate and arguments
      for (unsigned i = 0; i < a->getArity(); i++)
	{
	  stream << a->getArgument(i) << ',';
	}

      stream << a->getArgument(a->getArity()) << ')';
    }
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
