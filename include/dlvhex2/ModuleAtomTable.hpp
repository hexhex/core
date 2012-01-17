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
 * @file   ModuleAtomTable.hpp
 * @author Tri Kurniawan Wijaya <trikurniawanwijaya@gmail.com>
 * 
 * @brief  Table for storing Module Atoms (module calls --> @p[q1,q2]::r(a,b,c) )
 */

#ifndef MODULEATOMTABLE_HPP_INCLUDED__27112010
#define MODULEATOMTABLE_HPP_INCLUDED__27112010

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/Atoms.hpp"
#include "dlvhex/Table.hpp"

#include <boost/multi_index/member.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/random_access_index.hpp>

DLVHEX_NAMESPACE_BEGIN

class ModuleAtomTable:
	public Table<
		// value type is symbol struct
		ModuleAtom,
		// index is
		boost::multi_index::indexed_by<
			// address = running ID for constant access
			boost::multi_index::random_access<
				boost::multi_index::tag<impl::AddressTag>
			>,
			boost::multi_index::hashed_non_unique<
				boost::multi_index::tag<impl::PredicateTag>,
				BOOST_MULTI_INDEX_MEMBER(ModuleAtom,ID,predicate)
			>,
			boost::multi_index::hashed_unique<
				boost::multi_index::tag<impl::ElementTag>,
				boost::multi_index::composite_key<
				  ModuleAtom,
				  BOOST_MULTI_INDEX_MEMBER(ModuleAtom,ID,predicate),
				  BOOST_MULTI_INDEX_MEMBER(ModuleAtom,Tuple,inputs),
				  BOOST_MULTI_INDEX_MEMBER(ModuleAtom,ID,outputAtom)
				>
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
  typedef AddressIndex::iterator AddressIterator;
  typedef Container::index<impl::PredicateTag>::type PredicateIndex;
  typedef PredicateIndex::iterator PredicateIterator;
  typedef Container::index<impl::ElementTag>::type ElementIndex;
  typedef ElementIndex::iterator ElementIterator;

	// methods
public:
	// get the ModuleAtom by ID
	inline const ModuleAtom& getByID(ID id) const throw ();

        // get the ID of the module atom with predicate, inputs, and output atom specified
	inline ID getIDByElement(ID predicate1, const Tuple& inputs1, ID outputAtom1) const throw();

        // get all module atoms with certain predicate id
	inline std::pair<PredicateIterator, PredicateIterator>
	getRangeByPredicateID(ID id) const throw();

        // get range over all atoms sorted by address
        inline std::pair<AddressIterator, AddressIterator>
        getAllByAddress() const throw();

	// store atom, assuming it does not exist
	inline ID storeAndGetID(const ModuleAtom& atom) throw();

	// update
	// (oldStorage must be from getByID() or from *const_iterator)
	inline void update(
			const ModuleAtom& oldStorage, ModuleAtom& newStorage) throw();
};

// retrieve by ID
// assert that id.kind is correct for Term
// assert that ID exists
const ModuleAtom&
ModuleAtomTable::getByID(ID id) const throw ()
{
	assert(id.isAtom() || id.isLiteral());
	assert(id.isModuleAtom());

  ReadLock lock(mutex);
  const AddressIndex& idx = container.get<impl::AddressTag>();
  // the following check only works for random access indices, but here it is ok
  assert( id.address < idx.size() );
  return idx.at(id.address);
}

// @p2[q1,q2]::r(a)
// predicate = p2
// inputs = <q1, q2> = tuple
// outputAtom = r(a) 
ID ModuleAtomTable::getIDByElement(ID predicate1, const Tuple& inputs1, ID outputAtom1) const throw()
{
  ReadLock lock(mutex);
  const ElementIndex& sidx = container.get<impl::ElementTag>();
  ElementIndex::const_iterator it = sidx.find( boost::make_tuple(predicate1, inputs1, outputAtom1) );
  if( it == sidx.end() )
    return ID_FAIL;
  else
    {
      const AddressIndex& aidx = container.get<impl::AddressTag>();
      return ID(
		it->kind, // kind
		container.project<impl::AddressTag>(it) - aidx.begin() // address
		);
    }
}

// get all external atoms with certain predicate id
std::pair<ModuleAtomTable::PredicateIterator, ModuleAtomTable::PredicateIterator>
ModuleAtomTable::getRangeByPredicateID(ID id) const throw()
{
	assert(id.isTerm() && id.isConstantTerm());
  #warning this read-only iteration will probably need to be mutexed too!
  ReadLock lock(mutex);
  const PredicateIndex& idx = container.get<impl::PredicateTag>();
	return idx.equal_range(id);
}

// get range over all atoms sorted by address
std::pair<ModuleAtomTable::AddressIterator, ModuleAtomTable::AddressIterator>
ModuleAtomTable::getAllByAddress() const throw()
{
  #warning this read-only iteration will probably need to be mutexed too!
  ReadLock lock(mutex);
  const AddressIndex& idx = container.get<impl::AddressTag>();
	return std::make_pair(idx.begin(), idx.end());
}

// store symbol, assuming it does not exist (this is only asserted)
ID ModuleAtomTable::storeAndGetID(
		const ModuleAtom& atm) throw()
{
	assert(ID(atm.kind,0).isAtom());
	assert(ID(atm.kind,0).isModuleAtom());

	AddressIndex::const_iterator it;
	bool success;

  ReadLock lock(mutex);
	AddressIndex& idx = container.get<impl::AddressTag>();
  boost::tie(it, success) = idx.push_back(atm);
	(void)success;
	assert(success);

	return ID(
			atm.kind, // kind
			container.project<impl::AddressTag>(it) - idx.begin() // address
			);
}

void ModuleAtomTable::update(
		const ModuleAtom& oldStorage, ModuleAtom& newStorage) throw()
{
  bool success;

  WriteLock lock(mutex);
	AddressIndex& idx(container.get<impl::AddressTag>());
  AddressIndex::iterator it(idx.iterator_to(oldStorage));
  assert(it != idx.end());
  success = idx.replace(it, newStorage);
  (void)success;
	assert(success);
}

DLVHEX_NAMESPACE_END

#endif 
// BUILTINATOMTABLE_HPP_INCLUDED__12102010
