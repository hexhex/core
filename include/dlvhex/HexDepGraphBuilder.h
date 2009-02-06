/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2008, 2009 Thomas Krennwallner, DAO Tran Minh
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
 * @file HexDepGraphBuilder.h
 * @author Thomas Krennwallner
 * @author DAO Tran Minh
 * @date Mon Feb 02 10:54:24 CET 2009
 *
 * @brief Builder for HEX dependency graphs
 *
 *
 */


#if !defined(_DLVHEX_HEXDEPGRAPHBUILDER_H)
#define _DLVHEX_HEXDEPGRAPHBUILDER_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/DepGraphBuilder.h"
#include "dlvhex/HexDepGraph.h"

#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN


/**
 * @brief implementation of a dependency graph builder for HexDepGraph.
 */
class DLVHEX_EXPORT HexDepGraphBuilder : public DepGraphBuilder<HexDepGraphType>
{
 protected:
  /// the HEX dependency graph
  boost::shared_ptr<HexDepGraphType::type> dg;

  
 public:

  HexDepGraphBuilder()
    : dg(new HexDepGraphType::type)
  { }
  

  /** 
   * @return dependency graph
   */
  virtual boost::shared_ptr<HexDepGraphType::type>
  getDepGraph() const
  {
    return dg;
  }
  

  /** 
   * @return vertex range
   */
  virtual std::pair<HexDepGraphType::VertexIterator, HexDepGraphType::VertexIterator>
  getVertices()
  {
    return boost::vertices(*dg);
  }


  /** 
   * @return vertex property map
   */
  virtual HexDepGraphType::VertexProperty
  getVertexProperties()
  {
    return boost::get(boost::vertex_bundle, *dg);
  }

  
  /** 
   * @return edge range
   */
  virtual std::pair<HexDepGraphType::EdgeIterator, HexDepGraphType::EdgeIterator>
  getEdges()
  {
    return boost::edges(*dg);
  }


  /** 
   * @return edge property map
   */
  virtual HexDepGraphType::EdgeProperty
  getEdgeProperties()
  {
    return boost::get(boost::edge_bundle, *dg);
  }


  /** 
   * Add a new node to the dependency graph.
   * 
   * @return new vertex id
   */
  virtual HexDepGraphType::Vertex
  buildVertex()
  {
    return boost::add_vertex(*dg);
  }

  
  /** 
   * Add an edge to the dependency graph.
   * 
   * @param u start node
   * @param v end node
   * 
   * @return new edge id
   */
  virtual HexDepGraphType::Edge
  buildEdge(const HexDepGraphType::Vertex& u, const HexDepGraphType::Vertex& v)
  {
    std::pair<HexDepGraphType::Edge, bool> eret = boost::add_edge(u, v, *dg);
    return eret.first;
  }
};

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_HEXDEPGRAPHBUILDER_H */


// Local Variables:
// mode: C++
// End:
