/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2008, 2009 DAO Tran Minh, Thomas Krennwallner
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
 * @author DAO Tran Minh
 * @date Fri Jan 30 15:00:22 CET 2009
 *
 * @brief Types for HEX dependency graph related information.
 *
 *
 */


#if !defined(_DLVHEX_HEXDEPGRAPH_H)
#define _DLVHEX_HEXDEPGRAPH_H

#include "dlvhex/PlatformDefinitions.h"

#include <boost/graph/subgraph.hpp>
#include <boost/graph/adjacency_list.hpp>

DLVHEX_NAMESPACE_BEGIN


/**
 * @brief Property of a dependency vertex
 *
 * A vertex is an atom, that will be associated with it.
 */
struct DLVHEX_EXPORT VertexAttribute
{
  enum Type
  {
    BODY = 0x1,
    HEAD = 0x2
  };

  Type type;
  int atom;
};



/**
 * @brief Dependency between two Atoms.
 *
 * A dependency links two atoms, and has a type. If the dependency was
 * caused by a rule, the dependency will be associated with this rule.
 */
struct DLVHEX_EXPORT EdgeAttribute
{

  /**
   * @brief Types of dependencies
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
  int rule;
};


/// currently empty
struct GraphProperty
{ };


///@brief a bundle of vertex properties (we need the index for subgraphs)
typedef boost::property<boost::vertex_index_t, int, VertexAttribute> VertexBundleProperty;

///@brief a bundle of edge properties (we need the index for subgraphs)
typedef boost::property<boost::edge_index_t, int, EdgeAttribute> EdgeBundleProperty;

/**
 * @brief the HEX dependency graph
 *
 * - edges are in a std::vector (vecS) (warning, this allows duplicate edges)
 * - vertices are in a std::vector (vecS)
 * - we want both in_edges() and out_edges(), hence we need bidirectionalS
 */
typedef boost::subgraph<
  boost::adjacency_list<boost::vecS, boost::vecS,
			boost::bidirectionalS,
			VertexBundleProperty,
			EdgeBundleProperty,
			GraphProperty> > HexDepGraph;

///@brief traits of the HexDepGraph
typedef boost::graph_traits<HexDepGraph> HexDepGraphTraits;


/**
 * @brief the HEX dependency graph types
 */
struct HexDepGraphType
{
  ///@brief the type of a vertex
  typedef HexDepGraphTraits::vertex_descriptor Vertex;
  ///@brief the type of a vertex iterator
  typedef HexDepGraphTraits::vertex_iterator VertexIterator;
  ///@brief the type of an edge
  typedef HexDepGraphTraits::edge_descriptor Edge;
  ///@brief the type of an edge iterator
  typedef HexDepGraphTraits::edge_iterator EdgeIterator;

  ///@brief property type for accessing the vertex bundle
  typedef boost::property_map<HexDepGraph, boost::vertex_bundle_t>::type VertexProperty;
  ///@brief property type for accessing the edge bundle
  typedef boost::property_map<HexDepGraph, boost::edge_bundle_t>::type EdgeProperty;
};


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_HEXDEPGRAPH_H */


// Local Variables:
// mode: C++
// End:
