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

#include "dlvhex2/ComponentGraph.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/GraphvizHelpers.h"

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
  DBGLOG(DBG, "Building component graph");
  calculateComponents(dg);
}

ComponentGraph::~ComponentGraph()
{
  DBGLOG(DBG, "Destructing component graph " << this);
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
		if( eatom.getExtSourceProperties().isMonotonic() )
//		if( pa->isMonotonic() )
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
    ci.componentIsMonotonic = true; // assume it's monotonic

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

		// check if the rule uses default negation
		const Rule& r = reg->rules.getByID(id);
		BOOST_FOREACH (ID b, r.body){
			if (b.isNaf()) ci.componentIsMonotonic = false;
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

    // components are never monotonic if they contain disjunctions or nonmonotonic external atoms
    if (ci.disjunctiveHeads || ci.innerEatomsNonmonotonic || ci.outerEatomsNonmonotonic){
      ci.componentIsMonotonic = false;
    }

    // compute if this component has a fixed domain
    ci.fixedDomain = calculateFixedDomain(ci);

    // check, if the component contains recursive aggregates
    ci.recursiveAggregates = computeRecursiveAggregatesInComponent(ci);

    // compute stratification of default-negated literals and predicate input parameters
    calculateStratificationInfo(reg, ci);

    DBGLOG(DBG,"-> outerEatoms " << printrange(ci.outerEatoms));
    DBGLOG(DBG,"-> innerRules " << printrange(ci.innerRules));
    DBGLOG(DBG,"-> innerConstraints " << printrange(ci.innerConstraints));
    DBGLOG(DBG,"-> innerEatoms " << printrange(ci.innerEatoms));
    DBGLOG(DBG,"-> disjunctiveHeads=" << ci.disjunctiveHeads <<
				" negationInCycles=" << ci.negationInCycles <<
				" innerEatomsNonmonotonic=" << ci.innerEatomsNonmonotonic <<
				" outerEatomsNonmonotonic=" << ci.outerEatomsNonmonotonic <<
				" componentIsMonotonic=" << ci.componentIsMonotonic);

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


bool ComponentGraph::calculateFixedDomain(ComponentInfo& ci)
{
	DBGLOG(DBG, "calculateFixedDomain");

	bool fd = true;

	// pure external components have only a fixed domain if the output of all outer external atoms
	// contains no variables
	if (ci.innerRules.empty() && !ci.outerEatoms.empty()){
		BOOST_FOREACH (ID eaid, ci.outerEatoms){
			const ExternalAtom& ea = reg->eatoms.getByID(eaid);
			BOOST_FOREACH (ID ot, ea.tuple){
				if (ot.isVariableTerm()) return false;
			}
		}
		return true;
	}

	// get rule heads here
	// here we store the full atom IDs (we need to unify, the predicate is not sufficient)
	std::set<ID> headAtomIDs;
	// we only consider inner rules (constraints have no heads)
	BOOST_FOREACH(ID rid, ci.innerRules)
	{
		const Rule& rule = reg->rules.getByID(rid);
	
		BOOST_FOREACH(ID hid, rule.head)
		{
			if( !hid.isOrdinaryAtom() )
			{
				continue;
			}
			headAtomIDs.insert(hid);
		}
	}

	// now check output variables

	// here we need to check inner rules and inner constraints
	std::vector<ID>* ruleSets[] = {&ci.innerRules, &ci.innerConstraints};
	BOOST_FOREACH (std::vector<ID>* ruleSet, ruleSets){
		BOOST_FOREACH(ID rid, *ruleSet)
		{
			if( !rid.doesRuleContainExtatoms() )
			{
				continue;
			}

			const Rule& rule = reg->rules.getByID(rid);

			// find all variable outputs in all eatoms in this rule's body
			std::set<ID> varsToCheck;
			BOOST_FOREACH(ID lid, rule.body)
			{
				if( !lid.isExternalAtom() )
					continue;

				const ExternalAtom& eatom = reg->eatoms.getByID(lid);
				BOOST_FOREACH(ID tid, eatom.tuple)
				{
					if( tid.isVariableTerm() )
						varsToCheck.insert(tid);
				}
			}

			// for each variable:
			// if it is part of a positive body atom of r
			// and this positive body atom of r does not unify with any rule head in c
			// then e is safe
			BOOST_FOREACH(ID vid, varsToCheck)
			{
				// check strong safety of variable vid
				DBGLOG(DBG,"checking fixed domain of variable " << 
						printToString<RawPrinter>(vid,reg));

				bool variableSafe = false;
				BOOST_FOREACH(ID lid, rule.body)
				{
					// skip negative bodies
					if( lid.isNaf() )
						continue;

					// skip external atoms,
					// they could but cannot in general be assumed to limit the domain
					// (and that's the reason we need to check strong safety)
					if( lid.isExternalAtom() )
						continue;

					// skip non-ordinary atoms
					#warning can we use aggregates to limit the domain for strong safety?
					#warning can we use builtin atoms to limit the domain for strong safety?
					if( lid.isAggregateAtom() ||
							lid.isBuiltinAtom() )
						continue;

					assert(lid.isOrdinaryAtom());

					// check if this body literal contains the variable
					// and does not unify with any head
					// (only then the variable is safe)
					bool containsVariable = false;
					const OrdinaryAtom& oatom = reg->lookupOrdinaryAtom(lid);
					assert(!oatom.tuple.empty());
					Tuple::const_iterator itv = oatom.tuple.begin();
					itv++;
					while( itv != oatom.tuple.end() )
					{
						if( *itv == vid )
						{
							containsVariable = true;
							break;
						}
						itv++;
					}

					if( !containsVariable )
					{
						continue;
					}

					// oatom 'oatom' was retrieved using ID 'lid'
					DBGLOG(DBG,"checking unifications of body literal " <<
							printToString<RawPrinter>(lid, reg) << " with component rule heads");
					bool doesNotUnify = true;
					BOOST_FOREACH(ID hid, headAtomIDs)
					{
						DBGLOG(DBG,"checking against " <<
								printToString<RawPrinter>(hid, reg));
						assert(hid.isOrdinaryAtom());
						const OrdinaryAtom& hoatom = reg->lookupOrdinaryAtom(hid);
						if( oatom.unifiesWith(hoatom) )
						{
							DBGLOG(DBG,"unification successful "
									"-> literal does not limit the domain");
							doesNotUnify = false;
							break;
						}
					}

					if( doesNotUnify )
					{
						DBGLOG(DBG, "variable safe!");
						variableSafe = true;
						break;
					}
				}

				if( !variableSafe )
				{
					// check if the variable occurs in an external atom with unstratified nonmonotonic parameters
					bool nonmonotonicEA = false;
					BOOST_FOREACH(ID lid, rule.body)
					{
						if(!lid.isNaf() && lid.isExternalAtom()){
							if (ci.stratifiedLiterals.find(rid) == ci.stratifiedLiterals.end() ||
							    std::find(ci.stratifiedLiterals.at(rid).begin(), ci.stratifiedLiterals.at(rid).end(), lid) == ci.stratifiedLiterals.at(rid).end()){
								nonmonotonicEA = true;
								break;
							}
						}
					}

					fd = false;
				}else{
					DBGLOG(DBG, "Variable " << vid << " is strongly safe in rule " << rid << " (" << &ci << ")");
					ci.stronglySafeVariables[rid].insert(vid);
				}
			}
		}
	}
	return fd;
}


bool ComponentGraph::computeRecursiveAggregatesInComponent(ComponentInfo& ci)
{
	// get all head predicates
	std::set<ID> headPredicates;
	BOOST_FOREACH (ID ruleID, ci.innerRules){
		const Rule& rule = reg->rules.getByID(ruleID);
		BOOST_FOREACH (ID h, rule.head){
			const OrdinaryAtom& oatom = reg->lookupOrdinaryAtom(h);
			headPredicates.insert(oatom.tuple[0]);
		}
	}

	// go through all aggregate atoms
	std::set<ID> aatoms;
	BOOST_FOREACH (ID ruleID, ci.innerRules){
		const Rule& rule = reg->rules.getByID(ruleID);
		BOOST_FOREACH (ID b, rule.body){
			if (b.isAggregateAtom()){
				aatoms.insert(b);
			}
		}
	}

	// recursively check if the aggregate depend on head atoms from this component
	while (!aatoms.empty()){
		const AggregateAtom& aatom = reg->aatoms.getByID(*aatoms.begin());
		aatoms.erase(*aatoms.begin());
		BOOST_FOREACH (ID b, aatom.literals){
			if (b.isOrdinaryAtom()){
				const OrdinaryAtom& oatom = reg->lookupOrdinaryAtom(b);	
				if (headPredicates.find(oatom.tuple[0]) != headPredicates.end()) return true;
			}
			if (b.isExternalAtom()){
				const ExternalAtom& eatom = reg->eatoms.getByID(b);
				int i = 0;
				BOOST_FOREACH (ID inp, eatom.inputs){
					if (eatom.pluginAtom->getInputType(i) == PluginAtom::PREDICATE && headPredicates.find(eatom.inputs[0]) != headPredicates.end()) return true;
					i++;
				}
			}
			if (b.isAggregateAtom()){
				aatoms.insert(b);
			}
		}
	}

	return false;
}

bool ComponentGraph::calculateStratificationInfo(RegistryPtr reg, ComponentInfo& ci)
{
	DBGLOG(DBG, "calculateStratificationInfo");

	// get the head atoms of all rules in this component
	std::set<ID> headAtomIDs;
	BOOST_FOREACH(ID rid, ci.innerRules)
	{
		const Rule& rule = reg->rules.getByID(rid);
		
		BOOST_FOREACH(ID hid, rule.head)
		{
			if (!hid.isOrdinaryAtom()) continue;
			headAtomIDs.insert(hid);

			const OrdinaryAtom& oatom = reg->lookupOrdinaryAtom(hid);
			ci.predicatesInComponent.insert(oatom.tuple[0]);
		}
	}

	// for all default-negated literals and predicate input parameters in this component
	BOOST_FOREACH(ID rid, ci.innerRules)
	{
		const Rule& rule = reg->rules.getByID(rid);
		
		BOOST_FOREACH(ID bid, rule.body)
		{
			// default-negated literals
			if (!bid.isExternalAtom() && bid.isNaf()){
				// does it unify with a head atom in this component?
				bool stratified = true;
				BOOST_FOREACH (ID hid, headAtomIDs){
					const OrdinaryAtom& boatom = reg->lookupOrdinaryAtom(bid);
					const OrdinaryAtom& hoatom = reg->lookupOrdinaryAtom(hid);
					if (boatom.unifiesWith(hoatom)){
						stratified = false;
						break;
					}
				}
				if (stratified){
					ci.stratifiedLiterals[rid].insert(bid);
				}
			}
			// predicate input parameters
			if (bid.isExternalAtom() && !bid.isNaf()){
				const ExternalAtom& eatom = reg->eatoms.getByID(bid);
				bool stratified = true;
				for (int p = 0; p < eatom.inputs.size() && stratified; ++p){
					if (eatom.pluginAtom->getInputType(p) == PluginAtom::PREDICATE && eatom.getExtSourceProperties().isNonmonotonic(p)){
						// is this predicate defined in this component?
						if (ci.predicatesInComponent.count(eatom.inputs[p]) > 0){
							stratified = false;
							break;
						}
					}
				}
				if (stratified){
					ci.stratifiedLiterals[rid].insert(bid);
				}
			}
		}
	}
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
  #warning ComponentGraph::collapseComponents is deprecated, please use EvalGraphBuilder::createEvalUnit
  //LOG(WARNING,"ComponentGraph::collapseComponents is deprecated, please use EvalGraphBuilder::createEvalUnit");

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
	bool foundInternalNegativeRuleDependency = false;

	// iterate over all originals and over incoming dependencies
  // now also check for duplicate violations
	for(ito = originals.begin(); ito != originals.end(); ++ito)
	{
		DBGLOG(DBG,"original " << *ito << ":");
		DBGLOG_INDENT(DBG);

		// go over dependencies to original members of new component
		SuccessorIterator itsucc, itsucc_end;
		for(boost::tie(itsucc, itsucc_end) = getProvides(*ito);
				itsucc != itsucc_end; ++itsucc)
		{
			Dependency incoming_dep = *itsucc;
			Component source = sourceOf(incoming_dep);
			const DependencyInfo& incoming_di = propsOf(incoming_dep);
			if( originals.count(source) == 0 )
			{
				// the dependency comes from outside the new component

				DBGLOG(DBG,"incoming dependency from " << source);
				incoming[source] |= incoming_di;
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
			else
			{
				// the dependency comes from inside the new component (to inside)
				if( incoming_di.negativeRule )
					foundInternalNegativeRuleDependency = true;
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
    // information about strongly safe variables and stratified literals
		typedef std::pair<ID, std::set<ID> > Pair;
		BOOST_FOREACH (Pair p, cio.stronglySafeVariables){
			BOOST_FOREACH (ID id, p.second){
				ci.stronglySafeVariables[p.first].insert(id);
			}
		}
		ci.predicatesInComponent.insert(
				cio.predicatesInComponent.begin(), cio.predicatesInComponent.end());
		/*
		BOOST_FOREACH (Pair p, cio.stratifiedLiterals){
			BOOST_FOREACH (ID id, p.second){
				ci.stratifiedLiterals[p.first].insert(id);
			}
		}
		*/

		ci.disjunctiveHeads |= cio.disjunctiveHeads;
		// if we collapse two components which have no negation inside them,
		// but they negatively depend on each other, we must set this to true
		// example: a :- b. and :- not a. are collapsed -> resulting component has negationInCycles
		// TODO fix name: negationInCycles really should be negativeDependencyBetweenRules
		ci.negationInCycles |= cio.negationInCycles | foundInternalNegativeRuleDependency;
		// (we do not need to check for nonmonotonic dependencies from external atoms
		// which become internal nonmonotonic dependencies, because such dependencies
		// are handled by the innerEatomsNonmonotonic flag which will get true if there
		// any external atom that can create such a nonmonotonic dependency, e.g. nonmon_noloop.hex)
		ci.innerEatomsNonmonotonic |= cio.innerEatomsNonmonotonic;
		ci.componentIsMonotonic &= cio.componentIsMonotonic;

		// fixedDomain:
		// pure external components shall have no influence on this property
		// because domain restriction is always done in successor components
		if (!(!cio.outerEatoms.empty() && cio.innerRules.empty()))
			ci.fixedDomain &= cio.fixedDomain;

		ci.recursiveAggregates |= cio.recursiveAggregates;

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

	// recalculate stratification for the collapsed component
	calculateStratificationInfo(reg, ci);

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
    if( !ci.innerEatoms.empty() && ci.innerEatomsNonmonotonic )
      o << "{inner eatoms nonmonotonic}|";
    if( !ci.outerEatoms.empty() && ci.outerEatomsNonmonotonic )
      o << "{outer eatoms nonmonotonic}|";

    if( ci.fixedDomain )
      o << "{fixed domain}|";
    if( ci.recursiveAggregates )
      o << "{recursive aggregates}|";
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
  DBGLOG(DBG, "Building component graph");
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
