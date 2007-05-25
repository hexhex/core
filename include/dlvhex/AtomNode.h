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

/* -*- C++ -*- */

/**
 * @file AtomNode.h
 * @author Roman Schindlauer
 * @date Mon Jan 30 14:46:36 CET 2006
 *
 * @brief AtomNode, Dependency and NodeGraph classes.
 *
 *
 */


#ifndef _ATOMNODE_H
#define _ATOMNODE_H


#include <utility>

#include "dlvhex/Rule.h"
#include "dlvhex/Program.h"

#include "boost/shared_ptr.hpp"

//
// forward declaration
//
class AtomNode;
typedef boost::shared_ptr<AtomNode> AtomNodePtr;


/**
 * @brief Dependency between two AtomNodes.
 *
 * A dependency contains an AtomNode, which is the "target" of the dependency,
 * and a type. A dependency object is supposed to belong to an AtomNode object,
 * which is then the "source" of the dependency. If the dependency was caused by
 * a rule, the dependency will be associated with this rule (by storing its
 * pointer).
 */
class Dependency
{
public:

	/**
	 * @brief Type of Dependency.
	 *
	 * UNIFYING: The atoms of two nodes can be unified.
	 * PRECEDING: A preceding dependency points from a body atom node to its head
	 * atom node.
	 * NEG_PRECEDING: Like preceding, but with a weakly negated body atom.
	 * DISJUNCTIVE: Dependency between two head atom nodes of a disjunctive
	 * head.
	 * EXTERNAL: If an input argument of an external atom is of type
	 * PluginAtom::PREDICATE, it depends on all atoms with a matching predicate.
	 * EXTERNAL_AUX: If an input argument is nonground, an auxiliary atom will
	 * be created, being the target of a dependency of this type.
	 */
	typedef enum { UNIFYING = 0x1,
	               PRECEDING = 0x2,
	               NEG_PRECEDING = 0x4,
	               DISJUNCTIVE = 0x8,
	               EXTERNAL = 0x10,
	               EXTERNAL_AUX = 0x20
	} Type;

	/// Ctor.
	Dependency();

	/**
	 * Copy constructor.
	 */
	Dependency(const Dependency&);

	/**
	 * @brief Construct a dependency of a specific type to a given AtomNode
	 * target.
	 */
	Dependency(Rule*, const AtomNodePtr&, Type);

	/**
	 * @brief Return the dependency type.
	 */
	Type
	getType() const;

	/** 
	 * AtomNode uses those rules to create a list of rules on-the-fly.
	 *
	 * @return the rule that created this dependency.
	 */
	Rule*
	getRule() const;

	/**
	 * @brief Return the target AtomNode of the dependency.
	 */
	const AtomNodePtr&
	getAtomNode() const;

	/**
	 * @brief Add a dependency information to two AtomNodes.
	 *
	 */
	static void
	addDep(Rule*, const AtomNodePtr&, const AtomNodePtr&, Type);

	/**
	 * @brief Comparison operator needed for std::set<Dependency>.
	 */
	bool
	operator< (const Dependency& dep2) const;

private:

	/**
	 * @brief AtomNode that is the target of this dependency.
	 */
	AtomNodePtr atomNode;

	/**
	 * Type of the dependency.
	 */
	Type type;

	/**
	 * a dependency belongs to a certain rule.
	 */
	Rule* rule;

};


//
// verbose and debug serialization
//
std::ostream& operator<< (std::ostream& out, const Dependency& dep);



/**
 * @brief Single Node of a dependency Graph.
 *
 * An AtomNode is the representation of an Atom in a program's dependency
 * structure. It can have several dependencies, each possibly associated with
 * rules of the program.
 */
class AtomNode
{
public:

	/**
	 * @brief Constructs an AtomNode from a given Atom.
	 */
	AtomNode(const AtomPtr&);
	
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


/**
 * @brief Class for storing simple nodes of a dependency graph.
 *
 * A nodegraph is just a container for AtomNodes, taking care of creating and
 * deleting AtomNode objects. It provides specific functions for handling a set
 * of AtomNodes, like unique creation and finding a specific node.
 */
class NodeGraph
{
private:

	std::vector<AtomNodePtr> atomNodes;

	mutable std::vector<Rule*> prog;


public:

	/// Dtor.
	~NodeGraph();

	/**
	 * @brief returns the associated Program.
	 * @return prog
	 */
	const std::vector<Rule*>&
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
	void
	findNode(const AtomPtr&, AtomNodePtr&) const;
};


#endif /* _ATOMNODE_H_ */


/* vim: set noet sw=4 ts=4 tw=80: */
