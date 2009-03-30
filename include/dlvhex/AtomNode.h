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
 * @file AtomNode.h
 * @author Roman Schindlauer
 * @date Mon Jan 30 14:46:36 CET 2006
 *
 * @brief AtomNode class.
 *
 *
 */


#if !defined(_DLVHEX_ATOMNODE_H)
#define _DLVHEX_ATOMNODE_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/Rule.h"
#include "dlvhex/Program.h"
#include "dlvhex/Dependency.h"

#include <boost/shared_ptr.hpp>


DLVHEX_NAMESPACE_BEGIN


/**
 * @brief Single Node of a dependency Graph.
 *
 * An AtomNode is the representation of an Atom in a program's dependency
 * structure. It can have several dependencies, each possibly associated with
 * rules of the program.
 */
class DLVHEX_EXPORT AtomNode
{
public:

	/**
	 * @brief Constructs an AtomNode from a given Atom.
	 */
	AtomNode(const AtomPtr& = AtomPtr());
	
	/**
	 * @brief Sets the head-flag of the Node.
	 *
	 * For calculating the correct dependencies when a new AtomNode is added to
	 * a collection of existing Nodes (see NodeGraph), it is vital to know for
	 * each AtomNode whether it is associated with a head-atom or a body-atom.
	 * This function sets the head-flag of the AtomNode.
	 */
	void
	setHead();

	/**
	 * @brief Sets the body-flag of the Node.
	 *
	 * See setHead().
	 */
	void
	setBody();

	/**
	 * @brief Returns the head-flag of the AtomNode.
	 *
	 * See setHead().
	 */
	bool
	isHead() const;

	/**
	 * @brief Returns the body-flag of the AtomNode.
	 *
	 * See setHead().
	 */
	bool
	isBody() const;

	/**
	 * @brief Sets the aux-flag of the Node.
	 */
	void
	setAux();

	/**
	 * @brief Returns the aux-flag of the AtomNode.
	 */
	bool
	isAux() const;


	/**
	 * @brief Adds a preceding dependency for this AtomNode.
	 *
	 * A preceding dependency means that this AtomNode depends on another one.
	 */
	void
	addPreceding(const Dependency&);
  
	/**
	 * @brief Add succeeding dependency for this AtomNode.
	 *
	 * A succeeding dependency means that another AtomNode depends on this one.
	 */
	void
	addSucceeding(const Dependency&);

	/**
	 * @brief Returns the atom-object this Node is associated with.
	 */
	const AtomPtr&
	getAtom() const;

	/**
	 * @brief Returns all preceding dependencies of this AtomNode.
	 */
	const std::set<Dependency>&
	getPreceding() const;

	/**
	 * @brief Returns all succeeding dependencies of this AtomNode.
	 */
	const std::set<Dependency>&
	getSucceeding() const;

	/**
	 * @brief Returns all rules associated with this AtomNode.
	 */
	const std::vector<Rule*>&
	getRules() const;

	/**
	 * @brief Returns the unique Id of this AtomNode.
	 */
	unsigned
	getId() const;

private:

	/**
	 * @brief This AtomNode's Atom object.
	 */
	const AtomPtr atom;

	/**
	 * @brief head-flag.
	 */
	bool inHead;

	/**
	 * @brief body-flag.
	 */
	bool inBody;

	/**
	 * @brief auxiliary-flag.
	 */
	bool auxFlag;

	/**
	 * @brief Rules that belong to this AtomNode (in case it occured
	 * in a rule's head).
	 *
	 * This is a cache for the rules created in
	 * AtomNode::getRules. Must be mutable because of constness of
	 * getRules.
	 */
	mutable std::vector<Rule*> rules;

	/**
	 * @brief Preceding dependencies.
	 */
	std::set<Dependency> preceding;

	/**
	 * @brief succeeding dependencies.
	 */
	std::set<Dependency> succeeding;

	/**
	 * @brief Unique numerical index to facilitate interfacing of Component
	 * finder algorithms.
	 */
	unsigned nodeId;

	/**
	 * @brief Node counter for assigning node Ids.
	 */
	static unsigned nodeCount;
};



//
// verbose and debug serialization.
//
std::ostream& operator<< (std::ostream& out, const AtomNode& atomnode);


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_ATOMNODE_H */


/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
