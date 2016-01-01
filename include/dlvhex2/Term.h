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
 * @file   Term.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Term class: stores constants, constant strings, and variables.
 *
 * (Integers are implicitly stored within IDs.)
 */

#ifndef TERM_HPP_INCLUDED__12102010
#define TERM_HPP_INCLUDED__12102010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/fwd.h"

DLVHEX_NAMESPACE_BEGIN

/**
 * \brief Stores terms.
 *
 * Supported types: constants, variables and nested terms.
 * Integers do not use this class but rather store the value directly in the address field of the ID.
 */
struct DLVHEX_EXPORT Term:
private ostream_printable<Term>
{
    /** \brief The kind part of the ID of this symbol. */
    IDKind kind;

    // Anonymous variables: are parsed as one variable "_".
    // Then they are processed into new distinct variables,
    // each with the anonymous bit set and with a new ID.
    /** \brief the Textual representation of a  constant,  constant string (including ""), or variable. */
    std::string symbol;

    /** \brief Stores the arguments of nested terms.
     *
     * Nested terms are function terms consisting of a function symbol and its arguments.
     * For primitive terms (constant, constant string, variable), the only element is ID_FAIL.
     * For nested terms, arguments[0] is the function symbol (primitive term) and arguments[n] for n>=1 are the arguments (which might be nested themselves).
     * Moreover ,for nested terms, symbol contains a string representation of the whole term.
     */
    std::vector<ID> arguments;

    /**
     * Constructor.
     * @param kind Identifies the type of the term; will be reused by Registry to construct the ID.
     * @param symbol The term string, which can be a constant (with or without quotation marks), a variable, or a nested term (in text format).
     */
    Term(IDKind kind, const std::string& symbol): kind(kind), symbol(symbol) {
        assert(ID(kind,0).isTerm());

        arguments.push_back(ID_FAIL);
    }

    /**
     * Constructor for nested terms.
     * @param kind Identifies the type of the term; will be reused by Registry to construct the ID.
     * @param arguments Element [0] is the function symbol, the remaining elements specify its arguments.
     * @param reg Registry to use for interpreting term IDs.
     */
    Term(IDKind kind, const std::vector<ID>& arguments, RegistryPtr reg);

    /**
     * \brief Recomputes the text representation of nested terms given their arguments.
     *
     * Sets Term::symbol to the text representation generated from the arguments of the nested term.
     * @param reg Registry to use for interpreting term IDs.
     */
    void updateSymbolOfNestedTerm(Registry* reg);

    /**
     * \brief Checks if the constant term is quoted.
     * @return True if it is quoted and false otherwise. */
    bool isQuotedString() const
    {
        if ((symbol.at(0) == '"') && (symbol.at(symbol.length()-1) == '"'))
            return true;
        return false;
    }

    /**
     * \brief Checks if the term is nested.
     * @return True if it is nested and false otherwise. */
    bool isNestedTerm() const
    {
        return arguments[0] != ID_FAIL;
    }

    /**
     * \brief Returns a text representation of this term with quotes (independent of whether the term is stored with or without quotes).
     * @return Text representation of the term. */
    std::string getQuotedString() const
    {
        return '"' + getUnquotedString() + '"';
    }

    /**
     * \brief Returns a text representation of this term without quotes (independent of whether the term is stored with or without quotes).
     * @return Text representation of the term. */
    std::string getUnquotedString() const
    {
        ;
        if (isQuotedString())
            return symbol.substr(1, symbol.length()-2);
        return symbol;
    }

    /**
     * \brief Parses a nested term and splits the string representation in Term::symbol into arguments.
     * @param reg Registry to use for interpreting term IDs.
     */
    void analyzeTerm(RegistryPtr reg);

    /**
     * \brief Prints the term in form Term(symbol).
     * @param o Stream to print.
     * @return \p o.
     */
    std::ostream& print(std::ostream& o) const
    {
        return o << "Term(" << symbol << ")";
    }
};

DLVHEX_NAMESPACE_END
#endif                           // TERM_HPP_INCLUDED__12102010

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
