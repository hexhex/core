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
 * @file   PredicateTable.h
 * @author Tri Kurniawan Wijaya <trikurniawanwijaya@gmail.com>
 *
 * @brief  Table for storing Predicates
 */

#ifndef PREDICATETABLE_HPP_INCLUDED__20122010
#define PREDICATETABLE_HPP_INCLUDED__20102010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/Predicate.h"
#include "dlvhex2/Table.h"

#include <boost/multi_index/member.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/random_access_index.hpp>

DLVHEX_NAMESPACE_BEGIN

/** \brief Lookup table for predicates. */
class PredicateTable:
public Table<
// value type is symbol struct
Predicate,
// index is
boost::multi_index::indexed_by<
// address = running ID for constant access
boost::multi_index::random_access<
boost::multi_index::tag<impl::AddressTag>
>,
// unique IDs for unique symbol strings
boost::multi_index::hashed_unique<
boost::multi_index::tag<impl::PredicateNameTag>,
BOOST_MULTI_INDEX_MEMBER(Predicate,std::string,symbol)
>
>
// WARNING: do not put an index on arity, it might be changed (see below)
>
{
    public:
        // types
        typedef Container::index<impl::AddressTag>::type AddressIndex;
        typedef AddressIndex::iterator AddressIterator;
        typedef Container::index<impl::PredicateNameTag>::type PredicateNameIndex;

        // methods
    public:
        /** \brief Retrieve by ID.
         *
         * Assert that id.kind is correct for Term.Predicate.
         * Assert that ID exists.
         * @param id ID of the predicate to retrieve.
         * @return Predicate corresponding to \p id. */
        inline const Predicate& getByID(ID id) const throw ();

        /** Change the arity of a specific predicate with ID \p id.
         * @param id ID of a predicate in the table.
         * @param arity the new arity of predicate \p id. */
        inline void setArity(ID id, int arity);

        /** Given string, look if already stored.
         * @param str String representation of a predicate.
         * @return Return ID_FAIL if \p str is not stored as a predicate, otherwise return ID. */
        inline ID getIDByString(const std::string& str) const throw();

        /** \brief Get the Predicate by predicate name.
         * @param str String representation of a predicate.
         * @return Predicate corresponding to \p str. */
        inline const Predicate& getByString(const std::string& str) const throw();

        /** \brief Store symbol, assuming it does not exist.
         *
         * Assert that symbol did not exist.
         * @param symb Predicate to store.
         * @return ID of the stored Predicate. */
        inline ID storeAndGetID(const Predicate& symb) throw();

        /** \brief Get range over all atoms sorted by address.
         *
         * NOTE: you may need to lock the mutex also while iterating!
         * @return Pair of begin and end iterator representing all predicates in this table. */
        inline std::pair<AddressIterator, AddressIterator>
            getAllByAddress() const throw();

};

// retrieve by ID
// assert that id.kind is correct for Term
// assert that ID exists
const Predicate& PredicateTable::getByID(ID id) const throw ()
{
    assert(id.isTerm());
    assert(id.isPredicateTerm() );
    ReadLock lock(mutex);
    const AddressIndex& idx = container.get<impl::AddressTag>();
    // the following check only works for random access indices, but here it is ok
    assert( id.address < idx.size() );
    return idx.at(id.address);
}


// retrieve by ID
// assert that id.kind is correct for Term
// assert that ID exists
// change the arity
void PredicateTable::setArity(ID id, int arity)
{
    assert(id.isTerm());
    assert(id.isPredicateTerm() );
    assert(arity >= 0);

    WriteLock lock(mutex);
    const AddressIndex& idx = container.get<impl::AddressTag>();
    // the following check only works for random access indices, but here it is ok
    assert( id.address < idx.size() );
    AddressIterator it = idx.begin()+id.address;
    Predicate newPred((*it).kind, (*it).symbol, arity);
    container.replace(it, newPred);
    DBGLOG(DBG, "Change arity of " << (*it).symbol << " to " << arity);
}


// given string, look if already stored
// if no, return ID_FAIL, otherwise return ID
ID PredicateTable::getIDByString(const std::string& str) const throw()
{
    ReadLock lock(mutex);
    const PredicateNameIndex& sidx = container.get<impl::PredicateNameTag>();
    PredicateNameIndex::const_iterator it = sidx.find(str);
    if( it == sidx.end() )
        return ID_FAIL;
    else {
        const AddressIndex& aidx = container.get<impl::AddressTag>();
        return ID(it->kind,      // kind
                                 // address
            container.project<impl::AddressTag>(it) - aidx.begin()
            );
    }
}


const Predicate& PredicateTable::getByString(const std::string& str) const throw()
{
    ReadLock lock(mutex);
    const PredicateNameIndex& sidx = container.get<impl::PredicateNameTag>();
    PredicateNameIndex::const_iterator it = sidx.find(str);
    WARNING("this looks fishy, and PREDICATE_FAIL is not used anywhere else, when can this happen and if it is intended to fail, why don't we return a pointer or throw an exception?")
        if( it == sidx.end() )
        return PREDICATE_FAIL;
    else {
        return *it;
    }
}


// store symbol, assuming it does not exist
// assert that symbol did not exist
ID PredicateTable::storeAndGetID(const Predicate& symb) throw()
{
    assert(ID(symb.kind,0).isTerm());
    assert(ID(symb.kind,0).isPredicateTerm() );
    assert(!symb.symbol.empty());

    AddressIndex::const_iterator it;
    bool success;

    WriteLock lock(mutex);
    AddressIndex& idx = container.get<impl::AddressTag>();
    boost::tie(it, success) = idx.push_back(symb);
    (void)success;
    assert(success);

    return ID(symb.kind,         // kind
                                 // address
        container.project<impl::AddressTag>(it) - idx.begin()
        );
}


// get range over all atoms sorted by address
// NOTE: you may need to lock the mutex also while iterating!
std::pair<PredicateTable::AddressIterator, PredicateTable::AddressIterator>
PredicateTable::getAllByAddress() const throw()
{
    ReadLock lock(mutex);
    const AddressIndex& idx = container.get<impl::AddressTag>();
    return std::make_pair(idx.begin(), idx.end());
}


DLVHEX_NAMESPACE_END
#endif                           // PREDICATETABLE_HPP_INCLUDED__20122010
