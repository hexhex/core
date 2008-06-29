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
 * @file HexDepGraphBuilder.h
 * @author Thomas Krennwallner
 * @date Sun Jun 29 08:14:40 CEST 2008
 *
 * @brief Classes for creating the HEX dependency graph.
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
 * @brief a dependency graph builder for HEX programs
 */
class DLVHEX_EXPORT HexDepGraphBuilder : public DepGraphBuilder<HexDepGraph, HexDepGraph::vertex_descriptor, VertexProperty, EdgeProperty>
{
 protected:
  boost::shared_ptr<HexDepGraph> dg;

 public:
  HexDepGraphBuilder() : dg(new HexDepGraph)
  { }


  virtual boost::shared_ptr<HexDepGraph>
  getDepGraph() const
  {
    return dg;
  }


  virtual HexDepGraph::vertex_descriptor
  buildVertex(VertexProperty& vp)
  {
    return boost::add_vertex(vp, *dg);
  }


  virtual void
  buildEdge(HexDepGraph::vertex_descriptor u, HexDepGraph::vertex_descriptor v, EdgeProperty& ep)
  {
    std::pair<HexDepGraph::edge_descriptor, bool> eret = boost::add_edge(u, v, ep, *dg);
  }

};


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_HEXDEPGRAPHBUILDER_H */


// Local Variables:
// mode: C++
// End:
