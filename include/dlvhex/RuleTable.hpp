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
#include "dlvhex/Rule.hpp"
#include "dlvhex/Table.hpp"

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
			>
      // TODO more indices require?
		>
	>
{
	// types
public:
  typedef Container::index<impl::AddressTag>::type AddressIndex;
  typedef Container::index<impl::KindTag>::type KindIndex;

	// methods
public:
  // retrieve by ID
  // assert that id.kind is correct for Rule
  // assert that ID exists in table
	inline const Rule& getByID(ID id) const throw ();

	// store rule (no duplicate check is done/required)
	inline ID storeAndGetID(const Rule& rule) throw();
};

// retrieve by ID
// assert that id.kind is correct for Rule
// assert that ID exists
const Rule&
RuleTable::getByID(
  ID id) const throw ()
{
	assert(id.isRule());
	assert(id.isRuleRegular() || id.isRuleConstraint() || id.isRuleWeakConstraint());
  const AddressIndex& idx = container.get<impl::AddressTag>();
  // the following check only works for random access indices, but here it is ok
  assert( id.address < idx.size() );
  return idx.at(id.address);
}

// store rule (no duplicate check is done/required)
ID RuleTable::storeAndGetID(
		const Rule& rule) throw()
{
	assert(ID(rule.kind,0).isRule());
	assert(ID(rule.kind,0).isRuleRegular() ||
    ID(rule.kind,0).isRuleConstraint() ||
    ID(rule.kind,0).isRuleWeakConstraint());
	assert(!(rule.head.empty() && rule.body.empty()));

	AddressIndex& idx = container.get<impl::AddressTag>();

	AddressIndex::const_iterator it;
	bool success;
	boost::tie(it, success) = idx.push_back(rule);
	(void)success;
	assert(success);

	return ID(
			rule.kind, // kind
			container.project<impl::AddressTag>(it) - idx.begin() // address
			);
}

DLVHEX_NAMESPACE_END

#endif // RULETABLE_HPP_INCLUDED__12102010
