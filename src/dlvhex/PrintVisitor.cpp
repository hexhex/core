/* -*- C++ -*- */

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

PrintVisitor::PrintVisitor(std::ostream& s)
  : stream(s)
{ }


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
      for (RuleHead_t::const_iterator hl = r->getHead().begin();
	   hl != r->getHead().end() - 1;
	   ++hl)
	{
	  (*hl)->accept(*this);
	  stream << " v ";
	}

      r->getHead().back()->accept(*this);
    }

  if (!r->getBody().empty()) // output body with leading " :- "
    {
      stream << " :- ";
            
      for (RuleBody_t::const_iterator l = r->getBody().begin();
	   l != r->getBody().end() - 1;
	   ++l)
	{
	  (*l)->accept(*this);
	  stream << ", ";
	}

      r->getBody().back()->accept(*this);
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

  if (a->getArity() > 1)
    {
      stream << '(';
            
      for (unsigned i = 1; i < a->getArity() - 1; i++)
	{
	  stream << a->getArgument(i) << ',';
	}
      
      stream << a->getArgument(a->getArity() - 1) << ')';
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
      for (RuleBody_t::const_iterator l = aa->getBody().begin();
	   l != aa->getBody().end() - 1;
	   ++l)
        {
	  (*l)->accept(*this);
	  stream << ", ";
        }

      aa->getBody().back()->accept(*this);
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
            
      for (RuleBody_t::const_iterator l = wc->getBody().begin();
	   l != wc->getBody().end() - 1;
	   ++l)
	{
	  (*l)->accept(*this);
	  stream << ", ";
	}

      wc->getBody().back()->accept(*this);

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

      stream << "a_" << a->getArity() - 1 << '(';
            
      for (unsigned i = 0; i < a->getArity() - 1; i++)
        {
	  stream << a->getArgument(i) << ',';
        }
        
      stream << a->getArgument(a->getArity() - 1) << ')';
    }
  else // FO mode is always raw
    {
      RawPrintVisitor v(stream);
      v.visitAtom(a);
    }
}
