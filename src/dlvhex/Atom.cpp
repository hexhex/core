/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
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
 * @file Atom.cpp
 * @author Roman Schindlauer
 * @date Mon Sep  5 17:09:33 CEST 2005
 *
 * @brief Atom and BuiltinPredicate class.
 *
 *
 */


#include "dlvhex/Atom.h"
#include "dlvhex/Error.h"
#include "dlvhex/BaseVisitor.h"
#include "dlvhex/PrintVisitor.h"

#include <cassert>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

// this constructor is used by BuiltinPredicate!
Atom::Atom(): arguments(), isStrongNegated(false)
{ }


Atom::~Atom()
{ }


Atom::Atom(const Atom& atom2)
  : ProgramObject(),
    arguments(atom2.arguments),
    isStrongNegated(atom2.isStrongNegated)
{ }


Atom::Atom(const std::string& atom, bool neg)
  : isStrongNegated(neg)
{
  std::string::size_type par;
	
  //
  // not propositional?
  //
  if ((par = atom.find("(", 0)) != std::string::npos)
    {
      // predicate
      arguments.push_back(Term(atom.substr(0, par)));

      // split arguments
      typedef boost::tokenizer<boost::char_separator<char> > septok;
      boost::char_separator<char> sep(",");
      const std::string& pars = atom.substr(par + 1, atom.length() - par - 2);
      septok tok(pars, sep);

      // arguments
      for (septok::const_iterator p = tok.begin(); p != tok.end(); ++p)
	{
	  arguments.push_back(Term(*p));
	}

      //
      // the predicate itself must be constant (also in ho-mode, then it will be a
      // constant replacement symbol)
      //
      assert(!arguments.front().isVariable());
    }
  else
    {
      //
      // can only be propositional
      //
      arguments.push_back(Term(atom));

      if (arguments.front().isVariable())
	{
	  throw SyntaxError("propositional Atom must be ground. Probably not a HEX-program?");
	}
    }
}
	

Atom::Atom(const std::string& pred, const Tuple& arg, bool neg)
  : isStrongNegated(neg)
{
  // predicate
  arguments.push_back(Term(pred));

  //
  // propositonal atom must be ground:
  //
  if ((arg.size() == 0) && arguments.front().isVariable())
    {
      throw SyntaxError("propositional Atom must be ground");
    }

  // arguments
  arguments.insert(arguments.end(), arg.begin(), arg.end());
}

Atom::Atom(const std::string& pred, const PTuple& arg, bool neg)
  : isStrongNegated(neg)
{
  // predicate
	arguments.reserve(1+arg.size());
  arguments.push_back(Term(pred));

  //
  // propositonal atom must be ground:
  //
  if ((arg.size() == 0) && arguments.front().isVariable())
    {
      throw SyntaxError("propositional Atom must be ground");
    }

  // arguments
	BOOST_FOREACH(Term* trm, arg)
	{
		arguments.push_back(*trm);
	}
}
	

Atom::Atom(const Tuple& arg, bool neg)
  : arguments(arg), isStrongNegated(neg)
{
  if (arguments.size() == 0)
    {
      throw SyntaxError("Atom must contain at least one term");
    }
  else if ((arguments.size() == 1) && arguments.front().isVariable())
    {
      // propositonal atom must be ground:
      throw SyntaxError("propositional Atom must be ground");
    }
}

Atom::Atom(const PTuple& arg, bool neg)
  : arguments(), isStrongNegated(neg)
{
  // arguments
	arguments.reserve(arg.size());
	BOOST_FOREACH(Term* trm, arg)
	{
		arguments.push_back(*trm);
	}
  if (arguments.size() == 0)
    {
      throw SyntaxError("Atom must contain at least one term");
    }
  else if ((arguments.size() == 1) && arguments.front().isVariable())
    {
      // propositonal atom must be ground:
      throw SyntaxError("propositional Atom must be ground");
    }
}
	

const Term&
Atom::getPredicate() const
{
	return getArgument(0);
}


void
Atom::setArguments(const Tuple& nargs)
{
        assert(!arguments.empty());

        arguments.resize(1); // just keep the predicate

	// copy nargs to the 2nd position in arguments
	arguments.insert(arguments.end(), 
			 nargs.begin(),
			 nargs.end()
			 );
}

const Term&
Atom::getArgument(unsigned index) const
{
	assert(index < arguments.size());
	return arguments[index];
}

void Atom::setArgument(unsigned index, const Term& term2)
{
	this->arguments[index] = term2;
}
void Atom::setPredicate(const Term& term2)
{
	setArgument(0, term2);
}

unsigned
Atom::getArity() const
{
	return arguments.size() - 1;
}


bool
Atom::isStronglyNegated() const
{
	return isStrongNegated;
}


bool
Atom::unifiesWith(const AtomPtr& atom2) const
{
  //
  // atoms only unify with atoms with the same arity
  //
  if (typeid(*atom2) == typeid(Atom) && getArity() == atom2->getArity())
    {	
      // this atom unifies with atom2 iff all arguments unify pairwise
      return std::equal(arguments.begin(), arguments.end(),
			atom2->arguments.begin(),
			std::mem_fun_ref(&Term::unifiesWith));
    }

  return false;
}



bool
Atom::operator== (const Atom& atom2) const
{
  bool ret = false;

  if (typeid(*this) == typeid(atom2) &&
      this->getArity() == atom2.getArity() &&
      this->isStronglyNegated() == atom2.isStronglyNegated())
    {
      ret = std::equal(this->arguments.begin(), this->arguments.end(),
		       atom2.arguments.begin());
    }

  return ret;
}


bool
Atom::operator!= (const Atom& atom2) const
{
	return !(*this == atom2);
}


void
Atom::accept(BaseVisitor& v) const
{
  v.visit(this);
}


bool
Atom::isGround() const
{
  // an atom is ground if no argument is a variable
  return arguments.end() == std::find_if(arguments.begin(), arguments.end(), std::mem_fun_ref(&Term::isVariable));
}


std::ostream&
operator<< (std::ostream& out, const Atom& atom)
{
  RawPrintVisitor rpv(out);
  const_cast<Atom*>(&atom)->accept(rpv);
  return out;
}


bool
Atom::operator< (const Atom& atom2) const
{
	int n = getPredicate().compare(atom2.getPredicate());

	if (n < 0)
		return true;
	if (n > 0)
		return false;

	if (!isStrongNegated && atom2.isStrongNegated)
		return true;
	else if (isStrongNegated && !atom2.isStrongNegated)
		return false;

	//
	// predicate symbols are equal, now distinguish between arguments
	//
	if (getArity() < atom2.getArity())
	{
		//
		// equal predicates, different arities - may only happen for variable
		// predicates!
		//
		if (!this->getPredicate().isVariable() && !atom2.getPredicate().isVariable())
		{
			///@todo remove this check for now: causes problems for temporary
			/// auxiliary atoms, like the flp_heads, that are created with a
			/// suffixed index, based on the rule-index. Thus, in a different
			/// FLP-check, the same Atom-name could be used for a different
			/// arity.

			//std::cerr << *this << " - " << atom2 << std::endl;
			//throw SyntaxError("arity mismatch");
		}

		return true;
	}

	// lexicographically compare on the arguments, i.e. skip the predicate of arguments
	if (getArity() == atom2.getArity())
	{
	// lexicographical_compare returns true if the range of
	// elements [first1, last1) is lexicographically less than the
	// range of elements [first2, last2), and false otherwise.
	return std::lexicographical_compare(++arguments.begin(), arguments.end(),
						++atom2.arguments.begin(), atom2.arguments.end());
	}

	// getArity() > atom2.getArity()
	return false;
}


bool
Atom::isHigherOrder() const
{
	assert(!arguments.empty());
	return arguments.front().isVariable();
}


BuiltinPredicate::BuiltinPredicate(const Term& t1, const std::string& b)
{
	arguments.push_back(Term(b));
	arguments.push_back(t1);
}

BuiltinPredicate::BuiltinPredicate(const Term& t1, const Term& t2, const std::string& b)
{
	arguments.push_back(Term(b));
	arguments.push_back(t1);
	arguments.push_back(t2);
}

BuiltinPredicate::BuiltinPredicate(const Term& t1, const Term& t2, const Term& t3, const std::string& b)
{
	arguments.push_back(Term(b));
	arguments.push_back(t1);
	arguments.push_back(t2);
	arguments.push_back(t3);
}


void
BuiltinPredicate::accept(BaseVisitor& v) const
{
  v.visit(this);
}

bool BuiltinPredicate::isInfix() const
{
  switch(getArity())
  {
    case 1:
      // #int is always prefix
      return false;
    case 2:
      if( arguments.front().isSymbol() && arguments.front().getString() == "#succ" )
        // #succ is always prefix
        return false;
      else
        return true;
    default:
      return true;
  }
}

DLVHEX_NAMESPACE_END

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
