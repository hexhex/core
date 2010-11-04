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
#include "dlvhex/ProgramCtx.h"

#include <boost/graph/strong_components.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <sstream>

DLVHEX_NAMESPACE_BEGIN

ComponentGraph::ComponentGraph(const DependencyGraph& dg, RegistryPtr reg):
  reg(reg),
  #ifndef NDEBUG
  dg(dg),
  #endif
  cg()
{
  calculateComponents(dg);
}

ComponentGraph::~ComponentGraph()
{
}

void ComponentGraph::calculateComponents(const DependencyGraph& dg_)
{
  LOG_SCOPE("cCs", false);
  LOG("=calculateComponents");

  typedef DependencyGraph::Node Node;

  //
  // calculate SCCs
  //
  typedef std::vector<int> ComponentMap;
  ComponentMap scc;
  // resize property map
  scc.resize(dg.countNodes());
  // do it with boost
  unsigned scccount = boost::strong_components(dg.getInternalGraph(), &scc[0]);
  LOG("boost::strong_components created " << scccount << " components");

  //
  // calculate set of nodes for each SCC: sccMembers
  //
  typedef std::set<Node> NodeSet;
  typedef std::vector<NodeSet> SCCMap;
  SCCMap sccMembers;
  sccMembers.resize(scccount);
  for(unsigned n = 0; n < dg.countNodes(); ++n)
  {
    // get the component id from scc[n]
    // add the node id to the set of nodes of this component
    sccMembers[scc[n]].insert(static_cast<Node>(n));
  }

  //
  // create one component for each SCC
  //
  typedef std::vector<Component> SCCToComponentMap;
  SCCToComponentMap sccToComponent(scccount);
  for(unsigned s = 0; s < scccount; ++s)
  {
    //createComponentFromDepgraphSCC(dg, scc, *itc);
    const NodeSet& nodes = sccMembers[s];
    Component c = boost::add_vertex(cg);
    LOG("created component node " << c << " for scc " << s << " with depgraph nodes " << printset(nodes));
    sccToComponent[s] = c;
    ComponentInfo& ci = propsOf(c);
    // collect rule and eatom ids
    for(NodeSet::const_iterator itn = nodes.begin();
        itn != nodes.end(); ++itn)
    {
      #ifndef NDEBUG
      ci.sources.insert(*itn);
      #endif
      ID id = dg.propsOf(*itn).id;
      if( id.isRule() )
      {
        ci.rules.insert(id);
      }
      else if( id.isExternalAtom() )
      {
        ci.eatoms.insert(id);
      }
    }
    LOG("-> rules " << printset(ci.rules));
    LOG("-> eatoms " << printset(ci.eatoms));
    // TODO: find more component properties (for complex model building) (perhaps we can cover this in the loop below?)
  }

  //
  // create dependencies between components (now that all of them exist)
  //
  for(unsigned s = 0; s < scccount; ++s)
  {
    const NodeSet& nodes = sccMembers[s];
    Component c = sccToComponent[s];

    // look at out-dependencies only
    // (successors will find and create all dependencies to this SCC)
    for(NodeSet::const_iterator itn = nodes.begin();
        itn != nodes.end(); ++itn)
    {
      DependencyGraph::PredecessorIterator it, it_end;
      for(boost::tie(it, it_end) = dg.getDependencies(*itn);
          it != it_end; ++it)
      {
        DependencyGraph::Dependency dep = *it;
        Node targetnode = dg.targetOf(dep);
        const DependencyGraph::NodeInfo& ni = dg.propsOf(targetnode);
        unsigned targetscc = scc[targetnode];
        if( targetscc == s )
        {
          // dependency within SCC
          continue;
        }
        else
        {
          // dependency to other SCC
          LOG("found dependency from SCC " << s << " to SCC " << targetscc);
        }
      }

    } // collect dependencies outgoing from node *itn in SCC s
  } // create dependencies outgoing from SCC s
}

namespace
{
  inline std::string graphviz_node_id(ComponentGraph::Component c)
  {
    std::ostringstream os;
    os << "c" << std::hex << c;
    return os.str();
  }
}

void ComponentGraph::writeGraphVizComponentLabel(std::ostream& o, Component c, bool verbose) const
{
  const ComponentInfo& componentinfo = getComponentInfo(c);
  if( verbose )
  {
    o << "component" << c << ":\\n";
    RawPrinter printer(o, reg);
    #ifndef NDEBUG
    o << "sources: " << printset(componentinfo.sources) << "\\n";
    #endif
    if( !componentinfo.rules.empty() )
    {
      o << "rules:\\n";
      BOOST_FOREACH(ID id, componentinfo.rules)
      {
        printer.print(id);
        o << "\\n";
      }
    }
    if( !componentinfo.eatoms.empty() )
    {
      o << "eatoms:\\n";
      BOOST_FOREACH(ID id, componentinfo.eatoms)
      {
        printer.print(id);
        o << "\\n";
      }
    }
  }
  else
  {
    /*
    o << n << ":";
    switch(nodeinfo.id.kind >> ID::SUBKIND_SHIFT)
    {
    case 0x00: o << "o g atom"; break;
    case 0x01: o << "o n atom"; break;
    case 0x03: o << "agg atom"; break;
    case 0x06: o << "ext atom"; break;
    case 0x30: o << "rule"; break;
    case 0x31: o << "constraint"; break;
    case 0x32: o << "weak constraint"; break;
    default: o << "unknown type=0x" << std::hex << (nodeinfo.id.kind >> ID::SUBKIND_SHIFT); break;
    }
    o << ":" << nodeinfo.id.address;
    */
  }
}

void ComponentGraph::writeGraphVizDependencyLabel(std::ostream& o, Dependency dep, bool verbose) const
{
  const DependencyInfo& di = getDependencyInfo(dep);
  if( verbose )
  {
    //o << di;
  }
  else
  {
    /*
    o << "[" <<
      (di.positive?"+":"") << (di.negative?"-":"") <<
      (di.external?"ext":"") << " ";
    if( di.involvesRule )
      o << (di.constraint?"cnstr":"rule");
    else
      o << (di.disjunctive?"d":"") << (di.unifying?"u":"");
    o << "]";
    */
  }
}

// output graph as graphviz source
void ComponentGraph::writeGraphViz(std::ostream& o, bool verbose) const
{
  // boost::graph::graphviz is horribly broken!
  // therefore we print it ourselves

  o << "digraph G {" << std::endl <<
    "rankdir=BT;" << std::endl; // print root nodes at bottom, leaves at top!

  // print vertices
  ComponentIterator it, it_end;
  for(boost::tie(it, it_end) = boost::vertices(cg);
      it != it_end; ++it)
  {
    o << graphviz_node_id(*it) << "[label=\"";
    {
      std::stringstream ss;
      writeGraphVizComponentLabel(ss, *it, verbose);
      // escape " into \"
      boost::algorithm::replace_all_copy(
        std::ostream_iterator<char>(o),
        ss.str(),
        "\"",
        "\\\"");
    }
    o << "\"";
    if( !getComponentInfo(*it).eatoms.empty() )
      o << ",shape=box";
    o << "];" << std::endl;
  }

  // print edges
  DependencyIterator dit, dit_end;
  for(boost::tie(dit, dit_end) = boost::edges(cg);
      dit != dit_end; ++dit)
  {
    Component src = sourceOf(*dit);
    Component target = targetOf(*dit);
    o << graphviz_node_id(src) << " -> " << graphviz_node_id(target) <<
      "[label=\"";
    {
      std::stringstream ss;
      writeGraphVizDependencyLabel(o, *dit, verbose);
      // escape " into \"
      boost::algorithm::replace_all_copy(
        std::ostream_iterator<char>(o),
        ss.str(),
        "\"",
        "\\\"");
    }
    o << "\"];" << std::endl;
  }

  o << "}" << std::endl;
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
