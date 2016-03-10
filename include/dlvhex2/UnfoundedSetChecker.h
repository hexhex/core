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
 * @file   UnfoundedSetChecker.h
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 *
 * @brief  Unfounded set checker for programs with disjunctions and external atoms.
 */

#ifndef UNFOUNDEDSETCHECKER_H__
#define UNFOUNDEDSETCHECKER_H__

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/BaseModelGenerator.h"
#include "dlvhex2/AnnotatedGroundProgram.h"
#include "dlvhex2/ExternalAtomVerificationTree.h"

#include <boost/unordered_map.hpp>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
 * \brief Unfounded set checker for HEX programs (with external atoms).
 */
class UnfoundedSetChecker
{
    protected:
        /** \brief Reference to the model generator which shall be used for evaluating external atoms. Can be NULL if the UFS checker runs in ordinary mode. */
        BaseModelGenerator* mg;

        enum Mode
        {
            /** \brief Consider external atoms as ordinary ones. */
            Ordinary,
            /** \brief Consider external atoms as such (requires ggncmg to be set). */
            WithExt
        };
        /** \brief Defines the mode of the UFS checker (ordinary or WithExt). */
        Mode mode;

        /** \brief Program context. */
        ProgramCtx& ctx;
        /** \brief Registry. */
        RegistryPtr reg;

        // problem specification
        /** \brief The ground program for which the UFS checker is used. */
        const OrdinaryASPProgram& groundProgram;
        /** \brief Empty AnnotatedGroundProgram. */
        AnnotatedGroundProgram emptyagp;
        /** \brief UnfoundedSetChecker::groundProgram with additional meta information. */
        const AnnotatedGroundProgram& agp;
        /** \brief Set of all atoms in the component of the UFS checker. */
        InterpretationConstPtr componentAtoms;
        /** \brief Tree representation of GenuineGuessAndCheckModelGenerator::learnedEANogoods for verification purposes */
        ExternalAtomVerificationTree eavTree;
        /** Set of nogoods to be learned during UFS detection. */
        SimpleNogoodContainerPtr ngc;
        /** \brief Domain of all problem variables. */
        InterpretationPtr domain;

        /**
         * \brief Satisfiability solver for evaluating the UFS detection problem.
         *
         * AssumptionBasedUnfoundedSetChecker, solver is defined during the whole lifetime of the object.
         * An EncodingBasedUnfoundedSetChecker, solver is only defined while getUnfoundedSet runs.
         */
        SATSolverPtr solver;

        /**
         * \brief Checks if an UFS candidate is actually an unfounded set.
         * @param compatibleSet The interpretation over which we compute UFSs.
         * @param compatibleSetWithoutAux The interpretation over which we compute UFSs (without EA replacements).
         * @param ufsCandidate A candidate unfounded set (solution to the nogood set created by getUFSDetectionProblem).
         * @return True if ufsCandidate is an unfounded set and false otherwise.
         */
        bool isUnfoundedSet(InterpretationConstPtr compatibleSet, InterpretationConstPtr compatibleSetWithoutAux, InterpretationConstPtr ufsCandidate);

        /**
         * Transforms a nogood (valid input-output relationship of some external atom) learned in the main search for being used in the UFS search.
         * @param ng The nogood from the main search.
         * @param compatibleSet The current compatible set we do the unfounded set search with respect to.
         *                      Note: For AssumptionBasedUnfoundedSetChecker it is essential that the nogood transformation is independent of the compatible set.
         * @return A pair of a boolean, which denotes if the transformation yielded a nogood, an the transformed nogood in this case.
         */
        virtual std::pair<bool, Nogood> nogoodTransformation(Nogood ng, InterpretationConstPtr compatibleSet) = 0;

    private:

        /**
         * Defines data structures used for verification of UFS candidates.
         */
        struct UnfoundedSetVerificationStatus
        {
            /** \brief Input used for external atom evaluation. */
            InterpretationPtr eaInput;

            /**
             * \brief The auxiliaries which's new truth value needs to be checked.
             *
             * For each auxiliary A with address adr there is a unique index i such that auxiliariesToVerify[i]=adr.
             */
            std::vector<IDAddress> auxiliariesToVerify;

