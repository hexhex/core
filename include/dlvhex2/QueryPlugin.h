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
 * @file QueryPlugin.h
 * @author Peter Schueller
 *
 * @brief Plugin for cautions/brave ground/nonground queries in dlvhex.
 */

#ifndef QUERY_PLUGIN__HPP_INCLUDED_1518
#define QUERY_PLUGIN__HPP_INCLUDED_1518

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/PluginInterface.h"

DLVHEX_NAMESPACE_BEGIN

/** \brief Implements brave and cautious queries by rewriting them to answer set computation and postprocessind. */
class QueryPlugin:
public PluginInterface
{
    public:
        // stored in ProgramCtx, accessed using getPluginData<QueryPlugin>()
        class CtxData:
    public PluginData
    {
        public:
            /** \brief Stores if plugin is enabled. */
            bool enabled;

            /** Available reasoning modes.
             *
             * at the moment DEFAULT triggers an error,
             * so the user _must_ choose a reasoning mode. */
            enum Mode
            {
                DEFAULT,
                /** \brief Brave reasoning. */
                BRAVE,
                /** \brief Cautious reasoning. */
                CAUTIOUS
            };
            /** \brief Selected reasoning mode. */
            Mode mode;

            /** \brief True for ground queries, false for nonground. */
            bool ground;

            /** \brief The query (contains body literals).
             *
             * This is not directly stored into IDB or EDB. */
            Tuple query;

            /** \brief Auxiliary predicate symbols for nonground query evaluation. */
            ID varAuxPred;
            /** \brief Auxiliary predicate symbols for ground query evaluation. */
            ID novarAuxPred;

            /** \brief IDs of variables as they occur in auxiliary nonground predicate. */
            Tuple variableIDs;

            /** \brief Whether to display all witnesses for ground queries.
             *
             * Positive witnesses for brave and negative for cautious reasoning. */
            bool allWitnesses;

            CtxData();
            virtual ~CtxData() {};
    };

    public:
        /** \brief Constructor. */
        QueryPlugin();
        /** \brief Destructor. */
        virtual ~QueryPlugin();

        // output help message for this plugin
        virtual void printUsage(std::ostream& o) const;

        // accepted options: --query-enable --query-brave --query-cautious
        //
        // processes options for this plugin, and removes recognized options from pluginOptions
        // (do not free the pointers, the const char* directly come from argv)
        virtual void processOptions(std::list<const char*>& pluginOptions, ProgramCtx&);

        // create parser modules that extend and the basic hex grammar
        // this parser also stores the query information into the plugin
        virtual std::vector<HexParserModulePtr> createParserModules(ProgramCtx&);

        // rewrite program by adding auxiliary query rules
        virtual PluginRewriterPtr createRewriter(ProgramCtx&);

        // change model callback and register final callback
        virtual void setupProgramCtx(ProgramCtx&);

        // no atoms!
};

DLVHEX_NAMESPACE_END
#endif

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
