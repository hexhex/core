/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) Peter Schüller
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
 * @file   DependencyGraph.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Dependency Graph interface.
 */

#ifndef DEPENDENCY_GRAPH_HPP_INCLUDED__18102010
#define DEPENDENCY_GRAPH_HPP_INCLUDED__18102010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/ID.h"

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/multi_index/member.hpp>
#include <boost/multi_index/hashed_index.hpp>

//#include <cassert>

/*
 * TODO update paper/formal stuff:
 * definition of unifying dependency as in roman's thesis, not as in eswc paper
 * rule nodes added to graph
 * constraints have extra types of dependencies
 * negative dependencies unclear in roman's thesis: def 4.6.2 after second point 2:
 * "or b is a non-monotonic external atom" should imho be "or b \in B(r) and b a non-monotonic external atom"
 * (doesn't this give us fixed-point vs guess-and-check model generator choice for free?)
 * we will add those negative dependencies from rule to body atoms only
 * add external dependency if constant input has a variable created by output of other extatom (not covered by roman's thesis)
 * auxiliary rules do not take all body literals with the external atom's input variable, they only take positive literals!
 *   (this is stated differently in the thesis. perhaps it would be more efficient to take the
 *   transitive closure of dependencies over variables to make the body larger (and therefore reduce the grounding)
 *   example: &foo[X](), bar(X,Y), not baz(X,Z), boo(Z) -> is it more efficient to take all body atoms instead of only bar(X,Y)?)
 *
 * for eval only:
 * create auxiliary input collecting predicates/rule before creating depedency graph
 * pos dependency from ext. atom to its auxiliary input collecting predicate
 */

DLVHEX_NAMESPACE_BEGIN

/** \brief Implements the rule dependency graph. */
class DependencyGraph
{
    //////////////////////////////////////////////////////////////////////////////
    // types
    //////////////////////////////////////////////////////////////////////////////
    public:
        /** \brief Meta information about a single node in the graph. */
        struct NodeInfo:
    public ostream_printable<NodeInfo>
    {
        // ID storage:
        // store rule as rule
        // store external atom body literal as atom (in non-naf-negated form)
        // store nothing else as node
        /** \brief Rule ID. */
        ID id;

        /** \brief Constructor.
         * @param id Rule ID. */
        NodeInfo(ID id=ID_FAIL): id(id) {}
        std::ostream& print(std::ostream& o) const;
    };

    /** \brief Stores meta information about a single dependency in the graph.
     *
     * * dependency A -> B where A is a regular rule and B is a regular rule:
     *   * one of A's positive body ordinary atom literals
     *     unifies with one of B's head atoms -> "positiveRegularRule"
     *   * one of A's negative body ordinary atom literals
     *     unifies with one of B's head atoms -> "negativeRule"
     *   * one of A's head atoms unifies with one of B's head atoms
     *     -> "unifyingHead"
     *     if A or B has a disjunctive head -> "disjunctive"
     * * dependency A -> B where A is a constraint and B is a regular rule:
     *   * one of A's positive body ordinary atom literals
     *     unifies with one of B's head atoms -> "positiveConstraint"
     *   * one of A's negative body ordinary atom literals
     *     unifies with one of B's head atoms -> "negativeRule"
     * * dependency A -> X where A is a rule and X is an external atom:
     *   * X is present in the positive body of A and X is monotonic
     *     -> "positiveExternal"
     *   * X is present in the positive body of A and X is nonmonotonic
     *     -> "positiveExternal" and "negativeExternal"
     *   * X is present in the negative body of A and X is monotonic
     *     -> "negativeExternal"
     *   * X is present in the negative body of A and X is nonmonotonic
     *     -> "positiveExternal" and "negativeExternal"
     * * dependency X -> A where X is an external atom and A is a rule:
     *   * A is the auxiliary input rule providing input for X in rule/constraint B
     *     -> "externalConstantInput"
     *   * a predicate input of X matches one head of rule A
     *     -> "externalPredicateInput"
     *   * a nonmonotonic predicate input of X matches one head of rule A
     *     -> "externalPredicateInput" and "externalNonmonotonicPredicateInput". */
    struct DependencyInfo:
    public ostream_printable<DependencyInfo>
    {
        bool positiveRegularRule;
        bool positiveConstraint;
        bool negativeRule;
        bool unifyingHead;
        bool disjunctive;
        bool positiveExternal;
        bool negativeExternal;
        bool externalConstantInput;
        bool externalPredicateInput;
        bool externalNonmonotonicPredicateInput;

        /** \brief Constructor. */
        DependencyInfo():
        positiveRegularRule(false),
            positiveConstraint(false),
            negativeRule(false),
            unifyingHead(false),
            disjunctive(false),
            positiveExternal(false),
            negativeExternal(false),
            externalConstantInput(false),
            externalPredicateInput(false),
            externalNonmonotonicPredicateInput(false)
            {}

        /** \brief Merges another DependencyInfo into this one.
         *
         * Note that this is already possible without ambiguity.
         * @param other ComponentInfo to merge into this one.
         * @return Reference to this. */
        const DependencyInfo& operator|=(const DependencyInfo& other);
        std::ostream& print(std::ostream& o) const;
    };

    // for out-edge list we use vecS so we may have duplicate edges which is not a
    // problem (at least not for the SCC algorithm, for drawing the graph we must take
    // care a bit, but drawing a graph need not be efficient)
    //
    // for vertices it is necesssary to use vecS because so many nice algorithms need
    // implicit vertex_index
    //
    // TODO: do we need bidirectional? (at the moment yes, to find roots and leaves)
    typedef boost::adjacency_list<
        boost::vecS, boost::vecS, boost::bidirectionalS,
        NodeInfo, DependencyInfo> Graph;
    typedef boost::graph_traits<Graph> Traits;

    typedef Graph::vertex_descriptor Node;
    typedef Graph::edge_descriptor Dependency;
    typedef Traits::vertex_iterator NodeIterator;
    typedef Traits::edge_iterator DependencyIterator;
    typedef Traits::out_edge_iterator PredecessorIterator;
    typedef Traits::in_edge_iterator SuccessorIterator;

    protected:
        // the node mapping maps IDs of external atoms and rules
        // to nodes of the dependency graph
        /** \brief See boost:graph. */
        struct IDTag {};
        /** \brief See boost:graph. */
        struct NodeMappingInfo
        {
            ID id;
            Node node;
            NodeMappingInfo(): id(ID_FAIL) {}
            NodeMappingInfo(ID id, Node node): id(id), node(node) {}
        };
        typedef boost::multi_index_container<
            NodeMappingInfo,
            boost::multi_index::indexed_by<
            boost::multi_index::hashed_unique<
            boost::multi_index::tag<IDTag>,
            BOOST_MULTI_INDEX_MEMBER(NodeMappingInfo,ID,id)
            >
            >
            > NodeMapping;
        typedef NodeMapping::index<IDTag>::type NodeIDIndex;

    protected:
        typedef std::vector<Node> NodeList;
        /** \brief Stores for a given ordinary atom where it occurs. */
        struct HeadBodyInfo
        {
            /** \brief Ordinary ground or nonground atom id. */
            ID id;
            /** \brief True if \p id occurs in a head. */
            bool inHead;
            /** \brief True if \p id occurs in a body. */
            bool inBody;
            /** \brief True if \p id occurs in the head of a nondisjunctive rule. */
            NodeList inHeadOfNondisjunctiveRules;
            /** \brief True if \p id occurs in the head of a disjunctive rule. */
            NodeList inHeadOfDisjunctiveRules;
            /** \brief True if \p id occurs in the positive body of a rule which is not a constraint. */
            NodeList inPosBodyOfRegularRules;
            /** \brief True if \p id occurs in the positive body of a constraint. */
            NodeList inPosBodyOfConstraints;
            /** \brief True if \p id occurs in the negative body of any rule (regular and constraint). */
            NodeList inNegBodyOfRules;
            /** \brief Predicate of the atom; only specified if inHead is true. */
            ID headPredicate;
            /** \brief Pointer to the original OrdinaryAtom. */
            const OrdinaryAtom* oatom;

            /** \brief Constructor.
             * @param oatom See HeadBodyInfo::oatom. */
            HeadBodyInfo(const OrdinaryAtom* oatom = NULL):
            id(ID_FAIL), inHead(false), inBody(false),
                headPredicate(ID_FAIL), oatom(oatom) {}
        };

        /** \brief See boost::graph. */
        struct InHeadTag {};
        /** \brief See boost::graph. */
        struct InBodyTag {};
        /** \brief See boost::graph. */
        struct HeadPredicateTag {};
        /** \brief See boost::graph. */
        struct HeadBodyHelper
        {
            typedef boost::multi_index_container<
                HeadBodyInfo,
                boost::multi_index::indexed_by<
                boost::multi_index::hashed_unique<
                boost::multi_index::tag<IDTag>,
                BOOST_MULTI_INDEX_MEMBER(HeadBodyInfo,ID,id)
                >,
                boost::multi_index::hashed_non_unique<
                boost::multi_index::tag<InHeadTag>,
                BOOST_MULTI_INDEX_MEMBER(HeadBodyInfo,bool,inHead)
                >,
                boost::multi_index::hashed_non_unique<
                boost::multi_index::tag<InBodyTag>,
                BOOST_MULTI_INDEX_MEMBER(HeadBodyInfo,bool,inBody)
                >,
                boost::multi_index::hashed_non_unique<
                boost::multi_index::tag<HeadPredicateTag>,
                BOOST_MULTI_INDEX_MEMBER(HeadBodyInfo,ID,headPredicate)
                >
                >
                > HBInfos;
            typedef HBInfos::index<IDTag>::type IDIndex;
            typedef HBInfos::index<InHeadTag>::type InHeadIndex;
            typedef HBInfos::index<InBodyTag>::type InBodyIndex;
            typedef HBInfos::index<HeadPredicateTag>::type HeadPredicateIndex;

            /** \brief See boost::graph. */
            HBInfos infos;
        };

        //////////////////////////////////////////////////////////////////////////////
        // members
        //////////////////////////////////////////////////////////////////////////////
    protected:
        /** \brief ProgramCtx. */
        ProgramCtx& ctx;
        /** \brief Registry used for resolving IDs. */
        RegistryPtr registry;
        /** \brief Instance of the internal graph. */
        Graph dg;
        /** \brief See boost::graph. */
        NodeMapping nm;

        //////////////////////////////////////////////////////////////////////////////
        // methods
        //////////////////////////////////////////////////////////////////////////////
    private:
        /** \brief Copy-constructor.
         *
         * Not implemented on purpose because forbidden to use.
         * @param other Other graph. */
        DependencyGraph(const Dependency& other);
    public:
        /** \brief Constructor.
         * @param ctx See DependencyGraph::ctx.
         * @param registry See DependencyGraph::registry. */
        DependencyGraph(ProgramCtx& ctx, RegistryPtr registry);
        /** \brief Destructor. */
        virtual ~DependencyGraph();

        /** \brief Creates all dependencies and auxiliary input rules.
         * @param idb IDB of the program.
         * @param createdAuxRules Container to receive the auxiliary input rules. */
        void createDependencies(const std::vector<ID>& idb, std::vector<ID>& createdAuxRules);

        /** \brief Output graph as graphviz source (dot file).
         * @param o Stream to print the graph to.
         * @param verbose True to include more information. */
        virtual void writeGraphViz(std::ostream& o, bool verbose) const;

        /** \brief Retrieves the internal graph.
         * @return Internal graph. */
        const Graph& getInternalGraph() const
            { return dg; }

        // get node given some object id
        inline Node getNode(ID id) const
        {
            const NodeIDIndex& idx = nm.get<IDTag>();
            NodeIDIndex::const_iterator it = idx.find(id);
            assert(it != idx.end());
            return it->node;
        }

        /** \brief Get range over all nodes.
         * @param Node Some node of the graph.
         * @return NodeInfo of \p node. */
        inline std::pair<NodeIterator, NodeIterator> getNodes() const
            { return boost::vertices(dg); }

        /** \brief Get node info given node
         * @param Node Some node of the graph.
         * @return NodeInfo of \p node. */
        inline const NodeInfo& getNodeInfo(Node node) const
            { return dg[node]; }

        /** \brief Get dependency info given dependency
         * @param dep Some dependency of the graph.
         * @return DependencyInfo of \p dep. */
        inline const DependencyInfo& getDependencyInfo(Dependency dep) const
            { return dg[dep]; }

        /** \brief Get dependencies (to predecessors) = arcs from this node to others.
         * @param Node Some node of the graph.
         * @return Pair of begin and end iterator. */
        inline std::pair<PredecessorIterator, PredecessorIterator>
            getDependencies(Node node) const
            { return boost::out_edges(node, dg); }

