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
 * @file NodeGraph.cpp
 * @author Roman Schindlauer
 * @date Mon Jan 30 14:51:15 CET 2006
 *
 * @brief NodeGraph classes.
 *
 *
 */

#include "dlvhex/NodeGraph.h"
#include "dlvhex/AtomNode.h"
#include "dlvhex/globals.h"
#include "dlvhex/Registry.h"

DLVHEX_NAMESPACE_BEGIN


NodeGraph::~NodeGraph()
{ }


const Program&
NodeGraph::getProgram() const
{
  ///@todo this looks a bit weird
  if (this->prog.empty())
    {
      for (std::vector<AtomNodePtr>::const_iterator it = atomNodes.begin();
	   it != atomNodes.end(); ++it)
	{
	  const Program& p = (*it)->getRules();
	  prog.insert(prog.end(), p.begin(), p.end());
	}
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
	AtomNodePtr newnode = findNode(atom);

	if (!newnode)
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

			if ((*oldnode)->getAtom()->unifiesWith(*atom))
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
				  Dependency dep1(RulePtr(), *oldnode, Dependency::UNIFYING);
				  Dependency dep2(RulePtr(), newnode, Dependency::UNIFYING);

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
	AtomNodePtr newnode = findNode(atom);

	if (!newnode)
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
			if ((*oldnode)->getAtom()->unifiesWith(*atom))
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
				  Dependency dep1(RulePtr(), *oldnode, Dependency::UNIFYING);
				  Dependency dep2(RulePtr(), newnode, Dependency::UNIFYING);

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


AtomNodePtr
NodeGraph::findNode(const AtomPtr& atom) const
{
  //
  // test if atom does already exist as an AtomNode and return its pointer, if
  // this is the case
  //
  for (std::vector<AtomNodePtr>::const_iterator an = atomNodes.begin();
       an != atomNodes.end();
       ++an)
    {
      if (*atom.get() == *(*an)->getAtom())
	{
	  return *an;
	}
    }

  return AtomNodePtr();
}


DLVHEX_NAMESPACE_END

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
