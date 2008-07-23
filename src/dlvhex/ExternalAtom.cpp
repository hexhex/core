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
 * @file   ExternalAtom.cpp
 * @author Roman Schindlauer
 * @author Thomas Krennwallner
 * @date   Wed Sep 21 19:45:11 CEST 2005
 * 
 * @brief  External class atom.
 * 
 * 
 */

#include "dlvhex/ExternalAtom.h"
#include "dlvhex/BaseVisitor.h"

#include <cassert>
#include <sstream>
#include <functional>

DLVHEX_NAMESPACE_BEGIN

ExternalAtom::ExternalAtom()
{ }


ExternalAtom::ExternalAtom(const ExternalAtom& extatom)
  : BaseAtom(),
    inputList(extatom.inputList),
    outputList(extatom.outputList),
    functionName(extatom.functionName),
    auxPredicate(extatom.auxPredicate),
    replacementName(extatom.replacementName)
{ }


ExternalAtom&
ExternalAtom::operator= (const ExternalAtom& extatom)
{
  if (this != &extatom)
    {
      inputList = extatom.inputList;
      outputList = extatom.outputList;
      functionName = extatom.functionName;
      auxPredicate = extatom.auxPredicate;
      replacementName = extatom.replacementName;
    }
  return *this;
}


ExternalAtom::ExternalAtom(const Term& name,
			   const Tuple& params,
			   const Tuple& input)
  : inputList(input),
    outputList(params),
    functionName(name)
{
  // setup replacement name and so forth
  initReplAux();
}


void
ExternalAtom::initReplAux()
{
  ///@todo if the user names a predicate exEXTATOM, we clash with
  ///these auxiliary names here

  //
  // make replacement name
  //
  std::ostringstream oss;
  oss << "ex" << functionName;

  replacementName = Term(oss.str());

  //
  // remember this artificial atom name, we need to remove those
  // later, they shouldn't be in the actual result
  //
  Term::registerAuxiliaryName(oss.str());

  //
  // build auxiliary predicate
  //
  auxPredicate = Term("");

  if (!this->pureGroundInput())
    {
      ///@todo this simple aux name does not work for nonground input!
      ///Maybe we can build a  map similar to the dl-atom rewriting in
      ///GraphBuilder?

      //
      // also produce auxiliary predicate name, we need this for the
      // nonground input list
      //
      oss << "_aux";
      auxPredicate = Term(oss.str());
    }
}



const Term&
ExternalAtom::getAuxPredicate() const
{
  return auxPredicate;
}


void
ExternalAtom::setAuxPredicate(const Term& auxname)
{
  auxPredicate = auxname;
}


void
ExternalAtom::setPredicate(const Term& fname)
{
  // set new functionName
  this->functionName = fname;

  // and now setup the new replacement and aux names
  initReplAux();
}
	

const Term&
ExternalAtom::getPredicate() const
{
  return functionName;
}



const Tuple&
ExternalAtom::getArguments() const
{
  return outputList;
}


const Term&
ExternalAtom::getReplacementName() const
{
  return replacementName;
}


bool
ExternalAtom::pureGroundInput() const
{
  ///@todo move outside
  // if we cannot find a variable input argument, we are ground
  return inputList.end() == std::find_if(inputList.begin(),
					 inputList.end(),
					 std::mem_fun_ref(&Term::isVariable));
}


bool
ExternalAtom::isGround() const
{
  // this atom unifies with atom2 iff all arguments unify pairwise
  return pureGroundInput() &&
    outputList.end() == std::find_if(outputList.begin(),
				     outputList.end(),
				     std::mem_fun_ref(&Term::isVariable));
}


void
ExternalAtom::setArguments(const Tuple& nargs)
{
  outputList = nargs;
}


const Term&
ExternalAtom::operator[] (unsigned i) const
{
  if (i)
    {
      assert(i < outputList.size());
      return outputList[i];
    }
  return functionName;
}


Term&
ExternalAtom::operator[] (unsigned i)
{
  if (i)
    {
      assert(i < outputList.size());
      return outputList[i];
    }
  return functionName;
}


unsigned
ExternalAtom::getArity() const
{
  return outputList.size();
}


const Tuple&
ExternalAtom::getInputList() const
{
  return inputList;
}


void
ExternalAtom::setInputList(const Tuple& ninput)
{
  inputList = ninput;
	
  ///@todo new replacementname for nonground input?
  ///@todo check parameter length?
}


bool
ExternalAtom::unifiesWith(const BaseAtom& atom2) const
{
  //
  // external atoms only unify with external atoms of the the same
  // name, and the same arity of input and output lists
  //
  if (typeid(atom2) == typeid(ExternalAtom))
    {
      const ExternalAtom& ea2 = static_cast<const ExternalAtom&>(atom2);

      if (functionName == ea2.functionName &&
	  inputList.size() == ea2.inputList.size() &&
	  outputList.size() == ea2.outputList.size())
	{
	  // both input and output lists must unify
	  return
	    std::equal(outputList.begin(), outputList.end(),
		       ea2.outputList.begin(),
		       std::mem_fun_ref(&Term::unifiesWith))
	    &&
	    std::equal(inputList.begin(), inputList.end(),
		       ea2.inputList.begin(),
		       std::mem_fun_ref(&Term::unifiesWith));
	}
    }

  return false;
}


int
ExternalAtom::compare (const BaseAtom& atom2) const
{
  // don't forget, typeid gives the most derived type only if it is
  // applied to a non-pointer
  const std::type_info& type1 = typeid(ExternalAtom);
  const std::type_info& type2 = typeid(atom2);

  if (type1 == type2)
    {
      const ExternalAtom& ea2 = static_cast<const ExternalAtom&>(atom2);

      int fcmp = functionName.compare(ea2.functionName);

      if (fcmp == 0)
	{
	  unsigned is1 = inputList.size();
	  unsigned is2 = ea2.inputList.size();

	  if (is1 == is2)
	    {
	      std::pair<Tuple::const_iterator, Tuple::const_iterator> ires =
		std::mismatch(inputList.begin(), inputList.end(),
			      ea2.inputList.begin());

	      if (ires.first == inputList.end())
		{
		  unsigned os1 = inputList.size();
		  unsigned os2 = ea2.inputList.size();

		  if (os1 == os2)
		    {
		      std::pair<Tuple::const_iterator, Tuple::const_iterator> ores =
			std::mismatch(outputList.begin(), outputList.end(),
				      ea2.outputList.begin());

		      return ores.first == outputList.end() ? 0 :
			ores.first->compare(*ores.second);
		    }

		  return os1 - os2;
		}
	    }

	  return is1 - is2;
	}

      return fcmp;
    }

  return type1.before(type2) ? -1 : 1;
}


void
ExternalAtom::accept(BaseVisitor* const v)
{
  v->visit(this);
}

DLVHEX_NAMESPACE_END

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
