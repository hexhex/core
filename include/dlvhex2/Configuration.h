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
 * @file   Configuration.h
 * @author Roman Schindlauer, Peter Schueller
 * @date   Sat Nov  5 15:26:18 CET 2005
 *
 * @brief  configuration container (previously global variables)
 */

#if !defined(_DLVHEX_CONFIGURATION_HPP)
#define _DLVHEX_CONFIGURATION_HPP

#include "dlvhex2/PlatformDefinitions.h"

#include <string>
#include <vector>
#include <map>

DLVHEX_NAMESPACE_BEGIN

/**
 * @brief Definition of global variables.
 */
class DLVHEX_EXPORT Configuration
{
    public:
        /** \brief Constructor. */
        Configuration();

        /**
         * @brief List of possible verbose actions.
         */
        typedef enum {
            /** \brief Output program after conversion (see PluginInterface::PluginConverter). */
            DUMP_CONVERTED_PROGRAM,
            /** \brief Output parsed program. */
            DUMP_PARSED_PROGRAM,
            /** \brief Output program after rewriting (see PluginInterface::PluginRewriter). */
            DUMP_REWRITTEN_PROGRAM,
            /** \brief Output information about safety. */
            SAFETY_ANALYSIS,
            /** \brief Output dependency graph as .dot file. */
            DUMP_DEPENDENCY_GRAPH,
            /** \brief Output program after optimization (see PluginInterface::PluginOptimizer). */
            DUMP_OPTIMIZED_PROGRAM,
            /** \brief Output detailed information about plugin loading. */
            PLUGIN_LOADING,
            /** \brief Output detailed information about solving. */
            COMPONENT_EVALUATION,
            /** \brief Output detailed information about model generation. */
            MODEL_GENERATOR,
            /** \brief Output detailed information about boost graphs. */
            GRAPH_PROCESSOR,
            /** \brief Profiling. */
            PROFILING,
            /** \brief Dump output. */
            DUMP_OUTPUT
        } verboseAction_t;

        /**
         * \brief Return the value of the specified option identifier.
         * @param o Name of the option to retrieve.
         * @return Value of option \p o.
         */
        unsigned
            getOption(const std::string& o) const;

        /**
         * @brief Check if the specified verbose action \p a can be carried out.
         *
         * This function checks if the predefined (see Globals::Globals())
         * bit of the specified verbose action (see Globals::verboseLevel)
         * is set in the verbose level given as a parameter.
         * @param a Verbose action.
         * @return True if the specified verbose action \p a can be carried out and false otherwise.
         */
        bool
            doVerbose(verboseAction_t a);

        /**
         * Set an option with specified identifier to a value.
         *
         * Not using a reference here, because we use explicit strings in main to
         * call this method!
         */
        void
            setOption(const std::string&, unsigned);

        /**
         * @brief Add a predicate to be filtered.
         * @param p Predicate (as string) to be filtered.
         */
        void
            addFilter(const std::string&);

        /**
         * Adds an atom for inconsistency explanation.
         * @param atom Atom for the explanation. */
        void addExplanationAtom(const std::string& atom);

        /**
         * @brief Returns list of predicates to be filtered.
         * @return Vector of all predicates (as strings) to be filtered.
         */
        const std::vector<std::string>&
            getFilters() const;

        /**
         * @brief Returns list of atoms used for inconsistency explanation.
         * @return Vector of atoms for inconsistency explanation.
         */
        const std::vector<std::string>&
            getExplanationAtoms() const;

        /** \brief Retrieve the string value of an option.
         * @param key Name of the option.
         * @return String value of option \p key. */
        const std::string& getStringOption(const std::string& key) const;
        /** \brief Sets the value of a string option.
         * @param key Name of the string option to set.
         * @param value New value of option \p key. */
        void setStringOption(const std::string& key, const std::string& value);

    private:

        /**
         * @brief Associates a verbose action with a verbose level.
         */
        std::map<verboseAction_t, unsigned> verboseLevel;

        /**
         * @brief Associates option names with values.
         */
        std::map<std::string, unsigned> optionMap;
        /**
         * @brief Associates option names with string values.
         */
        std::map<std::string, std::string> stringOptionMap;

        /**
         * @brief List of filter-predicates.
         */
        std::vector<std::string> optionFilter;

        /** \brief Set of atoms used for inconsistency explanation. */
        std::vector<std::string> optionExplanation;
};

DLVHEX_NAMESPACE_END
#endif                           // _DLVHEX_CONFIGURATION_HPP

// vim: noet ts=8 sw=4 tw=80


// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
