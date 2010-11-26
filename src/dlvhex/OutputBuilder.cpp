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
 * @file   OutputBuilder.cpp
 * @author Peter Schueller
 * 
 * @brief  Generic output builders.
 */

#include "dlvhex/OutputBuilder.h"
#include "dlvhex/PluginInterface.h"
#include "dlvhex/ProgramCtx.h"

#include <cassert>
#include <string>
#include <ostream>

DLVHEX_NAMESPACE_BEGIN

void GenericOutputBuilder::buildResult(
  std::ostream& out, ResultsPtr results)
{
  assert(results != 0);
  AnswerSet::Ptr as = results->getNextAnswerSet();
  while( as != 0 )
  {
    printAnswerSet(out, as);
  }
}

void GenericOutputBuilder::printAnswerSet(
  std::ostream& out, AnswerSetPtr as)
{
  #warning no weak treatment!
  InterpretationConstPtr i = as->interpretation;
  std::cout << i << std::endl;
}

DLVHEX_NAMESPACE_END

/* vim: set noet sw=2 ts=8 tw=80: */

// Local Variables:
// mode: C++
// End:
