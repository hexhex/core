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
#include "dlvhex/Printer.hpp"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/Registry.hpp"
#include "dlvhex/GraphvizHelpers.hpp"

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

  bool checkEatomMonotonic(
      RegistryPtr reg,
      const DependencyGraph& dg,
      DependencyGraph::Node eatomnode)
  {
    return checkEatomMonotonic(reg, dg.propsOf(eatomnode).id);
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
    //createComponentFromDepgraphSCC(dg, scc, *itc);
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
          #warning innerEatomsMonotonicAndOnlyPositiveCycles should be called innerEatomsMonotonicAndInnerRulesFixpointCalculateable
          if( id.isRuleDisjunctive() )
          {
            // if we have just one disjunctive rule inside, we can no longer use fixpoint calculation with inner eatoms, even if they are monotonic and we have only positive cycles
            ci.innerEatomsMonotonicAndOnlyPositiveCycles = false;
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
          // only if we still think that we can use wellfounded model building
          if( ci.innerEatomsMonotonicAndOnlyPositiveCycles )
          {
            // check, if the newly added inner eatom is monotonic
            ci.innerEatomsMonotonicAndOnlyPositiveCycles &=
              checkEatomMonotonic(reg, dg, *itn);
          }
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
    DBGLOG(DBG,"-> innerEatomsMonotonicAndOnlyPositiveCycles " << ci.innerEatomsMonotonicAndOnlyPositiveCycles);

		// do positive cycle check if all eatoms monotonic
		// (i.e., only if we still think that we can use wellfounded model building)
		if( ci.innerEatomsMonotonicAndOnlyPositiveCycles )
		{
			// check, if the component contains only positive cycles
			ci.innerEatomsMonotonicAndOnlyPositiveCycles &=
        checkNoNegativeEdgesInComponent(dg, nodes);
		}

    DBGLOG(DBG,"-> outerEatoms " << printrange(ci.outerEatoms));
    DBGLOG(DBG,"-> innerRules " << printrange(ci.innerRules));
    DBGLOG(DBG,"-> innerConstraints " << printrange(ci.innerConstraints));
    DBGLOG(DBG,"-> innerEatoms " << printrange(ci.innerEatoms));
    DBGLOG(DBG,"-> innerEatomsMonotonicAndOnlyPositiveCycles " << ci.innerEatomsMonotonicAndOnlyPositiveCycles);

		assert( ci.outerEatoms.empty() ||
				   (ci.innerRules.empty() && ci.innerConstraints.empty() && ci.innerEatoms.empty()));
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
			o << "{" << prefix << ":" << count << "}|";
		}
	}
}

void ComponentGraph::writeGraphVizComponentLabel(std::ostream& o, Component c, unsigned index, bool verbose) const
{
  const ComponentInfo& ci = getComponentInfo(c);
	RawPrinter rp(o, reg);
  if( verbose )
  {
    o << "{idx=" << index << ",component=" << c << "|";
    #ifdef COMPGRAPH_SOURCESDEBUG
    o << "{sources|" << printrange(ci.sources, "\\{", ",", "\\}") << "}|";
    #endif
		printoutVerboseIfNotEmpty(o, rp, "outerEatoms", ci.outerEatoms);
		printoutVerboseIfNotEmpty(o, rp, "innerRules", ci.innerRules);
		printoutVerboseIfNotEmpty(o, rp, "innerEatoms", ci.innerEatoms);
		printoutVerboseIfNotEmpty(o, rp, "innerConstraints", ci.innerConstraints);
    if( !ci.innerEatoms.empty() || !ci.innerEatomsMonotonicAndOnlyPositiveCycles )
    {
      o << "{fixpoint?|";
      if( ci.innerEatomsMonotonicAndOnlyPositiveCycles )
        o << "true";
      else
        o << "false";
      o << "}|";
    }
		o << "}";
  }
  else
  {
    o << "{idx=" << index << "|";
		printoutTerseIfNotEmpty(o, rp, "outerEatoms", ci.outerEatoms);
		printoutTerseIfNotEmpty(o, rp, "innerRules", ci.innerRules);
		printoutTerseIfNotEmpty(o, rp, "innerEatoms", ci.innerEatoms);
		printoutTerseIfNotEmpty(o, rp, "innerConstraints", ci.innerConstraints);
    if( !ci.innerEatoms.empty() || !ci.innerEatomsMonotonicAndOnlyPositiveCycles )
    {
      o << "{fixpoint?|";
      if( ci.innerEatomsMonotonicAndOnlyPositiveCycles )
        o << "true";
      else
        o << "false";
      o << "}|";
    }
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
      std::ostringstream ss;
      writeGraphVizComponentLabel(ss, *it, index, verbose);
			graphviz::escape(o, ss.str());
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
      std::ostringstream ss;
      writeGraphVizDependencyLabel(o, *dit, verbose);
			graphviz::escape(o, ss.str());
    }
    o << "\"];" << std::endl;
  }

  o << "}" << std::endl;
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
