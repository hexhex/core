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
 * @file   LiberalSafetyChecker.h
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 *
 * @brief  Implements new safety criteria which may be used in place of
 * strong safety.
 */

#ifndef LIBERALSAFETYCHECKER_H_
#define LIBERALSAFETYCHECKER_H_

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/ID.h"

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/intrusive/unordered_set.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/hashed_index.hpp>

DLVHEX_NAMESPACE_BEGIN

class LiberalSafetyChecker;

/*
 * Base class for safety plugins which may integrate application-specific safety criteria.
 */
class DLVHEX_EXPORT LiberalSafetyPlugin
{
    protected:
        /** \brief Reference to the safety checker which provides meta-information about the program. */
        LiberalSafetyChecker& lsc;

    public:
        /** \brief Copy-constructor.
         * @param lsc Other instance of LiberalSafetyChecker. */
        LiberalSafetyPlugin(LiberalSafetyChecker& lsc) : lsc(lsc){}

        /** \brief The run method is iteratively called and shall add
         * * bound variables using lsc.addBoundedVariable
         * * variables bound by externals using lsc.addExternallyBoundedVariable
         * * de-safe attributes using lsc.addDomainExpansionSafeAttribute. */
        virtual void run() = 0;

        typedef boost::shared_ptr<LiberalSafetyPlugin> Ptr;
};
typedef LiberalSafetyPlugin::Ptr LiberalSafetyPluginPtr;

/*
 * Factory for safety plugins.
 */
class DLVHEX_EXPORT LiberalSafetyPluginFactory
{
    public:
        /** \brief Instantiates LiberalSafetyPlugin.
         * @param lsc Instance of LiberalSafetyChecker.
         * @return LiberalSafetyPlugin. */
        virtual LiberalSafetyPluginPtr create(LiberalSafetyChecker& lsc) = 0;
        typedef boost::shared_ptr<LiberalSafetyPluginFactory> Ptr;
};
typedef LiberalSafetyPluginFactory::Ptr LiberalSafetyPluginFactoryPtr;

/** \brief Implements liberal safety extensible by LiberalSafetyPlugin. */
class DLVHEX_EXPORT LiberalSafetyChecker
{
    public:
        /** \brief Stores an ordinary or external (input or output) attribute. */
        struct Attribute : private ostream_printable<Attribute>
        {
            /** \brief Type of the attribute. */
            enum Type
            {
                /** \brief Argument position of an ordinary predicate. */
                Ordinary,
                /** \brief Input or output argument position of an external predicate. */
                External
            };
            /** \brief Registry. */
            RegistryPtr reg;
            /** \brief Type of this attribute. */
            Type type;
            /** \brief ID_FAIL for ordinary attributes and the external atom whose attribute is to be defined otherwise. */
            ID eatomID;
            /** \brief Ordinary or external predicate. */
            ID predicate;
            /** \brief Input attributes for external attributes. */
            std::vector<ID> inputList;
            /** \brief Rule where the external attributes occurs (only for external attributes). */
            ID ruleID;
            /** \brief Input or output attribute for external attributes. */
            bool input;
            /** \brief Ordinary, input or output argument position. */
            int argIndex;

            /** \brief Attribute comparison.
             * @param at2 Attribute to compare to.
             * @return True if this attribute is equal to \p at2 and false otherwise. */
            bool operator==(const Attribute& at2) const;
            /** \brief Implements an artificial order of attributes to make them usable in sets.
             * @param at2 Attribute to compare to.
             * @return True if this attribute is smaller than \p at2 and false otherwise. */
            bool operator<(const Attribute& at2) const;
            /** \brief Prints the attribute in human-readable format.
             * @param o Output stream to print to.
             * @return \p o. */
            std::ostream& print(std::ostream& o) const;
        };

        typedef boost::adjacency_list<boost::setS, boost::vecS, boost::bidirectionalS, Attribute > Graph;
        typedef boost::graph_traits<Graph> Traits;

        typedef Graph::vertex_descriptor Node;
        typedef Graph::edge_descriptor Dependency;
        typedef Traits::vertex_iterator NodeIterator;
        typedef Traits::edge_iterator DependencyIterator;
        typedef Traits::out_edge_iterator PredecessorIterator;
        typedef Traits::in_edge_iterator SuccessorIterator;

                                 // stores rule ID and variable ID
        typedef std::pair<ID, ID> VariableLocation;
                                 // stores rule ID and atom ID
        typedef std::pair<ID, ID> AtomLocation;

