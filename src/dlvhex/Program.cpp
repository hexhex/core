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
 * @file Program.cpp
 * @author Roman Schindlauer
 * @date Tue Mar  7 16:51:41 CET 2006
 *
 * @brief Program class.
 *
 *
 *
 */


#include "dlvhex/Program.h"

#include <iostream>


DLVHEX_NAMESPACE_BEGIN

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

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
