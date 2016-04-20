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
 * @file   GenuineGuessAndCheckModelGenerator.hpp
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 *
 * @brief  Model generator for eval units that do not allow a fixpoint calculation.
 *
 * Those units may be of any form.
 */

#ifndef GENUINEGUESSANDCHECK_MODEL_GENERATOR_HPP_INCLUDED__09122011
#define GENUINEGUESSANDCHECK_MODEL_GENERATOR_HPP_INCLUDED__09122011

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/FLPModelGeneratorBase.h"
#include "dlvhex2/ComponentGraph.h"
#include "dlvhex2/PredicateMask.h"
#include "dlvhex2/GenuineSolver.h"
#include "dlvhex2/InternalGroundDASPSolver.h"
#include "dlvhex2/UnfoundedSetChecker.h"
#include "dlvhex2/NogoodGrounder.h"
#include "dlvhex2/ExternalAtomVerificationTree.h"

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

class GenuineGuessAndCheckModelGeneratorFactory;

/** \brief Model generator for arbitrary components. */
class GenuineGuessAndCheckModelGenerator:
public FLPModelGeneratorBase,
public ostream_printable<GenuineGuessAndCheckModelGenerator>,
public PropagatorCallback
{
    // types
    public:
        typedef GenuineGuessAndCheckModelGeneratorFactory Factory;

        // storage
    protected:
        // we store the factory again, because the base class stores it with the base type only!
        /** \brief Reference to the factory which created this model generator. */
        Factory& factory;
        /** \brief RegistryPtr. */
        RegistryPtr reg;

        // information about verification/falsification of current external atom guesses
        /** \brief The set of inner external atoms which were <em>not</em> inlined. */
        std::vector<ID> activeInnerEatoms;
        /** \brief Stores for each replacement atom the set of external atoms which shall be verified when the replacement atom is (re-)assigned. */
        boost::unordered_map<IDAddress, std::vector<int> > verifyWatchList;
        /** \brief Stores for each replacement atom the set of external atoms which shall be unverified when the replacement atom is (re-)assigned. */
        boost::unordered_map<IDAddress, std::vector<int> > unverifyWatchList;
        /** \brief Stores for each external atom guess if it already was checked against the semantics (i.e., it is either verified or falsified). */
        std::vector<bool> eaEvaluated;
        /** \brief Stores for each external atom guess if it already was checked against the semantics and this check succeeded. */
        std::vector<bool> eaVerified;
        /** \brief The set of currently verified external atom auxiliaries. */
        InterpretationPtr verifiedAuxes;
        /** \brief Stores for each inner external atom the cumulative atoms which potentially changes since last evaluation. */
        std::vector<InterpretationPtr> changedAtomsPerExternalAtom;

        // heuristics
        /** \brief Heuristics to be used for evaluating external atoms for which no dedicated heuristics is provided. */
        ExternalAtomEvaluationHeuristicsPtr defaultExternalAtomEvalHeuristics;
        /** \brief Stores for each external atom its evaluation heuristics; is either defaultExternalAtomEvalHeuristics or a dedicated one. */
        std::vector<ExternalAtomEvaluationHeuristicsPtr> eaEvalHeuristics;
        /* \brief Heuristics to be used for unfounded set checking over partial assignments. */
        UnfoundedSetCheckHeuristicsPtr ufsCheckHeuristics;

        /** \brief EDB + original (input) interpretation plus auxiliary atoms for evaluated external atoms. */
        InterpretationConstPtr postprocessedInput;
        /** \brief Non-external fact input, i.e., postprocessedInput before evaluating outer eatoms. */
        InterpretationPtr mask;

        // internal solver
        /** \brief Grounder for nonground nogoods. */
        NogoodGrounderPtr nogoodGrounder;
        /** \brief All nogoods learned from EA evaluations. */
        SimpleNogoodContainerPtr learnedEANogoods;
        /** \brief Tree representation of GenuineGuessAndCheckModelGenerator::learnedEANogoods for verification purposes */
        ExternalAtomVerificationTree eavTree;
        /** \brief The highest index in learnedEANogoods which has already been transferred to the solver. */
        int learnedEANogoodsTransferredIndex;
        /** \brief Grounder instance. */
        GenuineGrounderPtr grounder;
        /** \brief Solver instance. */
        GenuineGroundSolverPtr solver;
        /** \brief Number of models of this model generate (only compatible and minimal ones). */
        int cmModelCount;
        /** \brief Set of atoms used for inconsistency analysis (only defined if inconsistency analysis is used). */      
        InterpretationPtr explAtoms;
        /** \brief Stores if an inconsistency cause has been identified. */
        bool haveInconsistencyCause;
        /** \brief Stores the inconsistency cause as a nogood if GenuineGuessAndCheckModelGenerator::haveInconsistencyCause is set to true. */
        Nogood inconsistencyCause;
        /** \brief Manager for unfounded set checking. */
        UnfoundedSetCheckerManagerPtr ufscm;
        /** \brief All atoms in the program. */
        InterpretationPtr programMask;

        // members

        /**
          * \brief Inlines selected external atoms which provide support sets.
          * @param program The input non-ground program whose external atoms are to be inlined (if possible).
          * @param grounder The grounder used to ground \p program.
          * @param annotatedGroundProgram The current annotations of \p program <em>before</em> the inlining, i.e., considering external atoms as external.
          * @param activeInnerEatoms After the call this vector contains the external atoms which were <em>not</em> inlined.
          * @param Parameters \p program, \p grounder, \p annotatedGroundProgram and \p activeInnerEatoms are updated. */
        void inlineExternalAtoms(OrdinaryASPProgram& program, GenuineGrounderPtr& grounder, AnnotatedGroundProgram& annotatedGroundProgram, std::vector<ID>& activeInnerEatoms);

        /**
          * \brief If the atom represented by \p atomID uses is an external auxiliary from \p eliminatedExtAuxes,
          * then 'r' is replaced by 'R' and 'n' by 'N'.
          * @param atomID The atom whose predicate is to be replaced.
          * @param eliminatedExtPreds The external predicates whose auxiliaries are to be replaced.
          * @return The ID of the new atom.
          */
        ID replacePredForInlinedEAs(ID atomID, InterpretationConstPtr eliminatedExtAuxes);

        /**
         * \brief Identifies the set of atoms used to explain inconsistencies in this unit.
         *
         * @param program The program whose EDB needs to be modified for inconsistency analysis (explanation atoms are removed as facts and assumed instead).
         */
        void initializeExplanationAtoms(OrdinaryASPProgram& program);

        /**
         * \brief Initializes heuristics for external atom evaluation and UFS checking over partial assignments.
         */
        void initializeHeuristics();

        /**
         * \brief Adds watches to all external auxilies for incremental verification and unverification of external atoms.
         */
        void initializeVerificationWatchLists();

        /**
         * \brief Learns related nonground nogoods.
         * @param ng The nogood to generalize.
         */
        void generalizeNogood(Nogood ng);

        /**
         * \brief Learns all support sets provided by external sources and adds them to supportSets
         */
        void learnSupportSets();

        /**
         * \brief Triggern nonground nogood learning and instantiation.
         *
         * @param compatibleSet Current compatible set.
         * @param assigned Set of currently assigned atoms or NULL to represent a complete assignment (optimization).
         * @param changed Set of atoms which possibly changes since the previous call (optimization)
         *
         * Transferes new nogoods from learnedEANogoods to the solver and updates learnedEANogoodsTransferredIndex accordingly.
         */
        void updateEANogoods(InterpretationConstPtr compatibleSet, InterpretationConstPtr assigned = InterpretationConstPtr(), InterpretationConstPtr changed = InterpretationConstPtr());

        /**
         * \brief Checks after completion of an assignment if it is compatible.
         *
         * Depending on the eaVerificationMode, the compatibility is either directly checked in this function,
         * of previously recorded verfication results are used to compute the return value.
         *
         * @param modelCandidate The model candidate to check for compatibility
         * @return True if compatibel and false otherwise.
         */
        bool finalCompatibilityCheck(InterpretationConstPtr modelCandidate);

        /**
         * \brief Checks if a compatible set is a model, i.e., it does the FLP check.
         *
         * The details depend on the selected semantics (well-justified FLP or FLP) and the selected algorithm (explicit or ufs-based).
         * Depending on the eaVerificationMode, the compatibility is either directly checked in this function,
         * of previously recorded verfication results are used to compute the return value.
         *
         * @param compatibleSet The model candidate to check for minimality.
         * @return True if \p compatibleSet is an answer set and false otherwise.
         */
        bool isModel(InterpretationConstPtr compatibleSet);

        /**
         * Makes an unfounded set check over a (possibly) partial interpretation if useful.
         * @param partialInterpretation The current assignment.
         * @param assigned Currently assigned atoms (can be 0 if partial=false).
         * @param changed The set of atoms with modified truth value since the last call (can be 0 if partial=false).
         * @param partial True if the assignment is (potentially) partial; in this case the check is driven by a heuristic.
         * @return bool True if the check is passed, i.e., if there is *no* unfounded set. False if the check is failed, i.e., there *is* an unfounded set.
         */
        bool unfoundedSetCheck(InterpretationConstPtr partialInterpretation, InterpretationConstPtr assigned = InterpretationConstPtr(), InterpretationConstPtr changed = InterpretationConstPtr(), bool partial = false);

        /**
         * Finds a new atom in the scope of an external atom which shall be watched wrt. an interpretation.
         * @pre Some atom in the scope of the external atom is yet unassigned.
         * @param eaIndex The index of the inner external atom.
         * @param search Search interpretation; can be 0 to indicate that all atoms of the EA's mask are eligable.
         * @param truthValue Indicates whether to search for a true or a false atom in search.
         * @return Address of an atom to watch or ALL_ONES if none exists.
         */
        IDAddress getWatchedLiteral(int eaIndex, InterpretationConstPtr search, bool truthValue);

        /**
         * Removes verification results for external atoms if relevant parts of the input have changed.
         * @param changed The set of atoms with modified truth value since the last call.
         */
        void unverifyExternalAtoms(InterpretationConstPtr changed);

        /**
         * Heuristically decides if and which external atoms we evaluate.
         * @param partialInterpretation The current assignment.
         * @param assigned Currently assigned atoms.
         * @param changed The set of atoms with modified truth value since the last call.
         * @return bool True if evaluation yielded a conflict, otherwise false.
         */
        bool verifyExternalAtoms(InterpretationConstPtr partialInterpretation, InterpretationConstPtr assigned, InterpretationConstPtr changed);

        /**
         * Evaluates the inner external atom with index eaIndex (if possible, i.e., if the input is complete).
         * Learns nogoods if external learning is activated.
         * Sets eaVerified and eaEvaluated if eaVerificationMode == mixed.
         * @param eaIndex The index of the external atom to verify.
         * @param partialInterpretation The current assignment.
         * @param assigned Currently assigned atoms (if 0, then the assignment is assumed to be complete).
         * @param changed The set of atoms with modified truth value since the last call (if 0, then all atoms are assumed to have changed).
         * @param answeredFromCacheOrSupportSets Optional pointer to a boolean where it is to be stored whether the query was answered from cache or support sets (true) or the external source was actually called (false).
         * @return bool True if the assignment is conflicting wrt. this external atom, otherwise false.
         */
        bool verifyExternalAtom(int eaIndex, InterpretationConstPtr partialInterpretation,
            InterpretationConstPtr assigned = InterpretationConstPtr(),
            InterpretationConstPtr changed = InterpretationConstPtr(),
            bool* answeredFromCacheOrSupportSets = 0);
        /**
         * Evaluates the inner external atom with index eaIndex (if possible, i.e., if the input is complete) using explicit evaluation.
         * Learns nogoods if external learning is activated.
         * Sets eaVerified and eaEvaluated if eaVerificationMode == mixed.
         * @param eaIndex The index of the external atom to verify.
         * @param partialInterpretation The current assignment.
         * @param assigned Currently assigned atoms (if 0, then the assignment is assumed to be complete).
         * @param changed The set of atoms with modified truth value since the last call (if 0, then all atoms are assumed to have changed).
         * @param answeredFromCache Optional pointer to a boolean where it is to be stored whether the query was answered from cache (true) or the external source was actually called (false).
         * @return bool True if the assignment is conflicting wrt. this external atom, otherwise false.
         */
        bool verifyExternalAtomByEvaluation(int eaIndex, InterpretationConstPtr partialInterpretation,
            InterpretationConstPtr assigned = InterpretationConstPtr(),
            InterpretationConstPtr changed = InterpretationConstPtr(),
            bool* answeredFromCache = 0);

        /**
         * Evaluates the inner external atom with index eaIndex (if possible, i.e., if the input is complete) using complete support sets.
         * Learns nogoods if external learning is activated.
         * Sets eaVerified and eaEvaluated if eaVerificationMode == mixed.
         * @param eaIndex The index of the external atom to verify.
         * @param partialInterpretation The current assignment.
         * @param assigned Currently assigned atoms (if 0, then the assignment is assumed to be complete).
         * @param changed The set of atoms with modified truth value since the last call (if 0, then all atoms are assumed to have changed).
         * @return bool True if the assignment is conflicting wrt. this external atom, otherwise false.
         */
        bool verifyExternalAtomBySupportSets(int eaIndex, InterpretationConstPtr partialInterpretation,
            InterpretationConstPtr assigned = InterpretationConstPtr(),
            InterpretationConstPtr changed = InterpretationConstPtr());

        /**
         * Returns the ground program in this component
         * @return const std::vector<ID>& Reference to the ground program in this component
         */
        const OrdinaryASPProgram& getGroundProgram();

        /**
         * \brief Is called by the ASP solver in its propagation method to trigger fruther learning methods.
         *
         * This function can add additional (learned) nogoods to the solver to force implications or tell the solver that the current assignment is conflicting.
         *
         * @param partialInterpretation The current assignment.
         * @param assigned Currently assigned atoms.
         * @param changed The set of atoms with modified truth value since the last call.
         */
        void propagate(InterpretationConstPtr partialInterpretation, InterpretationConstPtr assigned, InterpretationConstPtr changed);

    public:
        /**
         * \brief Constructor.
         * @param factory Reference to the factory which created this model generator.
         * @param input Input interpretation to this model generator.
         */
        GenuineGuessAndCheckModelGenerator(Factory& factory, InterpretationConstPtr input);

        /**
         * \brief Destuctor.
         */
        virtual ~GenuineGuessAndCheckModelGenerator();

        // generate and return next model, return null after last model
        virtual InterpretationPtr generateNextModel();

        /** \brief Identifies a reason for an inconsistency in this unit.
         *
         * The method may only be called after generateNextModel() has returned NULL after first call. */
        void identifyInconsistencyCause();

        // handling inconsistencies propagated over multiple units
        virtual const Nogood* getInconsistencyCause();
        virtual void addNogood(const Nogood* cause);
};

