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
 * @file   OrdinaryAtom.hpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  OrdinaryAtom: store ordinary atoms
 */

#ifndef ORDINARYATOM_HPP_INCLUDED__12102010
#define ORDINARYATOM_HPP_INCLUDED__12102010

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/Logger.hpp"
#include "dlvhex/ID.hpp"

DLVHEX_NAMESPACE_BEGIN

struct OrdinaryAtom:
  private ostream_printable<OrdinaryAtom>
{
  // the kind part of the ID of this ordinary atom
  IDKind kind;

  // the textual representation of the whole thing
  // this is stored for efficient parsing and printing
  // @todo make this a template parameter of OrdinaryAtom, so that we can store various "efficient" representations here (depending on the solver dlvhex should work with; e.g., we could store clasp- or dlv-library internal atom representations here and index them) if we don't need it, we can replace it by an empty struct and conserve space
  std::string text;

  // the ID representation of this atom
  Tuple tuple;

  OrdinaryAtom(IDKind kind, const std::string& text, const Tuple& tuple):
    kind(kind), text(text), tuple(tuple) {}
  std::ostream& print(std::ostream& o) const
    { return o << "OrdinaryAtom(" << text << " " << printvector(tuple) << ")"; }
};


DLVHEX_NAMESPACE_END

#endif // ORDINARYATOM_HPP_INCLUDED__12102010
