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
 * @file   Term.hpp
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

DLVHEX_NAMESPACE_BEGIN

// Anonymous variables: are parsed as one variable "_".
// Then they are processed into new distinct variables,
// each with the anonymous bit set and with a new ID.
struct Term:
private ostream_printable<Term>
{
	// the kind part of the ID of this symbol
	IDKind kind;

	// the textual representation of a
	//  constant,
	//  constant string (including ""), or
	//  variable
	std::string symbol;

	Term(IDKind kind, const std::string& symbol): kind(kind), symbol(symbol) { 
		assert(ID(kind,0).isTerm()); 
	}
	
	bool isQuotedString() const {
		if ((symbol.at(0) == '"') && (symbol.at(symbol.length()-1) == '"'))
			return true;
		return false;
	}
	
	std::string getQuotedString() const {
		return '"' + getUnquotedString() + '"';
	}
	
	std::string getUnquotedString() const {;
		if (isQuotedString())
			return symbol.substr(1, symbol.length()-2);
		return symbol;
	}
	
  // the following method is not useful, as integers are always
  // represented in the ID.address field and never stored into a table
  // int getInt() const
	
	std::ostream& print(std::ostream& o) const { 
		return o << "Term(" << symbol << ")"; 
	}
};

DLVHEX_NAMESPACE_END

#endif // TERM_HPP_INCLUDED__12102010
