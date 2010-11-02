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
 * @file   Model.h
 * @author Thomas Krennwallner <tkren@kr.tuwien.ac.at>
 * @date   Thu Jul 15 13:47:22 2010
 * 
 * @brief  Model representation.
 * 
 * 
 */



#if !defined(_DLVHEX_MODEL_H)
#define _DLVHEX_MODEL_H

#include "dlvhex/PlatformDefinitions.h"

#include <boost/dynamic_bitset.hpp>

DLVHEX_NAMESPACE_BEGIN

/// bitset can be resized
typedef boost::dynamic_bitset<uint64_t> BitSet;


/// a partial model consists of two bitsets of same size to store the atom values
struct PartialModel
{
  enum AtomValue
  {
    FALSE = 0,
    TRUE = 1,
    UNKNOWN = 2,
    DC = 3
  };


  BitSet A;
  BitSet B;

  PartialModel(std::size_t n)
    : A(n, 0), B(n, 0)
  { }

  
  void
  resize(std::size_t n)
  {
    A.resize(n);
    B.resize(n);
  }
};

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_MODEL_H */


// Local Variables:
// mode: C++
// End:
