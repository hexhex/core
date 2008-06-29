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
 * @file DepGraphBuilder.h
 * @author Thomas Krennwallner
 * @date Sun Jun 29 08:14:40 CEST 2008
 *
 * @brief Classes for creating the dependency graph.
 *
 *
 */


#if !defined(_DLVHEX_DEPGRAPHBUILDER_H)
#define _DLVHEX_DEPGRAPHBUILDER_H

#include "dlvhex/PlatformDefinitions.h"

#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
 * @brief Abstract base class for building a DepGraph.
 */
template <class DG, class Vertex, class VP, class EP>
class DLVHEX_EXPORT DepGraphBuilder
{
 public:
  virtual boost::shared_ptr<DG>
  getDepGraph() const = 0;

  virtual Vertex
  buildVertex(VP& vp) = 0;

  virtual void
  buildEdge(Vertex u, Vertex v, EP& ep) = 0;
};


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_DEPGRAPHBUILDER_H */


// Local Variables:
// mode: C++
// End:
