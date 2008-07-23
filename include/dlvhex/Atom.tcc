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
 * @file Atom.tcc
 * @author Roman Schindlauer
 * @author Thomas Krennwallner
 * @author Alessandra Martello
 * @date Mon Sep  5 17:09:33 CEST 2005
 *
 * @brief Atom class.
 *
 *
 */


#if !defined(_DLVHEX_ATOM_TCC)
#define _DLVHEX_ATOM_TCC

#include "dlvhex/BaseVisitor.h"

#include <algorithm>

#include <boost/tokenizer.hpp>

DLVHEX_NAMESPACE_BEGIN


template<typename T> template<typename U>
Atom<T>::Atom(const Atom<U>& atom2)
  : arguments(atom2.arguments)
{ }


template<typename T> template<typename U>
Atom<T>&
Atom<T>::operator=(const Atom<U>& atom2)
{
  if (this != &atom2)
    {
      arguments = atom2.arguments;
    }
  return *this;
}


template<typename T>
Atom<T>::Atom(const std::string& atom)
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
    }
  else
    {
      //
      // can only be propositional
      //
      arguments.push_back(Term(atom));
    }
}


template<typename T>
Atom<T>::Atom(const Term& pred, const Tuple& arg)
{
  // predicate
  arguments.push_back(pred);

  // arguments
  arguments.insert(arguments.end(), arg.begin(), arg.end());
}
	

template<typename T>
Atom<T>::Atom(const Tuple& arg)
  : arguments(arg)
{
  assert(!arg.empty());
}
	

template<typename T>
const Term&
Atom<T>::getPredicate() const
{
  return arguments[0];
}


template<typename T>
void
Atom<T>::setArguments(const Tuple& nargs)
{
  assert(!arguments.empty());

  arguments.resize(1); // just keep the predicate

  // copy nargs to the 2nd position in arguments
  arguments.insert(arguments.end(), 
		   nargs.begin(),
		   nargs.end()
		   );
}


template<typename T>
const Term&
Atom<T>::operator[] (unsigned index) const
{
  assert(index < arguments.size());
  return arguments[index];
}


template<typename T>
Term&
Atom<T>::operator[] (unsigned index)
{
  assert(index < arguments.size());
  return arguments[index];
}


template<typename T>
void
Atom<T>::setPredicate(const Term& term2)
{
  arguments[0] = term2;
}


template<typename T>
unsigned
Atom<T>::getArity() const
{
  return arguments.size() - 1;
}


template<typename T>
bool
Atom<T>::unifiesWith(const BaseAtom& atom2) const
{
  //
  // atoms only unify with atoms with the same arity
  //
  if (typeid(atom2) == typeid(*this) && getArity() == atom2.getArity())
    {	
      // this atom unifies with atom2 iff all arguments unify pairwise
      return std::equal(getArguments().begin(), getArguments().end(),
			atom2.getArguments().begin(),
			std::mem_fun_ref(&Term::unifiesWith));
    }

  return false;
}


template<typename T>
bool
Atom<T>::isGround() const
{
  // an atom is ground if all arguments are not variables
  return arguments.end() == std::find_if(arguments.begin(),
					 arguments.end(),
					 std::mem_fun_ref(&Term::isVariable));
}


template<typename T>
void
Atom<T>::accept(BaseVisitor* const v)
{
  v->visit(this);
}



template<typename T>
int
Atom<T>::compare(const BaseAtom& atom2) const
{
  const std::type_info& type1 = typeid(*this);
  const std::type_info& type2 = typeid(atom2);

  if (type1 == type2)
    {
      int n = getPredicate().compare(atom2.getPredicate());

      if (n == 0)
	{
	  unsigned s1 = this->getArity();
	  unsigned s2 = atom2.getArity();

	  if (s1 == s2)
	    {
	      std::pair<Tuple::const_iterator, Tuple::const_iterator> mit =
		std::mismatch(++this->getArguments().begin(),
			      this->getArguments().end(),
			      ++atom2.getArguments().begin());

	      if (mit.first != this->getArguments().end()) //otw. n is already 0
		{
		  n = mit.first < mit.second ? -1 : 1;
		}
	    }
	  else
	    {
	      n = s1 - s2;
	    }
	}

      return n;
    }

  //
  // equal predicates, different arities - may only happen for
  // variable predicates!
  //
  ///@todo remove this check for now: causes problems for temporary
  /// auxiliary atoms, like the flp_heads, that are created with a
  /// suffixed index, based on the rule-index. Thus, in a different
  /// FLP-check, the same Atom-name could be used for a different
  /// arity.

  return type1.before(type2) ? -1 : 1;
}

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_ATOM_TCC */

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
