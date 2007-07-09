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
 * @file   ExternalAtom.cpp
 * @author Roman Schindlauer
 * @date   Wed Sep 21 19:45:11 CEST 2005
 * 
 * @brief  External class atom.
 * 
 * 
 */

#include <assert.h>
#include <string>
#include <iostream>
#include <sstream>
#include <iterator>

#include "dlvhex/PluginContainer.h"
#include "dlvhex/PluginInterface.h"
#include "dlvhex/ExternalAtom.h"
#include "dlvhex/Error.h"
#include "dlvhex/Registry.h"
#include "dlvhex/BaseVisitor.h"

ExternalAtom::ExternalAtom()
{
}



ExternalAtom::ExternalAtom(const ExternalAtom& extatom)
	: Atom(extatom.arguments),
	  inputList(extatom.inputList),
	  functionName(extatom.functionName),
	  auxPredicate(extatom.auxPredicate),
	  replacementName(extatom.replacementName),
	  line(extatom.line),
	  pluginAtom(extatom.pluginAtom)
{
//	  type = extatom.type;
}



ExternalAtom::ExternalAtom(const std::string& name,
						   const Tuple& params,
						   const Tuple& input,
						   const unsigned line)
	: Atom(name,params),
	  inputList(input),
	  functionName(name),
	  extAtomNo(uniqueNumber),
	  line(line),
	  pluginAtom(0)
{
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
	ss << functionName << "_" << extAtomNo;
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

void
ExternalAtom::findPluginAtom() const
{
	if (this->pluginAtom) return;

	std::ostringstream errorstr;

	//
	// first, get respective PluginAtom object
	//
	this->pluginAtom = PluginContainer::Instance()->getAtom(functionName);
	
	if (pluginAtom == 0)
		throwSourceError("function " + functionName + " unknown");

	//
	// is the desired arity equal to the parsed arity?
	//
	if (!this->pluginAtom->checkInputArity(inputList.size()))
		throwSourceError("input arity mismatch in function " + functionName);
	
	if (!this->pluginAtom->checkOutputArity(getArity()))
		throwSourceError("output arity mismatch in function " + functionName);
	
	bool inputIsGround(1);

	for (unsigned s = 0; s < inputList.size(); s++)
	{
		if (inputList[s].isVariable())
		{
			inputIsGround = 0;

			//
			// at the moment, we don't allow variable predicate input arguments:
			//
			if (this->pluginAtom->getInputType(s) == PluginAtom::PREDICATE)
			{
				errorstr << "Line " << line << ": "
						<< "Variable predicate input arguments not allowed (yet)";

				throw FatalError(errorstr.str());
			}
		}
	}
}


void
ExternalAtom::throwSourceError(const std::string& msg) const
{
   throw SyntaxError(msg, line);
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
  this->pluginAtom = 0;

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


PluginAtom::InputType
ExternalAtom::getInputType(unsigned idx) const
{
	// and now check if we have a new pluginatom to setup
	findPluginAtom();

	return pluginAtom->getInputType(idx);
}


void
ExternalAtom::groundInputList(const AtomSet& i,
							  std::vector<Tuple>& inputArguments) const
{
	//
	// first, we assume that we only take the original input list
	//
	inputArguments.push_back(inputList);

	//
	// did we create an auxiliary predicate before?
	//
	if (!auxPredicate.empty())
	{
		//
		// now that we know there are variable input arguments
		// (otherwise there wouldn't be such a dependency), we can start
		// over with the input list again and construct it from the
		// result of the auxiliary rules
		//
		inputArguments.clear();

		AtomSet arglist;

		//
		// get all the facts from i that match the auxiliary head atom
		// the arguments of those facts will be our input lists!
		//
		i.matchPredicate(auxPredicate, arglist);

		for (AtomSet::const_iterator argi = arglist.begin();
				argi != arglist.end();
				++argi)
		{
			inputArguments.push_back((*argi).getArguments());
		}
	}
}



void
ExternalAtom::evaluate(const AtomSet& i,
					   AtomSet& result) const
{
	// setup the pluginatom, may throw a syntax error
	findPluginAtom();

	std::vector<Tuple> inputArguments;

	groundInputList(i, inputArguments);

	std::string fnc(getFunctionName());

	//
	// evaluate external atom for each input tuple we have now
	//
	for (std::vector<Tuple>::const_iterator inputi = inputArguments.begin();
			inputi != inputArguments.end();
			++inputi)
	{
		AtomSet inputSet;

		//
		// extract input set from i according to the input parameters
		//
		for (unsigned s = 0; s < (*inputi).size(); s++)
		{
			const Term* inputTerm = &(*inputi)[s];

			//
			// at this point, the entire input list must be ground!
			//
			assert(!inputTerm->isVariable());

			switch(pluginAtom->getInputType(s))
			{
			case PluginAtom::CONSTANT:

				//
				// nothing to do, the constant will be passed directly to the plugin
				//

				break;

			case PluginAtom::PREDICATE:

				//
				// collect all facts from interpretation that we need for the input
				// of the external atom
				//


				/// @todo: since matchpredicate doesn't neet the output list, do we
				// need that factlist here?
				i.matchPredicate(inputTerm->getString(), inputSet);

				break;

			default:

				assert(0);

				break;
			}
		}

		//
		// build a query object:
		// - interpretation
		// - input list
		// - actual arguments of the external atom (maybe it is partly ground,
		// then the plugin can be more efficient)
		//
		PluginAtom::Query query(inputSet, *inputi, getArguments());

		PluginAtom::Answer answer;
		
		try
		{
			pluginAtom->retrieve(query, answer);
		}
		catch (PluginError& e)
		{
			std::ostringstream atomstr;

			atomstr << functionName << "[" << 
					inputList << "](" << getArguments() << ")" <<
					" in line " << line;

			e.setContext(atomstr.str());

			throw e;
		}
		

		//
		// build result with the replacement name for each answer tuple
		//
		for (std::vector<Tuple>::const_iterator s = (*answer.getTuples()).begin();
			 s != (*answer.getTuples()).end();
			 ++s)
		{
			//
			// the replacement atom contains both the input and the output list!
			// (*inputi must be ground here, since it comes from
			// groundInputList(i, inputArguments))
			//
			Tuple resultTuple(*inputi);

//			std::cerr << "got back: " << resultTuple << std::endl;

			//
			// add output list
			//
			resultTuple.insert(resultTuple.end(), (*s).begin(), (*s).end());

			AtomPtr ap(new Atom(getReplacementName(), resultTuple));

			result.insert(ap);
		}
	}
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
	//std::cout << "extatom equals: " << *this << " equals " << *atom2 << "?" << std::endl;

	if (typeid(*atom2) != typeid(ExternalAtom))
		return 0;

	ExternalAtom* ea = dynamic_cast<ExternalAtom*>(atom2.get());

	if (this->replacementName != ea->replacementName)
		return 0;

	if (this->functionName != ea->functionName)
		return 0;

	if (this->getArity() != ea->getArity())
		return 0;
	
	if (this->isStrongNegated != ea->isStrongNegated)
		return 0;

	if (this->inputList.size() != ea->inputList.size())
		return 0;

	for (unsigned i = 0; i <= this->getArity(); i++)
	{
		if (this->getArgument(i) != ea->getArgument(i))
			return 0;
	}
	
	for (unsigned i = 0; i < this->inputList.size(); i++)
	{
		if (this->inputList[i] != ea->inputList[i])
			return 0;
	}

	return 1;
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


/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
