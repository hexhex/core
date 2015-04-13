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
 * @file ID.cpp
 * @author Peter Schueller
 *
 * @brief Implementation of the ID concept.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#include "dlvhex2/ID.h"
#include <boost/functional/hash.hpp>
#include <iomanip>

DLVHEX_NAMESPACE_BEGIN

std::size_t hash_value(const ID& id)
{
    std::size_t seed = 0;
    boost::hash_combine(seed, id.kind);
    boost::hash_combine(seed, id.address);
    return seed;
}


std::ostream& ID::print(std::ostream& o) const
{
    if( *this == ID_FAIL )
        return o << "ID_FAIL";
    o << "ID(0x" <<
        std::setfill('0') << std::hex << std::setw(8) << kind << "," << std::setfill(' ') <<
        std::dec << std::setw(4) << address;

    if( !!(kind & NAF_MASK) )
        o << " naf";
    const unsigned MAINKIND_MAX = 4;
    const char* mainkinds[MAINKIND_MAX] = {
        " atom",
        " term",
        " literal",
        " rule",
    };
    const unsigned mainkind = (kind & MAINKIND_MASK) >> MAINKIND_SHIFT;
    assert(mainkind < MAINKIND_MAX);
    o << mainkinds[mainkind];

    const unsigned SUBKIND_MAX = 11;
    const char* subkinds[MAINKIND_MAX][SUBKIND_MAX] = {
        { " ordinary_ground", " ordinary_nonground", " builtin",         " aggregate", "", "", " external", "", "", "", " module"},
        { " constant",        " integer",            " variable",        " builtin",   " predicate", " nested", ""          },
        { " ordinary_ground", " ordinary_nonground", " builtin",         " aggregate", "", "", " external", "", "", "", " module"},
        { " regular",         " constraint",         " weak_constraint", "weight_rule",           "", "", ""          }
    };
    const unsigned subkind = (kind & SUBKIND_MASK) >> SUBKIND_SHIFT;
    assert(subkind < SUBKIND_MAX);
    assert(subkinds[mainkind][subkind][0] != 0);
    o << subkinds[mainkind][subkind];
    return o << ")";
}


// returns builtin term ID
// static
ID ID::termFromBuiltinString(const std::string& op)
{
    assert(!op.empty());
    switch(op.size()) {
        case 1:
            switch(op[0]) {
                case '=': return ID::termFromBuiltin(ID::TERM_BUILTIN_EQ);
                case '<': return ID::termFromBuiltin(ID::TERM_BUILTIN_LT);
                case '>': return ID::termFromBuiltin(ID::TERM_BUILTIN_GT);
                case '*': return ID::termFromBuiltin(ID::TERM_BUILTIN_MUL);
                case '+': return ID::termFromBuiltin(ID::TERM_BUILTIN_ADD);
                case '-': return ID::termFromBuiltin(ID::TERM_BUILTIN_SUB);
                case '/': return ID::termFromBuiltin(ID::TERM_BUILTIN_DIV);
                default: assert(false); return ID_FAIL;
            }
        case 2:
            if( op == "==" ) {
                return ID::termFromBuiltin(ID::TERM_BUILTIN_EQ);
            }
            else if( op == "!=" || op == "<>" ) {
                return ID::termFromBuiltin(ID::TERM_BUILTIN_NE);
            }
            else if( op == "<=" ) {
                return ID::termFromBuiltin(ID::TERM_BUILTIN_LE);
            }
            else if( op == ">=" ) {
                return ID::termFromBuiltin(ID::TERM_BUILTIN_GE);
            }
            else {
                assert(false); return ID_FAIL;
            }
    }
    if( op == "#succ" )
        { return ID::termFromBuiltin(ID::TERM_BUILTIN_SUCC); }
        else if( op == "#int" )
            { return ID::termFromBuiltin(ID::TERM_BUILTIN_INT); }
            else if( op == "#count" )
                { return ID::termFromBuiltin(ID::TERM_BUILTIN_AGGCOUNT); }
                else if( op == "#min" )
                    { return ID::termFromBuiltin(ID::TERM_BUILTIN_AGGMIN); }
                    else if( op == "#max" )
                        { return ID::termFromBuiltin(ID::TERM_BUILTIN_AGGMAX); }
                        else if( op == "#sum" )
                            { return ID::termFromBuiltin(ID::TERM_BUILTIN_AGGSUM); }
                            else if( op == "#times" )
                                { return ID::termFromBuiltin(ID::TERM_BUILTIN_AGGTIMES); }
                                else if( op == "#avg" )
                                    { return ID::termFromBuiltin(ID::TERM_BUILTIN_AGGAVG); }
                                    else if( op == "#any" )
                                        { return ID::termFromBuiltin(ID::TERM_BUILTIN_AGGANY); }
                                        else if( op == "#mod" )
                                            { return ID::termFromBuiltin(ID::TERM_BUILTIN_MOD); }
                                        else {
                                            assert(false);
        return ID_FAIL;
    }
}


namespace
{
    const char* builtinTerms[] = {
        "=",
        "!=",
        "<",
        "<=",
        ">",
        ">=",
        "*",
        "+",
        "-",
        "/",
        "#count",
        "#min",
        "#max",
        "#sum",
        "#times",
        "#avg",
        "#any",
        "#int",
        "#succ",
        "#mod",
    };
}


const char* ID::stringFromBuiltinTerm(IDAddress addr)
{
    assert(addr < (sizeof(builtinTerms)/sizeof(const char*)));
    return builtinTerms[addr];
}


DLVHEX_NAMESPACE_END
