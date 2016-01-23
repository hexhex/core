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
 * @file EvalHeuristicGreedy.cpp
 * @author Christoph Redl
 *
 * @brief Evaluation heuristic that groups components in as few units as possible.
 *        This maximizes the effect of external behavior learning.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#include "dlvhex2/EvalHeuristicGreedy.h"
#include "dlvhex2/EvalHeuristicShared.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/LiberalSafetyChecker.h"

#include <boost/unordered_map.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/properties.hpp>
#include <boost/scoped_ptr.hpp>

//#include <fstream>

DLVHEX_NAMESPACE_BEGIN

bool EvalHeuristicGreedy::mergeComponents(ProgramCtx& ctx, const ComponentGraph::ComponentInfo& ci1, const ComponentGraph::ComponentInfo& ci2, bool negativeExternalDependency) const
{

    if (ctx.config.getOption("LiberalSafety") && ctx.config.getOption("IncludeAuxInputInAuxiliaries")) {
        // here we could always merge
        // however, we do this only if there are no negative external dependencies between the components (as this comes at exponential cost)
        return !negativeExternalDependency;
    }
    else {

        // never merge components with outer external atoms (they could become inner ones)
        if (!ci1.outerEatoms.empty() || !ci2.outerEatoms.empty()) return false;

        // if both components have a fixed domain we can safely merge them
        // (both can be solved by guess&check mg)
        if (ci1.fixedDomain && ci2.fixedDomain) return true;

        // if both components are solved by wellfounded mg and none of them has outer external atoms, then we merge them
        // (the resulting component will still be wellfounded and an outer external atom can not become an inner one)
        if (!ci1.innerEatomsNonmonotonic && !ci1.negativeDependencyBetweenRules && !ci1.disjunctiveHeads && ci1.innerEatoms.size() > 0 && ci1.outerEatoms.size() == 0 &&
            !ci2.innerEatomsNonmonotonic && !ci2.negativeDependencyBetweenRules && !ci2.disjunctiveHeads && ci2.innerEatoms.size() > 0 && ci2.outerEatoms.size() == 0) return true;

        // otherwise: don't merge them
        return false;
    }
}


EvalHeuristicGreedy::EvalHeuristicGreedy():
Base()
{
}


EvalHeuristicGreedy::~EvalHeuristicGreedy()
{
}


typedef ComponentGraph::Component Component;
typedef ComponentGraph::ComponentIterator ComponentIterator;
typedef std::vector<Component> ComponentContainer;
typedef ComponentGraph::ComponentSet ComponentSet;

namespace internalgreedy
{

    // collect all components on the way
    struct DFSVisitor:
    public boost::default_dfs_visitor
    {
        const ComponentGraph& cg;
        ComponentSet& comps;
        DFSVisitor(const ComponentGraph& cg, ComponentSet& comps): boost::default_dfs_visitor(), cg(cg), comps(comps) {}
        DFSVisitor(const DFSVisitor& other): boost::default_dfs_visitor(), cg(other.cg), comps(other.comps) {}
        template<typename GraphT>
        void discover_vertex(Component comp, const GraphT&) {
            comps.insert(comp);
        }
    };

    template<typename ComponentGraph, typename Set>
    void transitivePredecessorComponents(const ComponentGraph& compgraph, Component from, Set& preds) {
        // we need a hash map, as component graph is no graph with vecS-storage
        //
        typedef boost::unordered_map<Component, boost::default_color_type> CompColorHashMap;
        typedef boost::associative_property_map<CompColorHashMap> CompColorMap;
        CompColorHashMap ccWhiteHashMap;
        // fill white hash map
        ComponentIterator cit, cit_end;
        for(boost::tie(cit, cit_end) = compgraph.getComponents();
        cit != cit_end; ++cit) {
            //boost::put(ccWhiteHashMap, *cit, boost::white_color);
            ccWhiteHashMap[*cit] = boost::white_color;
        }
        CompColorHashMap ccHashMap(ccWhiteHashMap);

        //
        // do DFS
        //
        DFSVisitor dfs_vis(compgraph, preds);
        //LOG("doing dfs visit for root " << *itr);
        boost::depth_first_visit(
            compgraph.getInternalGraph(),
            from,
            dfs_vis,
            CompColorMap(ccHashMap));
        DBGLOG(DBG,"predecessors of " << from << " are " << printrange(preds));
    }

}


// required for some GCCs for DFSVisitor CopyConstructible Concept Check
using namespace internalgreedy;

void EvalHeuristicGreedy::build(EvalGraphBuilder& builder)
{
    ProgramCtx& ctx = builder.getProgramCtx();
    ComponentGraph& compgraph = builder.getComponentGraph();
    #if 0
    {
        std::string fnamev = "my_initial_ClonedCompGraphVerbose.dot";
        std::ofstream filev(fnamev.c_str());
        compgraph.writeGraphViz(filev, true);
    }
    #endif

    bool didSomething;
    do {
        didSomething = false;

        //
        // forall external components e:
        // merge with all rules that
        // * depend on e
        // * do not contain external atoms
        // * do not depend on something e does not (transitively) depend on
        //
        {
            ComponentIterator cit;
                                 // do not use boost::tie here! the container is modified in the loop!
            for(cit = compgraph.getComponents().first;
            cit != compgraph.getComponents().second; ++cit) {
                Component comp = *cit;
                                 // || !compgraph.propsOf(comp).innerRules.empty() || !compgraph.propsOf(comp).innerConstraints.empty() )
                if( compgraph.propsOf(comp).outerEatoms.empty() )
                    continue;
                DBGLOG(DBG,"checking component " << comp);

                LOG(ANALYZE,"checking whether to collapse external component " << comp << " with successors");

                // get predecessors
                ComponentSet preds;
                transitivePredecessorComponents(compgraph, comp, preds);

                // get successors
                ComponentSet collapse;
                bool addedToCollapse;
                // do this as long as we find new ones
                // if we do not do this loop, we might miss something
                // as PredecessorIterator not necessarily honours topological order
                // (TODO this could be made more efficient)
                do {
                    addedToCollapse = false;

                    ComponentGraph::SuccessorIterator sit, sit_end;
                    for(boost::tie(sit, sit_end) = compgraph.getProvides(comp);
                    sit != sit_end; ++sit) {
                        Component succ = compgraph.sourceOf(*sit);

                        // skip successors with eatoms
                        if( !compgraph.propsOf(succ).outerEatoms.empty() )
                            continue;
                        // do not check found stuff twice
                        if( collapse.find(succ) != collapse.end() )
                            continue;

                        DBGLOG(DBG,"found successor " << succ);

                        ComponentGraph::PredecessorIterator pit, pit_end;
                        bool good = true;
                        for(boost::tie(pit, pit_end) = compgraph.getDependencies(succ);
                        pit != pit_end; ++pit) {
                            Component dependson = compgraph.targetOf(*pit);
                            if( preds.find(dependson) == preds.end() ) {
                                LOG(DBG,"successor bad as it depends on other node " << dependson);
                                good = false;
                                break;
                            }
                        }
                        if( good ) {
                            // collapse with this
                            collapse.insert(succ);
                            preds.insert(succ);
                            addedToCollapse = true;
                        }
                    }
                }
                while(addedToCollapse);

                // collapse if not nonempty
                if( !collapse.empty() ) {
                    collapse.insert(comp);
                    Component c = compgraph.collapseComponents(collapse);
                    LOG(ANALYZE,"collapse of " << printrange(collapse) << " yielded new component " << c);

                    // restart loop after collapse
                    cit = compgraph.getComponents().first;
                    didSomething = true;
                }
            }
        }

        //
        // forall components c1:
        // merge with all other components c2 such that no cycle is broken
        // that is, there must not be a path of length >=2 from c2 to c1
        //
        {
            ComponentIterator cit = compgraph.getComponents().first;
            while(cit != compgraph.getComponents().second) {
                Component comp = *cit;
                ComponentSet collapse;
                DBGLOG(DBG,"checking component " << comp);

                ComponentIterator cit2 =  cit;
                cit2++;
                while( cit2 != compgraph.getComponents().second ) {
                    Component comp2 = *cit2;
                    DBGLOG(DBG,"checking other component " << comp2);

                    bool breakCycle = false;

                    // check if there is a path of length >=2 from comp2 to comp
                    // that is, there is a path from a predecessor of comp2 to comp
                    ComponentSet preds2;
                    {
                        ComponentGraph::PredecessorIterator pit, pit_end;
                        for(boost::tie(pit, pit_end) = compgraph.getDependencies(comp2);
                        pit != pit_end; ++pit) {
                            preds2.insert(compgraph.targetOf(*pit));
                        }
                    }
                    BOOST_FOREACH (Component comp2s, preds2) {

                        ComponentSet reachable;
                        transitivePredecessorComponents(compgraph, comp2s, reachable);

                                 // path of length >=2
                        if (std::find(reachable.begin(), reachable.end(), comp) != reachable.end() && comp2s != comp) {
                            DBGLOG(DBG, "do not merge because this would break a cycle");
                            breakCycle = true;
                            break;
                        }
                    }

                    // check if there is a path of length >=2 from comp to comp2
                    // that is, there is a path from a predecessor of comp to comp2
                    ComponentSet preds;
                    {
                        ComponentGraph::PredecessorIterator pit, pit_end;
                        for(boost::tie(pit, pit_end) = compgraph.getDependencies(comp);
                        pit != pit_end; ++pit) {
                            preds.insert(compgraph.targetOf(*pit));
                        }
                    }
                    BOOST_FOREACH (Component comps, preds) {

                        ComponentSet reachable;
                        transitivePredecessorComponents(compgraph, comps, reachable);

                                 // path of length >=2
                        if (std::find(reachable.begin(), reachable.end(), comp2) != reachable.end() && comps != comp2) {
                            DBGLOG(DBG, "do not merge because this would break a cycle");
                            breakCycle = true;
                            break;
                        }
                    }

                    std::set<std::pair<ComponentGraph::Component, ComponentGraph::Component> > negdep;
                    std::set<ComponentGraph::Component> nonmonotonicPredecessor;
                    if (ctx.config.getOption("LiberalSafety") && ctx.config.getOption("IncludeAuxInputInAuxiliaries")) {
                        if (ctx.config.getOption("LiberalSafety") && ctx.config.getOption("IncludeAuxInputInAuxiliaries")) {
                            // check if there is a nonmonotonic external dependency from comp to comp2
                            BOOST_FOREACH (ComponentGraph::Dependency dep, compgraph.getDependencies()) {
                                const ComponentGraph::DependencyInfo& di = compgraph.getDependencyInfo(dep);
                                if (di.externalNonmonotonicPredicateInput) {
                                    // check if the nonmonotonic predicate dependency is eliminated if we consider only necessary external atoms
                                    BOOST_FOREACH (ComponentGraph::DependencyInfo::DepEdge de, di.depEdges) {
                                        if (de.get<2>().externalNonmonotonicPredicateInput &&
                                        ctx.liberalSafetyChecker->isExternalAtomNecessaryForDomainExpansionSafety(de.get<0>())) {
                                            // not eliminated
                                            negdep.insert(std::pair<ComponentGraph::Component, ComponentGraph::Component>(compgraph.sourceOf(dep), compgraph.targetOf(dep)));
                                            nonmonotonicPredecessor.insert(compgraph.sourceOf(dep));
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    bool o = false;
                    if (compgraph.propsOf(comp).innerConstraints.size() == 1 && compgraph.propsOf(comp2).innerRules.size() == 1) {
                        o = true;
                    }
                    if (compgraph.propsOf(comp2).innerConstraints.size() == 1 && compgraph.propsOf(comp).innerRules.size() == 1) {
                        o = true;
                    }

                    // if this is the case, then do not merge
                    if (!breakCycle) {
                        // we do not want to merge if a component in transitivePredecessorComponents is reachable from exactly one of comp and comp2
                        bool nd = false;
                        if (ctx.config.getOption("LiberalSafety") && ctx.config.getOption("IncludeAuxInputInAuxiliaries")) {
                            // never merge C1 and C2 if C1 has a nonmonotonic predecessor unit but C2 has not
                            // Example:
                            //     C4
                            //    /  \-
                            //   C3  C2
                            //    \  /
                            //     C1
                            // Here, C1 is not merged with C3, which is by intend: merging them would prevent the merging of C3 with C4
                            ComponentSet reachable1, reachable2;
                            transitivePredecessorComponents(compgraph, comp, reachable1);
                            transitivePredecessorComponents(compgraph, comp2, reachable2);
                            bool nonmonTrans1 = false;
                            bool nonmonTrans2 = false;
                            BOOST_FOREACH (Component c, reachable1) if (nonmonotonicPredecessor.find(c) != nonmonotonicPredecessor.end()) nonmonTrans1 = true;
                            BOOST_FOREACH (Component c, reachable2) if (nonmonotonicPredecessor.find(c) != nonmonotonicPredecessor.end()) nonmonTrans2 = true;
                            nd |= nonmonTrans1 != nonmonTrans2;

                            // never merge if one of the components has a nonmonotonic dependency to some predecessor
                            // (the dependency could become internal, which slows down grounding significantly)
                            //nd |= (nonmonotonicPredecessor.find(comp) != nonmonotonicPredecessor.end()) ||
                            //    (nonmonotonicPredecessor.find(comp2) != nonmonotonicPredecessor.end());

                            // never merge if this makes a nonmonotonic dependency internal
                            // (should be a more specific version of the commented check above)
                            nd |= (negdep.find(std::pair<ComponentGraph::Component, ComponentGraph::Component>(comp, comp2)) != negdep.end()) ||
                                  (negdep.find(std::pair<ComponentGraph::Component, ComponentGraph::Component>(comp2, comp)) != negdep.end());
                        }

                        if (mergeComponents(ctx, compgraph.propsOf(comp), compgraph.propsOf(comp2), nd)) {
                            if (std::find(collapse.begin(), collapse.end(), comp2) == collapse.end()) {
                                collapse.insert(comp2);
                                // merge only one pair at a time, otherwise this could create cycles which are not detected above:
                                // e.g. C1     C2 --> C3 --> C4
                                // C1 can be merged with C2 and C1 can be merged with C4, but it can't be merged with both of them because this would create a cycle
                                // This is only detected if we see {C1, C2} (or {C1, C4}) as intermediate result
                                break;
                            }
                        }
                    }

                    cit2++;
                }

                if( !collapse.empty() ) {
                    // collapse! (decreases graph size)
                    collapse.insert(comp);
                    assert(collapse.size() > 1);
                    Component c = compgraph.collapseComponents(collapse);
                    LOG(ANALYZE,"collapse of " << printrange(collapse) << " yielded new component " << c);

                    // restart loop after collapse
                    cit = compgraph.getComponents().first;
                    didSomething = true;
                }
                else {
                    // advance
                    ++cit;
                }
            }
        }

    }
    while(didSomething);

    //
    // create eval units using topological sort
    //
    ComponentContainer sortedcomps;
    evalheur::topologicalSortComponents(compgraph.getInternalGraph(), sortedcomps);
    LOG(ANALYZE,"now creating evaluation units from components " << printrange(sortedcomps));
    #if 0
    {
        std::string fnamev = "my_ClonedCompGraphVerbose.dot";
        std::ofstream filev(fnamev.c_str());
        compgraph.writeGraphViz(filev, true);
    }
    #endif
    for(ComponentContainer::const_iterator it = sortedcomps.begin();
    it != sortedcomps.end(); ++it) {
        // just create a unit from each component (we collapsed above)
        std::list<Component> comps;
        comps.push_back(*it);
        std::list<Component> ccomps;
        EvalGraphBuilder::EvalUnit u = builder.createEvalUnit(comps, ccomps);
        LOG(ANALYZE,"component " << *it << " became eval unit " << u);
    }
}


DLVHEX_NAMESPACE_END


// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
