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
 * @file Atom.cpp
 * @author Roman Schindlauer
 * @date Mon Sep  5 17:09:33 CEST 2005
 *
 * @brief Atom and GroundAtom class.
 *
 *
 */


#include "dlvhex/Atom.h"
#include "dlvhex/helper.h"
#include "dlvhex/Error.h"
#include "dlvhex/BaseVisitor.h"
#include "dlvhex/PrintVisitor.h"

#include <cassert>


DLVHEX_NAMESPACE_BEGIN

Atom::Atom()
{
}


Atom::~Atom()
{
}


Atom::Atom(const Atom& atom2)
	: ProgramObject(),
	  arguments(atom2.arguments),
	  isStrongNegated(atom2.isStrongNegated),
	  isAlwaysFO(atom2.isAlwaysFO)
{
}


Atom::Atom(const std::string& atom, bool neg)
	: isStrongNegated(neg),
	  isAlwaysFO(0)
{
	arguments.clear();
	
	std::string::size_type par;
	
	//
	// not propositional?
	//
	if ((par = atom.find("(", 0)) != std::string::npos)
	{
		std::vector<std::string> termlist = helper::stringExplode(atom.substr(par + 1, atom.length() - par - 2), ",");

		arguments.push_back(Term(atom.substr(0, par)));
		
		for (std::vector<std::string>::const_iterator g = termlist.begin();
			 g != termlist.end();
			 g++)
		{
			arguments.push_back(Term(*g));
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
			throw SyntaxError("propositional Atom must be ground. Probably not a HEX-program?");
	}
}
	

Atom::Atom(const std::string& pred, const Tuple& arg, bool neg)
	: isStrongNegated(neg),
	  isAlwaysFO(0)
{
	arguments.push_back(Term(pred));

	//
	// propositonal atom must be ground:
	//
	if ((arg.size() == 0) && arguments.front().isVariable())
		throw SyntaxError("propositional Atom must be ground");
	
	for (Tuple::const_iterator t = arg.begin(); t != arg.end(); ++t)
	{
		arguments.push_back(*t);
	}
}
	

Atom::Atom(const Tuple& arg, bool neg)
	: isStrongNegated(neg),
	  isAlwaysFO(0)
{
	if (arg.size() == 0)
		throw SyntaxError("Atom must contain at least one term");

	for (Tuple::const_iterator t = arg.begin(); t != arg.end(); ++t)
		arguments.push_back(*t);

	//
	// propositonal atom must be ground:
	//
	if ((arg.size() == 1) && arguments.front().isVariable())
		throw SyntaxError("propositional Atom must be ground");
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
	// atoms only unify with atoms
	//
	if (typeid(*atom2) != typeid(Atom))
		return false;

	if (getArity() != atom2->getArity())
		return false;
	
	bool ret = true;
	
	for (unsigned i = 0; i <= getArity(); i++)
	{
		if (!getArgument(i).unifiesWith(atom2->getArgument(i)))
			ret = false;
	}
	
	return ret;
}



bool
Atom::operator== (const Atom& atom2) const
{
	if (typeid(*this) != typeid(atom2))
		return 0;

	if (this->getArity() != atom2.getArity())
		return 0;

	if (this->isStronglyNegated() != atom2.isStronglyNegated())
		return 0;

	return std::equal(this->arguments.begin(), this->arguments.end(),
	                  atom2.arguments.begin());
}


bool
Atom::equals(const AtomPtr& atom2) const
{
	if (typeid(*this) != typeid(*atom2))
		return 0;

	if (this->getArity() != atom2->getArity())
		return 0;

	if (this->isStronglyNegated() != atom2->isStronglyNegated())
		return 0;

	return std::equal(this->arguments.begin(), this->arguments.end(),
	                  atom2->arguments.begin());
}


bool
Atom::operator!= (const Atom& atom2) const
{
	return !(*this == atom2);
}


void
Atom::setAlwaysFO()
{
	isAlwaysFO = 1;
}


bool
Atom::getAlwaysFO() const
{
	return isAlwaysFO;
}


void
Atom::accept(BaseVisitor& v) const
{
	v.visitAtom(this);
}


bool
Atom::isGround() const
{
	for (Tuple::const_iterator t = arguments.begin(); t != arguments.end(); t++)
	{
		if (t->isVariable())
				return false;
	}

	return true;
}


std::ostream&
operator<< (std::ostream& out, const Atom& atom)
{
	RawPrintVisitor rpv(out);
	atom.accept(rpv);
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
			//
			// remove this check for now: causes problems for temporary
			// auxiliary atoms, like the flp_heads, that are created with a
			// suffixed index, based on the rule-index. Thus, in a different
			// FLP-check, the same Atom-name could be used for a different
			// arity.
			//
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



BuiltinPredicate::BuiltinPredicate(const Term& t1, const Term& t2, const std::string& b)
{
	arguments.push_back(Term(b));
	arguments.push_back(t1);
	arguments.push_back(t2);
}


void
BuiltinPredicate::accept(BaseVisitor& v) const
{
  v.visitBuiltinPredicate(this);
}


DLVHEX_NAMESPACE_END

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
