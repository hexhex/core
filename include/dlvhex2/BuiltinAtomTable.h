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
 * @file   BuiltinAtomTable.hpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Table for storing Builtin Atoms
 */

#ifndef BUILTINATOMTABLE_HPP_INCLUDED__12102010
#define BUILTINATOMTABLE_HPP_INCLUDED__12102010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/Atoms.h"
#include "dlvhex2/Table.h"

DLVHEX_NAMESPACE_BEGIN

class BuiltinAtomTable:
	public Table<
		// value type is symbol struct
		BuiltinAtom,
		// index is
		boost::multi_index::indexed_by<
			// address = running ID for constant access
			boost::multi_index::random_access<
				boost::multi_index::tag<impl::AddressTag>
			>
      #if 0
			// kind TODO perhaps we do not need this index?
			boost::multi_index::ordered_non_unique<
				boost::multi_index::tag<impl::KindTag>,
				BOOST_MULTI_INDEX_MEMBER(OrdinaryAtom,IDKind,kind)
			>,
      #endif
		>
	>
{
	// types
public:
  typedef Container::index<impl::AddressTag>::type AddressIndex;
  //typedef Container::index<impl::KindTag>::type KindIndex;
	//typedef Container::index<impl::TupleTag>::type TupleIndex;

	// methods
public:
  // retrieve by ID
  // assert that id.kind is correct
  // assert that ID exists in table
	inline const BuiltinAtom& getByID(ID id) const throw ();

	// store atom, assuming it does not exist
	inline ID storeAndGetID(const BuiltinAtom& atom) throw();
};

// retrieve by ID
// assert that id.kind is correct for Term
// assert that ID exists
const BuiltinAtom&
BuiltinAtomTable::getByID(
  ID id) const throw ()
{
	assert(id.isAtom() || id.isLiteral());
	assert(id.isBuiltinAtom());
  ReadLock lock(mutex);
  const AddressIndex& idx = container.get<impl::AddressTag>();
  // the following check only works for random access indices, but here it is ok
  assert( id.address < idx.size() );
  return idx.at(id.address);
}

// store symbol, assuming it does not exist (this is only asserted)
ID BuiltinAtomTable::storeAndGetID(
		const BuiltinAtom& atm) throw()
{
	assert(ID(atm.kind,0).isAtom());
	assert(ID(atm.kind,0).isBuiltinAtom());
	assert(!atm.tuple.empty());

	bool success;
	AddressIndex::const_iterator it;

  WriteLock lock(mutex);
	AddressIndex& idx = container.get<impl::AddressTag>();
  boost::tie(it, success) = idx.push_back(atm);
	(void)success;
	assert(success);

	return ID(
			atm.kind, // kind
			container.project<impl::AddressTag>(it) - idx.begin() // address
			);
}

DLVHEX_NAMESPACE_END

#endif // BUILTINATOMTABLE_HPP_INCLUDED__12102010
