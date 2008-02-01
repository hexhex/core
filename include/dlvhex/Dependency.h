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
 * @file Dependency.h
 * @author Roman Schindlauer
 * @date Mon Jan 30 14:46:36 CET 2006
 *
 * @brief Dependency class.
 *
 *
 */


#if !defined(_DLVHEX_DEPENDENCY_H)
#define _DLVHEX_DEPENDENCY_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/Rule.h"
#include "dlvhex/Program.h"

#include <boost/shared_ptr.hpp>


DLVHEX_NAMESPACE_BEGIN

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
class DLVHEX_EXPORT Dependency
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



DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_DEPENDENCY_H */


/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
