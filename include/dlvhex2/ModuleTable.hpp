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
 * @file   ModuleTable.hpp
 * @author Tri Kurniawan Wijaya <trikurniawanwijaya@gmail.com>
 * 
 * @brief  Table for storing Modules: ModuleName, inputList, edb, idb. Use struct from Module.hpp
 */

#ifndef MODULETABLE_HPP_INCLUDED__20122010
#define MODULETABLE_HPP_INCLUDED__20102010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/Logger.hpp"
#include "dlvhex2/ID.hpp"
#include "dlvhex2/Module.hpp"
#include "dlvhex2/Table.hpp"

#include <boost/multi_index/member.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/random_access_index.hpp>

DLVHEX_NAMESPACE_BEGIN

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

  // retrieve by address
  inline const Module& getByAddress(int address) const throw ();

  // retrieve module name by address
  inline const std::string& getModuleName(int address) const throw ();

  // get range over all atoms sorted by address
  inline std::pair<AddressIterator, AddressIterator> getAllByAddress() const throw();

  // given a module name, look if already stored
  // if no, return MODULE_FAIL, otherwise return the module struct
  inline const Module& getModuleByName(const std::string& moduleName) const throw();

	// get address of a module by its name
  inline int getAddressByName(const std::string& moduleName) const throw();

  // store symbol, assuming it does not exist
  // assert that symbol did not exist
  inline int storeAndGetAddress(const Module& mod) throw();
  inline virtual std::ostream& print(std::ostream& o) const;

};

// get range over all atoms sorted by address
std::pair<ModuleTable::AddressIterator, ModuleTable::AddressIterator>
ModuleTable::getAllByAddress() const throw()
{
  #warning this read-only iteration will probably need to be mutexed too!
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
  while ( it != idx.end() )
    {
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
  if( it == sidx.end() ) 
    {
      return MODULE_FAIL;
    }
  else
    {
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
  if( it == sidx.end() ) 
    {
      return -1;
    }
  else
    {
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

#endif // MODULETABLE_HPP_INCLUDED__20122010
