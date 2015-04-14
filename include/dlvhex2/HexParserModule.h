/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schüller
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
 * @file   HexGrammar.h
 * @author Peter Schller
 * @date   Wed Jul  8 14:00:48 CEST 2009
 *
 * @brief  Grammar for parsing HEX using boost::spirit
 *
 * We code everything as intended by boost::spirit (use templates)
 * however we explicitly instantiate the template paramters in
 * a separate compilation unit HexGrammar.cpp to
 * 1) have faster compilation, and
 * 2) allow us to extend parsers by plugins from shared libraries
 *    (i.e., during runtime).
 */

#ifndef DLVHEX_HEX_PARSER_MODULE_H_INCLUDED
#define DLVHEX_HEX_PARSER_MODULE_H_INCLUDED

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/HexGrammar.h"

DLVHEX_NAMESPACE_BEGIN

/** \brief Implements a plugin into the parser for parsing special non-standard HEX-syntax. */
class HexParserModule
{
    public:
        /** \brief Defines on which level the parser is called. */
        enum Type
        {
            /** \brief On the level of rules (supports custom rule types or more general structures). */
            TOPLEVEL,
            /** \brief On the level of body atoms (supports body atoms with special syntax). */
            BODYATOM,
            /** \brief On the level of head atoms (supports head atoms with special syntax). */
            HEADATOM,
            /** \brief On the level of terms (supports terms with special syntax). */
            TERM
        };

        /** \brief Destructor. */
        virtual ~HexParserModule() {};

        /** \brief Gets the type of the module.
         * @return See HexParserModule::Type. */
        Type getType() const { return type; }
        /** \brief Constructor.
         * @param type See HexParserModule::Type. */
        HexParserModule(Type type): type(type) {}

        /** \brief Needs to be implemented and defines the custom syntax to parser.
         * @param Custom grammar. */
        virtual HexParserModuleGrammarPtr createGrammarModule() = 0;

    protected:
        /** \brief Type of the parser module. */
        Type type;
};

DLVHEX_NAMESPACE_END
#endif                           // DLVHEX_HEX_PARSER_MODULE_H_INCLUDED

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
