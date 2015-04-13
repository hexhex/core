/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schller
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
 * @file   ExternalAtomTable.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Table for storing External Atoms
 */

#ifndef EXTERNALATOMTABLE_HPP_INCLUDED__18102010
#define EXTERNALATOMTABLE_HPP_INCLUDED__18102010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/Atoms.h"
#include "dlvhex2/Table.h"

#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>

DLVHEX_NAMESPACE_BEGIN

/** \brief Lookup table for external atoms. */
class ExternalAtomTable:
public Table<
// value type is symbol struct
ExternalAtom,
// index is
boost::multi_index::indexed_by<
// address = running ID for constant access
boost::multi_index::random_access<
boost::multi_index::tag<impl::AddressTag>
>,
boost::multi_index::ordered_non_unique<
boost::multi_index::tag<impl::PredicateTag>,
BOOST_MULTI_INDEX_MEMBER(ExternalAtom,ID,predicate)
>
>
>
{
    // types
    public:
        typedef Container::index<impl::AddressTag>::type AddressIndex;
        typedef Container::index<impl::PredicateTag>::type PredicateIndex;
        typedef PredicateIndex::iterator PredicateIterator;

        // methods
    public:
        /** \brief Retrieve by ID.
         *
         * Aassert that id.kind is correct.
         * Assert that ID exists in table.
         * @param id ID of an external atom.
         * @return External atom corresponding to \p id. */
        inline const ExternalAtom& getByID(ID id) const throw ();

        /** \brief Get all external atoms with certain predicate id.
         * @param id External predicate ID.
         * @return Pair of begin and end iterator representing all external atoms in the table using the given predicate. */
        // NOTE: you may need to lock the mutex also while iterating!
        // if you intend to use this method frequently, consider to use a PredicateMask instead for better efficiency (iteration is slow)
        inline std::pair<PredicateIterator, PredicateIterator>
            getRangeByPredicateID(ID id) const throw();

        /** \brief Store atom, assuming it does not exist.
         * \param atom External atom to store.
         * \param ID of the stored external atom. */
        inline ID storeAndGetID(const ExternalAtom& atom) throw();

        /** \brief Updates an external atom in the table.
         *
         * oldStorage must be from getByID() or from *const_iterator.
         * @param oldStorage Old external atom.
         * @param newStorage New external atom. */
        inline void update(
            const ExternalAtom& oldStorage, ExternalAtom& newStorage) throw();

        /** \brief Prints the table in human-readable format.
         *
         * Implementation in Registry.cpp!
         *
         * @param o Stream to print to.
         * @param reg Registry used to resolve IDs.
         * @return \p o. */
        std::ostream& print(std::ostream& o, RegistryPtr reg) const throw();
};

// retrieve by ID
// assert that id.kind is correct for Term
// assert that ID exists
const ExternalAtom&
ExternalAtomTable::getByID(ID id) const throw ()
{
    assert(id.isAtom() || id.isLiteral());
    assert(id.isExternalAtom());

    ReadLock lock(mutex);
    const AddressIndex& idx = container.get<impl::AddressTag>();
    // the following check only works for random access indices, but here it is ok
    assert( id.address < idx.size() );
    return idx.at(id.address);
}


// get all external atoms with certain predicate id
// NOTE: you may need to lock the mutex also while iterating!
// if you intend to use this method frequently, consider to use a PredicateMask instead for better efficiency (iteration is slow)
std::pair<ExternalAtomTable::PredicateIterator, ExternalAtomTable::PredicateIterator>
ExternalAtomTable::getRangeByPredicateID(ID id) const throw()
{
    assert(id.isTerm() && id.isConstantTerm());
    ReadLock lock(mutex);
    const PredicateIndex& idx = container.get<impl::PredicateTag>();
    return idx.equal_range(id);
}


// store symbol, assuming it does not exist (this is only asserted)
ID ExternalAtomTable::storeAndGetID(
const ExternalAtom& atm) throw()
{
    assert(ID(atm.kind,0).isAtom());
    assert(ID(atm.kind,0).isExternalAtom());

    AddressIndex::const_iterator it;
    bool success;

    WriteLock lock(mutex);
    AddressIndex& idx = container.get<impl::AddressTag>();
    boost::tie(it, success) = idx.push_back(atm);
    (void)success;
    assert(success);

    return ID(
        atm.kind,                // kind
                                 // address
        container.project<impl::AddressTag>(it) - idx.begin()
        );
}


void ExternalAtomTable::update(
const ExternalAtom& oldStorage, ExternalAtom& newStorage) throw()
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
#endif                           // BUILTINATOMTABLE_HPP_INCLUDED__12102010
