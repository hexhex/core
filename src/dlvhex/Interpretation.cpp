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
 * @file Interpretation.cpp
 * @author Peter Schüller
 *
 * @brief Implementation of the (bitset-)interpretation.
 */

#include "dlvhex/Interpretation.hpp"
#include "dlvhex/Logger.hpp"
#include "dlvhex/ProgramCtx.h"

DLVHEX_NAMESPACE_BEGIN

Interpretation::Interpretation(RegistryPtr registry):
  registry(registry),
  bits()
{
}

Interpretation::~Interpretation()
{
}

std::ostream& Interpretation::print(std::ostream& o) const
{
  Storage::enumerator it = bits.first();
  o << "{";
  RawPrinter printer(o, registry);
  if( it != bits.end() )
  {
    printer.print(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *it));
    it++;
    for(; it != bits.end(); ++it)
    {
      o << ",";
      printer.print(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *it));
    }
  }
  return o << "}";
}

void Interpretation::add(const Interpretation& other)
{
  if( other.bits.size() > bits.size() )
    bits.resize(other.bits.size());
  bits |= other.bits;
}

void Interpretation::reserve(IDAddress maxid)
{
  if( bits.size() <= maxid )
    bits.resize(maxid+1);
}

void Interpretation::setFact(IDAddress id)
{
  //if( bits.size() <= id )
  //  bits.resize(id+1);
  bits.set(id);
}

DLVHEX_NAMESPACE_END
