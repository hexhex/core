/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2016 Peter Schüller
 * Copyright (C) 2011-2016 Christoph Redl
 * Copyright (C) 2015-2016 Tobias Kaminski
 * Copyright (C) 2015-2016 Antonius Weinzierl
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
 * @author Peter Schller
 *
 * @brief Implementation of the component graph.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

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

namespace
{
    typedef DependencyGraph::Node Node;
    typedef std::set<Node> NodeSet;
    typedef std::vector<Node> NodeVector;

    typedef ComponentGraph::ComponentSet ComponentSet;
    typedef ComponentGraph::ComponentInfo ComponentInfo;
    typedef ComponentGraph::DependencyInfo DependencyInfo;
    typedef ComponentGraph::DepMap DepMap;
}


const ComponentGraph::DependencyInfo&
ComponentGraph::DependencyInfo::operator|=(
const ComponentGraph::DependencyInfo& other)
{
    DependencyGraph::DependencyInfo::operator|=(other);

    BOOST_FOREACH (DepEdge de, other.depEdges) depEdges.push_back(de);

    //	depEdges.insert(depEdges.end(), other.depEdges.begin(), other.depEdges.end());

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


ComponentGraph::ComponentGraph(const DependencyGraph& dg, ProgramCtx& ctx, RegistryPtr reg):
ctx(ctx),
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


WARNING("fixed point: eatoms only in positive cycles is misleading, we demand only positive cycles for all atoms! (no negative or disjunctive edge at all)")
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
                WARNING("TODO review this decision criteria (not sure if "defensive" stuff is necessary)")
                    bool negative =
                    di.negativeRule |
                    di.negativeExternal |
                    di.unifyingHead;//defensive reasoning
                bool positive =
                    di.positiveRegularRule |
                    di.positiveConstraint |
                    di.unifyingHead |//defensive reasoning
                    di.positiveExternal |
                                 //defensive reasoning
                    di.externalConstantInput |
                                 //defensive reasoning
                    di.externalPredicateInput;
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
                                 // XOR 0x11 -> invert both bits
                        pto |= (p ^ 0x11);
                DBGLOG(DBG,"result polarity is " << pto);
                if( pto == PBOTH ) {
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
    const NodeVector& innerEatomNodes) {
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
        it != it_end; ++it) {
            if( nodesToCheck.find(*it) == nodesToCheck.end() ) {
                boost::put(cmap, *it, boost::two_bit_black);
            }
        }

        try
        {
            BOOST_FOREACH(const Node eatomNode, innerEatomNodes) {
                DBGLOG(DBG,"checking for eatom node " << eatomNode);
                // init color and polarity map for nodes in nodesToCheck
                BOOST_FOREACH(const Node n, nodesToCheck) {
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
        catch(const std::string& s) {
            if( s == "negcycle" ) {
                LOG(DBG,"found negative cycle!");
                return false;
            }
            else
                throw;
        }

        return true;
    }

}                                // namespace {}
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
    const NodeSet& nodesToCheck) {
        DBGLOG_SCOPE(DBG,"cNNEiC",false);
        BOOST_FOREACH(Node n, nodesToCheck) {
            DBGLOG(DBG,"checking predecessor edges of node " << n);
            DependencyGraph::PredecessorIterator it, it_end;
            for(boost::tie(it, it_end) = dg.getDependencies(n);
            it != it_end; ++it) {
                const DependencyGraph::DependencyInfo& di = dg.propsOf(*it);
                if( di.negativeRule | di.negativeExternal | di.disjunctive ) {
                    // found neg dependency, check if it is within SCC
                    Node pnode = dg.targetOf(*it);
                    if( nodesToCheck.find(pnode) != nodesToCheck.end() ) {
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
    ID eatomid) {
        DBGLOG(DBG,"checking whether eatom " << eatomid << " is monotonic");

        // check monotonicity
        const ExternalAtom& eatom = reg->eatoms.getByID(eatomid);
        assert(!!eatom.pluginAtom);
        PluginAtom* pa = eatom.pluginAtom;
        if( eatom.getExtSourceProperties().isMonotonic() ) {
            //		if( pa->isMonotonic() )
            DBGLOG(DBG,"  eatom " << eatomid << " is monotonic");
            return true;
        }
        else {
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
    unsigned scccount = boost::strong_components(dg.getInternalGraph(), boost::make_iterator_property_map(scc.begin(), get(boost::vertex_index, dg.getInternalGraph())));
    LOG(ANALYZE,"boost::strong_components created " << scccount << " components");

    //
    // calculate set of nodes for each SCC: sccMembers
    //
    typedef std::vector<NodeSet> SCCMap;
    SCCMap sccMembers;
    sccMembers.resize(scccount);
    for(unsigned n = 0; n < dg.countNodes(); ++n) {
        // get the component id from scc[n]
        // add the node id to the set of nodes of this component
        sccMembers[scc[n]].insert(static_cast<Node>(n));
    }

    //
    // create one component for each SCC
    //
    typedef std::vector<Component> SCCToComponentMap;
    SCCToComponentMap sccToComponent(scccount);
    for(unsigned s = 0; s < scccount; ++s) {
        const NodeSet& nodes = sccMembers[s];

        Component c = boost::add_vertex(cg);
        DBGLOG(DBG,"created component node " << c << " for scc " << s <<
            " with depgraph nodes " << printrange(nodes));
        bool multimember = nodes.size() > 1;
        NodeVector innerEatomNodes;
        sccToComponent[s] = c;
        ComponentInfo& ci = propsOf(c);
                                 // assume it's monotonic
        ci.componentIsMonotonic = true;

        // collect rule and eatom ids in scc
        for(NodeSet::const_iterator itn = nodes.begin();
        itn != nodes.end(); ++itn) {
            #ifdef COMPGRAPH_SOURCESDEBUG
            ci.sources.push_back(*itn);
            #endif
            ID id = dg.propsOf(*itn).id;
            if( id.isRule() ) {
                if( id.isRegularRule() ) {
                    ci.innerRules.push_back(id);
                    if( id.isRuleDisjunctive() ) {
                        ci.disjunctiveHeads = true;
                    }
                }
                else if( id.isConstraint() || id.isWeakConstraint() ) {
                    ci.innerConstraints.push_back(id);
                }
                else {
                    assert(false);
                }

                // check if the rule uses default negation
                const Rule& r = reg->rules.getByID(id);
                BOOST_FOREACH (ID b, r.body) {
                    if (b.isNaf()) ci.componentIsMonotonic = false;
                }
            }
            else if( id.isExternalAtom() ) {
                // if the SCC contains more than one node, and it contains external atoms,
                // then they are inner external atoms (there must be some loop)
                if( multimember || ctx.config.getOption("NoOuterExternalAtoms") ) {
                    ci.innerEatoms.push_back(id);
                    innerEatomNodes.push_back(*itn);

                    if( ci.innerEatomsNonmonotonic == false ) {
                        // check, if the newly added inner eatom is monotonic
                        ID eatomid = dg.propsOf(*itn).id;
                        if( !checkEatomMonotonic(reg, eatomid) ) {
                            ci.innerEatomsNonmonotonic = true;
                        }
                    }
                }
                else {
                    ci.outerEatoms.push_back(id);

                    if( ci.outerEatomsNonmonotonic == false ) {
                        // check, if the newly added inner eatom is monotonic
                        ID eatomid = dg.propsOf(*itn).id;
                        if( !checkEatomMonotonic(reg, eatomid) ) {
                            ci.outerEatomsNonmonotonic = true;
                        }
                    }
                }
            }
            else {
                assert(false);
            }
        }

        // check, if the component contains only positive cycles
        if( !checkNoNegativeEdgesInComponent(dg, nodes) ) {
            ci.negativeDependencyBetweenRules = true;
        }

        // components are never monotonic if they contain disjunctions or nonmonotonic external atoms
        if (ci.disjunctiveHeads || ci.innerEatomsNonmonotonic || ci.outerEatomsNonmonotonic) {
            ci.componentIsMonotonic = false;
        }

        // compute if this component has a fixed domain
        ci.fixedDomain = calculateFixedDomain(ci);

        // check, if the component contains recursive aggregates
        // Note: this also includes aggregates which depend on predicates defined in the component, even if there is no cyclic dependency!
        ci.recursiveAggregates = computeRecursiveAggregatesInComponent(ci);

        // recursive aggregates in the initial component graph are disallowed
        // however, they might occur after collapsing components because then they are not strictly recursive (see above)
        if (ci.recursiveAggregates && !ctx.config.getOption("AllowAggCycles")) throw GeneralError("Program contains recursive aggregates");

        // compute stratification of default-negated literals and predicate input parameters
        calculateStratificationInfo(reg, ci);

        // compute all predicate which occur in this component
        calculatePredicatesOfComponent(reg, ci);

        DBGLOG(DBG,"-> outerEatoms " << printrange(ci.outerEatoms));
        DBGLOG(DBG,"-> innerRules " << printrange(ci.innerRules));
        DBGLOG(DBG,"-> innerConstraints " << printrange(ci.innerConstraints));
        DBGLOG(DBG,"-> innerEatoms " << printrange(ci.innerEatoms));
        DBGLOG(DBG,"-> disjunctiveHeads=" << ci.disjunctiveHeads <<
            " negativeDependencyBetweenRules=" << ci.negativeDependencyBetweenRules <<
            " innerEatomsNonmonotonic=" << ci.innerEatomsNonmonotonic <<
            " outerEatomsNonmonotonic=" << ci.outerEatomsNonmonotonic <<
            " componentIsMonotonic=" << ci.componentIsMonotonic);

        assert(( ci.outerEatoms.empty() ||
            (ci.innerRules.empty() && ci.innerConstraints.empty() && ci.innerEatoms.empty())) &&
            "components with outer eatoms may not contain anything else");
    }

    WARNING("TODO if we have just one disjunctive rule inside, we can no longer use fixpoint calculation with inner eatoms, even if they are monotonic and we have only positive cycles ... ci.innerEatomsMonotonicAndOnlyPositiveCycles = false;")

    //
    // create dependencies between components (now that all of them exist)
    //
    for(unsigned s = 0; s < scccount; ++s) {
        const NodeSet& nodes = sccMembers[s];
        Component c = sccToComponent[s];

        // look at out-dependencies only
        // (successors will find and create all dependencies to this SCC)
        for(NodeSet::const_iterator itn = nodes.begin();
        itn != nodes.end(); ++itn) {
            DependencyGraph::PredecessorIterator it, it_end;
            for(boost::tie(it, it_end) = dg.getDependencies(*itn);
            it != it_end; ++it) {
                DependencyGraph::Dependency dep = *it;
                Node targetnode = dg.targetOf(dep);
                unsigned targetscc = scc[targetnode];
                if( targetscc == s ) {
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
                if( !success ) {
                    boost::tie(newdep, success) = boost::edge(c, targetc, cg);
                    assert(success);
                    // update existing dependency
                    propsOf(newdep) |= di;
                }

                // store original dependencies from the DependencyGraph
                const DependencyGraph::NodeInfo& ni1 = dg.getNodeInfo(dg.sourceOf(dep));
                const DependencyGraph::NodeInfo& ni2 = dg.getNodeInfo(dg.targetOf(dep));
                propsOf(newdep).depEdges.push_back(DependencyInfo::DepEdge(ni1.id, ni2.id, di));

            }                    // for each dependency of *itn
        }                        // collect dependencies outgoing from node *itn in SCC s
    }                            // create dependencies outgoing from SCC s

    // collapse components with predecessors which contain only external atoms
    if (ctx.config.getOption("NoOuterExternalAtoms")){
        ComponentIterator cit = getComponents().first;
        while(cit != getComponents().second) {
            bool restart = false;
            // for predecessors
            ComponentGraph::PredecessorIterator pit, pit_end;
            for (boost::tie(pit, pit_end) = getDependencies(*cit); pit != pit_end; ++pit) {
                Component dependsOn = targetOf(*pit);
                if (getComponentInfo(dependsOn).innerConstraints.size() == 0 && getComponentInfo(dependsOn).innerRules.size() == 0
                    && getComponentInfo(dependsOn).innerEatoms.size() == 1 && getComponentInfo(dependsOn).outerEatoms.size() == 0){

                    ComponentSet collapse;
                    collapse.insert(*cit);
                    collapse.insert(dependsOn);
                    collapseComponents(collapse);
                    restart = true;
                    break;
                }
            }
            if (restart) cit = getComponents().first;
            else cit++;
        }
    }
}


bool ComponentGraph::calculateFixedDomain(ComponentInfo& ci) const
{
    DBGLOG(DBG, "calculateFixedDomain");

    bool fd = true;

    // pure external components have only a fixed domain if the output of all outer external atoms
    // contains no variables
    if (ci.innerRules.empty() && !ci.outerEatoms.empty()) {
        BOOST_FOREACH (ID eaid, ci.outerEatoms) {
            const ExternalAtom& ea = reg->eatoms.getByID(eaid);
            BOOST_FOREACH (ID ot, ea.tuple) {
                if (ot.isVariableTerm()) return false;
            }
        }
        return true;
    }

    // get rule heads here
    // here we store the full atom IDs (we need to unify, the predicate is not sufficient)
    std::set<ID> headAtomIDs;
    // we only consider inner rules (constraints have no heads)
    BOOST_FOREACH(ID rid, ci.innerRules) {
        const Rule& rule = reg->rules.getByID(rid);

        BOOST_FOREACH(ID hid, rule.head) {
            if( !hid.isOrdinaryAtom() ) {
                continue;
            }
            headAtomIDs.insert(hid);
        }
    }

    // now check output variables

    // here we need to check inner rules and inner constraints
    std::vector<ID>* ruleSets[] = {&ci.innerRules, &ci.innerConstraints};
    BOOST_FOREACH (std::vector<ID>* ruleSet, ruleSets) {
        BOOST_FOREACH(ID rid, *ruleSet) {
            if( !rid.doesRuleContainExtatoms() ) {
                continue;
            }

            const Rule& rule = reg->rules.getByID(rid);

            // find all variable outputs in all eatoms in this rule's body
            std::set<ID> varsToCheck;
            BOOST_FOREACH(ID lid, rule.body) {
                if( !lid.isExternalAtom() )
                    continue;

                const ExternalAtom& eatom = reg->eatoms.getByID(lid);
                BOOST_FOREACH(ID tid, eatom.tuple) {
                    if( tid.isVariableTerm() )
                        varsToCheck.insert(tid);
                }
            }

            // for each variable:
            // if it is part of a positive body atom of r
            // and this positive body atom of r does not unify with any rule head in c
            // then e is safe
            BOOST_FOREACH(ID vid, varsToCheck) {
                // check strong safety of variable vid
                DBGLOG(DBG,"checking fixed domain of variable " <<
                    printToString<RawPrinter>(vid,reg));

                bool variableSafe = false;
                BOOST_FOREACH(ID lid, rule.body) {
                    // skip negative bodies
                    if( lid.isNaf() )
                        continue;

                    // skip external atoms,
                    // they could but cannot in general be assumed to limit the domain
                    // (and that's the reason we need to check strong safety)
                    if( lid.isExternalAtom() )
                        continue;

                    // skip non-ordinary atoms
                    WARNING("can we use aggregates to limit the domain for strong safety?")
                        WARNING("can we use builtin atoms to limit the domain for strong safety?")
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
                    while( itv != oatom.tuple.end() ) {
                        if( *itv == vid ) {
                            containsVariable = true;
                            break;
                        }
                        itv++;
                    }

                    if( !containsVariable ) {
                        continue;
                    }

                    // oatom 'oatom' was retrieved using ID 'lid'
                    DBGLOG(DBG,"checking unifications of body literal " <<
                        printToString<RawPrinter>(lid, reg) << " with component rule heads");
                    bool doesNotUnify = true;
                    BOOST_FOREACH(ID hid, headAtomIDs) {
                        DBGLOG(DBG,"checking against " <<
                            printToString<RawPrinter>(hid, reg));
                        assert(hid.isOrdinaryAtom());
                        const OrdinaryAtom& hoatom = reg->lookupOrdinaryAtom(hid);
                        if( oatom.unifiesWith(hoatom) ) {
                            DBGLOG(DBG,"unification successful "
                                "-> literal does not limit the domain");
                            doesNotUnify = false;
                            break;
                        }
                    }

                    if( doesNotUnify ) {
                        DBGLOG(DBG, "variable safe!");
                        variableSafe = true;
                        break;
                    }
                }

                if( !variableSafe ) {
                    // check if the variable occurs in an external atom with unstratified nonmonotonic parameters
                    bool nonmonotonicEA = false;
                    BOOST_FOREACH(ID lid, rule.body) {
                        if(!lid.isNaf() && lid.isExternalAtom()) {
                            if (ci.stratifiedLiterals.find(rid) == ci.stratifiedLiterals.end() ||
                            std::find(ci.stratifiedLiterals.at(rid).begin(), ci.stratifiedLiterals.at(rid).end(), lid) == ci.stratifiedLiterals.at(rid).end()) {
                                nonmonotonicEA = true;
                                break;
                            }
                        }
                    }

                    fd = false;
                }
                else {
                    DBGLOG(DBG, "Variable " << vid << " is strongly safe in rule " << rid << " (" << &ci << ")");
                    ci.stronglySafeVariables[rid].insert(vid);
                }
            }
        }
    }
    return fd;
}


bool ComponentGraph::computeRecursiveAggregatesInComponent(ComponentInfo& ci) const
{
    // get all head predicates
    std::set<ID> headPredicates;
    BOOST_FOREACH (ID ruleID, ci.innerRules) {
        const Rule& rule = reg->rules.getByID(ruleID);
        BOOST_FOREACH (ID h, rule.head) {
            const OrdinaryAtom& oatom = reg->lookupOrdinaryAtom(h);
            headPredicates.insert(oatom.tuple[0]);
        }
    }

    // go through all aggregate atoms
    std::set<ID> aatoms;
    BOOST_FOREACH (ID ruleID, ci.innerRules) {
        const Rule& rule = reg->rules.getByID(ruleID);
        BOOST_FOREACH (ID b, rule.body) {
            if (b.isAggregateAtom()) {
                aatoms.insert(b);
            }
        }
    }

    // recursively check if the aggregate depend on head atoms from this component
    while (!aatoms.empty()) {
        const AggregateAtom& aatom = reg->aatoms.getByID(*aatoms.begin());
        aatoms.erase(*aatoms.begin());
        BOOST_FOREACH (ID b, aatom.literals) {
            if (b.isOrdinaryAtom()) {
                const OrdinaryAtom& oatom = reg->lookupOrdinaryAtom(b);
                if (headPredicates.find(oatom.tuple[0]) != headPredicates.end()) return true;
            }
            if (b.isExternalAtom()) {
                const ExternalAtom& eatom = reg->eatoms.getByID(b);
                int i = 0;
                BOOST_FOREACH (ID inp, eatom.inputs) {
                    if (eatom.pluginAtom->getInputType(i) == PluginAtom::PREDICATE && headPredicates.find(eatom.inputs[0]) != headPredicates.end()) return true;
                    i++;
                }
            }
            if (b.isAggregateAtom()) {
                aatoms.insert(b);
            }
        }
    }

    return false;
}


void ComponentGraph::calculateStratificationInfo(RegistryPtr reg, ComponentInfo& ci)
{
    DBGLOG(DBG, "calculateStratificationInfo");

    // get the head atoms of all rules in this component
    std::set<ID> headAtomIDs;
    BOOST_FOREACH(ID rid, ci.innerRules) {
        const Rule& rule = reg->rules.getByID(rid);

        BOOST_FOREACH(ID hid, rule.head) {
            if (!hid.isOrdinaryAtom()) continue;
            headAtomIDs.insert(hid);

            const OrdinaryAtom& oatom = reg->lookupOrdinaryAtom(hid);
            ci.predicatesDefinedInComponent.insert(oatom.tuple[0]);
        }
    }

    // for all default-negated literals and predicate input parameters in this component
    BOOST_FOREACH(ID rid, ci.innerRules) {
        const Rule& rule = reg->rules.getByID(rid);

        BOOST_FOREACH(ID bid, rule.body) {
            // default-negated literals
            if (!bid.isExternalAtom() && bid.isNaf() &&  bid.isOrdinaryAtom()) {
                // does it unify with a head atom in this component?
                bool stratified = true;
                BOOST_FOREACH (ID hid, headAtomIDs) {
                    const OrdinaryAtom& boatom = reg->lookupOrdinaryAtom(bid);
                    const OrdinaryAtom& hoatom = reg->lookupOrdinaryAtom(hid);
                    if (boatom.unifiesWith(hoatom)) {
                        stratified = false;
                        break;
                    }
                }
                if (stratified) {
                    ci.stratifiedLiterals[rid].insert(bid);
                }
            }
            // predicate input parameters
            if (bid.isExternalAtom() && !bid.isNaf()) {
                const ExternalAtom& eatom = reg->eatoms.getByID(bid);
                bool stratified = true;
                for (uint32_t p = 0; p < eatom.inputs.size() && stratified; ++p) {
                    if (eatom.pluginAtom->getInputType(p) == PluginAtom::PREDICATE && eatom.getExtSourceProperties().isNonmonotonic(p)) {
                        // is this predicate defined in this component?
                        if (ci.predicatesDefinedInComponent.count(eatom.inputs[p]) > 0) {
                            stratified = false;
                            break;
                        }
                    }
                }
                if (stratified) {
                    DBGLOG(DBG, "Literal " << bid << " in rule " << rid << " is stratified");
                    ci.stratifiedLiterals[rid].insert(bid);
                }
            }
        }
    }
}

void ComponentGraph::calculatePredicatesOfComponent(RegistryPtr reg, ComponentInfo& ci)
{
    DBGLOG(DBG, "calculatePredicatesOfComponent");

    for (int setc = 1; setc <= 2; setc++) { // do this for inner rules and inner constraints
        std::vector<ID>& set = (setc == 1 ? ci.innerRules : ci.innerConstraints);

        BOOST_FOREACH(ID rid, set) {
            const Rule& rule = reg->rules.getByID(rid);

            BOOST_FOREACH(ID hid, rule.head) {
                if (!hid.isOrdinaryAtom()) continue;
                ci.predicatesOccurringInComponent.insert(reg->lookupOrdinaryAtom(hid).tuple[0]);
            }

            BOOST_FOREACH(ID bid, rule.body) {
                if (bid.isOrdinaryAtom()) {
                    ci.predicatesOccurringInComponent.insert(reg->lookupOrdinaryAtom(bid).tuple[0]);
                }
                if (bid.isExternalAtom()) {
                    const ExternalAtom& eatom = reg->eatoms.getByID(bid);
                    for (int i = 0; i < eatom.inputs.size(); i++){
                        if (eatom.pluginAtom->getInputType(i) == PluginAtom::PREDICATE) ci.predicatesOccurringInComponent.insert(eatom.inputs[i]);
                    }
                }
                if (bid.isAggregateAtom()) {
                    const AggregateAtom& aatom = reg->aatoms.getByID(bid);
                    BOOST_FOREACH (ID abid, aatom.literals){
                        if (abid.isOrdinaryAtom()) {
                            ci.predicatesOccurringInComponent.insert(reg->lookupOrdinaryAtom(abid).tuple[0]);
                        }
                    }
                }
            }
        }
    }
}

// Compute the dependency infos and component info
// before putting components `comps' and `sharedcomps' into a new component.
//
// `sharedcomps' may only contain components with constraints that can be shared.
//
// This method does not change the graph, it only changes the output arguments,
// hence it is const (and should stay const).
//
// This method throws an exception if this operation makes the DAG cyclic.
void ComponentGraph::computeCollapsedComponentInfos(
const ComponentSet& comps, const ComponentSet& sharedcomps,
DepMap& newIncomingDependencies, DepMap& newOutgoingDependencies,
                                 // see comment above about const!
ComponentInfo& newComponentInfo) const
{
    DBGLOG_SCOPE(DBG,"cCCI", false);
    DBGLOG(DBG,"= computeCollapsedComponentInfos(" <<
        printrange(comps) << "," << printrange(sharedcomps) << ",.,.,.)");

    // dependencies from the new components to other components
    DepMap& incoming = newIncomingDependencies;
    // dependencies from other components to the new component
    DepMap& outgoing = newOutgoingDependencies;

    // set of original components that depend on other original components
    // (we need this to find out whether an eatom in a component is an outer or
    // an inner eatom ... if it depends on stuff in the components it is inner)
    ComponentSet internallyDepends;

    // whether within the new component there is a negative rule dependency
    bool foundInternalNegativeRuleDependency = false;

    // iterate over all originals and over their outgoing dependencies (what they depend on)
    ComponentSet::const_iterator ito;
    for(ito = comps.begin(); ito != comps.end(); ++ito) {
        DBGLOG(DBG,"original " << *ito << ": (outgoing)");
        DBGLOG_INDENT(DBG);

        PredecessorIterator itpred, itpred_end;
        for(boost::tie(itpred, itpred_end) = getDependencies(*ito);
        itpred != itpred_end; ++itpred) {
            Dependency outgoing_dep = *itpred;
            Component target = targetOf(outgoing_dep);
            const DependencyInfo& outgoing_depInfo = propsOf(outgoing_dep);
            if( comps.find(target) == comps.end() ) {
                // dependency not within the new collapsed component
                DBGLOG(DBG,"outgoing dependency to " << target);
                outgoing[target] |= outgoing_depInfo;
            }
            else {
                // dependency within the new collapsed component
                DBGLOG(DBG,"internal dependency (to " << target << ")");
                internallyDepends.insert(*ito);

                if( outgoing_depInfo.negativeRule )
                    foundInternalNegativeRuleDependency = true;
            }
        }                        // iterate over predecessors
    }                            // iterate over `comps' originals

    // iterate over `comps' originals and over incoming dependencies
    // now also check for duplicate violations
    for(ito = comps.begin(); ito != comps.end(); ++ito) {
        DBGLOG(DBG,"original " << *ito << ": (incoming)");
        DBGLOG_INDENT(DBG);

        // go over dependencies to original members of new component
        SuccessorIterator itsucc, itsucc_end;
        for(boost::tie(itsucc, itsucc_end) = getProvides(*ito);
        itsucc != itsucc_end; ++itsucc) {
            Dependency incoming_dep = *itsucc;
            Component source = sourceOf(incoming_dep);
            const DependencyInfo& incoming_di = propsOf(incoming_dep);
            if( comps.find(source) == comps.end() ) {
                // the dependency comes from outside the new component

                DBGLOG(DBG,"incoming dependency from " << source);
                incoming[source] |= incoming_di;
                // ensure that we do not create cycles
                // (this check is not too costly, so this is no assertion but a real runtime check)
                DepMap::const_iterator itdm = outgoing.find(source);
                // if we have an incoming dep and an outgoing dep,
                // we create a cycle so this collapsing is invalid
                // (this is a bug in the code calling this method!)
                if( itdm != outgoing.end() ) {
                    std::stringstream s;
                    s << "Error: computeCollapsedComponentInfos tried to create a cycle via component " << source << " and " << *ito;
                    throw std::runtime_error(s.str());
                }
            }
            else {
                // the dependency comes from inside the new component (to inside)
                // nothing to do here (we already did it above
                assert(internallyDepends.find(source) != internallyDepends.end() &&
                    "we should have found everything above that we find here");
            }
        }                        // iterate over successors
    }                            // iterate over originals

    //
    // build newComponentInfo
    //
    ComponentInfo& ci = newComponentInfo;
    if( !sharedcomps.empty() )
        LOG(WARNING,"maybe we need to do more about shared constraint components `sharedcomps' here, at the moment we just copy them!");
    for(ito = sharedcomps.begin(); ito != sharedcomps.end(); ++ito) {
        const ComponentInfo& cio = propsOf(*ito);
        #ifdef COMPGRAPH_SOURCESDEBUG
        ci.sources.insert(ci.sources.end(),
            cio.sources.begin(), cio.sources.end());
        #endif
        if( !cio.innerRules.empty() ||
            !cio.innerEatoms.empty() )
            throw std::runtime_error("ccomps must only contain inner constraints!");
        // inner constraints stay inner constraints
        ci.innerConstraints.insert(ci.innerConstraints.end(),
            cio.innerConstraints.begin(), cio.innerConstraints.end());

        WARNING("do we need to do more here? perhaps negative dependencies to this unit? recursive aggregates?")
    }
    for(ito = comps.begin(); ito != comps.end(); ++ito) {
        const ComponentInfo& cio = propsOf(*ito);
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
        BOOST_FOREACH (Pair p, cio.stronglySafeVariables) {
            BOOST_FOREACH (ID id, p.second) {
                ci.stronglySafeVariables[p.first].insert(id);
            }
        }
        ci.predicatesDefinedInComponent.insert(
            cio.predicatesDefinedInComponent.begin(), cio.predicatesDefinedInComponent.end());
        ci.predicatesOccurringInComponent.insert(
            cio.predicatesOccurringInComponent.begin(), cio.predicatesOccurringInComponent.end());
        /*
            BOOST_FOREACH (Pair p, cio.stratifiedLiterals){
              BOOST_FOREACH (ID id, p.second){
                ci.stratifiedLiterals[p.first].insert(id);
              }
            }
        */

        ci.disjunctiveHeads |= cio.disjunctiveHeads;
        ci.negativeDependencyBetweenRules |= cio.negativeDependencyBetweenRules;

        ci.innerEatomsNonmonotonic |= cio.innerEatomsNonmonotonic;
        ci.componentIsMonotonic |= cio.componentIsMonotonic;

        WARNING("check if fixedDomain is propagated correctly, it is missing in operator|= but this might be on purpose")
        // fixedDomain:
        // pure external components shall have no influence on this property
        // because domain restriction is always done in successor components
            if (!(!cio.outerEatoms.empty() && cio.innerRules.empty()))
            ci.fixedDomain &= cio.fixedDomain;

        // outer external atoms which get input from the same component become inner ones
        BOOST_FOREACH (ID outerEA, cio.outerEatoms){
            const ExternalAtom& eatom = reg->eatoms.getByID(outerEA);
            bool becomesInner = false;
            becomesInner = (ci.predicatesDefinedInComponent.find(eatom.auxInputPredicate) != ci.predicatesDefinedInComponent.end());
            for (int i = 0; i < eatom.inputs.size(); ++i) {
                if (eatom.pluginAtom->getInputType(i) == PluginAtom::PREDICATE && ci.predicatesDefinedInComponent.find(eatom.inputs[i]) != ci.predicatesDefinedInComponent.end()){
                    becomesInner = true;
                    break;
                }
            }

            if (!becomesInner) {
                ci.outerEatoms.insert(ci.outerEatoms.end(), outerEA);
                ci.outerEatomsNonmonotonic |= eatom.getExtSourceProperties().isNonmonotonic();
            }else{
                ci.innerEatoms.insert(ci.innerEatoms.end(), outerEA);
                ci.innerEatomsNonmonotonic |= eatom.getExtSourceProperties().isNonmonotonic();
            }
        }
        WARNING("if " input " component consists only of eatoms, they may be nonmonotonic, and we still can have wellfounded model generator ... create testcase for this ? how about wellfounded2.hex?")
    }
    ci.negativeDependencyBetweenRules |= foundInternalNegativeRuleDependency;
                                 // recompute if the collapsed component contains recursive aggregates; note that this is not simply the logical or of the basic components
    ci.recursiveAggregates = computeRecursiveAggregatesInComponent(ci);
    calculateStratificationInfo(reg, ci);
}


// collapse components given in range into one new component
// originals are put into new component and then removed
// shared are just copied into new component
// collapse incoming and outgoing dependencies
// update properties of dependencies
// update properties of component
ComponentGraph::Component
ComponentGraph::collapseComponents(
const ComponentSet& originals, const ComponentSet& shared)
{
    DBGLOG_SCOPE(DBG,"cC", false);
    DBGLOG(DBG,"= collapseComponents(" << printrange(originals) << "," << printrange(shared) << ")");

    Component c = boost::add_vertex(cg);
    LOG(DBG,"created component node " << c << " for collapsed component");

    DepMap incoming;
    DepMap outgoing;
    ComponentInfo& ci = propsOf(c);
    computeCollapsedComponentInfos(originals, shared, incoming, outgoing, ci);

    // build incoming dependencies
    for(DepMap::const_iterator itd = incoming.begin();
    itd != incoming.end(); ++itd) {
        Dependency newdep;
        bool success;
        DBGLOG(DBG,"adding incoming edge " << itd->first << " -> " << c);
        boost::tie(newdep, success) = boost::add_edge(itd->first, c, itd->second, cg);
        assert(success);         // we only add new edges here, and each only once
    }

    // build outgoing dependencies
    for(DepMap::const_iterator itd = outgoing.begin();
    itd != outgoing.end(); ++itd) {
        Dependency newdep;
        bool success;
        DBGLOG(DBG,"adding outgoing edge " << c << " -> " << itd->first);
        boost::tie(newdep, success) = boost::add_edge(c, itd->first, itd->second, cg);
        assert(success);         // we only add new edges here, and each only once
    }

    // remove all original components
    for(ComponentSet::const_iterator ito = originals.begin();
    ito != originals.end(); ++ito) {
        boost::clear_vertex(*ito, cg);
        boost::remove_vertex(*ito, cg);
    }

    return c;
}


namespace
{
    std::string graphviz_node_id(ComponentGraph::Component c) {
        std::ostringstream os;
        os << "c" << std::hex << c;
        return os.str();
    }

    template<typename Range>
    void printoutVerboseIfNotEmpty(std::ostream& o, RegistryPtr reg, const char* prefix, Range idrange) {
        // see boost/range/iterator_range.hpp
        typedef typename Range::const_iterator Iterator;
        if( !boost::empty(idrange) ) {
            o << "{" << prefix << "|";
            graphviz::escape(o, printManyToString<RawPrinter>(idrange, "\n", reg));
            o << "}|";
        }
    }

    template<typename Range>
    void printoutTerseIfNotEmpty(std::ostream& o, RegistryPtr reg, const char* prefix, Range idrange) {
        // see boost/range/iterator_range.hpp
        typedef typename Range::const_iterator Iterator;
        if( !boost::empty(idrange) ) {
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
    if( verbose ) {
        o << "{idx=" << index << ",component=" << c << "|";
        #ifdef COMPGRAPH_SOURCESDEBUG
        o << "{sources|" << printrange(ci.sources, "\\{", ",", "\\}") << "}|";
        #endif
        printoutVerboseIfNotEmpty(o, reg, "outerEatoms", ci.outerEatoms);
        printoutVerboseIfNotEmpty(o, reg, "innerRules", ci.innerRules);
        printoutVerboseIfNotEmpty(o, reg, "innerEatoms", ci.innerEatoms);
        printoutVerboseIfNotEmpty(o, reg, "innerConstraints", ci.innerConstraints);
        if( !ci.innerRules.empty() ) {
            if( ci.disjunctiveHeads )
                o << "{rules contain disjunctive heads}|";
            if( ci.negativeDependencyBetweenRules )
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
    else {
        o << "{idx=" << index << "|";
        printoutTerseIfNotEmpty(o, reg, "outerEatoms", ci.outerEatoms);
        printoutTerseIfNotEmpty(o, reg, "innerRules", ci.innerRules);
        printoutTerseIfNotEmpty(o, reg, "innerEatoms", ci.innerEatoms);
        printoutTerseIfNotEmpty(o, reg, "innerConstraints", ci.innerConstraints);
        if( !ci.innerRules.empty() ) {
            if( ci.disjunctiveHeads )
                o << "{rules contain disjunctive heads}|";
            if( ci.negativeDependencyBetweenRules )
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
    if( verbose ) {
        o << di;
    }
    else {
        o <<
            (di.positiveRegularRule?" posR":"") <<
            (di.positiveConstraint?" posC":"") <<
            (di.negativeRule?" negR":"") <<
            (di.unifyingHead?" unifying":"") <<
            (di.positiveExternal?" posExt":"") <<
            (di.negativeExternal?" negExt":"") <<
            (di.externalConstantInput?" extConstInp":"") <<
            (di.externalPredicateInput?" extPredInp":"") <<
            (di.externalNonmonotonicPredicateInput?" extNonmonPredInp":"");
    }
}


// output graph as graphviz source
void ComponentGraph::writeGraphViz(std::ostream& o, bool verbose) const
{
    // boost::graph::graphviz is horribly broken!
    // therefore we print it ourselves

    o << "digraph G {" << std::endl <<
        "rankdir=BT;" << std::endl;// print root nodes at bottom, leaves at top!

    // print vertices
    ComponentIterator it, it_end;
    unsigned index = 0;
    for(boost::tie(it, it_end) = boost::vertices(cg);
    it != it_end; ++it, ++index) {
        o << graphviz_node_id(*it) << "[shape=record,label=\"";
        {
            writeGraphVizComponentLabel(o, *it, index, verbose);
        }
        o << "\"];" << std::endl;
    }

    // print edges
    DependencyIterator dit, dit_end;
    for(boost::tie(dit, dit_end) = boost::edges(cg);
    dit != dit_end; ++dit) {
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
ctx(other.ctx),
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


// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
