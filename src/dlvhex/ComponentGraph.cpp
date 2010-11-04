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
  // only do this once
  assert(scc.empty());
  assert(sccRepresentative.empty());

  // resize all property maps
  scc.resize(boost::num_vertices(dg));
  sccRepresentative.resize(boost::num_vertices(dg));

  // do the work
  calculateSCCs();
  calculateSpecialNodeSets();
}

void ComponentGraph::calculateSCCs()
{
  LOG_SCOPE("cSCCs", false);
  LOG("=calculateSCCs");

  unsigned scccount = boost::strong_components(
      dg,
      &scc[0], 
      boost::root_map(&sccRepresentative[0]));
  LOG("boost::strong_components created " << scccount << " components");

  // prepare storage for sccMembers
  sccMembers.resize(scccount);

  // calcualte sccMembers
  for(unsigned n = 0; n < scccount; ++n)
  {
    // get the component id from scc[n]
    // add the node id to the set of nodes of this component
    sccMembers[scc[n]].insert(static_cast<Node>(n));
  }
}

void ComponentGraph::calculateSpecialNodeSets()
{
  LOG_SCOPE("cSNS", false);
  LOG("=calculateSpecialNodeSets");
  NodeIterator it, it_end;
  for(boost::tie(it, it_end) = getNodes(); it != it_end; ++it)
  {
    PredecessorIterator pred, pred_end;
    boost::tie(pred, pred_end) = getDependencies(*it);
    if( pred == pred_end )
      leaves.insert(*it);

    SuccessorIterator succ, succ_end;
    boost::tie(succ, succ_end) = getProvides(*it);
    if( succ == succ_end )
      roots.insert(*it);
  }
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
    if( sccMembers[sccnumber].size() == 1 )
    {
      o << " scc=" << sccnumber;
    }
    else
    {
      o << " SCC=" << sccnumber;
      if( representative != n )
        o << " rep=" << representative;
    }
    o << (root?" root":"") << (leaf?" leaf":"");
  }
  else
  {
    if( sccMembers[sccnumber].size() > 1 )
    {
      o << "/SCC" << sccnumber;
    }
    else
    {
      o << "/scc" << sccnumber;
    }
  }
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
