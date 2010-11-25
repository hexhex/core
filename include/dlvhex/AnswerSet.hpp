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
 * @file   AnswerSet.hpp
 * @author Peter Schüller
 * 
 * @brief  Answer set container: holds interpretation and information about model cost (for weak constraints).
 */

#ifndef ANSWER_SET_HPP_INCLUDED__09112010
#define ANSWER_SET_HPP_INCLUDED__09112010

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/Interpretation.hpp"
#include "dlvhex/Logger.hpp"

#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

struct Registry;
typedef boost::shared_ptr<Registry> RegistryPtr;

// this is kind of a program context for pure (=non-HEX) ASPs
struct AnswerSet:
  public ostream_printable<AnswerSet>
{
  // types
  typedef boost::shared_ptr<AnswerSet> Ptr;
  typedef boost::shared_ptr<const AnswerSet> ConstPtr;

  // storage
  Interpretation::Ptr interpretation;
  int costWeight;
  int costLevel;

  AnswerSet(RegistryPtr registry):
    interpretation(new Interpretation(registry)), costWeight(-1), costLevel(-1) {}
  virtual ~AnswerSet() {}

  virtual std::ostream& print(std::ostream& o) const;
};

typedef boost::shared_ptr<AnswerSet> AnswerSetPtr;

DLVHEX_NAMESPACE_END

#endif // ANSWER_SET_HPP_INCLUDED__09112010

// Local Variables:
// mode: C++
// End:
