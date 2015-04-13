/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) Peter Schüller
 * Copyright (C) 2011-2015 Christoph Redl
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
 * @file   RuleTable.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Table for storing Rules
 */

#ifndef RULETABLE_HPP_INCLUDED__12102010
#define RULETABLE_HPP_INCLUDED__12102010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/Rule.h"
#include "dlvhex2/Table.h"

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/composite_key.hpp>

DLVHEX_NAMESPACE_BEGIN

/** \brief Lookup table for rules. */
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
BOOST_MULTI_INDEX_MEMBER(Rule,Tuple,headGuard),
BOOST_MULTI_INDEX_MEMBER(Rule,Tuple,bodyWeightVector),
BOOST_MULTI_INDEX_MEMBER(Rule,ID,bound),
BOOST_MULTI_INDEX_MEMBER(Rule,ID,weight),
BOOST_MULTI_INDEX_MEMBER(Rule,ID,level)
>
>
>
>
{
    // types
    public:
        typedef Container::index<impl::AddressTag>::type AddressIndex;
        typedef Container::index<impl::KindTag>::type KindIndex;
        typedef Container::index<impl::ElementTag>::type ElementIndex;
        typedef AddressIndex::iterator AddressIterator;
        typedef ElementIndex::iterator ElementIterator;
        // methods
    public:
        /** \brief Retrieve by ID.
         *
         * Assert that id.kind is correct for Rule.
         * Assert that ID exists in table.
         * @param id ID of the rule to retrieve.
         * @return Rule corresponding to \p id. */
        inline const Rule& getByID(ID id) const throw ();

        /** \brief Get the ID of the rule.
         * @param rule Rule whose ID shall be retrieved.
         * @return ID of \p rule. */
        inline ID getIDByElement(const Rule& rule) const throw();

        /** Stores rule.
         *
         * Assert that rule did not exist in table.
         * @param rule Rule to store.
         * @return ID of the stored rule.
         */
        inline ID storeAndGetID(const Rule& rule) throw();
        /** \brief Clears the table. */
        inline void clear();

        /** \brief Update a rule in the table.
         *
         * oldStorage must be from getByID() or from *const_iterator.
         * @param oldStorage Old rule.
         * @param newStorage New rule.
         */
        inline void update(
            const Rule& oldStorage, Rule& newStorage) throw();

        /** \brief Prints the table in human-readable format.
         *
         * Implementation in Registry.cpp!
         *
         * @param o Stream to print to.
         * @param reg Registry used to resolve IDs.
         * @return \p o. */
        std::ostream& print(std::ostream& o, RegistryPtr reg) const throw();

        /** \brief Get range over all atoms sorted by address.
         *
         * NOTE: you may need to lock the mutex also while iterating!
         * @return Pair of a begin and end iterator representing all rules in the table. */
        inline std::pair<AddressIterator, AddressIterator>
            getAllByAddress() const throw();
};

// retrieve by ID
// assert that id.kind is correct for Rule
// assert that ID exists
const Rule&
RuleTable::getByID(
ID id) const throw ()
{
    assert(id.isRule());
    assert(id.isRegularRule() || id.isConstraint() || id.isWeakConstraint() || id.isWeightRule());
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
    ElementIndex::const_iterator it = sidx.find( boost::make_tuple(rule.kind, rule.head, rule.body, rule.headGuard, rule.bodyWeightVector, rule.bound, rule.weight, rule.level) );
    if( it == sidx.end() )
        return ID_FAIL;
    else {
        const AddressIndex& aidx = container.get<impl::AddressTag>();
        return ID(
            it->kind,            // kind
                                 // address
            container.project<impl::AddressTag>(it) - aidx.begin()
            );
    }
}


// store rule
ID RuleTable::storeAndGetID(
const Rule& rule) throw()
{
    assert(ID(rule.kind,0).isRule());
    assert(ID(rule.kind,0).isRegularRule() ||
        ID(rule.kind,0).isConstraint() ||
        ID(rule.kind,0).isWeakConstraint() ||
        ID(rule.kind,0).isWeightRule());
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
        rule.kind,               // kind
                                 // address
        container.project<impl::AddressTag>(it) - idx.begin()
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


// get range over all rules sorted by address
// NOTE: you may need to lock the mutex also while iterating!
std::pair<RuleTable::AddressIterator, RuleTable::AddressIterator>
RuleTable::getAllByAddress() const throw()
{
    ReadLock lock(mutex);
    const AddressIndex& idx = container.get<impl::AddressTag>();
    return std::make_pair(idx.begin(), idx.end());
}


DLVHEX_NAMESPACE_END
#endif                           // RULETABLE_HPP_INCLUDED__12102010
