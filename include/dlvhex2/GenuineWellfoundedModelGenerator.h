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
 * @file   WellfoundedModelGenerator.hpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Model generator for eval units that allow a fixpoint calculation.
 *
 * Those units may contain external atoms at the input,
 * only monotonic eatoms and no negative cycles within the SCC.
 */

#ifndef GENUINEWELLFOUNDED_MODEL_GENERATOR_HPP_INCLUDED__09122011
#define GENUINEWELLFOUNDED_MODEL_GENERATOR_HPP_INCLUDED__09122011

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/BaseModelGenerator.h"
#include "dlvhex2/ComponentGraph.h"

DLVHEX_NAMESPACE_BEGIN

class GenuineWellfoundedModelGeneratorFactory;

/** \brief Model generator for purely monotonic components. */
class GenuineWellfoundedModelGenerator:
public BaseModelGenerator,
public ostream_printable<GenuineWellfoundedModelGenerator>
{
    // types
    public:
        typedef GenuineWellfoundedModelGeneratorFactory Factory;

        // storage
    protected:
        /** \brief Reference to the factory which created this model generator. */
        Factory& factory;

        /** \brief Result handle for asp solver evaluation, using externallyAugmentedInput. */
        ASPSolverManager::ResultsPtr currentResults;
        /** \brief True before first model was returned, false otherwise. */
        bool firstcall;

        // members
    public:
        /**
         * \brief Constructor.
         * @param factory Reference to the factory which created this model generator.
         * @param input Input interpretation to this model generator.
         */
        GenuineWellfoundedModelGenerator(Factory& factory, InterpretationConstPtr input);
        /** \brief Destructor. */
        virtual ~GenuineWellfoundedModelGenerator();

        // generate and return next model, return null after last model
        virtual InterpretationPtr generateNextModel();
};

/** \brief Factory for the GenuineWellfoundedModelGenerator. */
class GenuineWellfoundedModelGeneratorFactory:
public BaseModelGeneratorFactory,
public ostream_printable<GenuineWellfoundedModelGeneratorFactory>
{
    // types
    public:
        friend class GenuineWellfoundedModelGenerator;
        typedef ComponentGraph::ComponentInfo ComponentInfo;

        // storage
    protected:
        /** \brief Defines the solver to be used for external evaluation. */
        ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig;
        /** \brief ProgramCtx. */
        ProgramCtx& ctx;
        /** ComponentInfo of the component to be solved by the model generators instantiated by this factory. */
        ComponentInfo ci;        // should be a reference, but there is currently a bug in the copy constructor of ComponentGraph: it seems that the component info is shared between different copies of a component graph, hence it is deallocated when one of the copies dies.

        /** \brief Outer external atoms of the component. */
        std::vector<ID> outerEatoms;
        /** \brief Inner external atoms of the component. */
        std::vector<ID> innerEatoms;
        /** \brief Original IDB containing eatoms where all inputs are known.
         *
         * Auxiliary input rules of these eatoms must be in predecessor unit! */
        std::vector<ID> idb;
        /** \brief Rewritten IDB (containing replacements for eatoms).
         *
         * x stands for transformed. */
        std::vector<ID> xidb;

        /** \brief Inner external atoms in deidb. */
        std::vector<ID> deidbInnerEatoms;
        /** \brief IDB for domain exploration (cf. liberal safety).
         *
         * Equivalent to xidb, except that it does not contain domain predicates). */
        std::vector<ID> deidb;

        // methods
    public:
        /** \brief Constructor.
         *
         * @param ctx See GenuineGuessAndCheckModelGeneratorFactory::ctx.
         * @param ci See GenuineGuessAndCheckModelGeneratorFactory::ci.
         * @param externalEvalConfig See GenuineGuessAndCheckModelGeneratorFactory::externalEvalConfig.
         */
        GenuineWellfoundedModelGeneratorFactory(
            ProgramCtx& ctx, const ComponentInfo& ci,
            ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig);
        /** \brief Destructor. */
        virtual ~GenuineWellfoundedModelGeneratorFactory() {}

        /**
         * \brief Instantiates a model generator for the current component.
         * @param input Input interpretation to this model generator.
         * @return Model generator.
         */
        virtual ModelGeneratorPtr createModelGenerator(
            InterpretationConstPtr input)
            { return ModelGeneratorPtr(new GenuineWellfoundedModelGenerator(*this, input)); }

        /** \brief Prints information about the model generator for debugging purposes.
         * @param o Stream to print to. */
        virtual std::ostream& print(std::ostream& o) const;
};

DLVHEX_NAMESPACE_END
#endif                           // WELLFOUNDED_MODEL_GENERATOR_HPP_INCLUDED__09112010

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
