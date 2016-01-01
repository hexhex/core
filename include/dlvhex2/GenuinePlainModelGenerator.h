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
 * @file   GenuinePlainModelGenerator.hpp
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 *
 * @brief  Model generator for the "Plain" type of components using CDNL.
 */

#ifndef GENUINE_PLAIN_MODEL_GENERATOR_HPP_INCLUDED__09122011
#define GENUINE_PLAIN_MODEL_GENERATOR_HPP_INCLUDED__09122011

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/BaseModelGenerator.h"
#include "dlvhex2/ComponentGraph.h"
#include "dlvhex2/GenuineSolver.h"
#include "dlvhex2/InternalGroundDASPSolver.h"
#include "dlvhex2/InternalGrounder.h"

DLVHEX_NAMESPACE_BEGIN

class GenuinePlainModelGeneratorFactory;

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
class GenuinePlainModelGenerator:
public BaseModelGenerator,
public ostream_printable<GenuinePlainModelGenerator>
{
    // types
    public:
        typedef GenuinePlainModelGeneratorFactory Factory;

        // storage
    protected:
        /** \brief Reference to the factory which created this model generator. */
        Factory& factory;

        /** \brief EDB + original (input) interpretation plus auxiliary atoms for evaluated external atoms. */
        InterpretationConstPtr postprocessedInput;
        /** \brief Result handle for asp solver evaluation, using externallyAugmentedInput. */
        ASPSolverManager::ResultsPtr currentResults;

        /** \brief Solver instance. */
        GenuineSolverPtr solver;

        // members

    public:
        /**
         * \brief Constructor.
         * @param factory Reference to the factory which created this model generator.
         * @param input Input interpretation to this model generator.
         */
        GenuinePlainModelGenerator(Factory& factory, InterpretationConstPtr input);
        /** \brief Destructor. */
        virtual ~GenuinePlainModelGenerator();

        // generate and return next model, return null after last model
        virtual InterpretationPtr generateNextModel();
};

/** \brief Factory for the GenuinePlainModelGenerator. */
class GenuinePlainModelGeneratorFactory:
public BaseModelGeneratorFactory,
public ostream_printable<GenuinePlainModelGeneratorFactory>
{
    // types
    public:
        friend class GenuinePlainModelGenerator;
        typedef ComponentGraph::ComponentInfo ComponentInfo;

        // storage
    protected:
        /** \brief Defines the solver to be used for external evaluation. */
        ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig;
        /** \brief ProgramCtx. */
        ProgramCtx& ctx;
        /** ComponentInfo of the component to be solved by the model generators instantiated by this factory. */
        ComponentInfo ci;        // should be a reference, but there is currently a bug in the copy constructor of ComponentGraph: it seems that the component info is shared between different copies of a component graph, hence it is deallocated when one of the copies dies.

        /** \brief All external atoms of the component. */
        std::vector<ID> eatoms;

        /** \brief Original IDB containing eatoms where all inputs are known.
         *
         * Auxiliary input rules of these eatoms must be in predecessor unit! */
        std::vector<ID> idb;
        /** \brief Rewritten IDB (containing replacements for eatoms).
         *
         * x stands for transformed. */
        std::vector<ID> xidb;

        // methods
    public:
        /** \brief Constructor.
         *
         * @param ctx See GenuineGuessAndCheckModelGeneratorFactory::ctx.
         * @param ci See GenuineGuessAndCheckModelGeneratorFactory::ci.
         * @param externalEvalConfig See GenuineGuessAndCheckModelGeneratorFactory::externalEvalConfig.
         */
        GenuinePlainModelGeneratorFactory(
            ProgramCtx& ctx, const ComponentInfo& ci,
            ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig);
        /** \brief Destructor. */
        virtual ~GenuinePlainModelGeneratorFactory() {}

        /**
         * \brief Instantiates a model generator for the current component.
         * @param input Input interpretation to this model generator.
         * @return Model generator.
         */
        virtual ModelGeneratorPtr createModelGenerator(
            InterpretationConstPtr input)
            { return ModelGeneratorPtr(new GenuinePlainModelGenerator(*this, input)); }

        /** \brief Prints information about the model generator for debugging purposes.
         * @param o Stream to print to. */
        virtual std::ostream& print(std::ostream& o) const;
};

DLVHEX_NAMESPACE_END
#endif                           // PLAIN_MODEL_GENERATOR_HPP_INCLUDED__09112010

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
