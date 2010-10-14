/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
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
 * @file   ID.hpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Header to help using and debugging within the ID concept (see /ID-Concept.txt)
 */

#ifndef ID_HPP_INCLUDED__08102010
#define ID_HPP_INCLUDED__08102010

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/Logger.hpp"
#include <boost/cstdint.hpp>

DLVHEX_NAMESPACE_BEGIN

typedef uint32_t IDKind;
typedef uint32_t IDAddress;

struct ID:
  private ostream_printable<ID>
{
	IDKind kind;
	IDAddress address;
	ID(IDKind kind, IDAddress address): kind(kind), address(address) {}
	// no virtual here!
	// this struct must fit into an uint64_t and have no vtable!

	static const uint32_t ALL_ONES =             0xFFFFFFFF;

	static const IDKind NAF_MASK =               0x80000000;
	static const IDKind MAINKIND_MASK =          0x70000000;
	static const uint8_t MAINKIND_SHIFT =        28;
	static const IDKind SUBKIND_MASK =           0x0F000000;
	static const uint8_t SUBKIND_SHIFT =         24;
	static const IDKind PROPERTY_MASK =          0x00FF0000;
	static const uint8_t PROPERTY_SHIFT =        16;
	static const IDKind UNUSED_MASK =            0x0000FFFF;

	static const IDKind MAINKIND_ATOM =          0x00000000;
	static const IDKind MAINKIND_TERM =          0x10000000;
	static const IDKind MAINKIND_LITERAL =       0x20000000;
	static const IDKind MAINKIND_RULE =          0x30000000;

	static const IDKind SUBKIND_TERM_CONSTANT =  0x00000000;
	static const IDKind SUBKIND_TERM_INTEGER =   0x01000000;
	static const IDKind SUBKIND_TERM_VARIABLE =  0x02000000;
	static const IDKind SUBKIND_TERM_BUILTIN =   0x03000000;
	static const IDKind SUBKIND_ATOM_ORDINARYG = 0x00000000;
	static const IDKind SUBKIND_ATOM_ORDINARYN = 0x01000000;
	static const IDKind SUBKIND_ATOM_BUILTIN =   0x02000000;
	static const IDKind SUBKIND_ATOM_AGGREGATE = 0x03000000;
	static const IDKind SUBKIND_ATOM_EXTERNAL =  0x06000000;

	static const IDKind PROPERTY_ANONYMOUS =     0x00010000;

  // for builtin terms, this is the address part (no table)
  enum TermBuiltinAddress
  {
    TERM_BUILTIN_EQ,
    TERM_BUILTIN_NE,
    TERM_BUILTIN_LT,
    TERM_BUILTIN_LE,
    TERM_BUILTIN_GT,
    TERM_BUILTIN_GE,
    TERM_BUILTIN_AGGSUM,
    TERM_BUILTIN_AGGCOUNT,
    TERM_BUILTIN_AGGMIN,
    TERM_BUILTIN_AGGAVG,
    TERM_BUILTIN_INT,
    TERM_BUILTIN_SUCC,
    TERM_BUILTIN_MUL,
    TERM_BUILTIN_ADD,
  };

	inline bool isTerm() const          { return (kind & MAINKIND_MASK) == MAINKIND_TERM; }
	inline bool isTermConstant() const  { assert(isTerm()); return (kind & SUBKIND_MASK) == SUBKIND_TERM_CONSTANT; }
	inline bool isTermInteger() const   { assert(isTerm()); return (kind & SUBKIND_MASK) == SUBKIND_TERM_INTEGER; }
	inline bool isTermVariable() const  { assert(isTerm()); return (kind & SUBKIND_MASK) == SUBKIND_TERM_VARIABLE; }
	inline bool isTermBuiltin() const   { assert(isTerm()); return (kind & SUBKIND_MASK) == SUBKIND_TERM_BUILTIN; }

	inline bool isAtom() const          { return (kind & MAINKIND_MASK) == MAINKIND_ATOM; }
  // ground or nonground atoms (due t o the special bits this can be checked by checking bit 2 of this field only)
	inline bool isOrdinaryAtom() const  { assert(isAtom() || isLiteral()); return (kind & SUBKIND_ATOM_BUILTIN) != SUBKIND_ATOM_BUILTIN; }
	inline bool isBuiltinAtom() const   { assert(isAtom() || isLiteral()); return (kind & SUBKIND_MASK) == SUBKIND_ATOM_BUILTIN; }
	inline bool isAggregateAtom() const { assert(isAtom() || isLiteral()); return (kind & SUBKIND_MASK) == SUBKIND_ATOM_AGGREGATE; }
	inline bool isExternalAtom() const  { assert(isAtom() || isLiteral()); return (kind & SUBKIND_MASK) == SUBKIND_ATOM_EXTERNAL; }

	inline bool isLiteral() const       { return (kind & MAINKIND_MASK) == MAINKIND_LITERAL; }
  
	inline bool isRule() const          { return (kind & MAINKIND_MASK) == MAINKIND_RULE; }

	inline bool operator==(const ID& id2) const { return kind == id2.kind && address == id2.address; }

	std::ostream& print(std::ostream& o) const;
};

std::size_t hash_value(const ID& id);

const ID        ID_FAIL(ID::ALL_ONES, ID::ALL_ONES);

typedef std::vector<ID> Tuple;

DLVHEX_NAMESPACE_END

#endif // ID_HPP_INCLUDED__08102010
