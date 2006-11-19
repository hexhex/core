/* -*- C++ -*- */

/**
 * @file Program.cpp
 * @author Roman Schindlauer
 * @date Tue Mar  7 16:51:41 CET 2006
 *
 * @brief Program class.
 *
 *
 *
 */


#include <iostream>

#include "dlvhex/Program.h"


Program::Program()
{
}


void
Program::addRule(Rule* r)
{
	rules.insert(r);

	//
	// store the rule's external atoms also separately
	//
	for (RuleBody_t::const_iterator bi = r->getBody().begin();
			bi != r->getBody().end();
			++bi)
	{
		if (typeid(*(*bi)->getAtom()) == typeid(ExternalAtom))
			externalAtoms.push_back(dynamic_cast<ExternalAtom*>((*bi)->getAtom().get()));
	}
}


void
Program::deleteRule(iterator i)
{
	rules.erase(i.it);
}


void
Program::addWeakConstraint(WeakConstraint* wc)
{
	///todo don't add the same wc twice!

	rules.insert(wc);

	weakConstraints.push_back(wc);
}


const std::vector<WeakConstraint*>&
Program::getWeakConstraints() const
{
	return weakConstraints;
}


const std::vector<ExternalAtom*>&
Program::getExternalAtoms() const
{
	return externalAtoms;
}


void
Program::dump(PrintVisitor& v) const
{
	for (Program::const_iterator r = begin();
			r != end();
			++r)
	{
		(*r)->accept(v);
		v.getStream() << std::endl;
	}
}
