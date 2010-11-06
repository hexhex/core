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

#include <boost/range/iterator_range.hpp>
#include <boost/graph/strong_components.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <sstream>

DLVHEX_NAMESPACE_BEGIN

const ComponentGraph::DependencyInfo&
ComponentGraph::DependencyInfo::operator|=(
		const ComponentGraph::DependencyInfo& other)
{
	return DependencyGraph::DependencyInfo::operator|=(other);
}

std::ostream& ComponentGraph::DependencyInfo::print(std::ostream& o) const
{
	return o << static_cast<const DependencyGraph::DependencyInfo&>(*this);
}

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

void ComponentGraph::calculateComponents(
		#ifndef NDEBUG
		const DependencyGraph&
		#else
		const DependencyGraph& dg
		#endif
	)
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
    LOG("created component node " << c << " for scc " << s <<
				" with depgraph nodes " << printrange(nodes));
		bool multimember = nodes.size() > 1;
    sccToComponent[s] = c;
    ComponentInfo& ci = propsOf(c);
    // collect rule and eatom ids in scc
    for(NodeSet::const_iterator itn = nodes.begin();
        itn != nodes.end(); ++itn)
    {
      #ifndef NDEBUG
      ci.sources.push_back(*itn);
      #endif
      ID id = dg.propsOf(*itn).id;
      if( id.isRule() )
			{
				if( id.isRegularRule() )
				{
					ci.innerRules.push_back(id);
				}
				else if( id.isConstraint() || id.isWeakConstraint() )
				{
					ci.innerConstraints.push_back(id);
				}
				else
				{
					assert(false);
				}
      }
      else if( id.isExternalAtom() )
      {
				if( multimember )
				{
					ci.innerEatoms.push_back(id);
				}
				else
				{
					ci.outerEatoms.push_back(id);
				}
      }
			else
			{
				assert(false);
			}
    }
    LOG("-> outerEatoms " << printrange(ci.outerEatoms));
    LOG("-> innerRules " << printrange(ci.innerRules));
    LOG("-> innerConstraints " << printrange(ci.innerConstraints));
    LOG("-> innerEatoms " << printrange(ci.innerEatoms));

		assert( ci.outerEatoms.empty() ||
				   (ci.innerRules.empty() && ci.innerConstraints.empty() && ci.innerEatoms.empty()));

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
        unsigned targetscc = scc[targetnode];
        if( targetscc == s )
        {
          // dependency within SCC
          continue;
        }

				Component targetc = sccToComponent[targetscc];
				// dependency to other SCC -> store
				LOG("found dependency from SCC " << s << " to SCC " << targetscc);

				// create/update dependency
				Dependency newdep;
				bool success;

				// use dependencyinfo from original dependency
        //const DependencyGraph::NodeInfo& ni = dg.propsOf(targetnode);
        const DependencyGraph::DependencyInfo& di = dg.propsOf(dep);

				boost::tie(newdep, success) = boost::add_edge(c, targetc, DependencyInfo(di), cg);
				if( !success )
				{
					boost::tie(newdep, success) = boost::edge(c, targetc, cg);
					assert(success);
					// update existing dependency
					propsOf(newdep) |= di;
				}
      } // for each dependency of *itn
    } // collect dependencies outgoing from node *itn in SCC s
  } // create dependencies outgoing from SCC s
}

// collapse components given in range into one new component
// collapse incoming and outgoing dependencies
// update properties of dependencies
// update properties of component
// asserts that this operation does not make the DAG cyclic
ComponentGraph::Component
ComponentGraph::collapseComponents(
		const ComponentSet& originals)
{
	LOG_SCOPE("cC", false);
	LOG("= collapseComponents(" << printrange(originals) << ")");

	typedef std::map<Component, DependencyInfo> DepMap;

	// calculate set of dependencies incoming to the new component
	DepMap incoming;
	// calculate set of dependencies outgoing from the new component
	DepMap outgoing;

	// do this by iterating over all originals and over all their incoming and outgoing dependencies
	ComponentSet::const_iterator ito;
	for(ito = originals.begin(); ito != originals.end(); ++ito)
	{
		LOG("original " << *ito << ":");
		LOG_INDENT();

		PredecessorIterator itpred, itpred_end;
		for(boost::tie(itpred, itpred_end) = getDependencies(*ito);
				itpred != itpred_end; ++itpred)
		{
			Dependency outgoing_dep = *itpred;
			Component target = targetOf(outgoing_dep);
			if( originals.count(target) == 0 )
			{
				// do not count dependencies within the new collapsed component
				LOG("outgoing dependency to " << target);
				outgoing[target] |= propsOf(outgoing_dep);
			}
		} // iterate over predecessors
	} // iterate over originals

	// do again, but for outgoing, now also check for duplicate violations
	for(ito = originals.begin(); ito != originals.end(); ++ito)
	{
		LOG("original " << *ito << ":");
		LOG_INDENT();

		SuccessorIterator itsucc, itsucc_end;
		for(boost::tie(itsucc, itsucc_end) = getProvides(*ito);
				itsucc != itsucc_end; ++itsucc)
		{
			Dependency incoming_dep = *itsucc;
			Component source = sourceOf(incoming_dep);
			if( originals.count(source) == 0 )
			{
				// do not count dependencies within the new collapsed component
				LOG("incoming dependency from " << source);
				incoming[source] |= propsOf(incoming_dep);
				// assert that we do not create cycles
				#ifndef NDEBUG
				DepMap::const_iterator itdm = outgoing.find(source);
				// if we have an incoming dep and an outgoing dep,
				// we create a cycle so this collapsing is invalid
				assert(itdm == outgoing.end());
				#endif
			}
		} // iterate over successors
	} // iterate over originals

	//
	// we prepared all dependencies, so now we create the component
	//

	Component c = boost::add_vertex(cg);
	LOG("created component node " << c << " for collapsed component");

	// build combined component info
	ComponentInfo& ci = propsOf(c);
	for(ito = originals.begin(); ito != originals.end(); ++ito)
	{
		ComponentInfo& cio = propsOf(*ito);
		#ifndef NDEBUG
		ci.sources.insert(ci.sources.end(),
				cio.sources.begin(), cio.sources.end());
		#endif
		ci.outerEatoms.insert(ci.outerEatoms.end(),
				cio.outerEatoms.begin(), cio.outerEatoms.end());
		ci.innerRules.insert(ci.innerRules.end(),
				cio.innerRules.begin(), cio.innerRules.end());
		ci.innerEatoms.insert(ci.innerEatoms.end(),
				cio.innerEatoms.begin(), cio.innerEatoms.end());
		ci.innerConstraints.insert(ci.innerConstraints.end(),
				cio.innerConstraints.begin(), cio.innerConstraints.end());
	}

	// build incoming dependencies
	for(DepMap::const_iterator itd = incoming.begin();
			itd != incoming.end(); ++itd)
	{
		Dependency newdep;
		bool success;
		LOG("adding edge " << itd->first << " -> " << c);
		boost::tie(newdep, success) = boost::add_edge(itd->first, c, itd->second, cg);
		assert(success); // we only add new edges here, and each only once
	}

	// build outgoing dependencies
	for(DepMap::const_iterator itd = outgoing.begin();
			itd != outgoing.end(); ++itd)
	{
		Dependency newdep;
		bool success;
		LOG("adding edge " << c << " -> " << itd->first);
		boost::tie(newdep, success) = boost::add_edge(c, itd->first, itd->second, cg);
		assert(success); // we only add new edges here, and each only once
	}

	// remove all original components
	for(ito = originals.begin(); ito != originals.end(); ++ito)
	{
		boost::clear_vertex(*ito, cg);
		boost::remove_vertex(*ito, cg);
	}

	return c;
}

namespace
{
  std::string graphviz_node_id(ComponentGraph::Component c)
  {
    std::ostringstream os;
    os << "c" << std::hex << c;
    return os.str();
  }

	template<typename Range>
	void printoutVerboseIfNotEmpty(std::ostream& o, RawPrinter& rp, const char* prefix, Range idrange)
	{
		// see boost/range/iterator_range.hpp
		typedef typename Range::const_iterator Iterator;
		if( !boost::empty(idrange) )
		{
			o << "{" << prefix << "|";
			Iterator it = boost::begin(idrange);
			rp.print(*it);
			it++;
			for(; it != boost::end(idrange); ++it)
			{
				o << "\\n";
				rp.print(*it);
			}
			o << "}|";
		}
	}

	template<typename Range>
	void printoutTerseIfNotEmpty(std::ostream& o, RawPrinter& rp, const char* prefix, Range idrange)
	{
		// see boost/range/iterator_range.hpp
		typedef typename Range::const_iterator Iterator;
		if( !boost::empty(idrange) )
		{
			unsigned count = 0;
			for(Iterator it = boost::begin(idrange); it != boost::end(idrange); ++it, ++count)
				;
			o << "{" << count << " " << prefix << "}|";
		}
	}
}

void ComponentGraph::writeGraphVizComponentLabel(std::ostream& o, Component c, bool verbose) const
{
  const ComponentInfo& ci = getComponentInfo(c);
	RawPrinter rp(o, reg);
  if( verbose )
  {
    o << "{";
		o << c << "|";
    #ifndef NDEBUG
    o << "{sources|" << printrange(ci.sources, "\\{", ",", "\\}") << "}|";
    #endif
		printoutVerboseIfNotEmpty(o, rp, "outerEatoms", ci.outerEatoms);
		printoutVerboseIfNotEmpty(o, rp, "innerRules", ci.innerRules);
		printoutVerboseIfNotEmpty(o, rp, "innerEatoms", ci.innerEatoms);
		printoutVerboseIfNotEmpty(o, rp, "innerConstraints", ci.innerConstraints);
		o << "prop?}";
  }
  else
  {
    o << "{";
		printoutTerseIfNotEmpty(o, rp, "outerEatoms", ci.outerEatoms);
		printoutTerseIfNotEmpty(o, rp, "innerRules", ci.innerRules);
		printoutTerseIfNotEmpty(o, rp, "innerEatoms", ci.innerEatoms);
		printoutTerseIfNotEmpty(o, rp, "innerConstraints", ci.innerConstraints);
		o << "prop?}";
  }
}

void ComponentGraph::writeGraphVizDependencyLabel(std::ostream& o, Dependency dep, bool verbose) const
{
  const DependencyInfo& di = getDependencyInfo(dep);
  if( verbose )
  {
    o << di;
  }
  else
  {
    o <<
    (di.positiveRegularRule?" posR":"") <<
    (di.positiveConstraint?" posC":"") <<
    (di.negativeRule?" negR":"") <<
    (di.unifyingHead?" unifying":"") <<
    (di.positiveExternal?" posExt":"") <<
    (di.negativeExternal?" negExt":"") <<
    (di.externalConstantInput?" extConstInp":"") <<
    (di.externalPredicateInput?" extPredInp":"");
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
    o << graphviz_node_id(*it) << "[shape=record,label=\"";
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
    o << "\"];" << std::endl;
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
