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
 * @file   Term.h
 * @author Thomas Krennwallner <tkren@kr.tuwien.ac.at>
 * @date   Mon Jul 12 08:17:38 2010
 * 
 * @brief  Primitives for Symbol and Atom tables.
 * 
 * @todo Check efficiency and conversions. Maybe we need Tuple* for
 * the AtomTable? We also have to enforce that Atom and Term can be
 * packed into 64bit integers. Needs to be checked in configure.ac or
 * here...
 * 
 */


#if !defined(_DLVHEX_TERM_H)
#define _DLVHEX_TERM_H

#include "dlvhex/PlatformDefinitions.h"

#include <vector>
#include <iterator>
#include <algorithm>
#include <iosfwd>


DLVHEX_NAMESPACE_BEGIN

/// we reserve 32bits for addressing the SymbolTable
typedef uint32_t TermID;

/// used to encode Terms as single integer for easy Term copying
typedef uint64_t PackedTerm;

/// a Term consists of a type and an id
struct Term
{
  enum Type
    {
      SYMBOL = 0,  // dedicated symbol table for term symbols
      STRING = 2,  // dedicated symbol table for term strings (with namespaces collapsed)
      INTEGER = 1, // for integers, the id is the value (no symbol table)
      VARIABLE = 3 // dedicated symbol table for variables
    };

  Type type;     /// type of the atom
  TermID id;     /// id to address the SymbolTable


  // ctor
  Term(Type t, TermID i)
    : type(t), id(i)
  { }


  // Conversion between PackedTerm and Term. We assume that
  // sizeof(TermType)==4 and sizeof(TermID)==4. Term packing avoids
  // the need to specify hash_value(const Term&), operator==(const
  // Tuple&,const Tuple&), and operator==(const Term&) for
  // boost::multi_index_container.

  /// casts PackedTerm to Term by unpacking type and id from a 64bit
  /// integer
  Term(const PackedTerm& t)
    : type((Type)(t >> (Type)31)), // first 4 MSBs encode the type
      id(t & 0xffffffff) // the 4 LSBs encode the id
  { }

  /// casts Term to PackedTerm by packing type and id into a single
  /// 64bit integer
  operator PackedTerm() const
  {
    // we assume that sizeof(TermType)==4 and sizeof(TermID)==4
    return ( (PackedTerm)type << (PackedTerm)31 ) | ( (PackedTerm)id );
  }
};


/// a Tuple is a vector of Terms
typedef std::vector<Term> Tuple;


/// prints Tuple as list of PackedTerm
std::ostream&
operator<< (std::ostream& os, const Tuple& t)
{
  os << '(';

  if (!t.empty())
    {
      std::copy(t.begin(), t.end() - 1,
		std::ostream_iterator<PackedTerm>(os, " "));
      os << t.back();
    }

  return os << ')';
}


DLVHEX_NAMESPACE_END


#endif /* _DLVHEX_TERM_H */

/* vim: set noet sw=4 ts=4 tw=80: */


// Local Variables:
// mode: C++
// End:
