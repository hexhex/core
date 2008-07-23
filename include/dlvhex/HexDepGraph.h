/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2008 Thomas Krennwallner
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
 * @file HexDepGraph.h
 * @author Thomas Krennwallner
 * @date Sat Jun 28 19:05:55 CEST 2008
 *
 * @brief Classes for HEX dependency graph related information.
 *
 *
 */


#if !defined(_DLVHEX_HEXDEPGRAPH_H)
#define _DLVHEX_HEXDEPGRAPH_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/Rule.h"
#include "dlvhex/Atom.h"

#include <boost/graph/adjacency_list.hpp>

DLVHEX_NAMESPACE_BEGIN


/**
 * @brief Property of a dependency vertex
 */
struct DLVHEX_EXPORT VertexProperty
{
  enum Type
  {
    BODY = 0x1,
    HEAD = 0x2
  };

  Type type;
  AtomPtr atom;
};



/**
 * @brief Dependency between two Atoms.
 *
 * A dependency links two atoms, and has a type. If the dependency was
 * caused by a rule, the dependency will be associated with this rule
 * (by storing its pointer).
 */
struct DLVHEX_EXPORT EdgeProperty
{

  /**
   * @brief Type of an EdgeProperty.
   *
   * - UNIFYING: The atoms of two nodes can be unified.
   * - PRECEDING: A preceding dependency points from a body atom node to its head
   *   atom node.
   * - NEG_PRECEDING: Like preceding, but with a weakly negated body atom.
   * - DISJUNCTIVE: Dependency between two head atom nodes of a disjunctive
   *   head.
   * - EXTERNAL: If an input argument of an external atom is of type
   *   PluginAtom::PREDICATE, it depends on all atoms with a matching predicate.
   * - EXTERNAL_AUX: If an input argument is nonground, an auxiliary atom will
   *   be created, being the target of a dependency of this type.
   */
  enum Type
  {
    UNIFYING = 0x1,
    PRECEDING = 0x2,
    NEG_PRECEDING = 0x4,
    DISJUNCTIVE = 0x8,
    EXTERNAL = 0x10,
    EXTERNAL_AUX = 0x20
  };

  Type type;
  Rule* rule;
};


/// currently empty
struct GraphProperty
{ };


/**
 * @brief a HexDepGraph
 *
 * edges are in a std::vector (vecS) (warning, this allows duplicate edges)
 * vertices are in a std::vector (vecS)
 * we want both in_edges() and out_edges(), hence we need bidirectionalS
 */
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, VertexProperty, EdgeProperty, GraphProperty> HexDepGraph;



DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_HEXDEPGRAPH_H */


// Local Variables:
// mode: C++
// End:
