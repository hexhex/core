/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schüller
 * Copyright (C) 2011-2015 Christoph Redl
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
 * @file   ComponentGraph.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Component Graph interface.
 */

#ifndef COMPONENT_GRAPH_HPP_INCLUDED__18102010
#define COMPONENT_GRAPH_HPP_INCLUDED__18102010

#include "dlvhex2/DependencyGraph.h"

#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>
#include "boost/tuple/tuple.hpp"

#ifndef NDEBUG
# define COMPGRAPH_SOURCESDEBUG
#endif

DLVHEX_NAMESPACE_BEGIN

/** \brief Implements the component graph.
 *
 * A component graph is created from a dependency graph by collecting SCCs
 * into single nodes components.
 *
 * A component graph is a dag (acyclic by the above construction).
 *
 * Vertices (= components) store a set of rules and information about the dependencies
 * within the collapsed part of the dependency graph.
 *
 * Edges (= collapsed dependencies) store information about the collapsed
 * dependencies.
 *
 * A component contains
 * - external atoms depending only on other components (=outer eatoms),
 * - rules within the component (=inner rules),
 * - constraints within the component (=inner constraints), and
 * - external atoms depending on rules in the component (=inner eatoms).
 *
 * For each component, only one of these storages must hold an object, except for
 * inner eatoms which can only exist if there are inner rules.
 */
class DLVHEX_EXPORT ComponentGraph
{
    BOOST_CONCEPT_ASSERT((boost::Convertible<DependencyGraph::Node, unsigned int>));
    //////////////////////////////////////////////////////////////////////////////
    // types
    //////////////////////////////////////////////////////////////////////////////
    public:
        /** \brief Implements meta information about components. */
        struct DLVHEX_EXPORT ComponentInfo:
    public ostream_printable<ComponentInfo>
    {
        #ifdef COMPGRAPH_SOURCESDEBUG
        /** \brief List of nodes. */
        std::list<DependencyGraph::Node> sources;
        #endif

        /** \brief Outer external atoms in the component, i.e., external atoms which do not depend on atoms defined in this component. */
        std::vector<ID> outerEatoms;

        /** \brief Inner rules in the component (except constraints), i.e., rules which may cyclically depend on atoms defined in this component. */
        std::vector<ID> innerRules;
        /** \brief Inner external atoms in the component, i.e., external atoms which cyclically depend on atoms defined in this component. */
        std::vector<ID> innerEatoms;
        /** \brief Inner constraints in the component, i.e., rules which may cyclically depend on atoms defined in this component. */
        std::vector<ID> innerConstraints;
        /** \brief Stores for each rule the set of strongly safe variables in the rule. */
        boost::unordered_map<ID, std::set<ID> > stronglySafeVariables;
        /** \brief Stores for each rule the set of stratified literals in the rule, i.e., which do not depend on atoms derived in the component. */
        boost::unordered_map<ID, std::set<ID> > stratifiedLiterals;
        /** \brief Set of all predicates defined in the component. */
        std::set<ID> predicatesInComponent;

        // this is determined by calculateComponents
        // and used for selecting model generator factories
        /** \brief Component contains disjunctive heads. */
        bool disjunctiveHeads;
        /** \brief Component contains negative dependencies between rules. */
        bool negativeDependencyBetweenRules;
        /** \brief Component contains nonmonotonic inner external atoms. */
        bool innerEatomsNonmonotonic;
        /** \brief Component contains nonmonotonic outer external atoms. */
        bool outerEatomsNonmonotonic;
        /** \brief Component is purely monotonic. */
        bool componentIsMonotonic;
        /** \brief Component does not make use of value invention. */
        bool fixedDomain;
        /** \brief Component contains recursive aggregates. */
        bool recursiveAggregates;

        /** \brief Constructor. */
        ComponentInfo():
        disjunctiveHeads(false),
            negativeDependencyBetweenRules(false),
            innerEatomsNonmonotonic(false),
            outerEatomsNonmonotonic(false),
            componentIsMonotonic(true),
            fixedDomain(true),
            recursiveAggregates(false){}
        std::ostream& print(std::ostream& o) const;
    };

    /** \brief Meta information about rule dependencies in the component. */
    struct DependencyInfo:
    public DependencyGraph::DependencyInfo,
        public ostream_printable<DependencyInfo>
    {
        #ifdef COMPGRAPH_SOURCESDEBUG
        /** \brief Dependencies in the underlying graph. */
        std::set<DependencyGraph::Dependency> sources;
        #endif

        typedef boost::tuple<ID, ID, DependencyGraph::DependencyInfo> DepEdge;
        /** \brief Set of edges in the component. */
        std::vector<DepEdge> depEdges;

        /** \brief Constructor. */
        DependencyInfo() {}
        /** \brief Copy-constructor.
         * @param other DependencyInfo to copy. */
        DependencyInfo(const DependencyGraph::DependencyInfo& other):
        DependencyGraph::DependencyInfo(other) {}
        const DependencyInfo& operator|=(const DependencyInfo& other);
        std::ostream& print(std::ostream& o) const;
    };

    // we need listS because this graph will be changed a lot by collapsing nodes
    // use setS for out-edges because we don't want to have multiple edges
    typedef boost::adjacency_list<
        boost::setS, boost::listS, boost::bidirectionalS,
        ComponentInfo, DependencyInfo> Graph;
    typedef boost::graph_traits<Graph> Traits;

    typedef Graph::vertex_descriptor Component;
    typedef Graph::edge_descriptor Dependency;
    typedef Traits::vertex_iterator ComponentIterator;
    typedef Traits::edge_iterator DependencyIterator;
    typedef Traits::out_edge_iterator PredecessorIterator;
    typedef Traits::in_edge_iterator SuccessorIterator;

    typedef std::set<Component> ComponentSet;
    typedef std::map<Component, DependencyInfo> DepMap;

    //////////////////////////////////////////////////////////////////////////////
    // members
    //////////////////////////////////////////////////////////////////////////////
    protected:
        /** \brief ProgramCtx. */
        ProgramCtx& ctx;
        /** \brief Registry used for debugging and printing. */
        RegistryPtr reg;
    #ifdef COMPGRAPH_SOURCESDEBUG
        /** \brief In non-debug mode this graph's lifetime can end
         * after the constructor finished. */
        const DependencyGraph& dg;
    #endif
        /** \brief Internal component graph. */
        Graph cg;

        //////////////////////////////////////////////////////////////////////////////
        // methods
        //////////////////////////////////////////////////////////////////////////////
    protected:
        /** \brief Copy-constructor.
         *
         * Only to be used by explicit clone method.
         * @param other Component to copy. */
        ComponentGraph(const ComponentGraph& other);
    public:
        /** \brief Constructor to construct a component graph out of a DependencyGraph.
         * @param dp DependencyGraph used as basis for the ComponentGraph.
         * @param ctx ProgramCtx.
         * @param reg See ComponentGraph::reg. */
        ComponentGraph(const DependencyGraph& dg, ProgramCtx& ctx, RegistryPtr reg);
        /** \brief Destructor. */
        virtual ~ComponentGraph();

        /** \brief For explicit cloning of the graph.
         * @return Pointer to a copy of this ComponentGraph. */
        ComponentGraph* clone() const;

        //
        // modifiers
        //

        // collapse several components into one
        // originals are put into new component and then removed
        // shared are just copied into new component
        Component collapseComponents(
            const ComponentSet& originals, const ComponentSet& shared=ComponentSet());

        //
        // mighty helper for collapsing components
        //

        /** \brief Compute the dependency infos and component info
         * before putting components `comps' and `sharedcomps' into a new component.
         *
         * `sharedcomps' may only contain components with constraints that can be shared.
         *
         * This method does not change the graph, it only changes the output arguments,
         * hence it is const (and should stay const).
         *
         * This method throws an exception if this operation makes the DAG cyclic.
         * @param comps Set of overall components.
         * @param sharedcomps Set of components which are shared among multiple evaluation units.
         * @param newIncomingDependencies Collapsed incoming dependencies of the collapsed component.
         * @param newOutgoingDependencies Collapsed outgoing dependencies of the collapsed component.
         * @param newComponentInfo New ComponentInfo of the collapsed component. */
        void computeCollapsedComponentInfos(
            const ComponentSet& comps, const ComponentSet& sharedcomps,
            DepMap& newIncomingDependencies, DepMap& newOutgoingDependencies,
                                 // see comment above about const!
            ComponentInfo& newComponentInfo) const;

        //
        // accessors
        //

        // get const graph to apply external algorithms
        inline const Graph& getInternalGraph()
            const { return cg; }

        /** \brief Output graph as graphviz source (dot file).
         * @param o Stream to print the graph to.
         * @param verbose True to include more information. */
        virtual void writeGraphViz(std::ostream& o, bool verbose) const;

        /** \brief Get range over all components.
         * @return Pair of begin and end iterator. */
        inline std::pair<ComponentIterator, ComponentIterator> getComponents() const
            { return boost::vertices(cg); }
        /** \brief Get range over all edges.
         * @return Pair of begin and end iterator. */
        inline std::pair<DependencyIterator, DependencyIterator> getDependencies() const
            { return boost::edges(cg); }

        /** \brief Get node info given node.
         * @param c Some component of the graph.
         * @return ComponentInfo of \p c.
         */
        inline const ComponentInfo& getComponentInfo(Component c) const
            { return cg[c]; }

        /** \brief Get dependency info given dependency.
         * @param dep Some dependency of the graph.
         * @return DependencyInfo of \p dep. */
        inline const DependencyInfo& getDependencyInfo(Dependency dep) const
            { return cg[dep]; }

        /** \brief Get dependencies (to predecessors) = arcs from this component to others.
         * @param c Some component of the graph.
         * @return Pair of begin and end iterator. */
        inline std::pair<PredecessorIterator, PredecessorIterator>
            getDependencies(Component c) const
            { return boost::out_edges(c, cg); }

        /** \brief Get provides (dependencies to successors) = arcs from other component to this one.
         * @param c Some component of the graph.
         * @return PAir of begin and end iterator. */
        inline std::pair<SuccessorIterator, SuccessorIterator>
            getProvides(Component c) const
            { return boost::in_edges(c, cg); }

        /** \brief Get source of dependency = component that depends.
         * @param d Some dependency of the graph.
         * @return Component where \p d origins. */
        inline Component sourceOf(Dependency d) const
            { return boost::source(d, cg); }

        /** \brief Get target of dependency = component upon which the source depends.
         * @param d Some dependency of the graph.
         * @return Component where \p d ends. */
        inline Component targetOf(Dependency d) const
            { return boost::target(d, cg); }

        /** \brief Get node properties.
         * @param c Some component of the graph.
         * @return ComponentInfo of \p c. */
        inline const ComponentInfo& propsOf(Component c) const
            { return cg[c]; }
        /** \brief Get node properties.
         * @param c Some component of the graph.
         * @return ComponentInfo of \p c. */
        inline ComponentInfo& propsOf(Component c)
            { return cg[c]; }
        /** \brief Get dependency properties.
         * @param d Some dependency of the graph.
         * @return ComponentInfo of \p d. */
        inline const DependencyInfo& propsOf(Dependency d) const
            { return cg[d]; }
        /** \brief Get dependency properties.
         * @param d Some dependency of the graph.
         * @return ComponentInfo of \p d. */
        inline DependencyInfo& propsOf(Dependency d)
            { return cg[d]; }

        // counting -> mainly for allocating and testing
        /** \brief Retrieves the number of components.
         * @return Number of components. */
        inline unsigned countComponents() const
            { return boost::num_vertices(cg); }
        /** \brief Retrieves the number of depdencies.
         * @return Number of depdencies. */
        inline unsigned countDependencies() const
            { return boost::num_edges(cg); }

        //
        // helpers
        //
    protected:
        // helpers for writeGraphViz: extend for more output
        /** \brief Writes a single component in dot format.
         * @param o Stream to write to.
         * @param c Component to output.
         * @param index Index to use for identifying the component.
         * @param verbose True to include more detailed information. */
        virtual void writeGraphVizComponentLabel(std::ostream& o, Component c, unsigned index, bool verbose) const;
        /** \brief Writes a single dependency in dot format.
         * @param o Stream to write to.
         * @param dep Dependency to output.
         * @param verbose True to include more detailed information. */
        virtual void writeGraphVizDependencyLabel(std::ostream& o, Dependency dep, bool verbose) const;

    protected:
        // helpers for constructor
        /** \brief Computes the meta information about the dependencies in the graph.
         * @param dp DependencyGraph to compute the information for. */
        void calculateComponents(const DependencyGraph& dg);

        /** \brief Checks if a given component uses value invention.
         * @param ci ComponentInfo of the component to check.
         * @return False if \p ci uses value invention and true otherwise. */
        bool calculateFixedDomain(ComponentInfo& ci) const;
        /** \brief Checks if a given component uses recursive aggregates.
         * @param ci ComponentInfo of the component to check.
         * @return True if \p ci uses recursive aggregates and false otherwise. */
        bool computeRecursiveAggregatesInComponent(ComponentInfo& ci) const;

    public:
        /** \brief Computes stratification info for a component and stores it in the graph.
         * @param reg Registry used for resolving IDs.
         * @param ci ComponentInfo of the component to check. */
        static void calculateStratificationInfo(RegistryPtr reg, ComponentInfo& ci);
};

DLVHEX_NAMESPACE_END
#endif                           // DEPENDENCY_GRAPH_HPP_INCLUDED__18102010
