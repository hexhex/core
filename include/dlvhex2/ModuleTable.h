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
 * @file   ModuleTable.h
 * @author Tri Kurniawan Wijaya <trikurniawanwijaya@gmail.com>
 *
 * @brief  Table for storing Modules: ModuleName, inputList, edb, idb. Use struct from Module.hpp
 */

#ifndef MODULETABLE_HPP_INCLUDED__20122010
#define MODULETABLE_HPP_INCLUDED__20102010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/Module.h"
#include "dlvhex2/Table.h"

#include <boost/multi_index/member.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/random_access_index.hpp>

DLVHEX_NAMESPACE_BEGIN

/** \brief Lookup table for modules. */
class ModuleTable:
public Table<
// value type is symbol struct
Module,
// index is
boost::multi_index::indexed_by<
// address = running ID for constant access
boost::multi_index::random_access<
boost::multi_index::tag<impl::AddressTag>
>,
// unique IDs for unique symbol strings
boost::multi_index::hashed_unique<
boost::multi_index::tag<impl::ModuleNameTag>,
BOOST_MULTI_INDEX_MEMBER(Module,std::string,moduleName)
>
>
>
{
    public:
        // types
        typedef Container::index<impl::AddressTag>::type AddressIndex;
        typedef AddressIndex::iterator AddressIterator;
        typedef Container::index<impl::ModuleNameTag>::type ModuleNameIndex;

        // methods
    public:

        /** \brief Retrieve by address.
         * @param address Address of the module to retrieve.
         * @return Module corresonding to \p address. */
        inline const Module& getByAddress(int address) const throw ();

        /** \brief Retrieve module name by address.
         * @param address Address of the module to retrieve.
         * @return Module name corresonding to \p address. */
        inline const std::string& getModuleName(int address) const throw ();

        /** \brief Get range over all atoms sorted by address.
         *
         * NOTE: you may need to lock the mutex also while iterating!
         * @return Pair of a begin and an end iterator representing all modules in this table. */
        inline std::pair<AddressIterator, AddressIterator> getAllByAddress() const throw();

        /** \brief Given a module name, look if already stored.
         * @eturn MODULE_FAIL if \p moduleName is not stored, otherwise return the module struct. */
        inline const Module& getModuleByName(const std::string& moduleName) const throw();

        /** \brief Get address of a module by its name.
         * @param moduleName Name of the module to retrieve.
         * @return Address of the module correponding to \p moduleName. */
        inline int getAddressByName(const std::string& moduleName) const throw();

        /** \brief Store symbol, assuming it does not exist.
         *
         * Assert that symbol did not exist.
         * @return mod Module to store.
         * @return Address of the stored module. */
        inline int storeAndGetAddress(const Module& mod) throw();

        /** \brief Prints the table in human-readable format.
         *
         * Implementation in Registry.cpp!
         *
         * @param o Stream to print to.
         * @return \p o. */
        inline virtual std::ostream& print(std::ostream& o) const;

};

// get range over all atoms sorted by address
// NOTE: you may need to lock the mutex also while iterating!
std::pair<ModuleTable::AddressIterator, ModuleTable::AddressIterator>
ModuleTable::getAllByAddress() const throw()
{
    ReadLock lock(mutex);
    const AddressIndex& idx = container.get<impl::AddressTag>();
    return std::make_pair(idx.begin(), idx.end());
}


std::ostream& ModuleTable::print(std::ostream& o) const
{
    ReadLock lock(mutex);
    const AddressIndex& idx = container.get<impl::AddressTag>();
    AddressIndex::const_iterator it = idx.begin();
    int address = 0;
    while ( it != idx.end() ) {
        o << "[" << address << "]" << ": " << *it << std::endl;
        it++;
        address++;
    }
    return o;
}


// retrieve by address
const Module& ModuleTable::getByAddress(int address) const throw ()
{
    ReadLock lock(mutex);
    const AddressIndex& idx = container.get<impl::AddressTag>();
    // the following check only works for random access indices, but here it is ok
    const uint32_t& uaddress = address;
    assert( uaddress < idx.size() );
    return idx.at(address);
}


// retrieve by address
const std::string& ModuleTable::getModuleName(int address) const throw ()
{
    ReadLock lock(mutex);
    const AddressIndex& idx = container.get<impl::AddressTag>();
    // the following check only works for random access indices, but here it is ok
    const uint32_t& uaddress = address;
    assert( uaddress < idx.size() );
    return idx.at(address).moduleName;
}


// given a module name, look if already stored
// if no, return MODULE_FAIL, otherwise return the module struct
const Module& ModuleTable::getModuleByName(const std::string& moduleName) const throw()
{
    ReadLock lock(mutex);
    const ModuleNameIndex& sidx = container.get<impl::ModuleNameTag>();
    ModuleNameIndex::const_iterator it = sidx.find(moduleName);
    if( it == sidx.end() ) {
        return MODULE_FAIL;
    }
    else {
        return *it;
    }
}


// given a module name, look if already stored
// if no, return -1, otherwise return the address
int ModuleTable::getAddressByName(const std::string& moduleName) const throw()
{
    ReadLock lock(mutex);
    const ModuleNameIndex& sidx = container.get<impl::ModuleNameTag>();
    ModuleNameIndex::const_iterator it = sidx.find(moduleName);
    if( it == sidx.end() ) {
        return -1;
    }
    else {
        return container.project<impl::AddressTag>( it ) - container.get<impl::AddressTag>().begin();
    }
}


// store module, assuming it does not exist
// assert that symbol did not exist
int ModuleTable::storeAndGetAddress(const Module& mod) throw()
{
    assert(!mod.moduleName.empty());

    AddressIndex::const_iterator it;
    bool success;

    WriteLock lock(mutex);
    AddressIndex& idx = container.get<impl::AddressTag>();
    boost::tie(it, success) = idx.push_back(mod);
    (void)success;
    assert(success);

    return ( container.project<impl::AddressTag>(it) - idx.begin() );

}


DLVHEX_NAMESPACE_END
#endif                           // MODULETABLE_HPP_INCLUDED__20122010
