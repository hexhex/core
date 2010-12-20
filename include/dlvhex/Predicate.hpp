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
 * @file   Predicate.hpp
 * @author Tri Kurniawan Wijaya <trikurniawanwijaya@gmail.com>
 * 
 * @brief  Predicate class: stores predicate and its arity.
 *
 */

#ifndef PREDICATE_HPP_INCLUDED__20122010
#define PREDICATE_HPP_INCLUDED__20122010

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/ID.hpp"
#include "dlvhex/Logger.hpp"

DLVHEX_NAMESPACE_BEGIN

// Anonymous variables: are parsed as one variable "_".
// Then they are processed into new distinct variables,
// each with the anonymous bit set and with a new ID.
struct Predicate:
  private ostream_printable<Predicate>
{
  // the kind part of the ID of this symbol
  IDKind kind;

  // the textual representation of a
	//  constant,
	//  constant string (including ""), or
	//  variable
  std::string symbol;
  int arity;

  Term(IDKind kind, const std::string& symbol, const int& arity):
    kind(kind), symbol(symbol), arity(arity)
		{ assert(ID(kind,0).isTerm()); }
  std::ostream& print(std::ostream& o) const
    { return o << "Predicate(" << symbol << "/" << arity << ")"; }
};

DLVHEX_NAMESPACE_END

#endif // PREDICATE_HPP_INCLUDED__20122010
