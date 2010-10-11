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
 * @file   Symbol.hpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Symbol class: represents constants, constant strings, and variables
 */

#ifndef SYMBOL_HPP_INCLUDED__08102010
#define SYMBOL_HPP_INCLUDED__08102010

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/Logger.hpp"

// TODO: move ID business from here to ID.hpp once it's sufficiently stable
//#include "dlvhex/ID.hpp"

// TODO: move symbol table SymbolTable.hpp once it's sufficiently stable
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>

#ifndef NDEBUG
#include <iomanip>
#endif

DLVHEX_NAMESPACE_BEGIN

typedef uint32_t IDKind;
typedef uint32_t IDAddress;

struct ID
{
	IDKind kind;
	IDAddress address;
	ID(IDKind kind, IDAddress address): kind(kind), address(address) {}
	// no virtual here!
	// this struct must fit into an uint64_t and have no vtable!

	static const uint32_t ALL_ONES =           0xFFFFFFFF;

	static const IDKind NAF_MASK =             0x80000000;
	static const IDKind MAINKIND_MASK =        0x70000000;
  static const uint8_t MAINKIND_SHIFT =      28;
	static const IDKind SUBKIND_MASK =         0x0F000000;
  static const uint8_t SUBKIND_SHIFT =       24;
	static const IDKind PROPERTY_MASK =        0x00FF0000;
  static const uint8_t PROPERTY_SHIFT =      16;
	static const IDKind UNUSED_MASK =          0x0000FFFF;

	static const IDKind MAINKIND_ATOM =        0x00000000;
	static const IDKind MAINKIND_TERM =        0x10000000;
	static const IDKind MAINKIND_LITERAL =     0x20000000;
	static const IDKind MAINKIND_RULE =        0x30000000;

	static const IDKind SUBKIND_CONSTANT =     0x00000000;
	static const IDKind SUBKIND_QUOTEDSTRING = 0x02000000;
	static const IDKind SUBKIND_INTEGER =      0x01000000;
	static const IDKind SUBKIND_VARIABLE =     0x03000000;

	static const IDKind PROPERTY_ANONYMOUS =   0x00010000;

	inline bool isTerm() const { return (kind & MAINKIND_MASK) == MAINKIND_TERM; }
	inline bool isTermConstant() const { return isTerm() && (kind & SUBKIND_MASK) == SUBKIND_CONSTANT; }
	inline bool isTermQuotedString() const { return isTerm() && (kind & SUBKIND_MASK) == SUBKIND_QUOTEDSTRING; }
	inline bool isTermInteger() const { return isTerm() && (kind & SUBKIND_MASK) == SUBKIND_INTEGER; }
	inline bool isTermVariable() const { return isTerm() && (kind & SUBKIND_MASK) == SUBKIND_VARIABLE; }
	inline bool operator==(const ID& id2) const { return kind == id2.kind && address == id2.address; }

  std::ostream& print(std::ostream& o) const;
};

std::ostream& ID::print(std::ostream& o) const
{
  o << "ID(0x" <<
      std::hex << std::setw(8) << kind << "," <<
      std::dec << std::setw(4) << address << ",";
  if( !!(kind & NAF_MASK) )
    o << " naf";
  const unsigned MAINKIND_MAX = 4;
  const char* mainkinds[MAINKIND_MAX] = {
    " atom",
    " term",
    " literal",
    " rule",
  };
  const unsigned mainkind = (kind & MAINKIND_MASK) >> MAINKIND_SHIFT;
  assert(mainkind < MAINKIND_MAX);
  o << mainkinds[mainkind];

  const unsigned SUBKIND_MAX = 7;
  const char* subkinds[MAINKIND_MAX][SUBKIND_MAX] = {
    { " ordinary_ground", " ordinary_nonground", " builtin",         " aggregate", "", "", " external" },
    { " constant",        " integer",            " quotedstr",       " variable",  "", "", ""          },
    { " ordinary_ground", " ordinary_nonground", " builtin",         " aggregate", "", "", " external" },
    { " regular"          " constraint",         " weak_constraint", "",           "", "", ""          }
  };
  const unsigned subkind = (kind & SUBKIND_MASK) >> SUBKIND_SHIFT;
  assert(subkind < SUBKIND_MAX);
  assert(subkinds[mainkind][subkind][0] != 0);
  o << subkinds[mainkind][subkind];
  return o << ")";
}

const ID ID_FAIL(ID::ALL_ONES, ID::ALL_ONES);

namespace impl
{
  // these tags are common to all containers
	struct KindTag {};
	struct AddressTag {};

  // these tags are special
	struct SymbolTag {};
}

template<typename ValueT, typename IndexT>
class Table
{
	// types
public:
	// exception thrown if invalid key is looked up
	class NotFound: public std::exception { };
	// exception thrown if uniqueness condition is violated
	class Duplicate: public std::exception { };

	typedef typename
		boost::multi_index_container<ValueT, IndexT> Container;

	// storage
protected:
	Container container;

	// methods
public:
	Table(): container() {}
	// no virtual functions allowed, no virtual destructor
	// -> never store this in a ref to baseclass, destruction will not work!
	//
	// -> make all derived classes efficient using small inline methods
	//virtual ~Table() {}

	// TODO: make this inline {} for #ifdef NDEBUG and generic otherwise -> never override in derived classes
  #ifndef NDEBUG
	void logContents(const std::string& indent);
  #else
  inline void logContents(const std::string&) { }
  #endif
};

/*const OrdinaryAtom& OrdinaryGroundAtomTable::getByString(const std::string& str) const
const OrdinaryAtom& OrdinaryGroundAtomTable::getByPredicate(ID term) const
const OrdinaryAtom& OrdinaryNongroundAtomTable::getByPredicate(ID term) const
const ExternalAtom& ExternalAtomTable::getByFunction(ID term) const

const Symbol&       SymbolTable::getByString(const std::string& str) const
*/

template<typename ValueT, typename IndexT>
void Table<ValueT,IndexT>::logContents(const std::string& indent)
{
  LOG_METHOD(indent,this);
  // debugging assumes that each container can be iterated by AddressTag index and contains KindTag index
	typedef typename Container::template index<impl::AddressTag>::type AddressIndex;
	AddressIndex& aidx = container.template get<impl::AddressTag>();

	for(typename AddressIndex::const_iterator it = aidx.begin();
      it != aidx.end(); ++it)
  {
    const uint32_t address = static_cast<uint32_t>(it - aidx.begin());
    LOG(print_method(ID(it->kind, address)) << " -> " << print_method(static_cast<const ValueT&>(*it)));
  }
}

// anonymous variables get new names to become real and distinct variables, each anonymous variable gets a new ID
struct Symbol
{
  // this is the kind part of the ID of this symbol
  IDKind kind;
  // this is the textual representation of a
	//  constant,
	//  constant string (including ""), or
	//  variable (TODO: anonymous variables become new unique variables)
  std::string symbol;

  enum Type {
    CONSTANT,
    STRING,
    VARIABLE
  };

  Symbol(IDKind kind, const std::string& symbol): kind(kind), symbol(symbol) {}
  std::ostream& print(std::ostream& o) const;
};

std::ostream& Symbol::print(std::ostream& o) const
{
  return o << "Symbol(" << symbol << ")";
}

// TODO: rename once it's sufficiently stable
class MySymbolTable:
	public Table<
		// value type is symbol struct
		Symbol,
		// index is
		boost::multi_index::indexed_by<
			// address = running ID for constant access
			boost::multi_index::random_access<
				boost::multi_index::tag<impl::AddressTag>
			>,
			// kind
			boost::multi_index::ordered_non_unique<
				boost::multi_index::tag<impl::KindTag>,
				BOOST_MULTI_INDEX_MEMBER(Symbol,IDKind,kind)
			>,
			// unique IDs for unique symbol strings
			boost::multi_index::hashed_unique<
				boost::multi_index::tag<impl::SymbolTag>,
				BOOST_MULTI_INDEX_MEMBER(Symbol,std::string,symbol)
			>
		>
	>
{
	// types
public:

	// methods
public:
  // retrieve by ID
  // assert that id.kind is correct for Symbol
	inline const Symbol& getByID(ID id) const throw (NotFound);

	inline const Symbol& getByString(const std::string& str) const throw(NotFound);

	// special high performance method for parsing
	// given string, look if already stored
	// if no, return ID_FAIL, otherwise return ID
	inline ID getIDByStringNothrow(const std::string& str) const throw();

	// special high performance method for parsing
	// store symbol, assuming it does not exist (this is only asserted)
	inline ID storeAndGetID(const Symbol& symb) throw();
};

const Symbol&
MySymbolTable::getByID(
  ID id) const throw (NotFound)
{
	assert(id.isTerm());
	// integers are not allowed in this table!
	assert(id.isTermConstant() ||
			id.isTermQuotedString() ||
			id.isTermVariable());
  typedef Container::index<impl::AddressTag>::type AddressIndex;
  const AddressIndex& idx = container.get<impl::AddressTag>();
  if( id.address >= idx.size() ) // this only works for random access indices
    throw NotFound();
  return idx.at(id.address);
}

const Symbol&
MySymbolTable::getByString(
		const std::string& str) const throw(NotFound)
{
	typedef Container::index<impl::SymbolTag>::type SymbolIndex;
	const SymbolIndex& idx = container.get<impl::SymbolTag>();
	SymbolIndex::const_iterator it = idx.find(str);
	if( it == idx.end() )
		throw NotFound();
	return *it;
}

// special high performance method for parsing
// given string, look if already stored
// if no, return ID_FAIL, otherwise return ID
ID MySymbolTable::getIDByStringNothrow(
		const std::string& str) const throw()
{
	typedef Container::index<impl::SymbolTag>::type SymbolIndex;
	const SymbolIndex& idx = container.get<impl::SymbolTag>();
	SymbolIndex::const_iterator it = idx.find(str);
	if( it == idx.end() )
		return ID_FAIL;
	else
		return ID(
				it->kind, // kind
				container.project<impl::AddressTag>(it) - container.get<impl::AddressTag>().begin() // address
				);
}

// special high performance method for parsing
// store symbol, assuming it does not exist (this is only asserted)
ID MySymbolTable::storeAndGetID(
		const Symbol& symb) throw()
{
	assert(ID(symb.kind,0).isTerm());
	// integers are not allowed in this table!
	assert(ID(symb.kind,0).isTermConstant() ||
			ID(symb.kind,0).isTermQuotedString() ||
			ID(symb.kind,0).isTermVariable());
	assert(!symb.symbol.empty());

	typedef Container::index<impl::AddressTag>::type AddressIndex;
	AddressIndex& idx = container.get<impl::AddressTag>();

	AddressIndex::const_iterator it;
	bool success;
	boost::tie(it, success) = idx.push_back(symb);
	(void)success;
	assert(success);

	return ID(
			symb.kind, // kind
			container.project<impl::AddressTag>(it) - idx.begin() // address
			);
}

DLVHEX_NAMESPACE_END

#endif // SYMBOL_HPP_INCLUDED__08102010
