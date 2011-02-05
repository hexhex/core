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
 * @file Registry.cpp
 * @author Peter Schueller
 * @date 
 *
 * @brief Registry for program objects, addressed by IDs, organized in individual tables.
 */

#include "dlvhex/Registry.hpp"
#include "dlvhex/Error.h"
#include "dlvhex/Printer.hpp"

#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
 * auxiliary constant symbol type usage:
 * 'i': auxiliary input grounding predicates for external atoms in rules
 *      (source ID is an eatom)
 * 'r': replacement predicates for external atoms
 *      (source ID is a constant term)
 * 'n': negated replacement predicates for external atoms (for guessing rules)
 *      (source ID is a constant term)
 * 'f': FLP-calculation auxiliary predicate
 *      (source ID is a rule)
 */

namespace
{

struct AuxiliaryKey
{
  char type;
  ID id;

  AuxiliaryKey(char type, ID id):
    type(type), id(id) {}
	inline bool operator==(const AuxiliaryKey& k2) const
    { return type == k2.type && id == k2.id; }
};

std::size_t hash_value(const AuxiliaryKey& key)
{
  std::size_t seed = 0;
  boost::hash_combine(seed, key.type);
  boost::hash_combine(seed, key.id.kind);
  boost::hash_combine(seed, key.id.address);
  return seed;
}

// we cannot use Term here because we want to store the
// whole ID, not only the kind
struct AuxiliaryValue
{
  std::string symbol;
  ID id;
  AuxiliaryValue(const std::string& symbol, ID id):
    symbol(symbol), id(id) {}
};

typedef boost::unordered_map<AuxiliaryKey, AuxiliaryValue>
  AuxiliaryStorage;

} // namespace {

struct Registry::Impl
{
  AuxiliaryStorage auxSymbols;
};


Registry::Registry():
  pimpl(new Impl)
{
}

// creates a real deep copy
//explicit
Registry::Registry(const Registry& other):
  terms(other.terms),
  ogatoms(other.ogatoms),
  onatoms(other.onatoms),
  batoms(other.batoms),
  aatoms(other.aatoms),
  eatoms(other.eatoms),
  rules(other.rules),
  pimpl(new Impl(*other.pimpl))
{
}

// it is very important that this destructor is not in the .hpp file,
// because only in the .cpp file it knows how to free pimpl
Registry::~Registry()
{
}

std::ostream& Registry::print(std::ostream& o) const
{
  return
    o <<
      "REGISTRY BEGIN" << std::endl <<
      "terms:" << std::endl <<
      terms <<
      "preds:" << std::endl <<
      preds <<
      "ogatoms:" << std::endl <<
      ogatoms <<
      "onatoms:" << std::endl <<
      onatoms <<
      "batoms:" << std::endl <<
      batoms <<
      "aatoms:" << std::endl <<
      aatoms <<
      "eatoms:" << std::endl <<
      eatoms <<
      "matoms:" << std::endl <<
      matoms <<
      "rules:" << std::endl <<
      rules <<
      "module table:" << std::endl <<
      moduleTable <<
      "REGISTRY END" << std::endl;
}

// lookup ground or nonground ordinary atoms (ID specifies this)
const OrdinaryAtom& Registry::lookupOrdinaryAtom(ID id) const
{
  assert(id.isOrdinaryAtom());
  if( id.isOrdinaryGroundAtom() )
    return ogatoms.getByID(id);
  else
    return onatoms.getByID(id);
}

// get all external atom IDs in tuple and recursively in aggregates in tuple
// append these ids to second given tuple
void Registry::getExternalAtomsInTuple(
    const Tuple& t, Tuple& out) const
{
  for(Tuple::const_iterator itt = t.begin(); itt != t.end(); ++itt)
  {
    if( itt->isExternalAtom() )
    {
      out.push_back(*itt);
    }
    else if( itt->isAggregateAtom() )
    {
      // check recursively within!
      const AggregateAtom& aatom = aatoms.getByID(*itt);
      getExternalAtomsInTuple(aatom.atoms, out);
    }
  }
}

namespace
{
  // assume, that oatom.id and oatom.tuple is initialized!
  // assume, that oatom.text is not initialized!
  // oatom.text will be modified
  ID storeOrdinaryAtomHelper(
      Registry* reg,
      OrdinaryAtom& oatom,
      OrdinaryAtomTable& oat)
  {
    ID ret = oat.getIDByTuple(oatom.tuple);
    if( ret == ID_FAIL )
    {
      // text
      std::stringstream s;
      RawPrinter printer(s, reg);
      // predicate
      printer.print(oatom.tuple.front());
      if( oatom.tuple.size() > 1 )
      {
        Tuple t(oatom.tuple.begin()+1, oatom.tuple.end());
        s << "(";
        printer.printmany(t,",");
        s << ")";
      }
      oatom.text = s.str();

      ret = oat.storeAndGetID(oatom);
      DBGLOG(DBG,"stored oatom " << oatom << " which got " << ret);
    }
    return ret;
  }
}

// ground version
ID Registry::storeOrdinaryGAtom(OrdinaryAtom& ogatom)
{
  return storeOrdinaryAtomHelper(this, ogatom, ogatoms);
}

// nonground version
ID Registry::storeOrdinaryNAtom(OrdinaryAtom& onatom)
{
  return storeOrdinaryAtomHelper(this, onatom, onatoms);
}

ID Registry::storeTerm(Term& term)
{
  ID ret = terms.getIDByString(term.symbol);
  if( ret == ID_FAIL )
  {
    ret = terms.storeAndGetID(term);
    DBGLOG(DBG,"stored term " << term << " which got " << ret);
  }
  return ret;
}

ID Registry::getAuxiliaryConstantSymbol(char type, ID id)
{
  DBGLOG_SCOPE(DBG,"gACS",false);
  DBGLOG(DBG,"getAuxiliaryConstantSymbol for " << type << " " << id);

  // lookup auxiliary
  AuxiliaryKey key(type,id);
  AuxiliaryStorage::const_iterator it =
    pimpl->auxSymbols.find(key);
  if( it != pimpl->auxSymbols.end() )
  {
    DBGLOG(DBG,"found " << it->second.id);
    return it->second.id;
  }

  // not found

  // create symbol
  std::ostringstream s;
  s << "aux_" << type << "_" << std::hex << id.kind << "_" << id.address;
  AuxiliaryValue av(s.str(), ID_FAIL);
  DBGLOG(DBG,"created symbol '" << av.symbol << "'");
  Term term(
      ID::MAINKIND_TERM | ID::SUBKIND_TERM_CONSTANT | ID::PROPERTY_TERM_AUX,
      av.symbol);

  // register ID for symbol
  av.id = terms.getIDByString(term.symbol);
  if( av.id != ID_FAIL)
    throw FatalError("auxiliary collision with symbol '" +
        term.symbol + "' (or programming error)!");
  av.id = terms.storeAndGetID(term);

  // register auxiliary
  pimpl->auxSymbols.insert(std::make_pair(key, av));

  // return
  DBGLOG(DBG,"returning id " << av.id << " for aux symbol " << av.symbol);
  return av.id;
}

DLVHEX_NAMESPACE_END

