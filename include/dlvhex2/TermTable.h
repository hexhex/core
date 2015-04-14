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

/** \brief Lookup tables for terms. */
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
        typedef Container::index<impl::TermTag>::type TermIndex;

        // methods
    public:
        /** \brief Retrieve by ID.
         *
         * Assert that id.kind is correct for Term.
         * Assert that ID exists.
         * @param id Term ID.
         * @return Term corresponding to \p id.
         */
        inline const Term& getByID(ID id) const throw ();

        /** \brief Given string, look if already stored.
         *
         * @param str Term string to lookup.
         * @return ID_FAIL if term is not stored, otherwise return term ID. */
        inline ID getIDByString(const std::string& str) const throw();

        /** \brief Store term in the table.
         * Store symbol, assuming it does not exist.
         * Assert that symbol did not exist.
         *
         * @param symb Term to store.
         * @return ID of the stored term. */
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
    assert(id.isConstantTerm() || id.isVariableTerm() || id.isNestedTerm());
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
    else {
        const AddressIndex& aidx = container.get<impl::AddressTag>();
        return ID(
            it->kind,            // kind
                                 // address
            container.project<impl::AddressTag>(it) - aidx.begin()
            );
    }
}


#if 0
// given an argument vector, look if already stored
// if no, return ID_FAIL, otherwise return ID
ID TermTable::getIDByArguments(
const std::vector<ID>& args) const throw()
{
    typedef Container::index<impl::TupleTag>::type ArgIndex;
    ReadLock lock(mutex);
    const ArgIndex& sidx = container.get<impl::TupleTag>();
    ArgIndex::const_iterator it = sidx.find(args);
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
#endif

// store symbol, assuming it does not exist
// assert that symbol did not exist
ID TermTable::storeAndGetID(
const Term& symb) throw()
{
    assert(ID(symb.kind,0).isTerm());
    // integers are not allowed in this table!
    assert(ID(symb.kind,0).isConstantTerm() || ID(symb.kind,0).isVariableTerm() || ID(symb.kind,0).isNestedTerm());
    assert(!symb.symbol.empty());

    bool success;
    AddressIndex::const_iterator it;

    WriteLock lock(mutex);
    AddressIndex& idx = container.get<impl::AddressTag>();
    boost::tie(it, success) = idx.push_back(symb);
    (void)success;
    assert(success);

    return ID(
        symb.kind,               // kind
                                 // address
        container.project<impl::AddressTag>(it) - idx.begin()
        );
}


DLVHEX_NAMESPACE_END
#endif                           // TERMTABLE_HPP_INCLUDED__12102010
// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
