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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/ComponentGraph.hpp"
#include "dlvhex2/Logger.hpp"
#include "dlvhex2/Printer.hpp"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Registry.hpp"
#include "dlvhex2/GraphvizHelpers.hpp"

#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/two_bit_color_map.hpp>
#include <boost/graph/strong_components.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/subgraph.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/foreach.hpp>

#include <sstream>

DLVHEX_NAMESPACE_BEGIN

const ComponentGraph::DependencyInfo&
ComponentGraph::DependencyInfo::operator|=(
		const ComponentGraph::DependencyInfo& other)
{
	DependencyGraph::DependencyInfo::operator|=(other);
  return *this;
}

std::ostream& ComponentGraph::ComponentInfo::print(std::ostream& o) const
{
	if( !outerEatoms.empty() )
		o << "outerEatoms: " << printrange(outerEatoms) << std::endl;
	if( !innerRules.empty() )
		o << "innerRules: " << printrange(innerRules) << std::endl;
	if( !innerEatoms.empty() )
		o << "innerEatoms: " << printrange(innerEatoms) << std::endl;
	if( !innerConstraints.empty() )
		o << "innerConstraints: " << printrange(innerConstraints) << std::endl;
	return o;
}

std::ostream& ComponentGraph::DependencyInfo::print(std::ostream& o) const
{
	return o << static_cast<const DependencyGraph::DependencyInfo&>(*this);
}

ComponentGraph::ComponentGraph(const DependencyGraph& dg, RegistryPtr reg):
  reg(reg),
  #ifdef COMPGRAPH_SOURCESDEBUG
  dg(dg),
  #endif
  cg()
{
  calculateComponents(dg);
}

ComponentGraph::~ComponentGraph()
{
}

namespace
{
  typedef DependencyGraph::Node Node;
  typedef std::set<Node> NodeSet;
  typedef std::vector<Node> NodeVector;
}

#warning fixed point: eatoms only in positive cycles is misleading, we demand only positive cycles for all atoms! (no negative or disjunctive edge at all)
#if 0
namespace
{
  typedef unsigned Polarity;
  static const Polarity PUNSET = 0x00;
  static const Polarity PPOS =   0x01;
  static const Polarity PNEG =   0x10;
  static const Polarity PBOTH =  0x11;
  typedef boost::vector_property_map<Polarity> PolarityPropertyMap;

	template<typename Graph>
	class PolarityMarkingVisitor:
		public boost::default_bfs_visitor
	{
	public:
		typedef typename Graph::vertex_descriptor Vertex;
		typedef typename Graph::edge_descriptor Edge;

	public:
		PolarityMarkingVisitor(PolarityPropertyMap& ppm):
			ppm(ppm) {}

    /**
     * * for each discovered edge (u,v):
     *   mark v like u if edge positive (add mark)
     *   mark v different from u if edge negative (add mark)
     *   if initial node contain both marks -> return false
     */
		void examine_edge(Edge e, const Graph& g) const
		{
			Vertex from = boost::source(e, g);
			Vertex to = boost::target(e, g);
      DBGLOG(DBG,"examining edge from " << from << " to " << to);

      const DependencyGraph::DependencyInfo& di = g[e];
      #warning TODO review this decision criteria (not sure if "defensive" stuff is necessary)
      bool negative =
        di.negativeRule |
        di.negativeExternal |
        di.unifyingHead; //defensive reasoning
      bool positive =
        di.positiveRegularRule |
        di.positiveConstraint |
        di.unifyingHead | //defensive reasoning
        di.positiveExternal |
        di.externalConstantInput | //defensive reasoning
        di.externalPredicateInput; //defensive reasoning
      Polarity p = ppm[from];
      assert(((p == PPOS) || (p == PNEG)) &&
          "PUNSET means we did not correctly set the last vertex, "
          "PBOTH means we did not abort but we should have");
      Polarity pto = ppm[to];
      DBGLOG(DBG,"  edge is " <<
          (positive?"positive":"") << (negative?"negative":"") << 
          " with frompolarity " << p << " and topolarity " << pto);
      if( positive )
        pto |= p;
      if( negative )
        pto |= (p ^ 0x11); // XOR 0x11 -> invert both bits
      DBGLOG(DBG,"result polarity is " << pto);
      if( pto == PBOTH )
      {
        throw std::string("negcycle");
      }
      ppm[to] = pto;
		}

	protected:
		PolarityPropertyMap& ppm;
	};

  /*
   * strategy for calculation:
	 * * initialize a property map
	 * * for each inner eatom with ID "id"
   *   * init nodesToCheck with PUNSET
   *   * do a bfs visit, never leaving nodesToCheck:
	 *     * for initial vertex: mark as POS
	 *     * for each discovered edge (u,v):
	 *       mark v like u if edge positive (add mark)
	 *       mark v different from u if edge negative (add mark)
	 *       if initial node contain both marks -> return false
	 * * return true
   */
  bool checkEatomsOnlyInPositiveCycles(
      const DependencyGraph& dg,
			const NodeSet& nodesToCheck,
			const NodeVector& innerEatomNodes)
  {
    DBGLOG_SCOPE(DBG,"cEOiPC",false);
    LOG(DBG,"checking whether eatoms in nodes " <<
        printrange(innerEatomNodes) <<
        " are only in positive cycles within SCC of nodes " <<
        printrange(nodesToCheck));

    // init polarity map with size
    DBGLOG(DBG,"initializing property maps");
		PolarityPropertyMap
      ppm(dg.countNodes());
    // init color map with size
    boost::two_bit_color_map<boost::identity_property_map>
      cmap(dg.countNodes());

    // init black color (=do not touch) for nodes not in nodesToCheck
    DependencyGraph::NodeIterator it, it_end;
    for(boost::tie(it, it_end) = dg.getNodes();
        it != it_end; ++it)
    {
      if( nodesToCheck.find(*it) == nodesToCheck.end() )
      {
        boost::put(cmap, *it, boost::two_bit_black);
      }
    }

    try
    {
      BOOST_FOREACH(const Node eatomNode, innerEatomNodes)
      {
        DBGLOG(DBG,"checking for eatom node " << eatomNode);
        // init color and polarity map for nodes in nodesToCheck
        BOOST_FOREACH(const Node n, nodesToCheck)
        {
          boost::put(ppm, n, PUNSET);
          boost::put(cmap, n, boost::two_bit_white);
        }

        // startnode is positive
        ppm[eatomNode] = PPOS;

        PolarityMarkingVisitor<DependencyGraph::Graph>
          vis(ppm);

        // the lousy named parameters simply won't compile and are not documented properly
        boost::breadth_first_visit(
            dg.getInternalGraph(), eatomNode,
            visitor(vis).
            color_map(cmap));
      }
    }
    catch(const std::string& s)
    {
      if( s == "negcycle" )
      {
        LOG(DBG,"found negative cycle!");
        return false;
      }
      else
        throw;
    }
    
    return true;
  }

} // namespace {}
#endif

namespace
{
  /*
   * strategy for calculation:
   * * iterate through all nodes in nodesToCheck
   *   * iterate through outgoing edges
   *   * if negative and leading to node in nodesToCheck return false
	 * * return true
   */
  bool checkNoNegativeEdgesInComponent(
      const DependencyGraph& dg,
			const NodeSet& nodesToCheck)
  {
    DBGLOG_SCOPE(DBG,"cNNEiC",false);
    BOOST_FOREACH(Node n, nodesToCheck)
    {
      DBGLOG(DBG,"checking predecessor edges of node " << n);
      DependencyGraph::PredecessorIterator it, it_end;
      for(boost::tie(it, it_end) = dg.getDependencies(n);
          it != it_end; ++it)
      {
        const DependencyGraph::DependencyInfo& di = dg.propsOf(*it);
        if( di.negativeRule | di.negativeExternal | di.disjunctive )
        {
          // found neg dependency, check if it is within SCC
          Node pnode = dg.targetOf(*it);
          if( nodesToCheck.find(pnode) != nodesToCheck.end() )
          {
            DBGLOG(DBG,"found negative/disjunctive dependency to node " << pnode << " -> not wellfounded");
            return false;
          }
        }
      }
    }

    return true;
  }

  bool checkEatomMonotonic(
      RegistryPtr reg,
      ID eatomid)
  {
    DBGLOG(DBG,"checking whether eatom " << eatomid << " is monotonic");

		// check monotonicity
		const ExternalAtom& eatom = reg->eatoms.getByID(eatomid);
		assert(!!eatom.pluginAtom);
		PluginAtom* pa = eatom.pluginAtom;
		if( pa->isMonotonic() )
		{
			DBGLOG(DBG,"  eatom " << eatomid << " is monotonic");
			return true;
		}
		else
		{
			DBGLOG(DBG,"  eatom " << eatomid << " is nonmonotonic");
			return false;
		}
  }
}

void ComponentGraph::calculateComponents(const DependencyGraph& dg)
{
  LOG_SCOPE(ANALYZE,"cCs",true);
  DBGLOG(ANALYZE,"=calculateComponents");

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
  LOG(ANALYZE,"boost::strong_components created " << scccount << " components");

  //
  // calculate set of nodes for each SCC: sccMembers
  //
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
    const NodeSet& nodes = sccMembers[s];
    Component c = boost::add_vertex(cg);
    DBGLOG(DBG,"created component node " << c << " for scc " << s <<
				" with depgraph nodes " << printrange(nodes));
		bool multimember = nodes.size() > 1;
    NodeVector innerEatomNodes;
    sccToComponent[s] = c;
    ComponentInfo& ci = propsOf(c);
    // collect rule and eatom ids in scc
    for(NodeSet::const_iterator itn = nodes.begin();
        itn != nodes.end(); ++itn)
    {
      #ifdef COMPGRAPH_SOURCESDEBUG
      ci.sources.push_back(*itn);
      #endif
      ID id = dg.propsOf(*itn).id;
      if( id.isRule() )
			{
				if( id.isRegularRule() )
				{
					ci.innerRules.push_back(id);
          if( id.isRuleDisjunctive() )
          {
						ci.disjunctiveHeads = true;
          }
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
        // if the SCC contains more than one node, and it contains external atoms,
        // then they are inner external atoms (there must be some loop)
				if( multimember )
				{
					ci.innerEatoms.push_back(id);
          innerEatomNodes.push_back(*itn);

					if( ci.innerEatomsNonmonotonic == false )
					{
						// check, if the newly added inner eatom is monotonic
						ID eatomid = dg.propsOf(*itn).id;
						if( !checkEatomMonotonic(reg, eatomid) )
						{
							ci.innerEatomsNonmonotonic = true;
						}
          }
				}
				else
				{
					ci.outerEatoms.push_back(id);

					if( ci.outerEatomsNonmonotonic == false )
					{
						// check, if the newly added inner eatom is monotonic
						ID eatomid = dg.propsOf(*itn).id;
						if( !checkEatomMonotonic(reg, eatomid) )
						{
							ci.outerEatomsNonmonotonic = true;
						}
          }
				}
      }
			else
			{
				assert(false);
			}
    }
		
		// check, if the component contains only positive cycles
		if( !checkNoNegativeEdgesInComponent(dg, nodes) )
		{
			ci.negationInCycles = true;
		}

    DBGLOG(DBG,"-> outerEatoms " << printrange(ci.outerEatoms));
    DBGLOG(DBG,"-> innerRules " << printrange(ci.innerRules));
    DBGLOG(DBG,"-> innerConstraints " << printrange(ci.innerConstraints));
    DBGLOG(DBG,"-> innerEatoms " << printrange(ci.innerEatoms));
    DBGLOG(DBG,"-> disjunctiveHeads=" << ci.disjunctiveHeads <<
				" negationInCycles=" << ci.negationInCycles <<
				" innerEatomsNonmonotonic=" << ci.innerEatomsNonmonotonic <<
				" outerEatomsNonmonotonic=" << ci.outerEatomsNonmonotonic);

		assert(( ci.outerEatoms.empty() ||
				    (ci.innerRules.empty() && ci.innerConstraints.empty() && ci.innerEatoms.empty())) &&
				   "components with outer eatoms may not contain anything else");
  }

	#warning TODO if we have just one disjunctive rule inside, we can no longer use fixpoint calculation with inner eatoms, even if they are monotonic and we have only positive cycles ... ci.innerEatomsMonotonicAndOnlyPositiveCycles = false;

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
				DBGLOG(DBG,"found dependency from SCC " << s << " to SCC " << targetscc);

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
	DBGLOG_SCOPE(DBG,"cC", false);
	DBGLOG(DBG,"= collapseComponents(" << printrange(originals) << ")");

	typedef std::map<Component, DependencyInfo> DepMap;

	// set of dependencies from the new component to other components
	DepMap outgoing;
  // set of original components that depend on original components
  ComponentSet internallyDepends;

	// iterate over all originals and over outgoing dependencies
	ComponentSet::const_iterator ito;
	for(ito = originals.begin(); ito != originals.end(); ++ito)
	{
		DBGLOG(DBG,"original " << *ito << ":");
		DBGLOG_INDENT(DBG);

		PredecessorIterator itpred, itpred_end;
		for(boost::tie(itpred, itpred_end) = getDependencies(*ito);
				itpred != itpred_end; ++itpred)
		{
			Dependency outgoing_dep = *itpred;
			Component target = targetOf(outgoing_dep);
			if( originals.count(target) == 0 )
			{
				// dependency not within the new collapsed component
				DBGLOG(DBG,"outgoing dependency to " << target);
				outgoing[target] |= propsOf(outgoing_dep);
			}
      else
      {
				// dependency within the new collapsed component
				DBGLOG(DBG,"internal dependency (to " << target << ")");
				internallyDepends.insert(*ito);
      }
		} // iterate over predecessors
	} // iterate over originals

	// dependencies of other components on the new component
	DepMap incoming;

	// iterate over all originals and over incoming dependencies
  // now also check for duplicate violations
	for(ito = originals.begin(); ito != originals.end(); ++ito)
	{
		DBGLOG(DBG,"original " << *ito << ":");
		DBGLOG_INDENT(DBG);

		SuccessorIterator itsucc, itsucc_end;
		for(boost::tie(itsucc, itsucc_end) = getProvides(*ito);
				itsucc != itsucc_end; ++itsucc)
		{
			Dependency incoming_dep = *itsucc;
			Component source = sourceOf(incoming_dep);
			if( originals.count(source) == 0 )
			{
				// do not count dependencies within the new collapsed component
				DBGLOG(DBG,"incoming dependency from " << source);
				incoming[source] |= propsOf(incoming_dep);
				// ensure that we do not create cycles
        // (this check is not too costly, so this is no assertion but a real runtime check)
				DepMap::const_iterator itdm = outgoing.find(source);
				// if we have an incoming dep and an outgoing dep,
				// we create a cycle so this collapsing is invalid
        // (this is a bug in the code calling collapseComponents!)
        if( itdm != outgoing.end() )
        {
          throw std::runtime_error(
              "collapseComponents tried to create a cycle!");
        }
			}
		} // iterate over successors
	} // iterate over originals

	//
	// we prepared all dependencies, so now we create the component
	//

	Component c = boost::add_vertex(cg);
	LOG(DBG,"created component node " << c << " for collapsed component");

	// build combined component info
	ComponentInfo& ci = propsOf(c);
	for(ito = originals.begin(); ito != originals.end(); ++ito)
	{
		ComponentInfo& cio = propsOf(*ito);
    #ifdef COMPGRAPH_SOURCESDEBUG
		ci.sources.insert(ci.sources.end(),
				cio.sources.begin(), cio.sources.end());
		#endif
    // inner rules stay inner rules
		ci.innerRules.insert(ci.innerRules.end(),
				cio.innerRules.begin(), cio.innerRules.end());
    // inner eatoms always stay inner eatoms, they cannot become outer eatoms
		ci.innerEatoms.insert(ci.innerEatoms.end(),
				cio.innerEatoms.begin(), cio.innerEatoms.end());
    // inner constraints stay inner constraints
		ci.innerConstraints.insert(ci.innerConstraints.end(),
				cio.innerConstraints.begin(), cio.innerConstraints.end());

    ci.disjunctiveHeads |= cio.disjunctiveHeads;
    ci.negationInCycles |= cio.negationInCycles;
		ci.innerEatomsNonmonotonic |= cio.innerEatomsNonmonotonic;

    // if *ito does not depend on any component in originals
    // then outer eatoms stay outer eatoms
    // otherwise they become inner eatoms
    if( internallyDepends.find(*ito) == internallyDepends.end() )
    {
      // does not depend on other components
      ci.outerEatoms.insert(ci.outerEatoms.end(),
          cio.outerEatoms.begin(), cio.outerEatoms.end());
			ci.outerEatomsNonmonotonic |= cio.outerEatomsNonmonotonic;
    }
    else
    {
      // does depend on other components
      // -> former outer eatoms now become inner eatoms
      ci.innerEatoms.insert(ci.innerEatoms.end(),
          cio.outerEatoms.begin(), cio.outerEatoms.end());

      // here, outer eatom becomes inner eatom
			ci.innerEatomsNonmonotonic |= cio.outerEatomsNonmonotonic;
    }
    #warning if "input" component consists only of eatoms, they may be nonmonotonic, and we still can have wellfounded model generator ... create testcase for this ? how about wellfounded2.hex?
	}

	// build incoming dependencies
	for(DepMap::const_iterator itd = incoming.begin();
			itd != incoming.end(); ++itd)
	{
		Dependency newdep;
		bool success;
		DBGLOG(DBG,"adding edge " << itd->first << " -> " << c);
		boost::tie(newdep, success) = boost::add_edge(itd->first, c, itd->second, cg);
		assert(success); // we only add new edges here, and each only once
	}

	// build outgoing dependencies
	for(DepMap::const_iterator itd = outgoing.begin();
			itd != outgoing.end(); ++itd)
	{
		Dependency newdep;
		bool success;
		DBGLOG(DBG,"adding edge " << c << " -> " << itd->first);
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
	void printoutVerboseIfNotEmpty(std::ostream& o, RegistryPtr reg, const char* prefix, Range idrange)
	{
		// see boost/range/iterator_range.hpp
		typedef typename Range::const_iterator Iterator;
		if( !boost::empty(idrange) )
		{
			o << "{" << prefix << "|";
			graphviz::escape(o, printManyToString<RawPrinter>(idrange, "\n", reg));
			o << "}|";
		}
	}

	template<typename Range>
	void printoutTerseIfNotEmpty(std::ostream& o, RegistryPtr reg, const char* prefix, Range idrange)
	{
		// see boost/range/iterator_range.hpp
		typedef typename Range::const_iterator Iterator;
		if( !boost::empty(idrange) )
		{
			unsigned count = 0;
			for(Iterator it = boost::begin(idrange); it != boost::end(idrange); ++it, ++count)
				;
			o << "{" << prefix << ":" << count << "}|";
		}
	}
}

void ComponentGraph::writeGraphVizComponentLabel(std::ostream& o, Component c, unsigned index, bool verbose) const
{
  const ComponentInfo& ci = getComponentInfo(c);
  if( verbose )
  {
    o << "{idx=" << index << ",component=" << c << "|";
    #ifdef COMPGRAPH_SOURCESDEBUG
    o << "{sources|" << printrange(ci.sources, "\\{", ",", "\\}") << "}|";
    #endif
		printoutVerboseIfNotEmpty(o, reg, "outerEatoms", ci.outerEatoms);
		printoutVerboseIfNotEmpty(o, reg, "innerRules", ci.innerRules);
		printoutVerboseIfNotEmpty(o, reg, "innerEatoms", ci.innerEatoms);
		printoutVerboseIfNotEmpty(o, reg, "innerConstraints", ci.innerConstraints);
    if( !ci.innerRules.empty() )
		{
			if( ci.disjunctiveHeads )
				o << "{rules contain disjunctive heads}|";
			if( ci.negationInCycles )
				o << "{rules contain negation in cycles}|";
		}
    if( !ci.innerEatoms.empty() || ci.innerEatomsNonmonotonic )
      o << "{inner eatoms nonmonotonic}|";
    if( !ci.outerEatoms.empty() || ci.outerEatomsNonmonotonic )
      o << "{outer eatoms nonmonotonic}|";
		o << "}";
  }
  else
  {
    o << "{idx=" << index << "|";
		printoutTerseIfNotEmpty(o, reg, "outerEatoms", ci.outerEatoms);
		printoutTerseIfNotEmpty(o, reg, "innerRules", ci.innerRules);
		printoutTerseIfNotEmpty(o, reg, "innerEatoms", ci.innerEatoms);
		printoutTerseIfNotEmpty(o, reg, "innerConstraints", ci.innerConstraints);
    if( !ci.innerRules.empty() )
		{
			if( ci.disjunctiveHeads )
				o << "{rules contain disjunctive heads}|";
			if( ci.negationInCycles )
				o << "{rules contain negation in cycles}|";
		}
    if( !ci.innerEatoms.empty() || ci.innerEatomsNonmonotonic )
      o << "{inner eatoms nonmonotonic}|";
    if( !ci.outerEatoms.empty() || ci.outerEatomsNonmonotonic )
      o << "{outer eatoms nonmonotonic}|";
		o << "}";
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
  unsigned index = 0;
  for(boost::tie(it, it_end) = boost::vertices(cg);
      it != it_end; ++it, ++index)
  {
    o << graphviz_node_id(*it) << "[shape=record,label=\"";
    {
      writeGraphVizComponentLabel(o, *it, index, verbose);
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
      writeGraphVizDependencyLabel(o, *dit, verbose);
    }
    o << "\"];" << std::endl;
  }

  o << "}" << std::endl;
}

ComponentGraph::ComponentGraph(const ComponentGraph& other):
	reg(other.reg),
  #ifdef COMPGRAPH_SOURCESDEBUG
	dg(other.dg),
  #endif
	cg(other.cg)
{
}

// for explicit cloning of the graph
ComponentGraph* ComponentGraph::clone() const
{
	return new ComponentGraph(*this);
}


DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
