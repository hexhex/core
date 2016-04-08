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
 * @file Term.cpp
 * @author Christoph Redl
 *
 * @brief Implementation of Term.h
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#include "dlvhex2/Term.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Printhelpers.h"
#include "dlvhex2/Interpretation.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/OrdinaryAtomTable.h"

#include <boost/foreach.hpp>
#include <map>

DLVHEX_NAMESPACE_BEGIN

Term::Term(IDKind kind, const std::vector<ID>& arguments, RegistryPtr reg): kind(kind), arguments(arguments)
{
    assert(ID(kind,0).isTerm());
    assert(arguments.size() > 0);

    updateSymbolOfNestedTerm(reg.get());
}


void Term::updateSymbolOfNestedTerm(Registry* reg)
{
    std::stringstream ss;
    if ((kind & ID::PROPERTY_MASK) == ID::PROPERTY_TERM_RANGE) {
        // range terms are printed without function symbol and parantheses
        ss << arguments[1].address;
        ss << "..";
        ss << arguments[2].address;
    }else{
        ss << reg->terms.getByID(arguments[0]).symbol;
        if (arguments.size() > 1) {
            ss << "(";
            for (uint32_t i = 1; i < arguments.size(); ++i) {
                ss << (i > 1 ? "," : "");
                if (arguments[i].isIntegerTerm()) {
                    ss << arguments[i].address;
                }
                else {
                    ss << reg->terms.getByID(arguments[i]).symbol;
                }
            }
            ss << ")";
        }
    }
    symbol = ss.str();
}


// restores the hierarchical structure of a term from a string representation
void Term::analyzeTerm(RegistryPtr reg)
{

    // get token: function name and arguments
    bool quoted = false;
    bool primitive = true;
    int nestedcount = 0;
    int start = 0;
    int end = symbol.length();
    std::vector<std::string> tuple;
    //DBGLOG(DBG,"Analyzing Term '" << symbol << "'");
    for (int pos = 0; pos < end; ++pos) {
        if (symbol[pos] == '\"' &&
            (pos == 0 || symbol[pos-1] != '\\')) quoted = !quoted;
        if (symbol[pos] == '(' && !quoted ) {
            if (nestedcount == 0) {
                primitive = false;
                tuple.push_back(symbol.substr(start, pos - start));
                start = pos + 1;
                // eliminate closing bracket
                assert(symbol[end-1] == ')');
                end--;
            }
            nestedcount++;
        }
        if (symbol[pos] == ')' && !quoted) {
            nestedcount--;
        }
        if (symbol[pos] == ',' && !quoted && nestedcount == 1) {
            tuple.push_back(symbol.substr(start, pos - start));
            start = pos + 1;
        }
        if (pos == end - 1) {
            tuple.push_back(symbol.substr(start, pos - start + 1));
        }
    }
    // we did not find a ( -> it is primitive, or
    // we came into (, increased by one, and eliminated the closing )
    // therefore if it is not primitive we must leave the loop at 1
    assert(primitive || nestedcount == 1);
    #ifndef NDEBUG
    {
        std::stringstream ss;
        ss << "Term tuple: ";
        bool first = true;
        BOOST_FOREACH (std::string str, tuple) {
            if (!first) ss << ", ";
            first = false;
            ss << str;
        }
        DBGLOG(DBG, ss.str());
    }
    #endif

    // convert tuple of strings to terms
    arguments.clear();
    if (primitive) {
        // no arguments
        if (islower(symbol[0]) || symbol[0] == '\"') kind |= ID::SUBKIND_TERM_CONSTANT;
        if (isupper(symbol[0])) kind |= ID::SUBKIND_TERM_VARIABLE;
    }
    else {
        BOOST_FOREACH (std::string str, tuple) {
            Term t(ID::MAINKIND_TERM, str);
            t.analyzeTerm(reg);
            if (t.arguments[0] == ID_FAIL) {
                if (islower(t.symbol[0]) || t.symbol[0] == '\"') t.kind |= ID::SUBKIND_TERM_CONSTANT;
                if (isupper(t.symbol[0])) t.kind |= ID::SUBKIND_TERM_VARIABLE;
            }
            else {
                t.kind |= ID::SUBKIND_TERM_NESTED;
            }
            ID tid = reg->storeTerm(t);
            arguments.push_back(tid);
        }
        kind |= ID::SUBKIND_TERM_NESTED;
    }
}


DLVHEX_NAMESPACE_END

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
