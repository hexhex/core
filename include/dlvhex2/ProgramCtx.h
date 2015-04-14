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
 * @file ProgramCtx.h
 * @author Thomas Krennwallner
 * @author Peter Schueller
 *
 * @brief Program context
 */

#if !defined(_DLVHEX_PROGRAMCTX_H)
#define _DLVHEX_PROGRAMCTX_H

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/Configuration.h"
#include "dlvhex2/ASPSolverManager.h"
#include "dlvhex2/Interpretation.h"
#include "dlvhex2/PluginContainer.h"
#include "dlvhex2/InputProvider.h"
#include "dlvhex2/FinalEvalGraph.h"
#include "dlvhex2/EvalHeuristicBase.h"
#include "dlvhex2/EvalGraphBuilder.h"
#include "dlvhex2/ExternalAtomEvaluationHeuristicsInterface.h"
#include "dlvhex2/UnfoundedSetCheckHeuristics.h"
#include "dlvhex2/ModelBuilder.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Nogood.h"

#include <boost/shared_ptr.hpp>
#include <boost/functional/factory.hpp>

#include <typeinfo>
#include <vector>
#include <string>
#include <iosfwd>

DLVHEX_NAMESPACE_BEGIN

typedef boost::shared_ptr<EvalHeuristicBase<EvalGraphBuilder> >
EvalHeuristicPtr;

typedef boost::shared_ptr<ModelBuilder<FinalEvalGraph> >
ModelBuilderPtr;

typedef boost::function<ModelBuilder<FinalEvalGraph>*(ModelBuilderConfig<FinalEvalGraph>&)>
ModelBuilderFactory;

typedef std::map<std::string, PluginAtomPtr>
PluginAtomMap;

/**
 * @brief Program context class.
 *
 * A facade/state context for the subcomponents of dlvhex.
 */
class DLVHEX_EXPORT ProgramCtx
{
    public:
        /** \brief dlvhex settings, previously globals. */
        Configuration config;

        /** \brief Retrieve main Registry.
         * @return Registry. */
        const RegistryPtr& registry() const
            { return _registry; }
        /** \brief Retrieve plugin container.
         * @return PluginContainer. */
        const PluginContainerPtr& pluginContainer() const
            { return _pluginContainer; }

        /** \brief Setup the registry.
         *
         * It cannot bechanged if something is already stored here!
         * @param registry New registry. */
        void setupRegistry(RegistryPtr registry);

        /** \brief Change registry .
         *
         * This method may also be used if there is already something stored.
         * However, this might invaliddate the IDs used elsewhere!
         * @param registry New registry. */
        void changeRegistry(RegistryPtr registry);

        /** \brief Setup plugin container.
         * @param pluginContainer New plugin container. */
        void setupPluginContainer(PluginContainerPtr pluginContainer);

        /** \brief Factory for eval heuristics. */
        EvalHeuristicPtr evalHeuristic;
        /** \brief Factory for model builders. */
        ModelBuilderFactory modelBuilderFactory;
        /** \brief Factory for external atom evaluation heuristic and ufs check heuristic. */
        ExternalAtomEvaluationHeuristicsFactoryPtr defaultExternalAtomEvaluationHeuristicsFactory;
        /** \brief Factory for the unfounded set check heuristics. */
        UnfoundedSetCheckHeuristicsFactoryPtr unfoundedSetCheckHeuristicsFactory;

        /** \brief ASP solver backend. */
        ASPSolverManager::SoftwareConfigurationPtr aspsoftware;

        /** \brief Program input provider (if a converter is used, the converter consumes this input and replaces it by another input). */
        InputProviderPtr inputProvider;

        /** \brief The input parser. */
        HexParserPtr parser;

        /** \brief Program IDB. */
        std::vector<ID> idb;
        std::vector<std::vector<ID> > idbList;

        /** \brief Program EDB as Interpretation. */
        Interpretation::Ptr edb;
        /** \brief Program EDB as vector. */
        std::vector<InterpretationPtr> edbList;

        /** \brief maxint setting, this is ID_FAIL if it is not specified, an integer term otherwise. */
        uint32_t maxint;

        /** \brief Might be set to a plugin which provides a custom model generator factoy.
         *
         * If set to 0, then the default algorithms are used. */
        PluginInterfacePtr customModelGeneratorProvider;

        /** \brief Stores the weight vector of the best known model.
         *
         * If the vector is empty, then there was no solution so far.
         * This vector will always be updated, independent of the optimization settings, and also has statistical purpose. It does not directly influence the algorithms. */
        std::vector<int> currentOptimum;

        // used by plugins to store specific plugin data in ProgramCtx
        // default constructs PluginT::CtxData if it is not yet stored in ProgramCtx
        template<typename PluginT>
            typename PluginT::CtxData& getPluginData();

        /** \brief Used by plugins to store specific plugin data in ProgramCtx.
         *
         * Default constructs PluginT::Environment if it is not yet stored in ProgramCtx. */
        template<typename PluginT>
            typename PluginT::Environment& getPluginEnvironment();
        template<typename PluginT>
            const typename PluginT::Environment& getPluginEnvironment() const;

        // TODO: add visibility policy (as in clasp)
        /** \brief DependencyGraph. */
        DependencyGraphPtr depgraph;
        /** \brief ComponentGraph. */
        ComponentGraphPtr compgraph;
        /** \brief Plugins to LiberalSafetyChecker. */
        std::vector<LiberalSafetyPluginFactoryPtr> liberalSafetyPlugins;
        /** \brief LiberalSafetyChecker. */
        LiberalSafetyCheckerPtr liberalSafetyChecker;
        /** \brief FinalEvaluationGraph. */
        FinalEvalGraphPtr evalgraph;
        /** \brief Final unit in ProgramCtx::evalgraph. */
        FinalEvalGraph::EvalUnit ufinal;
        /** \brief Callbacks for models. */
        std::list<ModelCallbackPtr> modelCallbacks;
        /** \brief FinalCallback. */
        std::list<FinalCallbackPtr> finalCallbacks;
        /** \brief ModelBuilder being used, may be online or offline. */
        ModelBuilderPtr modelBuilder;
        // model graph is only accessible via modelbuilder->getModelGraph()!
        // (model graph is part of the model builder) TODO think about that

        /** \brief Stores which benchmarks shall be preserved at first model. */
        std::map<std::string, std::string> benchmarksToSnapshotAtFirstModel;

        /** \brief Current reasoner state. */
        StatePtr state;

        /** \brief Set to true if all processing on this ProgramCtx shall be aborted
         * (e.g., because of a global timeout).
         *
         * We currently use this to shutdown cleanly if signalled, but it could
         * become important for solving with multiple contexts where some must be
         * aborted. */
        bool terminationRequest;

        /** \brief Change reasoner state.
         * @param s New state. */
        void
            changeState(const boost::shared_ptr<State>& s);

    public:
        /** \brief Constructor. */
        ProgramCtx();
        /** \brief Destructor.
         *
         * Not virtual, we do not want to derive from this! */
        ~ProgramCtx();

        /** \brief Retrieves the configuration of the ASP solver backend being used.
         * @return See SoftwareConfiguration. */
        ASPSolverManager::SoftwareConfigurationPtr
            getASPSoftware() const;

        /** \brief Configures the ASP solver backend being used.
         * @param See SoftwareConfiguration. */
        void
            setASPSoftware(ASPSolverManager::SoftwareConfigurationPtr c);

        //
        // plugin helpers
        //

        /** \brief Processes options for each plugin loaded in this ProgramCtx.
         *
         * This is supposed to remove "recognized" options from pluginOptions.
         * @param pluginOptions Options not recognized by the dlvhex core. */
        void processPluginOptions(std::list<const char*>& pluginOptions);

        /** \brief Goes through _pluginContainer to get plugin atoms and adds them. */
        void addPluginAtomsFromPluginContainer();

        /** \brief Add \p atom to this ProgramCtx and link it to registry of this ProgramCtx.
         * @param atom The PluginInterface::PluginAtom to be added. */
        void addPluginAtom(PluginAtomPtr atom);

        /** \brief Associates external atoms in registry of this ProgramCtx
         * with plugin atoms in given \p idb.
         *
         * @param idb Program IDB.
         * @param failOnUnknownAtom Throws on unknown atom iff failOnUnknownAtom is true. */
        void associateExtAtomsWithPluginAtoms(const Tuple& idb, bool failOnUnknownAtom=true);

        /** \brief Setup this ProgramCtx (using setupProgramCtx() for of all plugins). */
        void setupByPlugins();

        /** \brief Resets the cache of Plugins (either all, or only those that use Environment).
         *
         * The default value is for backwards-compatibility with ActHex).
         * @param resetOnlyIfUsesEnvironment If true, only the caches of external atom are resetted which actually depend on the environment. */
        void resetCacheOfPlugins(bool resetOnlyIfUsesEnvironment=true);

        //
        // state processing
        // the following functions are given in intended order of calling
        // optional functions may be omitted
        //

        /** \brief See State.
         *
         * Optional. */
        void showPlugins();
        /** \brief See State.
         *
         * Optional. */
        void convert();
        /** \brief See State. */
        void parse();
        /** \brief See State. */
        void moduleSyntaxCheck();
        /** \brief See State. */
        void mlpSolver();
        /** \brief See State.
         *
         * Optional. */
        void rewriteEDBIDB();
        /** \brief See State.
         *
         * Optional if we know that the program is safe. */
        void safetyCheck();
        /** \brief See State. */
        void createDependencyGraph();
        /** \brief See State. */
        void liberalSafetyCheck();
        /** \brief See State.
         *
         * Optional. */
        void optimizeEDBDependencyGraph();
        /** \brief See State. */
        void createComponentGraph();
        /** \brief See State.
         *
         * Optional if we know that the program is strongly safe. */
        void strongSafetyCheck();
        /** \brief See State. */
        void createEvalGraph();
        /** \brief See State. */
        void setupProgramCtx();
        /** \brief See State. */
        void evaluate();
        /** \brief See State. */
        void postProcess();

        /** \brief Class for subprogram handling. */
        class SubprogramAnswerSetCallback : public ModelCallback
        {
            public:
                /** \brief Container for the answer sets of the subprogram. */
                std::vector<InterpretationPtr> answersets;
                /** \brief Is called when a new model is found.
                 * @param model Currently found model. */
                virtual bool operator()(AnswerSetPtr model);
                /** \brief Destructor. */
                virtual ~SubprogramAnswerSetCallback();
        };
        /** \brief Evaluates an already parsed subprogram.
         * @param edb EDB of the subprogram.
         * @param idb IDB of the subprogram.
         * @return Set of answer sets of the subprorgram. */
        std::vector<InterpretationPtr> evaluateSubprogram(InterpretationConstPtr edb, std::vector<ID>& idb);
        /** \brief Evaluates a subprogram which still needs to be parsed.
         * @param ip InputProvider which specifies the subprogram.
         * @param addFacts Facts to be added to the subprogram before it is evaluated.
         * @return Set of answer sets of the subprorgram. */
        std::vector<InterpretationPtr> evaluateSubprogram(InputProviderPtr& ip, InterpretationConstPtr addFacts);
        /** \brief Evaluates a subprogram given by a ProgramCtx.
         * @param parse True to read the program from ProgramCtx::inputProvider (\p pc), false to read it from ProgramCtx::edb and ProgramCtx::idb (\p pc).
         * @return Set of answer sets of the subprorgram. */
        std::vector<InterpretationPtr> evaluateSubprogram(ProgramCtx& pc, bool parse);

    protected:
        /** \brief Symbol storage of this program context.
         *
         * This is a shared ptr because we might want
         * to have multiple program contexts sharing the same registry). */
        RegistryPtr _registry;

        /** \brief Plugin container (this must be initialized with above registry!). */
        PluginContainerPtr _pluginContainer;

        typedef std::map<std::string, boost::shared_ptr<PluginData> > PluginDataContainer;
        /** \brief Data associated with one specific plugin.
         *
         * Externally we see this as a non-const reference, the shared_ptr is totally internal. */
        PluginDataContainer pluginData;

        typedef std::map<std::string, boost::shared_ptr<PluginEnvironment> > PluginEnvironmentContainer;
        /** \brief Environment associated with one specific plugin.
         *
         * Externally we see this as a non-const reference, the shared_ptr is totally internal. */
        PluginEnvironmentContainer pluginEnvironment;

        /** \brief Atoms usable for evaluation (loaded from plugins or manually added). */
        PluginAtomMap pluginAtoms;
};

// used by plugins to store specific plugin data in ProgramCtx
// default constructs PluginT::CtxData if it is not yet stored in ProgramCtx
template<typename PluginT>
typename PluginT::CtxData& ProgramCtx::getPluginData()
{
    const std::string pluginTypeName(typeid(PluginT).name());
    PluginDataContainer::const_iterator it =
        pluginData.find(pluginTypeName);
    if( it == pluginData.end() ) {
        it = pluginData.insert(std::make_pair(
            pluginTypeName,
            boost::shared_ptr<PluginData>(new typename PluginT::CtxData))
            ).first;
    }
    typename PluginT::CtxData* pret =
        dynamic_cast<typename PluginT::CtxData*>(it->second.get());
    assert(!!pret);
    return *pret;
}


// used by plugins to store specific plugin data in ProgramCtx
// default constructs PluginT::Environment if it is not yet stored in ProgramCtx
template<typename PluginT>
typename PluginT::Environment& ProgramCtx::getPluginEnvironment()
{
    const std::string pluginTypeName(typeid(PluginT).name());
    PluginEnvironmentContainer::const_iterator it =
        pluginEnvironment.find(pluginTypeName);
    if( it == pluginEnvironment.end() ) {
        it = pluginEnvironment.insert(std::make_pair(
            pluginTypeName,
            boost::shared_ptr<PluginEnvironment>(new typename PluginT::Environment))
            ).first;
    }
    typename PluginT::Environment* pret =
        dynamic_cast<typename PluginT::Environment*>(it->second.get());
    assert(!!pret);
    return *pret;
}


template<typename PluginT>
const typename PluginT::Environment& ProgramCtx::getPluginEnvironment() const
{
    const std::string pluginTypeName(typeid(PluginT).name());
    PluginEnvironmentContainer::const_iterator it =
        pluginEnvironment.find(pluginTypeName);
    if( it == pluginEnvironment.end() ) {
        throw std::runtime_error("Cannot use getPluginEnvironment const before using not const");
    }
    const typename PluginT::Environment* pret =
        dynamic_cast<const typename PluginT::Environment*>(it->second.get());
    assert(!!pret);
    return *pret;
}


DLVHEX_NAMESPACE_END
#endif                           /* _DLVHEX_PROGRAMCTX_H */



// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