/** \brief Factory for the GenuineGuessAndCheckModelGenerator. */
class GenuineGuessAndCheckModelGeneratorFactory:
public FLPModelGeneratorFactoryBase,
public ostream_printable<GenuineGuessAndCheckModelGeneratorFactory>
{
    // types
    public:
        friend class GenuineGuessAndCheckModelGenerator;
        typedef ComponentGraph::ComponentInfo ComponentInfo;

        // storage
    protected:
        /** \brief Defines the solver to be used for external evaluation. */
        ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig;
        /** \brief ProgramCtx. */
        ProgramCtx& ctx;
        /** ComponentInfo of the component to be solved by the model generators instantiated by this factory. */
        ComponentInfo ci;        // should be a reference, but there is currently a bug in the copy constructor of ComponentGraph: it seems that the component info is shared between different copies of a component graph, hence it is deallocated when one of the copies dies.
        WARNING("TODO see comment above about ComponentInfo copy construction bug")

        /** \brief Outer external atoms of the component. */
        std::vector<ID> outerEatoms;

        /** \brief Nogoods learned from successor units. */
        std::vector<std::pair<Nogood, int> > succNogoods;
    public:
        /** \brief Constructor.
         *
         * @param ctx See GenuineGuessAndCheckModelGeneratorFactory::ctx.
         * @param ci See GenuineGuessAndCheckModelGeneratorFactory::ci.
         * @param externalEvalConfig See GenuineGuessAndCheckModelGeneratorFactory::externalEvalConfig.
         */
        GenuineGuessAndCheckModelGeneratorFactory(
            ProgramCtx& ctx, const ComponentInfo& ci,
            ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig);

        /** \brief Destructor. */
        virtual ~GenuineGuessAndCheckModelGeneratorFactory() { }

        // add inconsistency explanation nogoods from successor components
        virtual void addInconsistencyCauseFromSuccessor(const Nogood* cause);

        /** \brief Instantiates a model generator for the component.
         * @param input The facts to be added before solving.
         * @return Model generator. */
        virtual ModelGeneratorPtr createModelGenerator(InterpretationConstPtr input);

        /** \brief Prints information about the model generator for debugging purposes.
         * @param o Stream to print to. */
        virtual std::ostream& print(std::ostream& o) const;
        /** \brief Prints information about the model generator for debugging purposes.
         * @param o Stream to print to.
         * @param verbose Provides more detailed output if true. */
        virtual std::ostream& print(std::ostream& o, bool verbose) const;
};

DLVHEX_NAMESPACE_END
#endif                           // GUESSANDCHECK_MODEL_GENERATOR_HPP_INCLUDED__09112010

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
