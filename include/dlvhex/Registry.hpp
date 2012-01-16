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
 * @file Registry.hpp
 * @author Peter Schueller
 * @date
 *
 * @brief Registry for program objects, addressed by IDs, organized in individual tables.
 */

#ifndef REGISTRY_HPP_INCLUDED_14012011
#define REGISTRY_HPP_INCLUDED_14012011

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/Printhelpers.hpp"
#include "dlvhex/ID.hpp"
#include "dlvhex/TermTable.hpp"
#include "dlvhex/PredicateTable.hpp"
#include "dlvhex/OrdinaryAtomTable.hpp"
#include "dlvhex/BuiltinAtomTable.hpp"
#include "dlvhex/AggregateAtomTable.hpp"
#include "dlvhex/ExternalAtomTable.hpp"
#include "dlvhex/ModuleAtomTable.hpp"
#include "dlvhex/RuleTable.hpp"
#include "dlvhex/ModuleTable.hpp"
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

DLVHEX_NAMESPACE_BEGIN

#warning namespaces where here
/*
typedef boost::bimaps::bimap<
  boost::bimaps::set_of<std::string>,
  boost::bimaps::set_of<std::string> > NamespaceTable;
  */

/**
 * Registry Plugin for printing auxiliary IDs.
 *
 * Derived classes implement the print method which decides
 * whether printing the ID is the responsibility of that class
 * and acts accordingly.
 */
class AuxPrinter
{
public:
  virtual ~AuxPrinter() {}
  // print an ID and return true,
  // or do not print it and return false
  // print prefix in front of printed thing if something is printed
  virtual bool print(std::ostream& out, ID id, const std::string& prefix) const = 0;
};

/**
 * @brief Registry for entities used in programs as IDs (collection of symbol tables)
 */
struct Registry:
  public ostream_printable<Registry>,
  public boost::enable_shared_from_this<Registry>
{
  Registry();
  // creates a real deep copy
  explicit Registry(const Registry& other);
  ~Registry();

  TermTable terms;
  PredicateTable preds;
  // ordinary ground atoms
  OrdinaryAtomTable ogatoms;
  // ordinary nonground atoms
  OrdinaryAtomTable onatoms;
  BuiltinAtomTable batoms;
  AggregateAtomTable aatoms;
  ExternalAtomTable eatoms;
  ModuleAtomTable matoms;
  RuleTable rules;
  ModuleTable moduleTable;
  std::vector<Tuple> inputList;

#warning namespaces
  //NamespaceTable namespaces;

#warning possible efficiency improvement
	#if 0
	
	// this can be done later, for now we can use hashtables and forget this more efficient method, TODO check how often this mapping is actually required

	//
	// "address range" concept
	//
	// from IDKind we obtain integers starting at zero,
	// for each distinct table a separate integer
	// this way we can create efficient mappings from IDs of various kinds to use mapKindToAddressRange() method
	// e.g., for looking up vertices in dependency graph by ID
	//   -> first lookup O(1) by IDKind, then lookup vertex in O(1) by address in vector
	//   -> vector storage with no useless storage allocation (one vector for each address range)
	enum AddressRange
	{
		ARTERM = 0,
		AROATOM,
		ARONATOM,
		ARBATOM,
		ARAATOM,
		AREATOM,
		ARRULE,
		AR_COUNT // this must stay the last entry
	};
	static inline AddressRange mapKindToAddressRange(IDKind kind);
	static inline AddressRange maxAddressRange() { return AR_COUNT; }
	#endif

  //
  // modifiers
  //

  // lookup by tuple, if does not exist create text and store as new atom
  // assume, that oatom.kind and oatom.tuple is initialized!
  // assume, that oatom.text is not initialized!
  // oatom.text will be modified
  //
  // ground version
  ID storeOrdinaryGAtom(OrdinaryAtom& ogatom);
  // nonground version
  ID storeOrdinaryNAtom(OrdinaryAtom& onatom);

  // lookup by symbol, if it does not exist create it in term table
  // assume term.kind and term.symbol is initialized
  // assume term is not an integer (i.e., term.symbol does not start with a digit)
  ID storeConstOrVarTerm(Term& term);

  // check if term is integer
  // if yes return integer id
  // otherwise
  // * add subkind flags (variable vs constant) to term.kind
  // * call storeConstOrVarTerm
  // assume term.kind is at least MAINKIND_TERM and term.symbol is fully initialized
  ID storeTerm(Term& term);

  // check if rule is contained in registry
  // if yes return integer id
  // otherwise store and return new id
  // assume rule is fully initialized
  ID storeRule(Rule& rule);

  //
  // auxiliary management
  //

  // must be called after construction and before any call to getAuxiliaryConstantSymbol
  void setupAuxiliaryGroundAtomMask();

  // create or lookup auxiliary constant symbol of type <type> for ID <id>
  // with multiple calls, for one <type>/<id> pair the same symbol/ID will be returned
  // we limit ourselves to types of one letter, this should be sufficient
  // see Registry.cpp for documentation of types used internally in dlvhex
  // (plugins may also want to use this method for their own auxiliaries)
  ID getAuxiliaryConstantSymbol(char type, ID id);

  // get predicate mask to auxiliary ground atoms
  InterpretationConstPtr getAuxiliaryGroundAtomMask();

  //
  // accessors
  //

	// cannot be nonconst as printing might change registry caches (TODO create mutable string caches in atoms)
  std::ostream& print(std::ostream& o);
  #warning TODO make registry const printable!
  virtual std::ostream& print(std::ostream& o) const { return const_cast<Registry*>(this)->print(o); }

  // lookup ground or nonground ordinary atoms (ID specifies this)
  const OrdinaryAtom& lookupOrdinaryAtom(ID id) const;
  inline const std::string& getTermStringByID(ID termid) const
    { return terms.getByID(termid).symbol; }

  // get all external atom IDs in tuple and recursively in aggregates in tuple
  // append these ids to second given tuple
  // tuple t contains IDs of literals or atoms
  void getExternalAtomsInTuple(const Tuple& t, Tuple& out) const;

  // get all IDs of variables in atom given by ID
  // add these ids to out
  // (returns even local variables for aggregates)
  // id is a literal or atom
  void getVariablesInID(ID id, std::set<ID>& out) const;

  // get all IDs of variables in atoms in given tuple
  // add these ids to out
  // (returns even local variables for aggregates)
  // tuple t contains IDs of literals or atoms
  void getVariablesInTuple(const Tuple& t, std::set<ID>& out) const;

  //
  // printing framework
  //

  // these printers are used as long as none prints it
  void registerUserAuxPrinter(AuxPrinterPtr printer);

  // this one printer is used last
  void registerUserDefaultAuxPrinter(AuxPrinterPtr printer);

  // true if anything was printed
  // false if nothing was printed
  // if it prints, prints prefix in front of printed thing (for printing lists efficiently)
  bool printAtomForUser(std::ostream& o, IDAddress address, const std::string& prefix="");

protected:
  struct Impl;
  boost::scoped_ptr<Impl> pimpl;
};
typedef boost::shared_ptr<Registry> RegistryPtr;

DLVHEX_NAMESPACE_END

#endif // REGISTRY_HPP_INCLUDED_14012011
