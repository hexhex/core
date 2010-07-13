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
 * @file   Atom.h
 * @author Thomas Krennwallner <tkren@kr.tuwien.ac.at>
 * @date   Mon Jul 12 08:17:38 2010
 * 
 * @brief  Primitives for the atom tables.
 * 
 * @todo Check efficiency and conversions. Maybe we need Tuple* for
 * the AtomTable? We also have to enforce that Atom and Term can be
 * packed into 64bit integers. Needs to be checked in configure.ac or
 * here...
 * 
 */


#if !defined(_DLVHEX_ATOM_H)
#define _DLVHEX_ATOM_H

#include "dlvhex/PlatformDefinitions.h"

DLVHEX_NAMESPACE_BEGIN


/// we reserve 32bits for addressing the atom tables
typedef uint32_t AtomID;


/// a Literal may be negative: used to pack Atom into a 64bit type
typedef int64_t Literal;


/// an Atom consists of a type and an id
struct Atom
{
  /// different types of atoms
  enum Type
    {
      ORDINARY = 0,
      BUILTIN = 1,
      AGGREGATE = 2,
      EXTERNAL = 3
    };


  uint32_t type; /// type of the atom
  AtomID id;     /// id to address the corresponding table specifed by type


  /// ctor
  Atom(Type t, AtomID i)
    : type(t), id(i)
  { }


  // Conversion between Literal and Atom. We assume that
  // sizeof(AtomType)==4 and sizeof(AtomID)==4

  //
  // trick 17 for computing absolute values without branching, see
  // http://graphics.stanford.edu/~seander/bithacks.html#IntegerAbs
  //
  // rationale: we cannot simply take the MSBs and LSBs of Literals
  // because negative integers are stored as two's complement of that
  // integer, i.e., the number -1 is represented as the 64bit value
  // 0xffffffffffffffff.
  //
# define LITERALMASK(v) (Literal)( v >> sizeof(Literal) * CHAR_BIT - 1 )
# define LITERALABS(v) (Literal)( ( v + LITERALMASK(v) ) ^ LITERALMASK(v) )

  /// casts a Literal (possibly negative) to Atom by unpacking type
  /// and id from a 64bit signed integer
  Atom(const Literal& l)
    : type( (uint32_t) ( LITERALABS(l) >> (uint32_t)31 ) ), // first 32 MSBs encode the type
      id( (uint32_t) ( LITERALABS(l) & 0xffffffff ) ) // the 32 LSBs encode the id
  { }

  /// casts Atom to a positive Literal by packing type and id into a
  /// single 64bit integer
  operator Literal() const
  {
    return ( (Literal)type << (Literal)31 ) | ( (Literal)id );
  }
};



DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_ATOM_H */

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
