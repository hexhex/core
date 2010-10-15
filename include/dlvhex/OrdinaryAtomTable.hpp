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

#include <boost/multi_index/composite_key.hpp>

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
        boost::multi_index::composite_key<
          //TODO check if this is more efficient: boost::reference_wrapper<const OrdinaryAtom>,
          OrdinaryAtom,
          BOOST_MULTI_INDEX_MEMBER(OrdinaryAtom::Atom,Tuple,tuple),
          BOOST_MULTI_INDEX_MEMBER(OrdinaryAtom,bool,neg)
        >
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

	// methods
public:
  // retrieve by ID
  // assert that id.kind is correct for OrdinaryGroundAtom
  // assert that ID exists in table
	inline const OrdinaryAtom& getByID(ID id) const throw ();

	// given string, look if already stored
	// if no, return ID_FAIL, otherwise return ID
	inline ID getIDByString(const std::string& text) const throw();

	// given neg and tuple in (partially specified atom) pa, look if already stored
	// if no, return ID_FAIL, otherwise return ID
	inline ID getIDByNegTuple(bool neg, const Tuple& tuple) const throw();

	// store atom, assuming it does not exist
  // assert that atom did not exist in table
	inline ID storeAndGetID(const OrdinaryAtom& atom) throw();
};

// retrieve by ID
// assert that id.kind is correct for Term
// assert that ID exists
const OrdinaryAtom&
OrdinaryAtomTable::getByID(
  ID id) const throw ()
{
	assert(id.isAtom());
	assert(id.isOrdinaryAtom());
  const AddressIndex& idx = container.get<impl::AddressTag>();
  // the following check only works for random access indices, but here it is ok
  assert( id.address < idx.size() );
  return idx.at(id.address);
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

// given string, look if already stored
// if no, return ID_FAIL, otherwise return ID
ID OrdinaryAtomTable::getIDByNegTuple(
    bool neg, const Tuple& tuple) const throw()
{
	typedef Container::index<impl::TupleTag>::type TupleIndex;
	const TupleIndex& sidx = container.get<impl::TupleTag>();
  const TupleIndex::key_from_value& extractor = sidx.key_extractor();
	TupleIndex::const_iterator it = sidx.find(boost::make_tuple(tuple, neg));
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

// store symbol, assuming it does not exist (this is only asserted)
ID OrdinaryAtomTable::storeAndGetID(
		const OrdinaryAtom& atm) throw()
{
	assert(ID(atm.kind,0).isAtom());
	assert(ID(atm.kind,0).isOrdinaryAtom());
	assert(!atm.text.empty());

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

DLVHEX_NAMESPACE_END

#endif // ORDINARYATOMTABLE_HPP_INCLUDED__12102010