        /** \brief Get provides (dependencies to successors) = arcs from other nodes to this one.
         * @param Node Some node of the graph.
         * @return Pair of begin and end iterator. */
        inline std::pair<SuccessorIterator, SuccessorIterator>
            getProvides(Node node) const
            { return boost::in_edges(node, dg); }

        /** \brief Get source of dependency = node that depends.
         * @param d Some depndency of the graph.
         * @return Source of \p d. */
        inline Node sourceOf(Dependency d) const
            { return boost::source(d, dg); }

        /** \brief Get target of dependency = node upon which the source depends.
         * @param d Some dependency of the graph.
         * @return Target of \p d. */
        inline Node targetOf(Dependency d) const
            { return boost::target(d, dg); }

        /** \brief Get node properties.
         * @param Node Some node of the graph.
         * @return NodeInfo of \p n. */
        inline const NodeInfo& propsOf(Node n) const
            { return dg[n]; }
        /** \brief Get node properties.
         * @param Node Some node of the graph.
         * @return NodeInfo of \p n. */
        inline NodeInfo& propsOf(Node n)
            { return dg[n]; }
        /** \brief Get dependency properties.
         * @param d Some dependency of the graph.
         * @return DependencyInfo of \p d. */
        inline const DependencyInfo& propsOf(Dependency d) const
            { return dg[d]; }
        /** \brief Get dependency properties.
         * @param d Some dependency of the graph.
         * @return DependencyInfo of \p d. */
        inline DependencyInfo& propsOf(Dependency d)
            { return dg[d]; }

        // counting -> mainly for allocating and testing
        inline unsigned countNodes() const
            { return boost::num_vertices(dg); }
        inline unsigned countDependencies() const
            { return boost::num_edges(dg); }

    protected:
        /** \brief Creates a node and updates the node mapping.
         * @param id Node to create.
         * @return New node in the graph. */
        inline Node createNode(ID id);

    protected:
        /** \brief Creates nodes for rules and external atoms.
         *
         * Create "positiveExternal" and "negativeExternal" dependencies.
         * Create "externalConstantInput" dependencies and auxiliary rules.
         * Fills HeadBodyHelper (required for efficient unification).
         * @param idb IDB of the program.
         * @param createdAuxRules Container to receive the auxiliary input rules.
         * @param hbh Information about heads and bodies. */
        void createNodesAndIntraRuleDependencies(
            const std::vector<ID>& idb,
            std::vector<ID>& createdAuxRules,
            HeadBodyHelper& hbh);
        // helpers
        /** \brief Creates edges for dependencies within rules.
         * @param idrule ID of the rule to create the dependencies for.
         * @param createdAuxRules Container to receive the auxiliary input rules.
         * @param hbh Information about heads and bodies. */
        void createNodesAndIntraRuleDependenciesForRule(
            ID idrule,
            std::vector<ID>& createdAuxRules,
            HeadBodyHelper& hbh);
        /** \brief Updates the graph after recognizing a head atom.
         * @param idat Head atom.
         * @param rule The rule atom \p idat belongs to.
         * @param nrule Node of \p rule.
         * @param hbh Information about heads and bodies. */
        void createNodesAndIntraRuleDependenciesForRuleAddHead(
            ID idat, const Rule& rule, Node nrule, HeadBodyHelper& hbh);
        /** \brief Updates the graph after recognizing a body atom.
         * @param idlit Body literal.
         * @param idrule ID of the rule literal \p idlit belongs to.
         * @param body Body of \p idrule.
         * @param nrule Node of \p body.
         * @param hbh Information about heads and bodies.
         * @param createdAuxRules Container to receive the auxiliary input rules.
         * @param inAggregateBody True to indicate that \p idlit is added to an aggregate body, false to add it to a regular rule body. */
        void createNodesAndIntraRuleDependenciesForBody(
            ID idlit, ID idrule, const Tuple& body, Node nrule,
            HeadBodyHelper& hbh, std::vector<ID>& createdAuxRules,
            bool inAggregateBody = false);
        /** \brief This method creates an auxiliary rule for the eatom wrt a rule body (not a rule!).
         *
         * This way we can use the method both for grounding aggregate bodies as well as rule bodies.
         *
         * * For each eatom in the rule with variable inputs:
         *   * create auxiliary input predicate for its input
         *   * create auxiliary rule collecting its input, use as dependencies all positive literals (including eatoms) in the rule
         *   (this potentially creates many aux rules (cf. extatom2.hex)).
         * @param body Rule body.
         * @param idlit Body literal.
         * @param idat Head predicate.
         * @param neatom Node of external atom \p eatom.
         * @param eatom ID of the external atom to create auxiliary rules for.
         * @param createdAuxRules Container to receive the auxiliary input rules.
         * @param hbh Information about heads and bodies. */
        void createAuxiliaryRuleIfRequired(
            const Tuple& body,
            ID idlit, ID idat, Node neatom, const ExternalAtom& eatom,
            const PluginAtom* pluginAtom,
            std::vector<ID>& createdAuxRules,
            HeadBodyHelper& hbh);
        /** \brief Create auxiliary rule head predicate (in registry) and return ID.
         * @param forEAtom ID of the external atom to create an auxiliary rule for.
         * @return ID of the auxiliary rule for \p forEAtom. */
        ID createAuxiliaryRuleHeadPredicate(ID forEAtom);
        /** \brief Create auxiliary rule head (in registry) and return ID.
         * @param idauxpre Predicate to use in the auxiliary rule.
         * @param variables Variable to pass from the rule body to the head.
         * @return ID of the auxiliary rule. */
        ID createAuxiliaryRuleHead(ID idauxpred, const std::list<ID>& variables);
        /** \brief Create auxiliary rule (in registry) and return ID.
         * @param head Head atom ID.
         * @param body Rule body.
         * @return ID of the auxiliary rule. */
        ID createAuxiliaryRule(ID head, const std::list<ID>& body);

