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
 * @file Registry.hpp
 * @author Peter Schueller
 * @date
 *
 * @brief Registry for program objects, addressed by IDs, organized in individual tables.
 */

#ifndef REGISTRY_HPP_INCLUDED_14012011
#define REGISTRY_HPP_INCLUDED_14012011

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/Printhelpers.hpp"
#include "dlvhex/ID.hpp"
#include "dlvhex/TermTable.hpp"
#include "dlvhex/OrdinaryAtomTable.hpp"
#include "dlvhex/BuiltinAtomTable.hpp"
#include "dlvhex/AggregateAtomTable.hpp"
#include "dlvhex/ExternalAtomTable.hpp"
#include "dlvhex/RuleTable.hpp"

#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

#warning namespaces
/*
typedef boost::bimaps::bimap<
  boost::bimaps::set_of<std::string>,
  boost::bimaps::set_of<std::string> > NamespaceTable;
  */

/**
 * @brief Registry for entities used in programs as IDs (collection of symbol tables)
 */
struct Registry:
  public ostream_printable<Registry>
{
  TermTable terms;
  // ordinary ground atoms
  OrdinaryAtomTable ogatoms;
  // ordinary nonground atoms
  OrdinaryAtomTable onatoms;
  BuiltinAtomTable batoms;
  AggregateAtomTable aatoms;
  ExternalAtomTable eatoms;
  RuleTable rules;

#warning namespaces
  //NamespaceTable namespaces;

#warning possible efficiency improvement
	#if 0
	
	// this can be done later, for now we can use hashtables and forget this more efficient method, TODO check how often this mapping is actually required

	//
	// "address range" concept
	//
	// from IDKind we obtain integers starting at zero,
	// for each distinct table a separate integer
	// this way we can create efficient mappings from IDs of various kinds to use mapKindToAddressRange() method
	// e.g., for looking up vertices in dependency graph by ID
	//   -> first lookup O(1) by IDKind, then lookup vertex in O(1) by address in vector
	//   -> vector storage with no useless storage allocation (one vector for each address range)
	enum AddressRange
	{
		ARTERM = 0,
		AROATOM,
		ARONATOM,
		ARBATOM,
		ARAATOM,
		AREATOM,
		ARRULE,
		AR_COUNT // this must stay the last entry
	};
	static inline AddressRange mapKindToAddressRange(IDKind kind);
	static inline AddressRange maxAddressRange() { return AR_COUNT; }
	#endif

  virtual std::ostream& print(std::ostream& o) const;
  // lookup ground or nonground ordinary atoms (ID specifies this)
  const OrdinaryAtom& lookupOrdinaryAtom(ID id) const;
};

typedef boost::shared_ptr<Registry> RegistryPtr;

DLVHEX_NAMESPACE_END

#endif // REGISTRY_HPP_INCLUDED_14012011
