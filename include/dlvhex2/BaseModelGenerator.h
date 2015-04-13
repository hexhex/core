/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schller
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
 * @file   BaseModelGenerator.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Base class for model generator factories and model generators.
 */

#ifndef BASE_MODEL_GENERATOR_HPP_INCLUDED__09112010
#define BASE_MODEL_GENERATOR_HPP_INCLUDED__09112010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/ModelGenerator.h"
#include "dlvhex2/Interpretation.h"
#include "dlvhex2/ASPSolverManager.h"
#include "dlvhex2/Atoms.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Nogood.h"
#include "dlvhex2/GenuineSolver.h"
#include "dlvhex2/ComponentGraph.h"

#include <list>
#include "dlvhex2/CDNLSolver.h"

DLVHEX_NAMESPACE_BEGIN

/**
 * \brief A model generator factory provides model generators
 * for a certain types of interpretations
 */
class DLVHEX_EXPORT BaseModelGeneratorFactory:
public ModelGeneratorFactoryBase<Interpretation>
{
    // methods
    public:
        /** \brief Constructor. */
        BaseModelGeneratorFactory() {}
        /** \brief Destructor. */
        virtual ~BaseModelGeneratorFactory() {}

    protected:
        /** \brief Rewrite all eatoms in body to auxiliary replacement atoms
         * store into registry and return id.
         *
         * @param ctx ProgramCtx.
         * @param ruleid ID of the rule to rewrite.
         * @return ID of the rewritten rule.
         */
        virtual ID convertRule(ProgramCtx& ctx, ID ruleid);
        /** \brief Rewrite all eatoms in body tuple to auxiliary replacement atoms
         * store new body into convbody.
         *
         * Works recursively for aggregate atoms,
         * will create additional "auxiliary" aggregate atoms in registry.
         * @param ctx ProgramCtx.
         * @param body Body of the rule to rewrite.
         * @param convbody Vector where new body is to be stored.
         */
        virtual void convertRuleBody(ProgramCtx& ctx, const Tuple& body, Tuple& convbody);

        /**
         * Adds domain predicates for inner external atoms (where necessary).
         * @param ci The component whose external atoms shall be prepared for liberal domain-expansion safety
         * @param ctx ProgramCtx
         * @param idb IDB of the unit
         * @param deidb Reference to a vector where a simplified version of the IDB will be stored that can be used later for computing the extensions of domain predicates (see computeExtensionOfDomainPredicates)
         * @param deidbInnerEatoms Reference to a vector which will store the inner external atoms which are relevant for liberal domain-expansion safety and can be used later for computing the extensions of domain predicates (see computeExtensionOfDomainPredicates)
         * @param outerEatoms External atoms which shall be treated as outer external atoms and are not included the domain expansion computation
         */
        void addDomainPredicatesAndCreateDomainExplorationProgram(const ComponentGraph::ComponentInfo& ci, ProgramCtx& ctx, std::vector<ID>& idb, std::vector<ID>& deidb, std::vector<ID>& deidbInnerEatoms, const std::vector<ID>& outerEatoms);
};

/**
 * \brief Base class for all model generators.
 */
class DLVHEX_EXPORT BaseModelGenerator:
public ModelGeneratorBase<Interpretation>
{
    friend class UnfoundedSetCheckerOld;
    friend class UnfoundedSetChecker;
    friend class EncodingBasedUnfoundedSetChecker;
    friend class AssumptionBasedUnfoundedSetChecker;
    // members
    public:
        /**
         * \brief Constructor.
         *
         * @param input Input interpretation, i.e., facts to be added before solving.
         */
        BaseModelGenerator(InterpretationConstPtr input):
        ModelGeneratorBase<Interpretation>(input) {}
        /** \brief Destructor. */
        virtual ~BaseModelGenerator() {}

    protected:
        //
        // ========== External Atom Evaluation Helpers ==========
        //

    public:
        /** \brief Base class for callback functions for handling external atom answer tuples. */
        struct ExternalAnswerTupleCallback
        {
            /** \brief Destructor. */
            virtual ~ExternalAnswerTupleCallback();
            // boolean return values specifies whether to continue the process
            // (true = continue, false = abort)

            /**
             * \brief Is called when the next eatom is encountered.
             *
             * @param eatom The external atom currently encountered.
             * @return True if enumeration shall be continued als false to abort.
             */
            virtual bool eatom(const ExternalAtom& eatom) = 0;

            /**
             * \brief Is called when the next input tuple is encountered.
             *
             * Preceded by eatom(...).
             *
             * @param input The input tuple currently encountered.
             * @return True if enumeration shall be continued als false to abort.
             */
            // encountering next input tuple
            virtual bool input(const Tuple& input) = 0;

            /**
             * \brief Is called when the next output tuple is encountered.
             *
             * Preceded by eatom(...) even for empty input tuples.
             *
             * @param eatom The output tuple currently encountered.
             * @return True if enumeration shall be continued als false to abort.
             */
            virtual bool output(const Tuple& output) = 0;
        };

        /**
         * \brief Callback function object for handling external atom answer tuples
         * by multiple callbacks.
         */
        struct ExternalAnswerTupleMultiCallback:
    public ExternalAnswerTupleCallback
    {
        /** \brief List of atomic callbacks. */
        std::vector<ExternalAnswerTupleCallback*> callbacks;

        virtual ~ExternalAnswerTupleMultiCallback();
        virtual bool eatom(const ExternalAtom& eatom);
        virtual bool input(const Tuple& input);
        virtual bool output(const Tuple& output);
    };

    /**
     * \brief Callback for checking whether external computations
     * reflect guesses of external atom truth values
     */
    struct VerifyExternalAnswerAgainstPosNegGuessInterpretationCB:
    public ExternalAnswerTupleCallback
    {
        /**
         * \brief Constructor.
         *
         * @param guess_pos Set of guessed positive replacement atoms.
         * @param guess_neg Set of guessed negative replacement atoms.
         */
        VerifyExternalAnswerAgainstPosNegGuessInterpretationCB(
            InterpretationPtr guess_pos,
            InterpretationPtr guess_neg);
        /** \brief Destructor. */
        virtual ~VerifyExternalAnswerAgainstPosNegGuessInterpretationCB() {}
        // remembers eatom and prepares replacement.tuple[0]
        virtual bool eatom(const ExternalAtom& eatom);
        // remembers input
        virtual bool input(const Tuple& input);
        // creates replacement ogatom and activates respective bit in output interpretation
        virtual bool output(const Tuple& output);
        protected:
            /** \brief RegistryPtr. */
            RegistryPtr reg;
            /** \brief See constructor. */
            InterpretationPtr guess_pos;
            /** \brief See constructor. */
            InterpretationPtr guess_neg;
            /** \brief Cache of positive replacement predicate. */
            ID pospred;
            /** \brief Cache of negative replacement predicate. */
            ID negpred;
            /** \brief Current replacement atom (temporary storage). */
            OrdinaryAtom replacement;
    };

    /**
     * \brief Verifies positive and negative replacement atoms against an external atom.
     */
    struct VerifyExternalAtomCB:
    public ExternalAnswerTupleCallback
    {
        protected:
            /** \brief External atom to verify. */
            const ExternalAtom& exatom;
            /** \brief Mask of the external atom to verify. */
            const ExternalAtomMask& eaMask;
            /** \brief RegistryPtr. */
            RegistryPtr reg;
            /** \brief Cache of positive replacement predicate. */
            ID pospred;
            /** \brief Cache of negative replacement predicate. */
            ID negpred;
            /** \brief Current replacement atom (temporary storage). */
            OrdinaryAtom replacement;
            /** \brief Set of guessed atoms. */
            InterpretationConstPtr guess;
            /** \brief Set of guessed atoms remaining to be verified. */
            InterpretationPtr remainingguess;
            /** \brief Intermediate result. */
            bool verified;
            /** \brief If verification fails, this field will contain a falsified replacement atom. */
            ID falsified;

        public:
            bool onlyNegativeAuxiliaries();

            /**
             * \brief Constructor.
             *
             * @param guess Set of guessed replacement atoms.
             * @param exatom External atom to verify against to.
             * @param eaMask Mask of \p exatom.
             */
            VerifyExternalAtomCB(InterpretationConstPtr guess, const ExternalAtom& exatom, const ExternalAtomMask& eaMask);
            /** \brief Destructor. */
            virtual ~VerifyExternalAtomCB();
            // remembers eatom and prepares replacement.tuple[0]
            virtual bool eatom(const ExternalAtom& eatom);
            // remembers input
            virtual bool input(const Tuple& input);
            // creates replacement ogatom and activates respective bit in output interpretation
            virtual bool output(const Tuple& output);

            /**
             * \brief Returns the verification result.
             *
             * @return Verification result.
             */
            bool verify();

            /** \brief Returns a falsified atom (positive or negative auxiliary).
             *
             * @return ID of a falsified atom if verify() returns false and ID_FAIL otherwise.
             */
            ID getFalsifiedAtom();
    };

    /**
     * \brief For usual model building where we want to collect all true answers
     * as replacement atoms in an interpretation.
     */
    struct IntegrateExternalAnswerIntoInterpretationCB:
    public ExternalAnswerTupleCallback
    {
        /**
         * \brief Constructor.
         * @param outputi Interpretation to add the external atom output to.
         */
        IntegrateExternalAnswerIntoInterpretationCB(
            InterpretationPtr outputi);
        virtual ~IntegrateExternalAnswerIntoInterpretationCB() {}
        // remembers eatom and prepares replacement.tuple[0]
        virtual bool eatom(const ExternalAtom& eatom);
        // remembers input
        virtual bool input(const Tuple& input);
        // creates replacement ogatom and activates respective bit in output interpretation
        virtual bool output(const Tuple& output);
        protected:
            /** \brief RegistryPtr. */
            RegistryPtr reg;
            /** \brief See constructor. */
            InterpretationPtr outputi;
            /** \brief Current replacement atom (temporary storage). */
            OrdinaryAtom replacement;
    };

    protected:
        /**
         * \brief Evaluates an external atom.
         *
         * Projects input interpretation for predicate inputs
         * calculates constant input tuples from auxiliary input predicates and from given constants
         * calls eatom function with each input tuple
         * reintegrates output tuples as auxiliary atoms into outputi
         * (inputi and outputi may point to the same interpretation)
         * fromCache may point to a boolean (or be 0) where the method stored whether the query was answered from cache
         *
         * @param eatomID The external atom to evaluate.
         * @param inputi Interpretation to use as input to the external atom.
         * @param cb Callback during evaluation of the external atom (see BaseModelGenerator::ExternalAnswerTupleCallback).
         * @param nogoods Container to add learned nogoods to (if external learning is enabled), can be NULL.
         * @param assigned Set of atoms currently assigned; can be used by the external atom to optimize evaluation; can be NULL to indicate that all atoms are assigned.
         * @param changed Set of atoms which possibly changed since last evaluation under the same input; can be used by the external atom to optimize evaluation; can be NULL to indicate that all atoms might have changed.
         * @param fromCache Pointer to a bool field which is is stored whether the query was answered from cache (true) or by actual evaluation (false); can be NULL.
         * @return False if process was aborted by callback and true otherwise.
         */
        virtual bool evaluateExternalAtom(ProgramCtx& ctx,
            ID eatomID,
            InterpretationConstPtr inputi,
            ExternalAnswerTupleCallback& cb,
            NogoodContainerPtr nogoods = NogoodContainerPtr(),
            InterpretationConstPtr assigned = InterpretationConstPtr(),
            InterpretationConstPtr changed = InterpretationConstPtr(),
            bool* fromCache = 0) const;
        /**
         * \brief Evaluates an external atom under a single and fixed input vector.
         *
         * @param query See PluginInterface::Query.
         * @param cb Callback during evaluation of the external atom (see BaseModelGenerator::ExternalAnswerTupleCallback).
         * @param nogoods Container to add learned nogoods to (if external learning is enabled), can be NULL.
         * @param fromCache Pointer to a bool field which is is stored whether the query was answered from cache (true) or by actual evaluation (false); can be NULL.
         * @return False if process was aborted by callback and true otherwise.
         */
        virtual bool evaluateExternalAtomQuery(
            PluginAtom::Query& query,
            ExternalAnswerTupleCallback& cb,
            NogoodContainerPtr nogoods,
            bool* fromCache = 0) const;

        /**
         * \brief Calculates constant input tuples from auxiliary input predicates and from given constants
         * calls eatom function with each input tuple and maximum input for support set learning.
         *
         * @param ctx ProgramCtx.
         * @param eatomID The external atom to evaluate.
         * @param nogoods Container to add learned nogoods to (if external learning is enabled), can be NULL.
         * @return False if process was aborted by callback and true otherwise.
         */
        virtual void learnSupportSetsForExternalAtom(ProgramCtx& ctx,
            ID eatomID,
            NogoodContainerPtr nogoods) const;

        /**
         * \brief Evaluates multiple external atoms.
         *
         * Calls BaseModelGenerator::evaluateExternalAtom for each atom in eatoms.
         *
         * @param eatoms Vector of all external atoms to evaluate.
         * @param inputi Interpretation to use as input to the external atoms.
         * @param cb Callback during evaluation of the external atoms (see BaseModelGenerator::ExternalAnswerTupleCallback).
         * @param nogoods Container to add learned nogoods to (if external learning is enabled), can be NULL.
         * @return False if process was aborted by callback and true otherwise.
         */
        virtual bool evaluateExternalAtoms(ProgramCtx& ctx,
            const std::vector<ID>& eatoms,
            InterpretationConstPtr inputi,
            ExternalAnswerTupleCallback& cb,
            NogoodContainerPtr nogoods = NogoodContainerPtr()) const;

        // helper methods used by evaluateExternalAtom

        /**
         * \brief Checks if an output tuple matches the output pattern in the program.
         *
         * @param reg RegistryPtr.
         * @param eatom The external atom whose output patter is to be used for the verification.
         * @param t The tuple to verify.
         * @return False if tuple does not unify with eatom output pattern
                    (the caller must decide whether to throw an exception or ignore the tuple), and true otherwise.
          */
        virtual bool verifyEAtomAnswerTuple(RegistryPtr reg,
            const ExternalAtom& eatom, const Tuple& t) const;

        /**
         * \brief Project a given interpretation to all predicates that are predicate inputs in the given eatom.
         *
         * @param reg RegistryPtr.
         * @param eatom The external atom whose output patter is to be used for the projection.
         * @return Projection of \p full as a new interpretation.
         */
        virtual InterpretationPtr projectEAtomInputInterpretation(RegistryPtr reg,
            const ExternalAtom& eatom, InterpretationConstPtr full) const;

        /**
         * \brief Extracts the ground input vectors from the interpretation.
         *
         * Using auxiliary input predicate and the eatom, the method
         * calculates all tuples that are inputs to the eatom
         * and store them as true bits into \p inputs, bits can be looked up in the EAInputTupleCache in registry.
         *
         * @param reg RegistryPtr.
         * @param eatom The external atom whose input vectors shall be computed.
         * @param i The input interpretation to the external atom.
         * @param inputs The set of input vector extracted from \p i; each bit can be resolved using Registry::EAInputTupleCache.
         */
        virtual void buildEAtomInputTuples(RegistryPtr reg,
            const ExternalAtom& eatom, InterpretationConstPtr i, InterpretationPtr inputs) const;

        /**
         * Computes the relevant domain atoms, i.e., the extensions of the domain predicates.
         * @param ci The component whose domain atoms to be computed
         * @param ctx ProgramCtx
         * @param edb Set of facts (usually the input model of the component)
         * @param deidb The IDB used for computing the domain expansion; this is a simplified version of the actual IDB and is computed by addDomainPredicatesAndCreateDomainExplorationProgram.
         * @param deidbInnerEatoms The inner atoms which are relevant for liberal domain-expansion safety; this is a subset of all inner external atoms in the unit and is computed by addDomainPredicatesAndCreateDomainExplorationProgram.
         * @param enumerateNonmonotonic If true, then the inner external atoms in deidbInnerEatoms are evaluated under all possible inputs to make sure that they are fully grounded; otherwise they are evaluated only under the current EDB, but then the grounding might be incomplete and might require incremental expansion.
         */
        InterpretationConstPtr computeExtensionOfDomainPredicates(const ComponentGraph::ComponentInfo& ci, ProgramCtx& ctx, InterpretationConstPtr edb, std::vector<ID>& deidb, std::vector<ID>& deidbInnerEatoms, bool enumerateNonmonotonic = true);
};

DLVHEX_NAMESPACE_END
#endif                           // BASE_MODEL_GENERATOR_HPP_INCLUDED__09112010