        /** \brief Create "externalPredicateInput" dependencies.
         * @param createdAuxRules Container to receive the auxiliary input rules. */
        void createExternalPredicateInputDependencies(const HeadBodyHelper& hbh);
        // helpers
        /** \brief Create "externalPredicateInput" dependencies.
         * @param nonmonotonic Specifies whether to create a monotonic or nonmonotonic dependency.
         * @param ni_eatom See boost::graph.
         * @param predicate Input predicate.
         * @param createdAuxRules Container to receive the auxiliary input rules. */
        void createExternalPredicateInputDependenciesForInput(
            bool nonmonotonic, const NodeMappingInfo& ni_eatom, ID predicate, const HeadBodyHelper& hbh);

        /** \brief Build all unifying dependencies ("{positive,negative}{Rule,Constraint}", "unifyingHead").
         * @param createdAuxRules Container to receive the auxiliary input rules. */
        void createUnifyingDependencies(const HeadBodyHelper& hbh);
        // helpers
        /** \brief Create "unifyingHead" dependencies.
         * @param createdAuxRules Container to receive the auxiliary input rules. */
        void createHeadHeadUnifyingDependencies(const HeadBodyHelper& hbh);
        /** \brief Create "{positive,negative}{Rule,Constraint}" dependencies.
         * @param createdAuxRules Container to receive the auxiliary input rules. */
        void createHeadBodyUnifyingDependencies(const HeadBodyHelper& hbh);

    protected:
        // helpers for writeGraphViz: extend for more output
        /** \brief Writes a single node in dot format.
         * @param o Stream to write to.
         * @param n Node to output.
         * @param verbose True to include more detailed information. */
        virtual void writeGraphVizNodeLabel(std::ostream& o, Node n, bool verbose) const;
        /** \brief Writes a single dependency in dot format.
         * @param o Stream to write to.
         * @param dep Dependency to output.
         * @param verbose True to include more detailed information. */
        virtual void writeGraphVizDependencyLabel(std::ostream& o, Dependency dep, bool verbose) const;
};

DependencyGraph::Node DependencyGraph::createNode(ID id)
{
    DBGLOG(DBG,"creating node for ID " << id);
    Node n = boost::add_vertex(NodeInfo(id), dg);
    {
        NodeIDIndex::const_iterator it;
        bool success;
        boost::tie(it, success) = nm.insert(NodeMappingInfo(id, n));
        assert(success);
    }
    return n;
}


DLVHEX_NAMESPACE_END
#endif                           // DEPENDENCY_GRAPH_HPP_INCLUDED__18102010
