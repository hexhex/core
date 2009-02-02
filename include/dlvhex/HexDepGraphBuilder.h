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
class DLVHEX_EXPORT HexDepGraphBuilder 
  : public DepGraphBuilder<HexDepGraph, 
			   HexDepGraphType::Vertex, 
			   HexDepGraphType::Edge, 
			   HexDepGraphType::VertexProperty, 
			   HexDepGraphType::EdgeProperty>
{
 protected:
  /// the HEX dependency graph
  boost::shared_ptr<HexDepGraph> dg;

  
 public:

  HexDepGraphBuilder()
    : dg(new HexDepGraph)
  { }
  

  /** 
   * @return dependency graph
   */
  virtual boost::shared_ptr<HexDepGraph>
  getDepGraph() const
  {
    return dg;
  }
  
  
  /** 
   * Add a new node to the dependency graph.
   * 
   * @param vp vertex property
   * 
   * @return new vertex
   */
  virtual HexDepGraphType::Vertex
  buildVertex(HexDepGraphType::VertexProperty& vp)
  {
    return boost::add_vertex(vp, *dg);
  }

  
  /** 
   * Add an edge to the dependency graph.
   * 
   * @param u start node
   * @param v end node
   * @param ep edge property
   * 
   * @return new edge
   */
  virtual HexDepGraphType::Edge
  buildEdge(HexDepGraphType::Vertex u,
	    HexDepGraphType::Vertex v,
	    HexDepGraphType::EdgeProperty& ep)
  {
    std::pair<HexDepGraphType::Edge, bool> eret = boost::add_edge(u, v, ep, *dg);
    return eret.first;
  }
};

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_HEXDEPGRAPHBUILDER_H */


// Local Variables:
// mode: C++
// End:
