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
 * @file   AlphaModelGenerator.h
 * @author 
 *
 * @brief  
 */

#ifndef ALPHA_MODEL_GENERATOR_HPP_INCLUDED__09112010
#define ALPHA_MODEL_GENERATOR_HPP_INCLUDED__09112010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/BaseModelGenerator.h"
#include "dlvhex2/ComponentGraph.h"

DLVHEX_NAMESPACE_BEGIN

class AlphaModelGeneratorFactory;

//
// A model generator does the following:
// * it is constructed by a ModelGeneratorFactory which knows the program
//   (and can precompute information for evaluation,
//    and may also provide this to the model generator)
// * it is evaluated on a (probably empty) input interpretation
// * this evaluation can be performed online
// * evaluation yields a (probably empty) set of output interpretations
//

/** \brief A model generator for components without inner (i.e. non-cyclic) external atoms (outer external atoms are allowed). */
class AlphaModelGenerator:
public BaseModelGenerator,
public ostream_printable<AlphaModelGenerator>
{
    // types
    public:
        typedef AlphaModelGeneratorFactory Factory;
        
        /** \brief Reference to the factory which created this model generator. */
        Factory& factory;

        // storage
    protected:

        /** \brief EDB + original (input) interpretation plus auxiliary atoms for evaluated external atoms. */
        InterpretationConstPtr postprocessedInput;
        /** \brief Result handle for asp solver evaluation, using externallyAugmentedInput. */
        ASPSolverManager::ResultsPtr currentResults;

        // members
    public:
        /**
         * \brief Constructor.
         * @param factory Reference to the factory which created this model generator.
         * @param input Input interpretation to this model generator.
         */
        AlphaModelGenerator(Factory& factory, InterpretationConstPtr input);

        /**
         * \brief Destuctor.
         */
        virtual ~AlphaModelGenerator() {}

        // generate and return next model, return null after last model
        virtual InterpretationPtr generateNextModel();
        InterpretationConstPtr computeRelevantDomain(ProgramCtx& ctx, InterpretationConstPtr edb, std::vector<ID>& deidb, std::vector<ID>& deidbInnerEatoms, bool enumerateNonmonotonic = true);
             
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
        bool evaluateExternalAtomFacade(ProgramCtx& ctx,
        ID eatomID,
        InterpretationConstPtr inputi,
        ExternalAnswerTupleCallback& cb,
        NogoodContainerPtr nogoods = NogoodContainerPtr(),
        InterpretationConstPtr assigned = InterpretationConstPtr(),
        InterpretationConstPtr changed = InterpretationConstPtr(),
        bool* fromCache = 0) const;
};

// global pointer to keep reference in jvm
extern AlphaModelGenerator* amgPointer;

/** \brief Factory for the AlphaModelGenerator. */
class AlphaModelGeneratorFactory:
public BaseModelGeneratorFactory,
public ostream_printable<AlphaModelGeneratorFactory>
{
    // types
    public:
        friend class AlphaModelGenerator;
        typedef ComponentGraph::ComponentInfo ComponentInfo;
        /** \brief ProgramCtx. */
        ProgramCtx& ctx;
        /** \brief All external atoms of the component. */
        std::vector<ID> outerEatoms;
        std::vector<ID> innerEatoms;
        std::set<ID> relevantatomextensions;
        std::set<ID> relevantguesses;

        // storage
    protected:
        /** \brief Defines the solver to be used for external evaluation. */
        ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig;
        /** \brief Original IDB containing eatoms where all inputs are known.
         *
         * Auxiliary input rules of these eatoms must be in predecessor unit!
         */
        std::vector<ID> idb;
        /** \brief Rewritten idb (containing replacements for eatoms).
         *
         * x stands for transformed. */
        std::vector<ID> xidb;
        
        std::vector<ID> ridb;
        
        std::vector<ID> deidb;
        std::vector<ID> deidbInnerEatoms;
        std::set<ID> nonmonotonicinputs;
        
        // methods
    public:
        /** \brief Constructor.
         *
         * @param ctx See GenuineGuessAndCheckModelGeneratorFactory::ctx.
         * @param ci See GenuineGuessAndCheckModelGeneratorFactory::ci.
         * @param externalEvalConfig See GenuineGuessAndCheckModelGeneratorFactory::externalEvalConfig.
         */
        AlphaModelGeneratorFactory(
            ProgramCtx& ctx, const ComponentInfo& ci,
            ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig);
        /** \brief Destructor. */
        virtual ~AlphaModelGeneratorFactory() {}

        /**
         * \brief Instantiates a model generator for the current component.
         * @param input Input interpretation to this model generator.
         * @return Model generator.
         */
        virtual ModelGeneratorPtr createModelGenerator(
            InterpretationConstPtr input)
            { return ModelGeneratorPtr(new AlphaModelGenerator(*this, input)); }

        /** \brief Prints information about the model generator for debugging purposes.
         * @param o Stream to print to. */
        virtual std::ostream& print(std::ostream& o) const;
};

DLVHEX_NAMESPACE_END
#endif                           // ALPHA_MODEL_GENERATOR_HPP_INCLUDED__09112010

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