            /**
             * \brief Stores for each auxiliary A with index i (see above) the external atoms auxiliaryDependsOnEA[i] which are remain to be evaluated before the truth/falsity of A is certain.
             *
             * Note: since it needs to store the external atoms which *remain to be verified*, we cannot use the features of AnnotatedGroundProgram).
             */
            std::vector<std::set<ID> > auxIndexToRemainingExternalAtoms;

            /**
             * Stores for each external atom with address adr the indices eaToAuxIndex[adr] in the vector auxiliaryDependsOnEA which depend on this external atom.
             *
             * Note: since we need only certain auxiliaries, we cannot use the features of AnnotatedGroundProgram).
             */
            std::vector<std::vector<int> > externalAtomAddressToAuxIndices;

            /**
             * \brief Prepares data structures used for verification of an unfounded set candidate wrt. a compatible set.
             * @param agp The program over which the UFS check is done.
             * @param domain Domain of this unfounded set check.
             * @param ufsCandidate Representation of the UFS candidate.
             * @param compatibleSet Compatible set.
             */
            UnfoundedSetVerificationStatus(
                const AnnotatedGroundProgram& agp,
                InterpretationConstPtr domain, InterpretationConstPtr ufsCandidate, InterpretationConstPtr compatibleSet, InterpretationConstPtr compatibleSetWithoutAux);
        };

        /**
         * \brief Explicitly evaluates an external atom and verifies or falsifies the auxiliaries which depend on it.
         * @param ufsCandidate Representation of the UFS candidate.
         * @param compatibleSet Compatible Set.
         * @param ufsVerStatus Represents the current verification status (to be prepared by indexEatomsForUfsVerification).
         * @return True if verification succeeded and false otherwise.
         */
        bool verifyExternalAtomByEvaluation(
            ID eaID,
            InterpretationConstPtr ufsCandidate, InterpretationConstPtr compatibleSet,
            UnfoundedSetVerificationStatus& ufsVerStatus);

    public:
        /**
         * \brief Initialization for UFS search considering external atoms as ordinary ones.
         * @param groundProgram Ground program over which the ufs check is done.
         * @param componentAtoms The atoms in the strongly connected component in the atom dependency graph; if NULL, then all atoms in groundProgram are considered to be in the SCC.
         * @param ngc Set of valid input-output relationships learned in the main search (to be extended by this UFS checker).
         */
        UnfoundedSetChecker(
            ProgramCtx& ctx,
            const OrdinaryASPProgram& groundProgram,
            InterpretationConstPtr componentAtoms = InterpretationConstPtr(),
            SimpleNogoodContainerPtr ngc = SimpleNogoodContainerPtr());

        /**
         * \brief Initialization for UFS search under consideration of the semantics of external atoms.
         * @param ggncmg Reference to the G&C model generator for which this UnfoundedSetChecker runs.
         * @param ctx ProgramCtx.
         * @param groundProgram Ground program over which the ufs check is done.
         * @param agp Annotated version of the ground program; may be a superset of groundProgram, but must contain meta information about all external atoms in groundProgram.
         * @param componentAtoms The atoms in the strongly connected component in the atom dependency graph; if 0, then all atoms in groundProgram are considered to be in the SCC.
         * @param ngc Set of valid input-output relationships learned in the main search (to be extended by this UFS checker).
         */
        UnfoundedSetChecker(
            BaseModelGenerator* mg,
            ProgramCtx& ctx,
            const OrdinaryASPProgram& groundProgram,
            const AnnotatedGroundProgram& agp,
            InterpretationConstPtr componentAtoms = InterpretationConstPtr(),
            SimpleNogoodContainerPtr ngc = SimpleNogoodContainerPtr());

        virtual ~UnfoundedSetChecker() {}

        /**
         * \brief Returns an unfounded set of groundProgram with respect to a compatibleSet;
         * If the empty set is returned,
         * then there does not exist a greater (nonempty) unfounded set.
         *
         * The method supports also unfounded set detection over partial interpretations.
         * For this purpose, skipProgram specifies all rules which shall be ignored
         * in the search. The interpretation must be complete and compatible over the non-ignored part.
         * Each detected unfounded set will remain an unfounded set for all possible
         * completions of the interpretation.
         *
         * @param compatibleSet The interpretation for which we want to compute an unfounded set.
         * @param skipProgram The set of rules which shall be ignored in the UFS check (because the assignment might be incomplete wrt. these rules).
         * @return An unfounded set (might be of size 0).
         */
        virtual std::vector<IDAddress> getUnfoundedSet(InterpretationConstPtr compatibleSet, const std::set<ID>& skipProgram) = 0;

        /**
         * Forces the unfounded set checker to learn nogoods from main search now
         */
        virtual void learnNogoodsFromMainSearch(bool reset) = 0;

        /**
         * Constructs a nogood which encodes the essence of an unfounded set using one of the overloaded versions of the method.
         * @param ufs The unfounded set to construct the nogood for.
         * @param interpretation The interpretation which was used to compute the unfounded set for.
         * @return The UFS-nogood.
         */
        Nogood getUFSNogood(
            const std::vector<IDAddress>& ufs,
            InterpretationConstPtr interpretation);

        /**
         * Constructs a nogood which encodes the essence of an unfounded set based on the reduct.
         * @param ufs The unfounded set to construct the nogood for.
         * @param interpretation The interpretation which was used to compute the unfounded set for.
         * @return The UFS-nogood.
         */
        Nogood getUFSNogoodReductBased(
            const std::vector<IDAddress>& ufs,
            InterpretationConstPtr interpretation);

        /**
         * Constructs a nogood which encodes the essence of an unfounded set based on the UFS itself.
         * @param ufs The unfounded set to construct the nogood for.
         * @param interpretation The interpretation which was used to compute the unfounded set for.
         * @return The UFS-nogood.
         */
        Nogood getUFSNogoodUFSBased(
            const std::vector<IDAddress>& ufs,
            InterpretationConstPtr interpretation);

        typedef boost::shared_ptr<UnfoundedSetChecker> Ptr;
        typedef boost::shared_ptr<const UnfoundedSetChecker> ConstPtr;
};

typedef UnfoundedSetChecker::Ptr UnfoundedSetCheckerPtr;
typedef UnfoundedSetChecker::ConstPtr UnfoundedSetCheckerConstPtr;

class EncodingBasedUnfoundedSetChecker : public UnfoundedSetChecker
{
    private:
        /**
         * \brief Constructs the nogood set used for unfounded set detection.
         *
         * The construction depends on the interpretation (encoding-based UFS detection).
         * The constructed UFS detection problem is written to ufsDetectionProblem
         *
         * @param compatibleSet The compatible set to create the UFS check for.
         * @param compatibleSetWithoutAux The compatible set without external atom auxiliaries.
         * @param skipProgram The set of rules considered in the UFS search.
         * @param ufsProgram the set of rules in the program but ignored in the UFS search.
         */
        void constructUFSDetectionProblem(
            NogoodSet& ufsDetectionProblem,
            InterpretationConstPtr compatibleSet,
            InterpretationConstPtr compatibleSetWithoutAux,
            const std::set<ID>& skipProgram,
            std::vector<ID>& ufsProgram);

        /**
         * \brief Constructs the necessary part of the nogood set used for unfounded set detection.
         *
         * The construction depends on the interpretation (encoding-based UFS detection).
         * The constructed UFS detection problem is written to ufsDetectionProblem
         *
         * @param compatibleSet The compatible set to create the UFS check for.
         * @param compatibleSetWithoutAux The compatible set without external atom auxiliaries.
         * @param skipProgram The set of rules considered in the UFS search.
         * @param ufsProgram the set of rules in the program but ignored in the UFS search.
         */
        void constructUFSDetectionProblemNecessaryPart(
            NogoodSet& ufsDetectionProblem,
            int& auxatomcnt,
            InterpretationConstPtr compatibleSet,
            InterpretationConstPtr compatibleSetWithoutAux,
            const std::set<ID>& skipProgram,
            std::vector<ID>& ufsProgram);

        /**
         * \brief Constructs the optional optimization part of the nogood set used for unfounded set detection.
         *
         * The construction depends on the interpretation (encoding-based UFS detection).
         * The constructed UFS detection problem is written to ufsDetectionProblem
         *
         * @param compatibleSet The compatible set to create the UFS check for.
         * @param compatibleSetWithoutAux The compatible set without external atom auxiliaries.
         * @param skipProgram The set of rules considered in the UFS search.
         * @param ufsProgram the set of rules in the program but ignored in the UFS search.
         */
        void constructUFSDetectionProblemOptimizationPart(
            NogoodSet& ufsDetectionProblem,
            int& auxatomcnt,
            InterpretationConstPtr compatibleSet,
            InterpretationConstPtr compatibleSetWithoutAux,
            const std::set<ID>& skipProgram,
            std::vector<ID>& ufsProgram);

