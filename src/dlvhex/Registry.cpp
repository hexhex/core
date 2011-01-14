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

DLVHEX_NAMESPACE_END

