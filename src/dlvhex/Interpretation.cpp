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
#include "dlvhex/Printer.hpp"

DLVHEX_NAMESPACE_BEGIN

Interpretation::Interpretation(RegistryPtr registry):
  registry(registry),
  bits()
{
}

Interpretation::~Interpretation()
{
}

unsigned Interpretation::filter(FilterCallback cb)
{
  // simply enumerating and clearing bits does not work,
  // as modifying bits invalidates iterators (even the end iterator)
  // so we collect all things to filter out in a separate bitset

  Storage resetThose;

  // go through one-bits
  for(Storage::enumerator it = bits.first();
      it != bits.end(); ++it)
  {
    if( !cb(*it) )
    {
      resetThose.set(*it);
    }
  }

  for(Storage::enumerator it = resetThose.first();
      it != resetThose.end(); ++it)
  {
    // now clear bits
    clearFact(*it);
  }

  return resetThose.count();
}

std::ostream& Interpretation::print(std::ostream& o) const
{
  return print(o, "{", ",", "}");
}

std::ostream& Interpretation::printWithoutPrefix(std::ostream& o) const
{
  return printWithoutPrefix(o, "{", ",", "}");
}

std::ostream& Interpretation::printAsFacts(std::ostream& o) const
{
  print(o, "", ".", "");
  // make sure the last fact (if any fact exists) gets a dot
  if( bits.first() != bits.end() )
    o << ".";
  return o;
}

std::ostream& Interpretation::print(
    std::ostream& o,
    const char* first, const char* sep, const char* last) const
{
  Storage::enumerator it = bits.first();
  o << first;
  RawPrinter printer(o, registry);
  if( it != bits.end() )
  {
    printer.print(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *it));
    it++;
    for(; it != bits.end(); ++it)
    {
      o << sep;
      printer.print(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *it));
    }
  }
  return o << last;
}

std::ostream& Interpretation::printWithoutPrefix(
    std::ostream& o,
    const char* first, const char* sep, const char* last) const
{
  Storage::enumerator it = bits.first();
  o << first;
  RawPrinter printer(o, registry);
  if( it != bits.end() )
  {
    printer.printWithoutPrefix(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *it));
    it++;
    for(; it != bits.end(); ++it)
    {
      o << sep;
      printer.printWithoutPrefix(ID(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG, *it));
    }
  }
  return o << last;
}


void Interpretation::add(const Interpretation& other)
{
  bits |= other.bits;
}

void Interpretation::bit_and(const Interpretation& other)
{
  bits &= other.bits;
}

bool Interpretation::operator==(const Interpretation& other) const
{
  return bits == other.bits;
}

bool Interpretation::operator<(const Interpretation& other) const
{
  return bits < other.bits;
}

DLVHEX_NAMESPACE_END