        /**
         * \brief Constructs the optional optimization part of the nogood set used for unfounded set detection
         *        such that the search is restricted to atoms which are true in the compatible set.
         *
         * The construction depends on the interpretation (encoding-based UFS detection).
         * The constructed UFS detection problem is written to ufsDetectionProblem
         *
         * @param compatibleSet The compatible set to create the UFS check for.
         * @param compatibleSetWithoutAux The compatible set without external atom auxiliaries.
         * @param skipProgram The set of rules considered in the UFS search.
         * @param ufsProgram the set of rules in the program but ignored in the UFS search.
         */
        void constructUFSDetectionProblemOptimizationPartRestrictToCompatibleSet(
            NogoodSet& ufsDetectionProblem,
            int& auxatomcnt,
            InterpretationConstPtr compatibleSet,
            InterpretationConstPtr compatibleSetWithoutAux,
            const std::set<ID>& skipProgram,
            std::vector<ID>& ufsProgram);

        /**
         * \brief Constructs the optional optimization part of the nogood set used for unfounded set detection,
         *        exploiting the fact that the truth value of external atoms cannot change if no input atom is unfounded.
         *
         * The construction depends on the interpretation (encoding-based UFS detection).
         * The constructed UFS detection problem is written to ufsDetectionProblem
         *
         * @param compatibleSet The compatible set to create the UFS check for.
         * @param compatibleSetWithoutAux The compatible set without external atom auxiliaries.
         * @param skipProgram The set of rules considered in the UFS search.
         * @param ufsProgram the set of rules in the program but ignored in the UFS search.
         */
        void constructUFSDetectionProblemOptimizationPartBasicEAKnowledge(
            NogoodSet& ufsDetectionProblem,
            int& auxatomcnt,
            InterpretationConstPtr compatibleSet,
            InterpretationConstPtr compatibleSetWithoutAux,
            const std::set<ID>& skipProgram,
            std::vector<ID>& ufsProgram);

        /**
         * \brief Constructs the optional optimization part of the nogood set used for unfounded set detection
         *        using learned nogoods from the main search.
         *
         * The construction depends on the interpretation (encoding-based UFS detection).
         * The constructed UFS detection problem is written to ufsDetectionProblem
         *
         * @param compatibleSet The compatible set to create the UFS check for.
         * @param compatibleSetWithoutAux The compatible set without external atom auxiliaries.
         * @param skipProgram The set of rules considered in the UFS search.
         * @param ufsProgram the set of rules in the program but ignored in the UFS search.
         */
        void constructUFSDetectionProblemOptimizationPartLearnedFromMainSearch(
            NogoodSet& ufsDetectionProblem,
            int& auxatomcnt,
            InterpretationConstPtr compatibleSet,
            InterpretationConstPtr compatibleSetWithoutAux,
            const std::set<ID>& skipProgram,
            std::vector<ID>& ufsProgram);

        /**
         * \brief Constructs the optional optimization part of the nogood set used for unfounded set detection
         *        which tries to keep the truth values of external atoms unchanged.
         *
         * The construction depends on the interpretation (encoding-based UFS detection).
         * The constructed UFS detection problem is written to ufsDetectionProblem
         *
         * @param compatibleSet The compatible set to create the UFS check for.
         * @param compatibleSetWithoutAux The compatible set without external atom auxiliaries.
         * @param skipProgram The set of rules considered in the UFS search.
         * @param ufsProgram the set of rules in the program but ignored in the UFS search.
         */
        void constructUFSDetectionProblemOptimizationPartEAEnforement(
            NogoodSet& ufsDetectionProblem,
            int& auxatomcnt,
            InterpretationConstPtr compatibleSet,
            InterpretationConstPtr compatibleSetWithoutAux,
            const std::set<ID>& skipProgram,
            std::vector<ID>& ufsProgram);

