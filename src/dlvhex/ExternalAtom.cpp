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
 * @date   Wed Sep 21 19:45:11 CEST 2005
 * 
 * @brief  External class atom.
 * 
 * 
 */

#include "dlvhex/PluginContainer.h"
#include "dlvhex/PluginInterface.h"
#include "dlvhex/ExternalAtom.h"
#include "dlvhex/Error.h"
#include "dlvhex/Registry.h"
#include "dlvhex/BaseVisitor.h"

#include <cassert>
#include <string>
#include <iostream>
#include <sstream>
#include <iterator>

DLVHEX_NAMESPACE_BEGIN

ExternalAtom::ExternalAtom()
{ }


ExternalAtom::ExternalAtom(const ExternalAtom& extatom)
	: Atom(extatom.arguments),
	  inputList(extatom.inputList),
	  functionName(extatom.functionName),
	  auxPredicate(extatom.auxPredicate),
	  replacementName(extatom.replacementName),
	  line(extatom.line)
{ }



ExternalAtom::ExternalAtom(const std::string& name,
			   const Tuple& params,
			   const Tuple& input,
			   const unsigned line)
  : Atom(name,params),
    inputList(input),
    functionName(name),
    extAtomNo(uniqueNumber),
    line(line)
{
  ///@todo global counters are always crap, this breaks replacement
  ///names, where the external atom is syntactically equal!!

	//
	// increase absolute extatom counter
	//
	uniqueNumber++;

	//
	// and now setup replacement name and so on
	//
	initReplAux();
}

void
ExternalAtom::initReplAux()
{
	//
	// make replacement name, unique for each extatom
	//
	std::stringstream ss;
	ss << "ex" << functionName << "_" << extAtomNo;
	replacementName = ss.str();

	//
	// remember this artificial atom name, we need to remove those later, they
	// shouldn't be in the actual result
	//
	Term::registerAuxiliaryName(replacementName);

	bool inputIsGround(1);

	for (unsigned s = 0; s < inputList.size(); s++)
		if (inputList[s].isVariable())
			inputIsGround = 0;

	//
	// build auxiliary predicate
	//
	auxPredicate.clear();

	if (!inputIsGround)
	{
		//
		// also produce auxiliary predicate name, we need this for the
		// nonground input list
		//
		ss << "_aux";
		auxPredicate = ss.str();
		Term::registerAuxiliaryName(auxPredicate);
	}
}



const std::string&
ExternalAtom::getAuxPredicate() const
{
	return auxPredicate;
}


void
ExternalAtom::setFunctionName(const std::string& name)
{
  this->functionName = name;
//   this->pluginAtom = boost::shared_ptr<PluginAtom>(); // reset pluginAtom

  // and now setup the new replacement and aux names
  initReplAux();
}


const std::string&
ExternalAtom::getFunctionName() const
{
	return functionName;
}


const std::string&
ExternalAtom::getReplacementName() const
{
	return replacementName;
}


bool
ExternalAtom::pureGroundInput() const
{
	for (Tuple::const_iterator ti = inputList.begin();
		 ti != inputList.end();
		 ++ti)
	{
		if ((*ti).isVariable())
			return 0;
	}

	return 1;
}


const Tuple&
ExternalAtom::getInputTerms() const
{
	return inputList;
}

void
ExternalAtom::setInputTerms(const Tuple& ninput)
{
	inputList.clear();
	
	// copy nargs to the 2nd position in inputList
	inputList.insert(inputList.end(), 
			 ninput.begin(),
			 ninput.end()
			 );

	///@todo new replacementname for nonground input?
}


bool
ExternalAtom::unifiesWith(const AtomPtr& /* atom */) const
{
	return 0;
}


bool
ExternalAtom::operator== (const ExternalAtom& atom2) const
{
	if (typeid(*this) != typeid(atom2))
		return 0;

	// hm, should we really check the replacement names? they are
	// always different for two different instances of ExternalAtom
	if (this->replacementName != atom2.replacementName)
		return 0;

	if (this->functionName != atom2.functionName)
		return 0;

	if (this->getArity() != atom2.getArity())
		return 0;
	
	if (this->isStrongNegated != atom2.isStrongNegated)
		return 0;

	if (this->inputList.size() != atom2.inputList.size())
		return 0;

	for (unsigned i = 0; i <= this->getArity(); i++)
	{
		if (this->getArgument(i) != atom2.getArgument(i))
			return 0;
	}
	
	for (unsigned i = 0; i < this->inputList.size(); i++)
	{
		if (this->inputList[i] != atom2.inputList[i])
			return 0;
	}

//	std::cout << "extcomp: " << *this << " equals " << atom2 << "!" << std::endl;
	return 1;
}


bool
ExternalAtom::equals(const AtomPtr& atom2) const
{
  bool ret = true;

  // don't forget, typeid gives the most derived type only if it is
  // applied to a non-pointer
  if (typeid(*atom2.get()) != typeid(ExternalAtom))
    {
      ret = false;
    }
  else
    {
      const ExternalAtom* ea = dynamic_cast<ExternalAtom*>(atom2.get());

      if (this->functionName != ea->functionName ||
	  this->getArity() != ea->getArity() ||
	  this->isStrongNegated != ea->isStrongNegated ||
	  this->inputList != ea->inputList ||
	  this->getArguments() != ea->getArguments()
	  )
	{
	  ret = false;
	}
    }

  return ret;
}


void
ExternalAtom::accept(BaseVisitor& v) const
{
	v.visitExternalAtom(this);
}


unsigned ExternalAtom::uniqueNumber = 0;


unsigned
ExternalAtom::getLine() const
{
	return line;
}

DLVHEX_NAMESPACE_END

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
