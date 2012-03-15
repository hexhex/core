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
 * @file   OrdinaryASPProgram.hpp
 * @author Christoph Redl
 * @date Fri Jan 20 2012
 * 
 * @brief  Declaration of ASP-programs as passed to
 * InternalGroundASPSolver, InternalGroundDASPSolver and InternalGrounder.
 * 
 */

#if !defined(_DLVHEX_ORDINARYASPPROGRAM_HPP)
#define _DLVHEX_ORDINARYASPPROGRAM_HPP


#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/AnswerSet.h"
#include "dlvhex2/Error.h"
#include "dlvhex2/ConcurrentMessageQueueOwning.h"

#include <boost/shared_ptr.hpp>
#include <vector>
#include <list>

DLVHEX_NAMESPACE_BEGIN

struct Registry;
typedef boost::shared_ptr<Registry> RegistryPtr;

// this is kind of a program context for pure (=non-HEX) ASPs
struct OrdinaryASPProgram
{
  RegistryPtr registry;
  std::vector<ID> idb;
  Interpretation::ConstPtr edb;
  uint32_t maxint;
  Interpretation::ConstPtr mask;

  OrdinaryASPProgram(
      RegistryPtr registry,
      const std::vector<ID>& idb,
      Interpretation::ConstPtr edb,
      uint32_t maxint = 0,
      Interpretation::ConstPtr mask = Interpretation::ConstPtr()):
    registry(registry), idb(idb), edb(edb), maxint(maxint), mask(mask) {}
};

DLVHEX_NAMESPACE_END

#endif // _DLVHEX_ORDINARYASPPROGRAM_HPP

// Local Variables:
// mode: C++
// End:
