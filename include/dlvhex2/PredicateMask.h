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
 * @file   PredicateMask.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Incrementally managed bitmask for projecting ground
 *         interpretations to certain predicates.
 */

#ifndef PREDICATEMASK_HPP_INCLUDED__27012011
#define PREDICATEMASK_HPP_INCLUDED__27012011

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/ID.h"

#include <boost/thread/mutex.hpp>

#include <set>

DLVHEX_NAMESPACE_BEGIN

class PredicateMask
{
public:
  PredicateMask();
  ~PredicateMask();

  // copy constructor and assignment  have tricky implementations
  //
  // copying a mask is not useful, masks should and can be shared
  // (for a new registry they need to be recreated anyways)
  // therefore copy constructing with maski != NULL will log a warning
  PredicateMask(const PredicateMask&);
  // assigning a mask is like copying and will likewise log a warning
  PredicateMask& operator=(const PredicateMask&);

  // set registry (cannot change registry!) and create initial interpretation
  void setRegistry(RegistryPtr registry);

  // add predicate
  // incrementally updates mask for new pred up to known address of other preds
  // (does not update mask for other preds!)
  void addPredicate(ID pred);

  // incrementally updates mask for all predicates
  void updateMask();

  // get mask
  InterpretationConstPtr mask() const
    { return maski; }

protected:
  // addresses of IDs of all relevant input predicates for this eatom
  // (the corresponding IDKinds are ID::MAINKIND_TERM | ID::SUBKIND_CONSTANT_TERM
  // with maybe auxiliary bit set
  std::set<IDAddress> predicates;
  // bitset interpretation for masking inputs
  InterpretationPtr maski;
  // address of the last ogatom already inspected for updating mask
  IDAddress knownAddresses;

  boost::mutex updateMutex;
};

DLVHEX_NAMESPACE_END

#endif // PREDICATEMASK_HPP_INCLUDED__27012011

