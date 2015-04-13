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
 * @file   OrdinaryAtomTable.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Table for storing Ordinary Atoms (ground or nonground)
 */

#ifndef ORDINARYATOMTABLE_HPP_INCLUDED__12102010
#define ORDINARYATOMTABLE_HPP_INCLUDED__12102010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/Atoms.h"
#include "dlvhex2/Table.h"

#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/random_access_index.hpp>

DLVHEX_NAMESPACE_BEGIN

/** \brief Implements a lookup table for ordinary atoms. */
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
        /** \brief Retrieve by ID.
         *
         * Assert that id.kind is correct for OrdinaryGroundAtom.
         * Assert that ID exists in table.
         * @param id Term ID.
         * @return Ordinary atom corresponding to \p id.
         */
        inline const OrdinaryAtom& getByID(ID id) const throw ();

        /** \brief Retrieve by address (ignore kind).
         *
         * Assert that address exists in table.
         * @param addr Address of the ordinary atom to retrieve.
         * @return Ordinary atom corresponding to \p addr.
         */
        inline const OrdinaryAtom& getByAddress(IDAddress addr) const throw ();

        /** \brief Retrieve ID by address (ignore kind).
         *
         * Assert that address exists in table.
         * @param addr Address of the ordinary atom to retrieve.
         * @return Ordinary atom corresponding to \p addr.
         */
        inline ID getIDByAddress(IDAddress addr) const throw ();

        /** \brief Given string, look if already stored.
         * @param text String representation of the ordinary atom to retrieve.
         * @return ID_FAIL if not stored, otherwise return ID. */
        inline ID getIDByString(const std::string& text) const throw();

        /** \brief Given tuple, look if already stored.
         * @param tuple Tuple representation of the ordinary atom to retrieve.
         * @return ID_FAIL if not stored, otherwise return ID.
         */
        inline ID getIDByTuple(const Tuple& tuple) const throw();

        /** \brief Get ID given storage retrieved by other means.
         *
         * Storage must have originated from iterator from here.
         * @param atom Atom to retrieve; must be in the table.
         * @return ID of \param atom. */
        inline ID getIDByStorage(const OrdinaryAtom& atom) const throw ();

        /** \brief Get IDAddress of given storage retrieved by other means.
         *
         * Storage must have originated from iterator from here.
         * @param atom Atom to retrieve; must be in the table.
         * @return IDAddress of \param atom. */
        inline IDAddress getIDAddressByStorage(const OrdinaryAtom& atom) const throw ();

        /** \brief Store atom, assuming it does not exist.
         *
         * Assert that atom did not exist in table.
         * @param atom Atom to retrieve; must be in the table.
         * @return ID of \param atom. */
        inline ID storeAndGetID(const OrdinaryAtom& atom) throw();

        /** \brief Get all ordinary atoms with certain predicate id.
         *
         * NOTE: you may need to lock the mutex also while iterating!
         * If you intend to use this method frequently, consider to use a PredicateMask instead for better efficiency (iteration is slow).
         * @param id Predicate ID.
         * @return Pair of begin and end iterator representing all atoms in the table with the given predicate. */
        inline std::pair<PredicateIterator, PredicateIterator>
            getRangeByPredicateID(ID id) const throw();

        /** \brief Get all ordinary atoms in the table.
         *
         * NOTE: you may need to lock the mutex also while iterating!
         * @param id Predicate ID.
         * @return Pair of begin and end iterator representing all atoms in the table. */
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
    ReadLock lock(mutex);
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
    ReadLock lock(mutex);
    const AddressIndex& idx(container.get<impl::AddressTag>());
    // the following check only works for random access indices, but here it is ok
    assert( addr < idx.size() );
    return idx.at(addr);
}


// retrieve ID by address
// assert that address exists in table
ID
OrdinaryAtomTable::getIDByAddress(
IDAddress addr) const throw ()
{
    const OrdinaryAtom& atom = getByAddress(addr);
    return ID(atom.kind, addr);
}


// given string, look if already stored
// if no, return ID_FAIL, otherwise return ID
ID OrdinaryAtomTable::getIDByString(
const std::string& str) const throw()
{
    typedef Container::index<impl::TextTag>::type OrdinaryAtomIndex;
    ReadLock lock(mutex);
    const TextIndex& sidx(container.get<impl::TextTag>());
    TextIndex::const_iterator it(sidx.find(str));
    if( it == sidx.end() )
        return ID_FAIL;
    else {
        const AddressIndex& aidx(container.get<impl::AddressTag>());
        return ID(
            it->kind,            // kind
                                 // address
            container.project<impl::AddressTag>(it) - aidx.begin()
            );
    }
}


// given tuple, look if already stored
// if no, return ID_FAIL, otherwise return ID
ID OrdinaryAtomTable::getIDByTuple(
const Tuple& tuple) const throw()
{
    typedef Container::index<impl::TupleTag>::type TupleIndex;
    ReadLock lock(mutex);
    const TupleIndex& sidx(container.get<impl::TupleTag>());
    TupleIndex::const_iterator it(sidx.find(tuple));
    if( it == sidx.end() )
        return ID_FAIL;
    else {
        const AddressIndex& aidx(container.get<impl::AddressTag>());
        return ID(
            it->kind,            // kind
                                 // address
            container.project<impl::AddressTag>(it) - aidx.begin()
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
    ReadLock lock(mutex);
    const AddressIndex& aidx(container.get<impl::AddressTag>());
    AddressIndex::const_iterator it(aidx.iterator_to(atom));
    assert(atom.kind == it->kind);
    return ID(
        atom.kind,               // kind
        it - aidx.begin()        // address
        );
}


// get IDAddress of given storage retrieved by other means
// (storage must have originated from iterator from here)
IDAddress OrdinaryAtomTable::getIDAddressByStorage(
const OrdinaryAtom& atom) const throw ()
{
    // we cannot assert anything really useful here!
    // (if the user specifies another storage, iterator_to will segfault
    //  anyway as there is no associated internal multi_index storage node)
    ReadLock lock(mutex);
    const AddressIndex& aidx(container.get<impl::AddressTag>());
    AddressIndex::const_iterator it(aidx.iterator_to(atom));
    assert(atom.kind == it->kind);
    return it - aidx.begin();
}


// store symbol, assuming it does not exist (this is only asserted)
ID OrdinaryAtomTable::storeAndGetID(
const OrdinaryAtom& atm) throw()
{
    assert(ID(atm.kind,0).isAtom());
    assert(ID(atm.kind,0).isOrdinaryAtom());
    assert(!atm.text.empty());
    assert(!(
        (atm.tuple.front().kind & ID::PROPERTY_AUX) != 0 &&
        (atm.kind & ID::PROPERTY_AUX) == 0 ) &&
        "atom must be auxiliary if predicate term is auxiliary");

    AddressIndex::const_iterator it;
    bool success;

    WriteLock lock(mutex);
    AddressIndex& idx(container.get<impl::AddressTag>());
    boost::tie(it, success) = idx.push_back(atm);
    (void)success;
    assert(success);

    return ID(
        atm.kind,                // kind
                                 // address
        container.project<impl::AddressTag>(it) - idx.begin()
        );
}


// get all ordinary atoms with certain predicate id
// NOTE: you may need to lock the mutex also while iterating!
// if you intend to use this method frequently, consider to use a PredicateMask instead for better efficiency (iteration is slow)
std::pair<OrdinaryAtomTable::PredicateIterator, OrdinaryAtomTable::PredicateIterator>
OrdinaryAtomTable::getRangeByPredicateID(ID id) const throw()
{
    assert(id.isTerm());
    ReadLock lock(mutex);
    const PredicateIndex& idx = container.get<impl::PredicateTag>();
    return idx.equal_range(id);
}


// get range over all atoms sorted by address
// NOTE: you may need to lock the mutex also while iterating!
std::pair<OrdinaryAtomTable::AddressIterator, OrdinaryAtomTable::AddressIterator>
OrdinaryAtomTable::getAllByAddress() const throw()
{
    ReadLock lock(mutex);
    const AddressIndex& idx = container.get<impl::AddressTag>();
    return std::make_pair(idx.begin(), idx.end());
}


DLVHEX_NAMESPACE_END
#endif                           // ORDINARYATOMTABLE_HPP_INCLUDED__12102010
