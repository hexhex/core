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
 * @file WeakConstraintPlugin.h
 * @author Christoph Redl
 *
 * @brief Implements weak constraints
 */

#ifndef WEAKCONSTRAINT_PLUGIN__HPP_INCLUDED
#define WEAKCONSTRAINT_PLUGIN__HPP_INCLUDED

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/PluginInterface.h"

DLVHEX_NAMESPACE_BEGIN

/** \brief Implements weak plugins by rewriting them to ordinary rules. */
class WeakConstraintPlugin:
public PluginInterface
{
    public:
        // stored in ProgramCtx, accessed using getPluginData<HigherOrderPlugin>()
        class CtxData:
    public PluginData
    {
        public:
            /** \brief Stores if plugin is enabled. */
            bool enabled;

            /** \brief Set to true in order to display all (also non-optimal) models even under weak constraints. */
            bool allmodels;

            CtxData();
            virtual ~CtxData() {};
    };

    public:
        /** \brief Constructor. */
        WeakConstraintPlugin();
        /** \brief Destructor. */
        virtual ~WeakConstraintPlugin();

        // output help message for this plugin
        virtual void printUsage(std::ostream& o) const;

        // accepted options: --aggregate-enable
        //
        // processes options for this plugin, and removes recognized options from pluginOptions
        // (do not free the pointers, the const char* directly come from argv)
        virtual void processOptions(std::list<const char*>& pluginOptions, ProgramCtx&);

        // rewrite program: rewrite aggregate atoms to external atoms
        virtual PluginRewriterPtr createRewriter(ProgramCtx&);

        // register model callback which transforms all auxn(p,t1,...,tn) back to p(t1,...,tn)
        virtual void setupProgramCtx(ProgramCtx&);

        virtual std::vector<PluginAtomPtr> createAtoms(ProgramCtx&) const;

        // no atoms!
};

DLVHEX_NAMESPACE_END
#endif
