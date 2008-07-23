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
 * @file NodeGraph.h
 * @author Roman Schindlauer
 * @date Mon Jan 30 14:46:36 CET 2006
 *
 * @brief NodeGraph class.
 *
 *
 */


#if !defined(_DLVHEX_NODEGRAPH_H)
#define _DLVHEX_NODEGRAPH_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/Program.h"
#include "dlvhex/AtomNode.h"

#include <boost/shared_ptr.hpp>


DLVHEX_NAMESPACE_BEGIN

/**
 * @brief Class for storing simple nodes of a dependency graph.
 *
 * A nodegraph is just a container for AtomNodes, taking care of creating and
 * deleting AtomNode objects. It provides specific functions for handling a set
 * of AtomNodes, like unique creation and finding a specific node.
 */
class DLVHEX_EXPORT NodeGraph
{
private:

	std::vector<AtomNodePtr> atomNodes;

	mutable Program prog;


public:

	/// Dtor.
	~NodeGraph();

	/**
	 * @brief returns the associated Program.
	 * @return prog
	 */
	const Program&
	getProgram() const;

	/**
	 * @brief Returns entire set of AtomNodes.
	 */
	const std::vector<AtomNodePtr>&
	getNodes() const;

	/**
	 * @brief Clears internal AtomNodes and Program.
	 */
	void
	reset();
  
	/**
	 * @brief Return a node with a specific AtomNode Id.
	 */
	//const AtomNodePtr
	//getNode(unsigned);

	AtomNodePtr
	addNode();

	/**
	 * @brief Create a new node for this head atom and return its pointer or
	 * return pointer to already existing node that matches this atom.
	 *
	 * If a new node is created, all existing nodes are searched for atoms that
	 * unify with the new one. If a node is found, whose atom occured in a
	 * rule's body, a unifying dependency from the new node to the found one is
	 * created.
	 */
	AtomNodePtr
	addUniqueHeadNode(const AtomPtr&);

	/**
	 * @brief Create a new node for this body atom and return its pointer or
	 * return pointer to already existing node that matches this atom.
	 *
	 * See also addUniqueHeadNode. All existing nods are searched for a node
	 * that unfies with the new one. If one is found that occured in a rule's
	 * head, a unifying dependency from the existing one to the new one is
	 * creates.
	 */
	AtomNodePtr
	addUniqueBodyNode(const AtomPtr&);

	/**
	 * @brief Finds an AtomNode that is associated with a specific Atom object.
	 */
	AtomNodePtr
	findNode(const AtomPtr&) const;
};


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_NODEGRAPH_H */


/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