        std::pair<bool, Nogood> nogoodTransformation(Nogood ng, InterpretationConstPtr compatibleSet);

    public:
        /**
         * \brief Initializes the UFS checker without support for external atoms (they are considered as ordinary ones).
         * @param ctx ProgramCtx
         * @param groundProgram Ground program used for UFS checking.
         * @param componentAtoms Atoms in the component the UFS checker is initialized for.
         * @param ngc Pointer to a container with valid input-output relationships (EANogoods)
         */
        EncodingBasedUnfoundedSetChecker(
            ProgramCtx& ctx,
            const OrdinaryASPProgram& groundProgram,
            InterpretationConstPtr componentAtoms = InterpretationConstPtr(),
            SimpleNogoodContainerPtr ngc = SimpleNogoodContainerPtr());

        /**
         * \brief Initializes the UFS checker with support for external atoms.
         * @param mg Reference to a model generator (used to evaluate the external atoms).
         * @param ctx ProgramCtx.
         * @param groundProgram Ground program used for UFS checking.
         * @param agp Ground program with meta information used for optimized UFS checking.
         * @param choiceRuleCompatible This parameter is necessary for the clasp backend, which implements non-head cycle free disjunctive rules using choice rules.
         *                             However, this transformation must be regarded in the optimization of UFS checking. More specifically, the UFS check MUST NOT BE SKIPPED
         *                             for HFC-free components if they contain such choice rules. For more information, see examples/trickyufs.hex.
         * @param ngc Pointer to a container with valid input-output relationships (EANogoods)
         */
        EncodingBasedUnfoundedSetChecker(
            BaseModelGenerator& mg,
            ProgramCtx& ctx,
            const OrdinaryASPProgram& groundProgram,
            const AnnotatedGroundProgram& agp,
            InterpretationConstPtr componentAtoms = InterpretationConstPtr(),
            SimpleNogoodContainerPtr ngc = SimpleNogoodContainerPtr());

        /**
         * \brief Forces all unfounded set checker in this manager to learn nogoods from main search now.
         * @param reset Specifies if the nogood container from the main search shall be scanned from the beginning (otherwise only nogoods added at the back will be recognized).
         */
        void learnNogoodsFromMainSearch(bool reset);

        /**
         * \brief Transforms a nogood from the main search to the UFS search.
         * @param ng Nogodo to transform.
         * @param compatibleSet Compatible set for which the UFS check shall be performed.
         * @return A set of nogoods which can be used in the unfounded set search.
         */
        std::vector<IDAddress> getUnfoundedSet(
            InterpretationConstPtr compatibleSet,
            const std::set<ID>& skipProgram);
};

class AssumptionBasedUnfoundedSetChecker : public UnfoundedSetChecker
{
    private:
        /** \brief A special atom "a_i" for each atom "a" in the program, representing the truth value of "a" in the compatible set. */
        boost::unordered_map<IDAddress, IDAddress> interpretationShadow;
        /** \brief A special atom "a_j" for each atom "a" in the program, representing the truth value of "a" in I u -X. */
        boost::unordered_map<IDAddress, IDAddress> residualShadow;
        /** \brief A special atom "a_f" for each atom "a" in the program, representing a change from of the truth value of a from true in I to false in I u -X. */
        boost::unordered_map<IDAddress, IDAddress> becomeFalse;
        /** \brief A special atom "a_{IandU}" for each atom "a" in the program, representing that a is true in I and member of U. */
        boost::unordered_map<IDAddress, IDAddress> IandU;
        /** \brief A special atom "a_{\overline{I}orU}" for each atom "a" in the program, representing that a is false in I or member of U. */
        boost::unordered_map<IDAddress, IDAddress> nIorU;

        /** \brief Counter for auxiliary atoms. */
        int atomcnt;

        /** \brief Number of program rules respected in the encoding (allows for incremental addition of further rules). */
        int problemRuleCount;

        /** \brief Allows for extension of the problem encoding when additional rules are added. */
        ID hookAtom;

        // stores how many nogoods in ngc we have already transformed and learned in the UFS search
        int learnedNogoodsFromMainSearch;

        /** \brief Goes through EDB and IDB and sets all facts in domain. */
        void constructDomain();

        /**
         * \brief Encodes that facts cannot be in the unfounded set.
         */
        void constructUFSDetectionProblemFacts(NogoodSet& ns);

        /**
         * \brief Sets up interpretationShadow and residualShadow.
         */
        void constructUFSDetectionProblemCreateAuxAtoms();

        /**
         * \brief Defines the auxiliary variables.
         * @param ns The nogood set to add the nogodos to.
         */
        void constructUFSDetectionProblemDefineAuxiliaries(NogoodSet& ns);

        /**
         * \brief Encodes a given program rule.
         * @param ns The nogood set to add the nogodos to.
         * @param ruleID Rule to encode.
         */
        void constructUFSDetectionProblemRule(NogoodSet& ns, ID ruleID);

        /**
         * \brief Encodes that we are looking for a nonempty unfounded set.
         * @param ns The nogood set to add the nogodos to.
         */
        void constructUFSDetectionProblemNonempty(NogoodSet& ns);

        /**
         * \brief Restricts the search to the current strongly connected component.
         * @param ns The nogood set to add the nogodos to.
         */
        void constructUFSDetectionProblemRestrictToSCC(NogoodSet& ns);

        /**
         * \brief Optimization: Basic behavior of external atoms.
         * @param ns The nogood set to add the nogodos to.
         */
        void constructUFSDetectionProblemBasicEABehavior(NogoodSet& ns);

        /**
         * \brief Constructs the nogood set used for unfounded set detection and instantiates the solver
         */
        void constructUFSDetectionProblemAndInstantiateSolver();

        /**
         * \brief Extends the nogood set used for unfounded set detection and reinstantiates the solver
         */
        void expandUFSDetectionProblemAndReinstantiateSolver();

        /**
         * \brief Prepares the list of assumptions for an unfounded set search over a given compatible set
         * @param compatibleSet The compatible set over which we do the UFS search
         * @param skipProgram The set of rules ignored in the UFS check
         */
        void setAssumptions(InterpretationConstPtr compatibleSet, const std::set<ID>& skipProgram);

        std::pair<bool, Nogood> nogoodTransformation(Nogood ng, InterpretationConstPtr compatibleSet);

    public:
        /**
         * \brief Initializes the UFS checker without support for external atoms (they are considered as ordinary ones).
         * @param ctx ProgramCtx
         * @param groundProgram Ground program used for UFS checking.
         * @param componentAtoms Atoms in the component the UFS checker is initialized for.
         * @param ngc Pointer to a container with valid input-output relationships (EANogoods).
         */
        AssumptionBasedUnfoundedSetChecker(
            ProgramCtx& ctx,
            const OrdinaryASPProgram& groundProgram,
            InterpretationConstPtr componentAtoms = InterpretationConstPtr(),
            SimpleNogoodContainerPtr ngc = SimpleNogoodContainerPtr());

        /**
         * \brief Initializes the UFS checker with support for external atoms.
         * @param mg Reference to a model generator (used to evaluate the external atoms).
         * @param ctx ProgramCtx.
         * @param groundProgram Ground program used for UFS checking.
         * @param agp Ground program with meta information used for optimized UFS checking.
         * @param choiceRuleCompatible This parameter is necessary for the clasp backend, which implements non-head cycle free disjunctive rules using choice rules.
         *                             However, this transformation must be regarded in the optimization of UFS checking. More specifically, the UFS check MUST NOT BE SKIPPED
         *                             for HFC-free components if they contain such choice rules. For more information, see examples/trickyufs.hex.
         * @param ngc Pointer to a container with valid input-output relationships (EANogoods).
         */
        AssumptionBasedUnfoundedSetChecker(
            BaseModelGenerator& mg,
            ProgramCtx& ctx,
            const OrdinaryASPProgram& groundProgram,
            const AnnotatedGroundProgram& agp,
            InterpretationConstPtr componentAtoms = InterpretationConstPtr(),
            SimpleNogoodContainerPtr ngc = SimpleNogoodContainerPtr());

        void learnNogoodsFromMainSearch(bool reset);

        /**
         * \brief Transforms a nogood from the main search to the UFS search.
         * @param ng Nogodo to transform.
         * @param compatibleSet Compatible set for which the UFS check shall be performed.
         * @return A set of nogoods which can be used in the unfounded set search.
         */
        std::vector<IDAddress> getUnfoundedSet(InterpretationConstPtr compatibleSet, const std::set<ID>& skipProgram);
};

