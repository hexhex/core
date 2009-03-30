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
 * @file AtomNode.cpp
 * @author Roman Schindlauer
 * @date Mon Jan 30 14:51:15 CET 2006
 *
 * @brief AtomNode class.
 *
 *
 */

#include "dlvhex/AtomNode.h"
#include "dlvhex/globals.h"
#include "dlvhex/Registry.h"


DLVHEX_NAMESPACE_BEGIN

unsigned AtomNode::nodeCount = 0;

/*
AtomNode::AtomNode()
	: atom(AtomPtr()),
	  inHead(0),
	  inBody(0),
	  nodeId(nodeCount++)
{
}
*/


AtomNode::AtomNode(const AtomPtr& atom)
    : atom(atom),
      inHead(false),
      inBody(false),
      auxFlag(false)
{
    ///@todo here, we increase the nodecounter and assign it to the
    ///node id. can we be sure that every time a new node is created -
    ///and only then! - this constructor is called? anyway, having a
    ///static counter for the id's is maybe too restrictive.
    nodeId = nodeCount++;
}


void
AtomNode::setHead()
{
	inHead = true;
}


void
AtomNode::setBody()
{
	inBody = true;
}


void
AtomNode::setAux()
{
	auxFlag = true;
}

bool
AtomNode::isHead() const
{
	return inHead;
}


bool
AtomNode::isBody() const
{
	return inBody;
}

bool
AtomNode::isAux() const
{
	return auxFlag;
}


void
AtomNode::addPreceding(const Dependency& dep)
{
	rules.clear(); // start creating rules in AtomNode::getRules
	preceding.insert(dep);
}


void
AtomNode::addSucceeding(const Dependency& dep)
{
	succeeding.insert(dep);
}


const AtomPtr&
AtomNode::getAtom() const
{
	return atom;
}


const std::set<Dependency>&
AtomNode::getPreceding() const
{
	return preceding;
}


const std::set<Dependency>&
AtomNode::getSucceeding() const
{
	return succeeding;
}


const std::vector<Rule*>&
AtomNode::getRules() const
{
	//
	// only start the rule-creation machinery if this AtomNode is a
	// head-node and the cache is still empty
	//
	if (this->rules.empty() && isHead()) // we call getRules for the very first time
	{
		typedef std::set<Rule*> SetOfRules;
		SetOfRules ruleset;

		for (std::set<Dependency>::const_iterator d = getPreceding().begin();
				d != getPreceding().end(); ++d)
		{
			Dependency::Type deptype = d->getType();

			//
			// if deptype is none of DISJ, PREC, or NEG_PREC, we
			// skip it and continue our search for rule candidates
			// only these types of dependency stem from actual rules
			//
			if (deptype & ~(Dependency::DISJUNCTIVE | Dependency::PRECEDING | Dependency::NEG_PRECEDING))
			{
				continue; // we only take care of head or body dependencies
			}

			// try to create a new rule in the ruleset
			ruleset.insert(d->getRule());
		}

		// and now add the fresh rules to our own "rule cache"
		std::copy(ruleset.begin(), ruleset.end(), std::inserter(this->rules, this->rules.begin()));
	}

	return this->rules;
}


unsigned
AtomNode::getId() const
{
	return nodeId;
}


std::ostream& operator<< (std::ostream& out, const AtomNode& atomnode)
{
	out << atomnode.getId() << ": ";

	const AtomPtr& ap = atomnode.getAtom();

	out << *ap;

	if (typeid(*ap) == typeid(ExternalAtom))
	{
		const ExternalAtom* ea =  static_cast<ExternalAtom*>(ap.get());
		out << " " <<  ea->getReplacementName() << " ";
	}

	if (atomnode.getPreceding().size() > 0)
	{
		for (std::set<Dependency>::const_iterator d = atomnode.getPreceding().begin();
			d != atomnode.getPreceding().end();
			++d)
		{
			out << std::endl << "  depends on: " << *d;
		}
	}

	if (atomnode.getSucceeding().size() > 0)
	{
		for (std::set<Dependency>::const_iterator d = atomnode.getSucceeding().begin();
			d != atomnode.getSucceeding().end();
			++d)
		{
			out << std::endl << "  dependents: " << *d;
		}
	}

	/*
	 * let each dependency dump its rule separately
	 *
	std::vector<Rule*> rules = atomnode.getRules();

	if (rules.size() > 0)
		out << std::endl << "  rules:";

	for (std::vector<Rule*>::const_iterator ri = rules.begin(); ri != rules.end(); ++ri)
	{
		out << " " << *(*ri);
	}
	*/
	
	return out;
}



DLVHEX_NAMESPACE_END

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
