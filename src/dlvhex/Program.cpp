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
#include "dlvhex/Error.h"
#include "dlvhex/BaseVisitor.h"

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

Program::Program():
	higherOrder(false), aggregateAtoms(false)
{
}

void
Program::addRule(Rule* r)
{
	rules.insert(r);

	higherOrder |= r->isHigherOrder();
	aggregateAtoms |= r->hasAggregateAtoms();
	externalAtoms.insert(externalAtoms.end(),
			r->getExternalAtoms().begin(),
			r->getExternalAtoms().end());

	// in higher-order mode we cannot have aggregates, because then they would
	// almost certainly be recursive, because of our atom-rewriting!
	if( higherOrder && aggregateAtoms )
		throw SyntaxError("Aggregates and Higher Order Constructions must not be used together");
}


void
Program::deleteRule(iterator i)
{
	rules.erase(i.it);

	/// @todo: if this is used often, we have a problem: we have to check all rules for higher order-ness (we should instead have 'deleteRules(set<iterator>)' or something similar) (this method is only used by plugins)

	higherOrder = false;
	aggregateAtoms = false;
	externalAtoms.clear();

	BOOST_FOREACH(Rule* rule, rules)
	{
		higherOrder |= rule->isHigherOrder();
		aggregateAtoms |= rule->hasAggregateAtoms();
		externalAtoms.insert(externalAtoms.end(),
				rule->getExternalAtoms().begin(),
				rule->getExternalAtoms().end());
	}
	BOOST_FOREACH(WeakConstraint* wc, weakConstraints)
	{
		higherOrder |= wc->isHigherOrder();
		aggregateAtoms |= wc->hasAggregateAtoms();
		externalAtoms.insert(externalAtoms.end(),
				wc->getExternalAtoms().begin(),
				wc->getExternalAtoms().end());
	}
}


void
Program::addWeakConstraint(WeakConstraint* wc)
{
	///@todo don't add the same wc twice!

	rules.insert(wc);
	weakConstraints.push_back(wc);

	///@todo added 'externalAtoms.insert' because it was missing, not sure if this introduces a bug
	higherOrder |= wc->isHigherOrder();
	aggregateAtoms |= wc->hasAggregateAtoms();
	externalAtoms.insert(externalAtoms.end(),
			wc->getExternalAtoms().begin(),
			wc->getExternalAtoms().end());

	// in higher-order mode we cannot have aggregates, because then they would
	// almost certainly be recursive, because of our atom-rewriting!
	if( higherOrder && aggregateAtoms )
		throw SyntaxError("Aggregates and Higher Order Constructions must not be used together");
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
Program::accept(BaseVisitor& v) const
{
  v.visit(this);
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
