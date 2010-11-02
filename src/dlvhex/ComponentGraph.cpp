/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
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
 * @file ComponentGraph.cpp
 * @author Peter Schüller
 *
 * @brief Implementation of the component graph.
 */

#include "dlvhex/ComponentGraph.hpp"
#include "dlvhex/Logger.hpp"

#include <boost/graph/strong_components.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/foreach.hpp>

#include <sstream>

DLVHEX_NAMESPACE_BEGIN

ComponentGraph::ComponentGraph(RegistryPtr registry):
  DependencyGraph(registry)
{
}

ComponentGraph::~ComponentGraph()
{
}

void ComponentGraph::calculateComponentInfo()
{
  calculateSCCs();
  calculateSCCMembers();
  calculateSpecialNodeSets();
}

void ComponentGraph::calculateSCCs()
{
  LOG_SCOPE("cSCCs", false);
  LOG("=calculateSCCs");
  // TODO
}

void ComponentGraph::calculateSCCMembers()
{
  LOG_SCOPE("cSCCM", false);
  LOG("=calculateSCCMembers");
  // TODO
}

void ComponentGraph::calculateSpecialNodeSets()
{
  LOG_SCOPE("cSNS", false);
  LOG("=calculateSpecialNodeSets");
  // TODO
}

void ComponentGraph::writeGraphVizNodeLabel(std::ostream& o, Node n, bool verbose) const
{
  // additionally print SCC index and root/leaf membership
  DependencyGraph::writeGraphVizNodeLabel(o, n, verbose);

  assert(n < scc.size());
  int sccnumber = scc[n];
  assert(n < sccRepresentative.size());
  Node representative = sccRepresentative[n];
  bool root = roots.count(n)>0;
  bool leaf = leaves.count(n)>0;

  if( verbose )
  {
    o << "\\nscc=" << sccnumber << " rep=" << representative;
  }
  else
  {
    o << "/scc" << sccnumber << ",rep" << representative;
  }
  o << (root?" root":"") << (leaf?" leaf":"");
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
