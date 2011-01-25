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
 * @file   AnswerSetPrinterCallback.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Implementation of logging facility.
 */

#include "dlvhex/AnswerSetPrinterCallback.hpp"

// activate benchmarking if activated by configure option --enable-debug
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "dlvhex/Benchmarking.h"
#include "dlvhex/AnswerSet.hpp"
#include "dlvhex/Registry.hpp"

DLVHEX_NAMESPACE_BEGIN

AnswerSetPrinterCallback::AnswerSetPrinterCallback(
    bool keepAuxiliaryPredicates):
  keepAuxiliaryPredicates(keepAuxiliaryPredicates)
{
}

namespace
{
  struct FilterCallback
  {
    OrdinaryAtomTable& ogat;
    FilterCallback(RegistryPtr reg):
      ogat(reg->ogatoms)
    {
    }

    bool operator()(IDAddress addr)
    {
      const OrdinaryAtom& oa = ogat.getByAddress(addr);
      if( (oa.kind & ID::PROPERTY_ATOM_AUX) != 0 )
      {
        return false;
      }
      else
        return true;
    }
  };
}

bool AnswerSetPrinterCallback::operator()(
    AnswerSetPtr as)
{
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"AnswerSetPrinterCallback");
  FilterCallback cb(as->interpretation->getRegistry());
  unsigned rejected = as->interpretation->filter(cb);
  DBGLOG(DBG,"ASPrinterCB filtered " << rejected << " auxiliaries from interpretation");
  std::cout << *as << std::endl;
  // never abort
  return true;
}

DLVHEX_NAMESPACE_END

