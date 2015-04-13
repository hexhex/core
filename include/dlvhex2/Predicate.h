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
 * @file   Predicate.h
 * @author Tri Kurniawan Wijaya <trikurniawanwijaya@gmail.com>
 *
 * @brief  Predicate structure: stores predicate and its arity.
 *
 */

#ifndef PREDICATE_HPP_INCLUDED__20122010
#define PREDICATE_HPP_INCLUDED__20122010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/Logger.h"

DLVHEX_NAMESPACE_BEGIN

/** \brief Predicate structure, used in PredicateTable.h. */
struct Predicate:
private ostream_printable<Predicate>
{
    /** \brief The kind part of the ID of this symbol. */
    IDKind kind;
    /** \brief The actual predicate as string. */
    std::string symbol;
    /** \brief The arity of the predicate. */
    int arity;

    /** \brief Constructor.
     * @param kind See Predicate::kind.
     * @param symbol See Predicate::symbol.
     * @param arity See Predicate::arity. */
    Predicate(IDKind kind, const std::string& symbol, const int& arity): kind(kind), symbol(symbol), arity(arity)
        { assert(ID(kind,0).isTerm()); }
    /** \brief Writes the predicate p and its arity a as string of kind "Predicate(p,a)" to stream \p o.
     * @param o The output stream to write to.
     * @return \p o. */
    std::ostream& print(std::ostream& o) const
        { return o << "Predicate(" << symbol << " / " << arity << ")"; }
};

WARNING("see warning in PredicateTable.h")
const Predicate PREDICATE_FAIL(ID::MAINKIND_TERM | ID::SUBKIND_TERM_PREDICATE, "", -1);

DLVHEX_NAMESPACE_END
#endif                           // PREDICATE_HPP_INCLUDED__20122010
