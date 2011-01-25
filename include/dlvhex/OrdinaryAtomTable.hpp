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
 * @file   OrdinaryAtomTable.hpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Table for storing Ordinary Atoms (ground or nonground)
 */

#ifndef ORDINARYATOMTABLE_HPP_INCLUDED__12102010
#define ORDINARYATOMTABLE_HPP_INCLUDED__12102010

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/Atoms.hpp"
#include "dlvhex/Table.hpp"

#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/random_access_index.hpp>

DLVHEX_NAMESPACE_BEGIN

class OrdinaryAtomTable:
	public Table<
		// value type is symbol struct
		OrdinaryAtom,
		// index is
		boost::multi_index::indexed_by<
			// address = running ID for constant access
			boost::multi_index::random_access<
				boost::multi_index::tag<impl::AddressTag>
			>,
      #if 0
			// kind TODO perhaps we do not need this index?
			boost::multi_index::ordered_non_unique<
				boost::multi_index::tag<impl::KindTag>,
				BOOST_MULTI_INDEX_MEMBER(OrdinaryAtom,IDKind,kind)
			>,
      #endif
			// (unique) textual representation (parsing index, see TODO above)
			boost::multi_index::hashed_unique<
				boost::multi_index::tag<impl::TextTag>,
				BOOST_MULTI_INDEX_MEMBER(OrdinaryAtom,std::string,text)
			>,
			// unique IDs for unique tuples
			boost::multi_index::hashed_unique<
				boost::multi_index::tag<impl::TupleTag>,
				BOOST_MULTI_INDEX_MEMBER(OrdinaryAtom::Atom,Tuple,tuple)
			>,
			boost::multi_index::hashed_non_unique<
				boost::multi_index::tag<impl::PredicateTag>,
        // we cannot use BOOST_MULTI_INDEX_CONST_MEM_FUN here, it required MemberFunName to be in Class
				//BOOST_MULTI_INDEX_CONST_MEM_FUN(OrdinaryAtom,ID,front)
        boost::multi_index::const_mem_fun_explicit<OrdinaryAtom,ID,
           ID (Atom::*)() const,&Atom::front>
			>
		>
	>
{
	// types
public:
  typedef Container::index<impl::AddressTag>::type AddressIndex;
  //typedef Container::index<impl::KindTag>::type KindIndex;
	typedef Container::index<impl::TextTag>::type TextIndex;
	typedef Container::index<impl::TupleTag>::type TupleIndex;
  typedef Container::index<impl::PredicateTag>::type PredicateIndex;
	typedef AddressIndex::iterator AddressIterator;
	typedef PredicateIndex::iterator PredicateIterator;

	// methods
public:
  // retrieve by ID
  // assert that id.kind is correct for OrdinaryGroundAtom
  // assert that ID exists in table
	inline const OrdinaryAtom& getByID(ID id) const throw ();

  // retrieve by address (ignore kind)
  // assert that address exists in table
	inline const OrdinaryAtom& getByAddress(IDAddress addr) const throw ();

	// given string, look if already stored
	// if no, return ID_FAIL, otherwise return ID
	inline ID getIDByString(const std::string& text) const throw();

	// given tuple, look if already stored
	// if no, return ID_FAIL, otherwise return ID
	inline ID getIDByTuple(const Tuple& tuple) const throw();

  // get ID given storage retrieved by other means
  // (storage must have originated from iterator from here)
	inline ID getIDByStorage(const OrdinaryAtom& atom) const throw ();

	// store atom, assuming it does not exist
  // assert that atom did not exist in table
	inline ID storeAndGetID(const OrdinaryAtom& atom) throw();

  // get all ordinary atoms with certain predicate id
	inline std::pair<PredicateIterator, PredicateIterator>
	getRangeByPredicateID(ID id) const throw();

  // get range over all atoms sorted by address
	inline std::pair<AddressIterator, AddressIterator>
	getAllByAddress() const throw();
};