/**
 * \brief Creates independent unfounded set checkers for all components of a program and automatically calls them appropriately.
 *
 * Depending on the settings, the class keeps one UFS checker for the program or a separate one for all components.
 * During UFS checker, the single components are checked until a UFS is found. The class further exploits decision criteria
 * which allows for skipping the UFS check for the overall program or single components.
 */
class DLVHEX_EXPORT UnfoundedSetCheckerManager
{
    private:
        /** \brief Program context. */
        ProgramCtx& ctx;

        /** \brief Reference to the model generator which shall be used for evaluating external atoms. Can be NULL if the UFS checker runs in ordinary mode. */
        BaseModelGenerator* mg;
        /** \brief UnfoundedSetChecker::groundProgram with additional meta information. */
        const AnnotatedGroundProgram& agp;
        /** \brief Used for detecting extensions of the AnnotatedGroundProgram. */
        int lastAGPComponentCount;
        /** \brief Temporary storage for the UFS nogood of the last detected unfounded set. */
        Nogood ufsnogood;
        /** Pointer to a container with valid input-output relationships (EANogoods). */
        SimpleNogoodContainerPtr ngc;

        /** Unfounded set checkers for all components. */
        std::map<int, UnfoundedSetCheckerPtr> preparedUnfoundedSetCheckers;

        /** \brief Stores for each component if it intersects with non-head-cycle-free rules. */
        std::vector<bool> intersectsWithNonHCFDisjunctiveRules;

        /**
         * \brief Stores if the UFS checker should reduce optimization such that an implementation of non-HCF rules via choice rules is possible.
         *
         * This parameter is necessary for the clasp backend, which implements non-head cycle free disjunctive rules using choice rules.
         * However, this transformation must be regarded in the optimization of UFS checking. More specifically, the UFS check MUST NOT BE SKIPPED
         * for HFC-free components if they contain such choice rules. For more information, see examples/trickyufs.hex.
         */
        bool choiceRuleCompatible;

        /**
         * \brief Computes for all components if they intersect with non-HCF rules and stores the results in UnfoundedSetCheckerManager::intersectsWithNonHCFDisjunctiveRules.
         * @param choiceRuleCompatible See UnfoundedSetCheckerManager::choiceRuleCompatible.
         */
        void computeChoiceRuleCompatibility(bool choiceRuleCompatible);

        /**
         * \brief Computes for a given components if it intersects with non-HCF rules and stores the results in UnfoundedSetCheckerManager::intersectsWithNonHCFDisjunctiveRules.
         * @param choiceRuleCompatible See UnfoundedSetCheckerManager::choiceRuleCompatible.
         * @param comp Component index.
         */
        void computeChoiceRuleCompatibilityForComponent(bool choiceRuleCompatible, int comp);

        /**
         * \brief Initializes the UFS checker without support for external atoms (they are considered as ordinary ones).
         * @param ctx ProgramCtx
         * @param groundProgram Ground program used for UFS checking.
         * @param componentAtoms Atoms in the component the UFS checker is initialized for.
         * @param ngc Pointer to a container with valid input-output relationships (EANogoods)
         */
        UnfoundedSetCheckerPtr instantiateUnfoundedSetChecker(
            ProgramCtx& ctx,
            const OrdinaryASPProgram& groundProgram,
            InterpretationConstPtr componentAtoms = InterpretationConstPtr(),
            SimpleNogoodContainerPtr ngc = SimpleNogoodContainerPtr());

        /**
         * \brief Initializes the UFS checker with support for external atoms.
         * @param mg Reference to a model generator (used to evaluate the external atoms).
         * @param ctx ProgramCtx.
         * @param groundProgram Ground program used for UFS checking.
         * @param agp Ground program with meta information used for optimized UFS checking.
         * @param choiceRuleCompatible This parameter is necessary for the clasp backend, which implements non-head cycle free disjunctive rules using choice rules.
         *                             However, this transformation must be regarded in the optimization of UFS checking. More specifically, the UFS check MUST NOT BE SKIPPED
         *                             for HFC-free components if they contain such choice rules. For more information, see examples/trickyufs.hex.
         * @param ngc Pointer to a container with valid input-output relationships (EANogoods)
         */
        UnfoundedSetCheckerPtr instantiateUnfoundedSetChecker(
            BaseModelGenerator& mg,
            ProgramCtx& ctx,
            const OrdinaryASPProgram& groundProgram,
            const AnnotatedGroundProgram& agp,
            InterpretationConstPtr componentAtoms = InterpretationConstPtr(),
            SimpleNogoodContainerPtr ngc = SimpleNogoodContainerPtr());

    public:
        /**
         * \brief Initializes the UFS checker with support for external atoms.
         * @param mg Reference to a model generator (used to evaluate the external atoms)
         * @param ctx ProgramCtx
         * @param agp Ground program with meta information used for optimized UFS checking
         * @param choiceRuleCompatible This parameter is necessary for the clasp backend, which implements non-head cycle free disjunctive rules using choice rules.
         *                             However, this transformation must be regarded in the optimization of UFS checking. More specifically, the UFS check MUST NOT BE SKIPPED
         *                             for HFC-free components if they contain such choice rules. For more information, see examples/trickyufs.hex.
         * @param ngc Pointer to a container with valid input-output relationships (EANogoods)
         */
        UnfoundedSetCheckerManager(
            BaseModelGenerator& mg,
            ProgramCtx& ctx,
            const AnnotatedGroundProgram& agp,
            bool choiceRuleCompatible = false,
            SimpleNogoodContainerPtr ngc = SimpleNogoodContainerPtr());

        /**
         * \brief Initializes the UFS checker without support for external atoms (they are considered as ordinary ones).
         * @param ctx ProgramCtx
         * @param agp Ground program with meta information used for optimized UFS checking
         * @param choiceRuleCompatible This parameter is necessary for the clasp backend, which implements non-head cycle free disjunctive rules using choice rules.
         *                             However, this transformation must be regarded in the optimization of UFS checking. More specifically, the UFS check MUST NOT BE SKIPPED
         *                             for HFC-free components if they contain such choice rules. For more information, see examples/trickyufs.hex.
         */
        UnfoundedSetCheckerManager(
            ProgramCtx& ctx,
            const AnnotatedGroundProgram& agp,
            bool choiceRuleCompatible = false);

        /**
         * \brief Tries to detect an unfounded set with the possibility to ignore rules and learn nogoods.
         * @param interpretation The compatible set the UFS check shall be performed form. Must be complete over all non-ignored rules (\p skipProgram).
         * @param skipProgram Set of rule IDs to ignore during the check.
         * @param ngc NogoodContainer to add learned nogoods to (can be NULL).
         * @return Unfounded set as set of ground atoms, or the empty set of no unfounded set exists.
         */
        std::vector<IDAddress> getUnfoundedSet(
            InterpretationConstPtr interpretation,
            const std::set<ID>& skipProgram,
            SimpleNogoodContainerPtr ngc = SimpleNogoodContainerPtr());

        /**
         * \brief Tries to detect an unfounded set, but does not skip program parts or learn nogoods.
         * @param interpretation The compatible set the UFS check shall be performed form. Must be complete over all non-ignored rules (\p skipProgram).
         * @return Unfounded set as set of ground atoms, or the empty set of no unfounded set exists.
         */
        std::vector<IDAddress> getUnfoundedSet(
            InterpretationConstPtr interpretation);

        /**
         * \brief Initializes the unfounded set checkers for all program components.
         */
        void initializeUnfoundedSetCheckers();

        /**
         * \brief Forces all unfounded set checker in this manager to learn nogoods from main search now.
         * @param reset Specifies if the nogood container from the main search shall be scanned from the beginning (otherwise only nogoods added at the back will be recognized).
         */
        void learnNogoodsFromMainSearch(bool reset);

        /**
         * \brief Returns the UFS nogood for the most recent detected unfounded set.
         * @return UFS nogood, see UnfoundedSetChecker::getUFSNogood.
         */
        Nogood getLastUFSNogood() const;

        typedef boost::shared_ptr<UnfoundedSetCheckerManager> Ptr;
};

typedef UnfoundedSetCheckerManager::Ptr UnfoundedSetCheckerManagerPtr;

DLVHEX_NAMESPACE_END
#endif

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
