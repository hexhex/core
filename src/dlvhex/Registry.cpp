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
#include "dlvhex/Printer.hpp"

DLVHEX_NAMESPACE_BEGIN

std::ostream& Registry::print(std::ostream& o) const
{
  return
    o <<
      "REGISTRY BEGIN" << std::endl <<
      "terms:" << std::endl <<
      terms <<
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
      "rules:" << std::endl <<
      rules <<
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

DLVHEX_NAMESPACE_END

