/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schüller
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
 * @file   BuiltinAtomTable.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Table for storing Builtin Atoms
 */

#ifndef BUILTINATOMTABLE_HPP_INCLUDED__12102010
#define BUILTINATOMTABLE_HPP_INCLUDED__12102010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/Atoms.h"
#include "dlvhex2/Table.h"

DLVHEX_NAMESPACE_BEGIN

/** \brief Lookup table for builtin atoms. */
class BuiltinAtomTable:
public Table<
// value type is symbol struct
BuiltinAtom,
// index is
boost::multi_index::indexed_by<
// address = running ID for constant access
boost::multi_index::random_access<
boost::multi_index::tag<impl::AddressTag>
>
>
>
{
    // types
    public:
        typedef Container::index<impl::AddressTag>::type AddressIndex;
        //typedef Container::index<impl::KindTag>::type KindIndex;
        //typedef Container::index<impl::TupleTag>::type TupleIndex;

        // methods
    public:
        /** \brief Retrieve by ID.
         *
         * Assert that id.kind is correct.
         * Assert that ID exists in table.
         * @param id ID of a builtin atom.
         * @return BuiltinAtom corresponding to \p id. */
        inline const BuiltinAtom& getByID(ID id) const throw ();

        /** \brief Store atom, assuming it does not exist.
         * @param atom BuiltinAtom to store.
         * @return ID of the stored BuiltinAtom. */
        inline ID storeAndGetID(const BuiltinAtom& atom) throw();
};

// retrieve by ID
// assert that id.kind is correct for Term
// assert that ID exists
const BuiltinAtom&
BuiltinAtomTable::getByID(
ID id) const throw ()
{
    assert(id.isAtom() || id.isLiteral());
    assert(id.isBuiltinAtom());
    ReadLock lock(mutex);
    const AddressIndex& idx = container.get<impl::AddressTag>();
    // the following check only works for random access indices, but here it is ok
    assert( id.address < idx.size() );
    return idx.at(id.address);
}


// store symbol, assuming it does not exist (this is only asserted)
ID BuiltinAtomTable::storeAndGetID(
const BuiltinAtom& atm) throw()
{
    assert(ID(atm.kind,0).isAtom());
    assert(ID(atm.kind,0).isBuiltinAtom());
    assert(!atm.tuple.empty());

    bool success;
    AddressIndex::const_iterator it;

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


DLVHEX_NAMESPACE_END
#endif                           // BUILTINATOMTABLE_HPP_INCLUDED__12102010
// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
