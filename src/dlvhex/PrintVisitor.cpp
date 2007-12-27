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
 * @file   PrintVisitor.cpp
 * @author Thomas Krennwallner
 * @date   Mon Oct 23 19:03:41 2006
 * 
 * @brief  
 * 
 * 
 */


#include "dlvhex/PrintVisitor.h"
#include "dlvhex/AtomSet.h"
#include "dlvhex/Rule.h"
#include "dlvhex/Literal.h"
#include "dlvhex/Atom.h"
#include "dlvhex/ExternalAtom.h"
#include "dlvhex/AggregateAtom.h"

#include <iostream>
#include <iterator>

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
PrintVisitor::visitAtomSet(const AtomSet* as)
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
PrintVisitor::visitRule(const Rule* r)
{
	if (!r->getHead().empty()) // output head
	{
		unsigned n(0);
		for (RuleHead_t::const_iterator hl = r->getHead().begin();
				hl != r->getHead().end();
				++hl)
		{
			(*hl)->accept(*this);
			if (++n < r->getHead().size())
				stream << " v ";
		}
	}

	if (!r->getBody().empty()) // output body with leading " :- "
	{
		stream << " :- ";

		unsigned n(0);
		for (RuleBody_t::const_iterator l = r->getBody().begin();
				l != r->getBody().end();
				++l)
		{
			(*l)->accept(*this);
			if (++n < r->getBody().size())
				stream << ", ";
		}
	}

	stream << '.';
}


void
PrintVisitor::visitLiteral(const Literal* l)
{
	if (l->isNAF())
		stream << "not ";

	l->getAtom()->accept(*this);
}


void
PrintVisitor::visitAtom(const Atom* a)
{
	if (a->isStronglyNegated())
		stream << '-';

	stream << a->getArgument(0);

	if (a->getArity() > 0)
	{
		stream << '(';

		for (unsigned i = 1; i < a->getArity(); i++)
		{
			stream << a->getArgument(i) << ',';
		}

		stream << a->getArgument(a->getArity()) << ')';
	}
}


void
PrintVisitor::visitBuiltinPredicate(const BuiltinPredicate* bp)
{
	stream << bp->getArgument(1)
		<< bp->getArgument(0)
		<< bp->getArgument(2);
}


void
PrintVisitor::visitAggregateAtom(const AggregateAtom* aa)
{
	if (aa->getCmpLeft() != "")
		stream << aa->getLeft() << ' ' << aa->getCmpLeft() << ' ';

	stream << aa->getType() << '{'
		<< aa->getVars() << " : ";

	if (!aa->getBody().empty())
	{
		unsigned n(0);
		for (RuleBody_t::const_iterator l = aa->getBody().begin();
				l != aa->getBody().end();
				++l)
		{
			(*l)->accept(*this);
			if (++n < aa->getBody().size())
				stream << ", ";
		}
	}

	stream << '}';

	if (aa->getCmpRight() != "")
		stream << ' ' << aa->getCmpRight() << ' ' << aa->getRight();
}



RawPrintVisitor::RawPrintVisitor(std::ostream& s)
: PrintVisitor(s)
{ }


void
RawPrintVisitor::visitWeakConstraint(const WeakConstraint* wc)
{
	if (!wc->getBody().empty()) // output body with leading ":~ "
	{
		stream << ":~ ";

		unsigned n(0);
		for (RuleBody_t::const_iterator l = wc->getBody().begin();
				l != wc->getBody().end();
				++l)
		{
			(*l)->accept(*this);
			if (++n < wc->getBody().size())
				stream << ", ";
		}

		stream << ". [" << wc->getWeight() << ':' << wc->getLevel() << ']';
	}
}


void
RawPrintVisitor::visitExternalAtom(const ExternalAtom* ea)
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
DLVPrintVisitor::visitAtomSet(const AtomSet* as)
{
	if (!as->empty())
	{
		for (AtomSet::atomset_t::const_iterator a = as->atoms.begin(); a != --as->atoms.end(); ++a)
		{
			(*a)->accept(*this);
			stream << '.' << std::endl;
		}

		(*(--as->atoms.end()))->accept(*this);
		stream << '.' << std::endl;
	}
}


void
DLVPrintVisitor::visitRule(const Rule* r)
{
	PrintVisitor::visitRule(r);
	stream << std::endl;
}


void
DLVPrintVisitor::visitWeakConstraint(const WeakConstraint* wc)
{
	visitRule(wc);
}


void
DLVPrintVisitor::visitExternalAtom(const ExternalAtom* ea)
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
HOPrintVisitor::visitAtom(const Atom* a)
{
	if (!a->getAlwaysFO())
	{
		if (a->isStronglyNegated())
			stream << '-';

		stream << "a_" << a->getArity() << '(';

		for (unsigned i = 0; i < a->getArity(); i++)
		{
			stream << a->getArgument(i) << ',';
		}

		stream << a->getArgument(a->getArity()) << ')';
	}
	else // FO mode is always raw
	{
		RawPrintVisitor v(stream);
		v.visitAtom(a);
	}
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
