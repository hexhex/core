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
 * @file AtomNode.cpp
 * @author Roman Schindlauer
 * @date Mon Jan 30 14:51:15 CET 2006
 *
 * @brief AtomNode, Dependency and NodeGraph classes.
 *
 *
 */

#include "dlvhex/AtomNode.h"
#include "dlvhex/globals.h"
#include "dlvhex/Registry.h"

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


AtomNode::AtomNode(const AtomPtr& atom = AtomPtr())
    : atom(atom),
      inHead(0),
      inBody(0)
{
    //
    // TODO: here, we increase the nodecounter and assign it to the node id.
    // can we be sure that every time a new node is created - and only then! -
    // this constructor is called?
    //
    nodeId = nodeCount++;
}


void
AtomNode::setHead()
{
	inHead = 1;
}


void
AtomNode::setBody()
{
	inBody = 1;
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

	out << *(atomnode.getAtom());

	if (typeid(*(atomnode.getAtom())) == typeid(ExternalAtom))
	{
		const ExternalAtom* ea =  dynamic_cast<ExternalAtom*>(atomnode.getAtom().get());
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



Dependency::Dependency()
{
}


Dependency::Dependency(const Dependency& dep2)
	: atomNode(dep2.atomNode),
	  type(dep2.type),
	  rule(dep2.rule)
{
}


Dependency::Dependency(Rule* r, const AtomNodePtr& an, Type t)
  : atomNode(an), type(t), rule(r)
{
}


Dependency::Type
Dependency::getType() const
{
	return type;
}


const AtomNodePtr&
Dependency::getAtomNode() const
{
	assert(atomNode);

	return atomNode;
}


Rule*
Dependency::getRule() const
{
	return rule;
}

void
Dependency::addDep(Rule* rule, const AtomNodePtr& from, const AtomNodePtr& to, Dependency::Type type)
{
	Dependency dep1(rule, from, type);
	Dependency dep2(rule, to, type);

	from->addSucceeding(dep2);
	to->addPreceding(dep1);
}


bool
Dependency::operator< (const Dependency& dep2) const
{
	if (this->rule < dep2.rule)
		return true;
	if (this->rule > dep2.rule)
		return false;

	if (this->atomNode->getId() < dep2.atomNode->getId())
		return true;
	if (this->atomNode->getId() > dep2.atomNode->getId())
		return false;

	if (this->type < dep2.type)
		return true;
	if (this->type > dep2.type)
		return false;

	return false;
}


std::ostream& operator<< (std::ostream& out, const Dependency& dep)
{
	out << *(dep.getAtomNode()->getAtom());
	if (typeid(*(dep.getAtomNode()->getAtom())) == typeid(ExternalAtom))
	{
		const ExternalAtom* ea =  dynamic_cast<ExternalAtom*>(dep.getAtomNode()->getAtom().get());
		out << " " <<  ea->getReplacementName() << " ";
	}


	out << " [";

	switch (dep.getType())
	{
	case Dependency::UNIFYING:
		out << "unifying";
		break;

	case Dependency::PRECEDING:
		out << "head-body";
		break;

	case Dependency::NEG_PRECEDING:
		out << "head-body NAF";
		break;

	case Dependency::DISJUNCTIVE:
		out << "disjunctive";
		break;

	case Dependency::EXTERNAL:
		out << "external";
		break;

	case Dependency::EXTERNAL_AUX:
		out << "external aux";
		break;

	default:
		assert(0);
		break;
	}

	out << "]";
	
	if (dep.getRule() != 0)
		out << " rule: " << *(dep.getRule());

	return out;
}



NodeGraph::~NodeGraph()
{
	/*
	for (std::vector<AtomNodePtr>::const_iterator an = atomNodes.begin();
		 an != atomNodes.end();
		 ++an)
	{
		delete *an;
	}
	*/
}


const std::vector<Rule*>&
NodeGraph::getProgram() const
{
  if (this->prog.empty())
	{
	  std::set<Rule*> ruleset;
	  
	  for (std::vector<AtomNodePtr>::const_iterator it = atomNodes.begin();
	   it != atomNodes.end(); ++it)
	{
	  std::vector<Rule*> rules = (*it)->getRules();
	  std::copy(rules.begin(), rules.end(), std::inserter(ruleset, ruleset.begin()));
	}

	  std::copy(ruleset.begin(), ruleset.end(), std::inserter(this->prog, this->prog.begin()));
	}

  return this->prog;
}


const std::vector<AtomNodePtr>&
NodeGraph::getNodes() const
{
	return atomNodes;
}


void
NodeGraph::reset()
{
  atomNodes.clear();
  prog.clear();
}


/*
const AtomNodePtr
NodeGraph::getNode(unsigned nodeId)
{
	for (std::vector<AtomNodePtr>::const_iterator an = atomNodes.begin();
		 an != atomNodes.end();
		 ++an)
	{
		if ((*an)->getId() == nodeId)
			return *an;
	}
}
*/


AtomNodePtr
NodeGraph::addNode()
{
	//
	// create node
	//
	AtomNodePtr newnode(new AtomNode);

	//
	// add the new node to the graph
	//
	atomNodes.push_back(newnode);

	return newnode;
}


AtomNodePtr
NodeGraph::addUniqueHeadNode(const AtomPtr& atom)
{
	//
	// does a node with exactly this atom already exist?
	// (same predicate, same arguments)
	//
	//AtomNodePtr newnode = findNode(atom);
	AtomNodePtr newnode;
	findNode(atom, newnode);


	if (newnode.use_count() == 0)
	{
		//
		// no - create node
		//
		newnode = AtomNodePtr(new AtomNode(atom));

		//std::cout << "new headnode: " << *(newnode->getAtom()) << std::endl;

		//
		// add the new node to the graph
		//
		atomNodes.push_back(newnode);
	}

	//
	// if the node occurred in a head the first time (or was just created - in
	// this case it is neither a head nor a body node yet), we have to update
	// dependencies by looking through the existing nodes
	//
	if (!(newnode->isHead()))
	{
		//std::cout << "headnode: " << *(newnode->getAtom()) << std::endl;
		
		//
		// search all existing nodes for an atom that unifies
		// with this new one - then we can add the unifying-dependency to both
		//
		for (std::vector<AtomNodePtr>::const_iterator oldnode = atomNodes.begin();
			 oldnode != atomNodes.end();
			 ++oldnode)
		{
			//
			// if the node already existed (as a body atom), we might encounter
			// it here again - test for equality
			// if equal, take next one
			//
			if (*oldnode == newnode)
				continue;

			if ((*oldnode)->getAtom()->unifiesWith(atom))
			{
				//
				// in this function, we only search for existing BODY atoms!
				//
				if ((*oldnode)->isBody())
				{
					//
					// add only one dependency: from the new node to the
					// existing node. The existing node is a body atom and the
					// new one is obviously a head atom (otherwise we would not
					// be in that function), so the dependency goes from the
					// head into the body.
					//
					///@todo is this rule-id correct?
					Dependency dep1(0, *oldnode, Dependency::UNIFYING);
					Dependency dep2(0, newnode, Dependency::UNIFYING);

					(*oldnode)->addPreceding(dep2);
					newnode->addSucceeding(dep1);

					//std::cout << " unifies with " << *((*oldnode)->getAtom()) << std::endl;
				}
			}
		}

	}

	//
	// wherever this node occured before - now it is a head node!
	// 
	newnode->setHead();

	return newnode;
}


AtomNodePtr
NodeGraph::addUniqueBodyNode(const AtomPtr& atom)
{
	//
	// does a node with exactly this atom already exist?
	// (same predicate, same arguments)
	//
	//std::cout << "==trying to add bodynode: " << *atom << std::endl;
	AtomNodePtr newnode;
	findNode(atom, newnode);

	if (newnode.use_count() == 0)
	{
		//
		// no - create node
		//
		newnode = AtomNodePtr(new AtomNode(atom));
		
		//std::cout << "new bodynode: " << *(newnode->getAtom()) << std::endl;

		//
		// set this node to be a body node - but only if we just created it!
		//
		newnode->setBody();
		
		//
		// search for all existing nodes if an atom exists that unifies
		// with this new one - then we can add the unifying-dependency to both
		//
		for (std::vector<AtomNodePtr>::const_iterator oldnode = atomNodes.begin();
			 oldnode != atomNodes.end();
			 ++oldnode)
		{
			if ((*oldnode)->getAtom()->unifiesWith(atom))
			{
				//
				// in this function, we only search for existing HEAD atoms!
				//
				if ((*oldnode)->isHead())
				{
					//
					// add only one dependency: from the existing node to the
					// new node. The existing node is a head atom and the new
					// one is obviously a body atom (otherwise we would not be
					// in that function), so the dependency goes from the head
					// into the body.
					//
					///@todo is this rule-id correct?
					Dependency dep1(0, *oldnode, Dependency::UNIFYING);
					Dependency dep2(0, newnode, Dependency::UNIFYING);

					(*oldnode)->addSucceeding(dep2);
					newnode->addPreceding(dep1);

					//std::cout << " unifies with " << *((*oldnode)->getAtom()) << std::endl;
				}
			}
		}

		//
		// add the new node to the graph
		//
		atomNodes.push_back(newnode);
	}

	return newnode;
}


void
NodeGraph::findNode(const AtomPtr& atom, AtomNodePtr& ptr) const
{
	//
	// test if atom does already exist as an AtomNode and return its pointer, if
	// this is the case
	//
	for (std::vector<AtomNodePtr>::const_iterator an = atomNodes.begin();
		 an != atomNodes.end();
		 ++an)
	{
		if (atom->equals((*an)->getAtom()))
		//if (*atom == *(*an)->getAtom())
		{
			ptr = *an;
			return;
		}
	}
}


/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