// retrieve by ID
// assert that id.kind is correct for Term
// assert that ID exists
const OrdinaryAtom&
OrdinaryAtomTable::getByID(
  ID id) const throw ()
{
	assert(id.isAtom() || id.isLiteral());
	assert(id.isOrdinaryAtom());
  const AddressIndex& idx = container.get<impl::AddressTag>();
  // the following check only works for random access indices, but here it is ok
  assert( id.address < idx.size() );
  return idx.at(id.address);
}

// retrieve by address (ignore kind)
// assert that address exists in table
const OrdinaryAtom&
OrdinaryAtomTable::getByAddress(
    IDAddress addr) const throw ()
{
  const AddressIndex& idx = container.get<impl::AddressTag>();
  // the following check only works for random access indices, but here it is ok
  assert( addr < idx.size() );
  return idx.at(addr);
}

// given string, look if already stored
// if no, return ID_FAIL, otherwise return ID
ID OrdinaryAtomTable::getIDByString(
		const std::string& str) const throw()
{
	typedef Container::index<impl::TextTag>::type OrdinaryAtomIndex;
	const TextIndex& sidx = container.get<impl::TextTag>();
	TextIndex::const_iterator it = sidx.find(str);
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

// given tuple, look if already stored
// if no, return ID_FAIL, otherwise return ID
ID OrdinaryAtomTable::getIDByTuple(
    const Tuple& tuple) const throw()
{
	typedef Container::index<impl::TupleTag>::type TupleIndex;
	const TupleIndex& sidx = container.get<impl::TupleTag>();
	TupleIndex::const_iterator it = sidx.find(tuple);
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

// get ID given storage retrieved by other means
// (storage must have originated from iterator from here)
ID OrdinaryAtomTable::getIDByStorage(
    const OrdinaryAtom& atom) const throw ()
{
  // we cannot assert anything really useful here!
  // (if the user specifies another storage, iterator_to will segfault
  //  anyway as there is no associated internal multi_index storage node)
  const AddressIndex& aidx = container.get<impl::AddressTag>();
  AddressIndex::const_iterator it = aidx.iterator_to(atom);
  assert(atom.kind == it->kind);
  return ID(
      atom.kind, // kind
      it - aidx.begin() // address
      );
}

// store symbol, assuming it does not exist (this is only asserted)
ID OrdinaryAtomTable::storeAndGetID(
		const OrdinaryAtom& atm) throw()
{
	assert(ID(atm.kind,0).isAtom());
	assert(ID(atm.kind,0).isOrdinaryAtom());
	assert(!atm.text.empty());
  assert(!(
      (atm.tuple.front().kind & ID::PROPERTY_TERM_AUX) != 0 &&
      (atm.kind & ID::PROPERTY_ATOM_AUX) == 0 ) &&
      "atom must be auxiliary if predicate term is auxiliary");

	AddressIndex& idx = container.get<impl::AddressTag>();

	AddressIndex::const_iterator it;
	bool success;
	boost::tie(it, success) = idx.push_back(atm);
	(void)success;
	assert(success);

	return ID(
			atm.kind, // kind
			container.project<impl::AddressTag>(it) - idx.begin() // address
			);
}

// get all ordinary atoms with certain predicate id
std::pair<OrdinaryAtomTable::PredicateIterator, OrdinaryAtomTable::PredicateIterator>
OrdinaryAtomTable::getRangeByPredicateID(ID id) const throw()
{
	assert(id.isTerm());
  const PredicateIndex& idx = container.get<impl::PredicateTag>();
	return idx.equal_range(id);
}

// get range over all atoms sorted by address
std::pair<OrdinaryAtomTable::AddressIterator, OrdinaryAtomTable::AddressIterator>
OrdinaryAtomTable::getAllByAddress() const throw()
{
  const AddressIndex& idx = container.get<impl::AddressTag>();
	return std::make_pair(idx.begin(), idx.end());
}

DLVHEX_NAMESPACE_END

#endif // ORDINARYATOMTABLE_HPP_INCLUDED__12102010
