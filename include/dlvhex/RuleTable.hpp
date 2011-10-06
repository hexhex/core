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
 * @file   RuleTable.hpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Table for storing Rules
 */

#ifndef RULETABLE_HPP_INCLUDED__12102010
#define RULETABLE_HPP_INCLUDED__12102010

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/fwd.hpp"
#include "dlvhex/Rule.hpp"
#include "dlvhex/Table.hpp"

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/composite_key.hpp>

DLVHEX_NAMESPACE_BEGIN

class RuleTable:
	public Table<
		// value type is symbol struct
		Rule,
		// index is
		boost::multi_index::indexed_by<
			// address = running ID for constant access
			boost::multi_index::random_access<
				boost::multi_index::tag<impl::AddressTag>
			>,
			// kind
			boost::multi_index::ordered_non_unique<
				boost::multi_index::tag<impl::KindTag>,
				BOOST_MULTI_INDEX_MEMBER(Rule,IDKind,kind)
			>,
			// element
			boost::multi_index::hashed_unique<
				boost::multi_index::tag<impl::ElementTag>,
				boost::multi_index::composite_key<
				  Rule,
				  BOOST_MULTI_INDEX_MEMBER(Rule,IDKind,kind),
				  BOOST_MULTI_INDEX_MEMBER(Rule,Tuple,head),
				  BOOST_MULTI_INDEX_MEMBER(Rule,Tuple,body),
				  BOOST_MULTI_INDEX_MEMBER(Rule,ID,weight),
				  BOOST_MULTI_INDEX_MEMBER(Rule,ID,level)
				>
			>
      // TODO more indices required?
		>
	>
{
	// types
public:
  typedef Container::index<impl::AddressTag>::type AddressIndex;
  typedef Container::index<impl::KindTag>::type KindIndex;
  typedef Container::index<impl::ElementTag>::type ElementIndex;
  typedef ElementIndex::iterator ElementIterator;
	// methods
public:
  // retrieve by ID
  // assert that id.kind is correct for Rule
  // assert that ID exists in table
	inline const Rule& getByID(ID id) const throw ();

        // get the ID of the rule
	inline ID getIDByElement(const Rule& rule) const throw();

	// store rule (no duplicate check is done/required)
	inline ID storeAndGetID(const Rule& rule) throw();
	inline void clear();

	// update
	// (oldStorage must be from getByID() or from *const_iterator)
	inline void update(
			const Rule& oldStorage, Rule& newStorage) throw();

	// implementation in Registry.cpp !
	std::ostream& print(std::ostream& o, RegistryPtr reg) const throw();
};

// retrieve by ID
// assert that id.kind is correct for Rule
// assert that ID exists
const Rule&
RuleTable::getByID(
  ID id) const throw ()
{
	assert(id.isRule());
	assert(id.isRegularRule() || id.isConstraint() || id.isWeakConstraint());
  ReadLock lock(mutex);
  const AddressIndex& idx = container.get<impl::AddressTag>();
  // the following check only works for random access indices, but here it is ok
  assert( id.address < idx.size() );
  return idx.at(id.address);
}


// getID by rule
ID RuleTable::getIDByElement(const Rule& rule) const throw()
{
  ReadLock lock(mutex);
  const ElementIndex& sidx = container.get<impl::ElementTag>();
  ElementIndex::const_iterator it = sidx.find( boost::make_tuple(rule.kind, rule.head, rule.body, rule.weight, rule.level) );
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

// store rule (no duplicate check is done/required)
ID RuleTable::storeAndGetID(
		const Rule& rule) throw()
{
	assert(ID(rule.kind,0).isRule());
	assert(ID(rule.kind,0).isRegularRule() ||
    ID(rule.kind,0).isConstraint() ||
    ID(rule.kind,0).isWeakConstraint());
	assert(!(rule.head.empty() && rule.body.empty()));
	assert(!(rule.head.empty() && ID(rule.kind,0).isRegularRule()));
	assert(!(rule.head.size() > 1 && !ID(rule.kind,0).isRuleDisjunctive()));
 
	AddressIndex::const_iterator it;
	bool success;

  WriteLock lock(mutex);
	AddressIndex& idx = container.get<impl::AddressTag>();
  boost::tie(it, success) = idx.push_back(rule);
	(void)success;
	assert(success);

	return ID(
			rule.kind, // kind
			container.project<impl::AddressTag>(it) - idx.begin() // address
			);
}

void RuleTable::clear()
{
  WriteLock lock(mutex);
  container.clear();
}

void RuleTable::update(
		const Rule& oldStorage, Rule& newStorage) throw()
{
  bool success;

  WriteLock lock(mutex);
	AddressIndex& idx = container.get<impl::AddressTag>();
  AddressIndex::iterator it(idx.iterator_to(oldStorage));
  assert(it != idx.end());
  success = idx.replace(it, newStorage);
  (void)success;
	assert(success);
}

DLVHEX_NAMESPACE_END

#endif // RULETABLE_HPP_INCLUDED__12102010
