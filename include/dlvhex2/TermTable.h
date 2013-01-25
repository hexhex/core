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
 * @file   TermTable.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Table for storing Terms
 */

#ifndef TERMTABLE_HPP_INCLUDED__12102010
#define TERMTABLE_HPP_INCLUDED__12102010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/Term.h"
#include "dlvhex2/Table.h"

#include <boost/multi_index/member.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/random_access_index.hpp>

DLVHEX_NAMESPACE_BEGIN

class TermTable:
	public Table<
		// value type is symbol struct
		Term,
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
				BOOST_MULTI_INDEX_MEMBER(Term,IDKind,kind)
			>,
      #endif
			// unique IDs for unique symbol strings
			boost::multi_index::hashed_unique<
				boost::multi_index::tag<impl::TermTag>,
				BOOST_MULTI_INDEX_MEMBER(Term,std::string,symbol)
			>
		>
	>
{
	// types
public:
  typedef Container::index<impl::AddressTag>::type AddressIndex;
  //typedef Container::index<impl::KindTag>::type KindIndex;
	typedef Container::index<impl::TermTag>::type TermIndex;

	// methods
public:
  // retrieve by ID
  // assert that id.kind is correct for Term
  // assert that ID exists
	inline const Term& getByID(ID id) const throw ();

	// given string, look if already stored
	// if no, return ID_FAIL, otherwise return ID
	inline ID getIDByString(const std::string& str) const throw();

	// store symbol, assuming it does not exist
  // assert that symbol did not exist
	inline ID storeAndGetID(const Term& symb) throw();

  // retrieve range by kind (return lower/upper bound iterators, +provide method to get ID from iterator)
};

// retrieve by ID
// assert that id.kind is correct for Term
// assert that ID exists
const Term&
TermTable::getByID(
  ID id) const throw ()
{
	assert(id.isTerm());
	// integers are not allowed in this table!
	assert(id.isConstantTerm() || id.isVariableTerm() || id.isNullTerm());
  ReadLock lock(mutex);
  const AddressIndex& idx = container.get<impl::AddressTag>();
  // the following check only works for random access indices, but here it is ok
  assert( id.address < idx.size() );
  return idx.at(id.address);
}

// given string, look if already stored
// if no, return ID_FAIL, otherwise return ID
ID TermTable::getIDByString(
		const std::string& str) const throw()
{
	typedef Container::index<impl::TermTag>::type TermIndex;
  ReadLock lock(mutex);
	const TermIndex& sidx = container.get<impl::TermTag>();
	TermIndex::const_iterator it = sidx.find(str);
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

// store symbol, assuming it does not exist
// assert that symbol did not exist
ID TermTable::storeAndGetID(
		const Term& symb) throw()
{
	assert(ID(symb.kind,0).isTerm());
	// integers are not allowed in this table!
	assert(ID(symb.kind,0).isConstantTerm() || ID(symb.kind,0).isVariableTerm() || ID(symb.kind,0).isNullTerm());
	assert(!symb.symbol.empty());

  bool success;
  AddressIndex::const_iterator it;

  WriteLock lock(mutex);
  AddressIndex& idx = container.get<impl::AddressTag>();
  boost::tie(it, success) = idx.push_back(symb);
	(void)success;
	assert(success);

	return ID(
			symb.kind, // kind
			container.project<impl::AddressTag>(it) - idx.begin() // address
			);
}

DLVHEX_NAMESPACE_END

#endif // TERMTABLE_HPP_INCLUDED__12102010
