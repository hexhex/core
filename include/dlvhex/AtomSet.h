/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
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
 * @file   AtomSet.h
 * @author Roman Schindlauer
 * @author Thomas Krennwallner
 * @date   Tue Feb  7 17:20:32 CET 2006
 * 
 * @brief  AtomSet class.
 * @todo   Introduce a proper Interpretation class.
 * 
 */

#if !defined(_DLVHEX_ATOMSET_H)
#define _DLVHEX_ATOMSET_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/BaseAtom.h"

#include <set>
#include <list>

DLVHEX_NAMESPACE_BEGIN

/**
 * @brief Custom compare operator.
 *
 * In order to treat the internal atom storage as a set of Atoms instead of
 * a set of AtomPtr, we define a custom compare operator that dereferences
 * the AtomPtrs.
 */
struct DLVHEX_EXPORT AtomCompare
{
  bool 
  operator() (const AtomPtr& a, const AtomPtr& b) const
  {
    return *a < *b;
  }
};


/**
 * @brief An AtomSet is a set of Atoms.
 *
 * \ingroup dlvhextypes
 *
 * The atom storage is a set of AtomPtrs, using std::set with a custom
 * compare operator that dereferences the AtomPtrs. This ensures that not
 * the pointers are uniquely inserted, but the Atoms themselves
 * (std::set::insert() uses the compare operator for determining element
 * existence).
 */
typedef std::set<AtomPtr, AtomCompare> AtomSet;


/**
 * @brief An AtomList is a list of Atoms.
 *
 * \ingroup dlvhextypes
 *
 * The atom storage is a list of AtomPtrs. Use
 * boost::make_indirect_iterator for easy access.
 */
typedef std::list<AtomPtr> AtomList;


//
// for compatibility reasons
//
typedef AtomSet Interpretation;


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_ATOMSET_H */


// Local Variables:
// mode: C++
// End:
