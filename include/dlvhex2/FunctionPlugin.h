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
 * @file FunctionPlugin.h
 * @author Christoph Redl
 *
 * @brief Support for function symbol handling via external atoms
 */

#ifndef FUNCTION_PLUGIN__HPP_INCLUDED_
#define FUNCTION_PLUGIN__HPP_INCLUDED_

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/PluginInterface.h"
#include <set>

DLVHEX_NAMESPACE_BEGIN

/** \brief Implements function symbols either by native handling or by rewriting to external atoms. */
class FunctionPlugin:
public PluginInterface
{
    public:
        // stored in ProgramCtx, accessed using getPluginData<ExistsPlugin>()
        class CtxData:
    public PluginData
    {
        public:
            /** \brief Maximal input arity for external atoms which handle functional terms. */
            int maxArity;

            /** \brief True to rewrite function symbols to external atoms and false to handle them natively. */
            bool rewrite;

            /** \brief Enable parser for functionals. */
            bool parser;

            CtxData();
            virtual ~CtxData() {};
    };

    public:
        /** \brief Constructor. */
        FunctionPlugin();
        /** \brief Destructor. */
        virtual ~FunctionPlugin();

        // output help message for this plugin
        virtual void printUsage(std::ostream& o) const;

        // accepted options: --exists-enable  --exists-maxarity=<N>
        //
        // processes options for this plugin, and removes recognized options from pluginOptions
        // (do not free the pointers, the const char* directly come from argv)
        virtual void processOptions(std::list<const char*>& pluginOptions, ProgramCtx&);

        // create parser modules that extend and the basic hex grammar
        virtual std::vector<HexParserModulePtr> createParserModules(ProgramCtx&);

        // rewrite program by adding auxiliary constraints
        virtual PluginRewriterPtr createRewriter(ProgramCtx&);

        // plugin atoms
        virtual std::vector<PluginAtomPtr> createAtoms(ProgramCtx& ctx) const;

        // change model callback (print auxiliaries as negative atoms)
        virtual void setupProgramCtx(ProgramCtx&);

        // no atoms!
};

DLVHEX_NAMESPACE_END
#endif

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
