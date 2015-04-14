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
 * @file   Table.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Base class for all Tables
 */

#ifndef TABLE_HPP_INCLUDED__12102010
#define TABLE_HPP_INCLUDED__12102010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/Logger.h"

#include <boost/multi_index_container.hpp>
#include <boost/thread/shared_mutex.hpp>

DLVHEX_NAMESPACE_BEGIN

namespace impl
{
    // these tags are common to all tables
    struct KindTag {};
    struct AddressTag {};

    // these tags are special
    struct TermTag               // Term
    {
    };
    struct PredicateNameTag      // Predicate
    {
    };
    struct ModuleNameTag         // Module
    {
    };
    struct TextTag               // OrdinaryGroundAtom
    {
    };
    struct TupleTag              // OrdinaryAtom, OrdinaryGroundAtom
    {
    };
    struct PredicateTag          // ExternalAtom
    {
    };
    struct ElementTag            // for MLPSolver
    {
    };
    struct InstTag               // instantiation Tag, for ordinary ground atom (for MLP case)
    {
    };
}


/** \brief Implements a lookup table for custom key and value types. */
template<typename ValueT, typename IndexT>
class Table:
public ostream_printable< Table<ValueT,IndexT> >
{
    // types
    public:
        typedef typename
            boost::multi_index_container<ValueT, IndexT> Container;

        // public, because other algorithms might need to lock this
        /** \brief Multithreading mutex. */
        mutable boost::shared_mutex mutex;
        typedef boost::shared_lock<boost::shared_mutex> ReadLock;
        typedef boost::unique_lock<boost::shared_mutex> WriteLock;

        // storage
    protected:
        /** \brief Internal container. */
        Container container;

        // methods
    public:
        /** \brief Constructor. */
        Table() {}
        // no virtual functions allowed, no virtual destructor
        // -> never store this in a ref to baseclass, destruction will not work!
        //
        // -> make all derived classes efficient using small inline methods
        //virtual ~Table() {}

        // all accessors using indices are specific to the respective tables
        // we will only create those accessors we really need
        // two important objectives: space efficiency and time efficiency

        /** \brief Prints the table in human-readable format.
         * @param o Stream to print to.
         * @return \p o. */
        std::ostream& print(std::ostream& o) const;

        /** \brief Copy-constructor.
         * @param other Other table. */
        Table(const Table& other):
        container(other.container) {
        }

        /** \brief Assignment operator.
         * @param other Other table.
         * @return This table. */
        Table& operator=(const Table& other) {
            WriteLock lock(mutex);
            container = other.container;
        }

        /** \brief Retrieves the size of the table.
         * @return Table size. */
        inline unsigned getSize() const
        {
            ReadLock lock(mutex);
            return container.size();
        }
};

template<typename ValueT, typename IndexT>
std::ostream& Table<ValueT,IndexT>::print(std::ostream& o) const
{
    ReadLock lock(mutex);
    // debugging assumes that each container can be iterated by AddressTag index and contains KindTag index
    typedef typename Container::template index<impl::AddressTag>::type AddressIndex;
    const AddressIndex& aidx = container.template get<impl::AddressTag>();

    for(typename AddressIndex::const_iterator it = aidx.begin();
    it != aidx.end(); ++it) {
        const uint32_t address = static_cast<uint32_t>(it - aidx.begin());
        // for the next line to work, ValueT must be derived from ostream_printable<ValueT>
        o <<
            "  " << ID(it->kind, address) << std::endl <<
            "   -> " << static_cast<const ValueT&>(*it) << std::endl;
    }
    return o;
}


DLVHEX_NAMESPACE_END
#endif                           // TABLE_HPP_INCLUDED__12102010

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
