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
 * @file DepGraphBuilder.h
 * @author Thomas Krennwallner
 * @author DAO Tran Minh
 * @date Mon Feb 02 11:30:40 CEST 2009
 *
 * @brief Abstract base class for creating a dependency graph.
 *
 *
 */


#if !defined(_DLVHEX_DEPGRAPHBUILDER_H)
#define _DLVHEX_DEPGRAPHBUILDER_H

#include "dlvhex/PlatformDefinitions.h"

#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
 * @brief Abstract base class for building a dependency graph with
 * traits DG.
 */
template <typename DG>
class DLVHEX_EXPORT DepGraphBuilder
{
 public:
  /** 
   * @return the dependency graph
   */
  virtual boost::shared_ptr<typename DG::type>
  getDepGraph() const = 0;

  /** 
   * Create a new node in the dependency graph.
   * 
   * @return new vertex id
   */
  virtual typename DG::Vertex
  buildVertex() = 0;

  /** 
   * @return the vertex properties of the graph
   */
  virtual typename DG::VertexProperty
  getVertexProperties() = 0;

  /** 
   * @return begin and end iterator range of the vertices
   */
  virtual std::pair<typename DG::VertexIterator, typename DG::VertexIterator>
  getVertices() = 0;

  /** 
   * Create a new edge in the dependency graph.
   * 
   * @param u start node
   * @param v end node
   * 
   * @return new edge id
   */
  virtual typename DG::Edge
  buildEdge(const typename DG::Vertex& u, const typename DG::Vertex& v) = 0;

  /** 
   * @return the edge properties of the graph
   */
  virtual typename DG::EdgeProperty
  getEdgeProperties() = 0;

  /** 
   * @return begin and end iterator range of the edges
   */
  virtual std::pair<typename DG::EdgeIterator, typename DG::EdgeIterator>
  getEdges() = 0;
};


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_DEPGRAPHBUILDER_H */

// Local Variables:
// mode: C++
// End:
