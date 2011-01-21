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
 * @file ComfortPluginInterface.cpp
 * @author Peter Schueller
 *
 * @brief comfortable plugin interface implementation
 */

#include "dlvhex/ComfortPluginInterface.hpp"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "dlvhex/Benchmarking.h"
#include "dlvhex/Printer.hpp"
#include "dlvhex/ProgramCtx.h"

#include <sstream>

DLVHEX_NAMESPACE_BEGIN

std::ostream& ComfortTerm::print(std::ostream& o) const
{
  if( isInteger() )
    return o << intval;
  else
    return o << strval;
}

void ComfortAtom::calculateStrVal() const
{
  ComfortTuple::const_iterator it = tuple.begin();
  assert(it != tuple.end());

  std::ostringstream os;
  os << *it;
  it++;
  if( it != tuple.end() )
  {
    os << "(" << *it;
    it++;
    while(it != tuple.end())
    {
      os << "," << *it;
      it++;
    }
    os << ")";
  }
  strval = os.str();
}

std::ostream& ComfortAtom::print(std::ostream& o) const
{
  return o << toString();
}

// insert one atom
void ComfortInterpretation::insert(const ComfortAtom&)
{
  throw "not implemented";
  #warning todo implement
}

// insert all atoms from other interpretation
void ComfortInterpretation::insert(const ComfortInterpretation&)
{
  throw "not implemented";
  #warning todo implement
}

// remove atoms whose predicate matches a string in the given set
void ComfortInterpretation::remove(const std::set<std::string>& predicates)
{
  throw "not implemented";
  #warning todo implement
}

// remove atoms whose predicate does not match any string in the given set
void ComfortInterpretation::keep(const std::set<std::string>& predicates)
{
  throw "not implemented";
  #warning todo implement
}

// remove negative atoms
void ComfortInterpretation::keepPos()
{
  throw "not implemented";
  #warning todo implement
}

bool ComfortInterpretation::isConsistent() const
{
  throw "not implemented";
  #warning todo implement
}

// copy all atoms that match the specified predicate into destination interpretation
void ComfortInterpretation::matchPredicate(const std::string& predicate, ComfortInterpretation& destination) const
{
  throw "not implemented";
  #warning todo implement
}

// copy all atoms that unify with the specified predicate into destination interpretation
void ComfortInterpretation::matchAtom(const ComfortAtom& atom, ComfortInterpretation& destination) const
{
  throw "not implemented";
  #warning todo implement
}

// return set difference *this \ subtractThis
ComfortInterpretation ComfortInterpretation::difference(const ComfortInterpretation& subtractThis) const
{
  throw "not implemented";
  #warning todo implement
}

std::ostream& ComfortInterpretation::print(std::ostream& o) const
{
  return o << printrange(*this, "{", "}", ",");
}

void ComfortPluginAtom::retrieve(const Query&, Answer&)
{
  #warning TODO convert and all the stuff
}

DLVHEX_NAMESPACE_END