        /** \brief Registry. */
        RegistryPtr reg;
        /** \brief IDB of the program. */
        const std::vector<ID>& idb;

    private:
        /** \brief Attribute graph. */
        Graph ag;
        /** \brief Stores for each ordinary predicate its attributes. */
        boost::unordered_map<ID, std::vector<Attribute> > attributesOfPredicate;
        struct NodeInfoTag {};
        /** \brief See boost::graph. */
        struct NodeMappingInfo
        {
            Attribute at;
            Node node;
            NodeMappingInfo(Attribute at, Node node): at(at), node(node) {}
        };
        typedef boost::multi_index_container<
            NodeMappingInfo,
            boost::multi_index::indexed_by<
            boost::multi_index::hashed_unique<
            boost::multi_index::tag<NodeInfoTag>,
            BOOST_MULTI_INDEX_MEMBER(NodeMappingInfo,Attribute,at)
            >
            >
            > NodeMapping;
        /** \brief See boost::graph. */
        NodeMapping nm;
        typedef NodeMapping::index<NodeInfoTag>::type NodeNodeInfoIndex;
        /** \brief Strongly connected components in LiberalSafetyChecker::ag. */
        std::vector<std::vector<Attribute> > depSCC;

        // some indices
        /** \brief Stores which variables still need to be bounded.
         *
         * Also stores which attributes need to become safe in order to make another attribute safe. */
        typedef std::pair<std::set<VariableLocation>, boost::unordered_set<Attribute> > SafetyPreconditions;

        /** \brief Stores for each attributes the preconditios for becoming safe. */
        boost::unordered_map<Attribute, SafetyPreconditions> safetyPreconditions;

        /** \brief Stores for each variable the attributes whose safety depends on this variable. */
        boost::unordered_map<VariableLocation, boost::unordered_set<Attribute> > attributesSafeByVariable;
        /** \brief Stores for each attribute the attributes whose safety depends on this attribute. */
        boost::unordered_map<Attribute, boost::unordered_set<Attribute> > attributesSafeByAttribute;

        /** \brief Stores for each attribute the atoms where it occurs. */
        boost::unordered_map<Attribute, std::set<AtomLocation> > attributeOccursIn;
        /** \brief Stores for each variable the atoms where it occurs. */
        boost::unordered_map<VariableLocation, std::set<AtomLocation> > variableOccursIn;

        // output
        /** \brief Arity of a given (ordinary) predciate. */
        boost::unordered_map<ID, int> predicateArity;
        /** \brief Stores all attributes which occur in cycles. */
        std::set<Node> cyclicAttributes;
        /** \brief Currently bounded variables. */
        boost::unordered_set<VariableLocation> boundedVariables;
        /** \brief Current domain-expansion safe attributes. */
        boost::unordered_set<Attribute> domainExpansionSafeAttributes;
        /** \brief External atoms which are necessary to establish domain-expansion safety. */
        boost::unordered_set<IDAddress> necessaryExternalAtoms;
        /** \brief Variables bounded by externals, but not (yet) by ordinary atoms. */
        boost::unordered_set<std::pair<ID, VariableLocation> > boundedByExternals;

    public:
        /** \brief Constructs an external attribute.
         * @param eatomID External atom.
         * @param inputList Input parameters.
         * @param ruleID Rule where \p eatomID occurs.
         * @param inputAttribute True to generate an input attribute and false to generate an output attribute.
         * @param argumentIndex Index of the attribute in the input or output list, depending on \p inputAttribute.
         * @return External input or output attribute. */
        Attribute getAttribute(ID eatomID, ID predicate, std::vector<ID> inputList, ID ruleID, bool inputAttribute, int argumentIndex);
        /** \brief Constructs an ordinary attribute.
         * @param predicate Predicate ID.
         * @param argumentIndex Argument position of \p predicate.
         * @return Ordinary attribute. */
        Attribute getAttribute(ID predicate, int argumentIndex);

    private:
        /** \brief Retrieves the internal node from the attribute dependency graph corresponding to an attribute.
         * @param at Attribute.
         * @return Node. */
        Node getNode(Attribute at);

        // helper
        /** \brief Checks if there is flow of information from one builtin atom to another.
         * @param builtinflow Detected information flow.
         * @param from First builtin.
         * @param to Second builtin.
         * @return True if information flows from \p from to \p to, and false otherwise. */
        bool hasInformationFlow(boost::unordered_map<ID, boost::unordered_set<ID> >& builtinflow, ID from, ID to);
        /** \brief Checks if \p at has recently been declared safe.
         * @param at Attribute to check.
         * @return True if \p at has recently been declared safe and false otherwise. */
        bool isNewlySafe(Attribute at);

    public:
        // trigger functions
        /** \brief Called for adding variables bounded by external atoms. */
        void addExternallyBoundedVariable(ID extAtom, VariableLocation vl);
        /** \brief Called after a new variable has become bounded to trigger further actions. */
        void addBoundedVariable(VariableLocation vl);
        /** \brief Called after an attribute has become safe to trigger further actions. */
        void addDomainExpansionSafeAttribute(Attribute at);

        /** \brief Retrieves the IDB for which the checker was instantiated.
         * @return IDB. */
        const std::vector<ID>& getIdb();
        /** \brief Retrieves the internal attribute dependency graph.
         * @return Attribute dependency graph. */
        const Graph& getAttributeGraph();
        /** \brief Retrieves the strongly connected components of the internal attribute dependency graph.
         * @return Strongly connected components of the attribute dependency graph. */
        const std::vector<std::vector<Attribute> >& getDepSCC();
        /** \brief Retrieves the attributes which are liberally domain-expansion safe.
         * @return Set of attributes. */
        const boost::unordered_set<Attribute>& getDomainExpansionSafeAttributes();
        /** \brief Retrieves the set of variables which have been shown to be bounded.
         * @return Set of variables consisting of the rule ID and the variable term. */
        const boost::unordered_set<VariableLocation>& getBoundedVariables();
        void getReachableAttributes(Attribute start, std::set<Node>& output);
        /** \brief Retrieves the arity of an ordinary predicate.
         * @param predicate Ordinary predicate.
         * @return Arity of \p predicate. */
        int getPredicateArity(ID predicate) const;

    private:
        // initialization
        /** \brief Computes for a given rule the information exchange between variables through builtins.
         * @param rule Rule to compute the information for.
         * @param builtinflow See LiberalSafetyChecker::hasInformationFlow. */
        void computeBuiltinInformationFlow(const Rule& rule, boost::unordered_map<ID, boost::unordered_set<ID> >& builtinflow);

        /** \brief Create dependency graph of ordinary and external predicates. */
        void createDependencyGraph();
        /** \brief The indices stored in the class members. */
        void createPreconditionsAndLocationIndices();
        /** \brief Compute attributes which occur in or depend on cycles */
        void computeCyclicAttributes();

        // computation
        /** \brief Restricts the optimization of necessary to keep ordinary safety. */
        void ensureOrdinarySafety();

        /** \brief Calls the previous methods until no more safe attributes can be derived. */
        void computeDomainExpansionSafety();

        /** \brief List of loaded LiberalSafetyPlugins. */
        std::vector<LiberalSafetyPluginPtr> safetyPlugins;
    public:
        /** \brief Constructor.
         * @param reg See LiberalSafetyChecker::reg.
         * @param idb See LiberalSafetyChecker::idb.
         * @param customSafetyPlugins See LiberalSafetyChecker::safetyPlugins. */
        LiberalSafetyChecker(RegistryPtr reg, const std::vector<ID>& idb, std::vector<LiberalSafetyPluginFactoryPtr> customSafetyPlugins);

        /** \brief Checks if the program is liberally domain-expansion safe.
         * @return True if the program is liberally domain-expansion safe and false otherwise. */
        bool isDomainExpansionSafe() const;
        /** \brief Checks if a given external atom is necessary for establishing liberal domain-expansion safety.
         * @param eaomID The external atom whose relevance shall be checked.
         * @return True if \p eatomID is necessary for establishing liberal domain-expansion safety and false otherwise. */
        bool isExternalAtomNecessaryForDomainExpansionSafety(ID eatomID) const;

        /** \brief Output the attribute dependency graph as graphviz source (dot file).
         * @param o Stream to write the graph to.
         * @param verbose True to include more detailed information. */
        virtual void writeGraphViz(std::ostream& o, bool verbose) const;
};

DLVHEX_EXPORT std::size_t hash_value(const LiberalSafetyChecker::Attribute& at);
DLVHEX_EXPORT std::size_t hash_value(const LiberalSafetyChecker::VariableLocation& vl);

typedef boost::shared_ptr<LiberalSafetyChecker> LiberalSafetyCheckerPtr;
typedef boost::shared_ptr<const LiberalSafetyChecker> LiberalSafetyCheckerConstPtr;

DLVHEX_NAMESPACE_END
#endif
