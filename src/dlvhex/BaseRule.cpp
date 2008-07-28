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
 * @file BaseRule.h
 * @author Roman Schindlauer
 * @author Thomas Krennwallner
 * @date Mon Jul 14 11:20:24 2008
 *
 * @brief Base Rule class.
 *
 */


#include "dlvhex/BaseRule.h"

#include "dlvhex/PrintVisitor.h"

DLVHEX_NAMESPACE_BEGIN


BaseRule::~BaseRule()
{ }


std::ostream&
operator<< (std::ostream& out, const BaseRule& rule)
{
  RawPrintVisitor rpv(out);
  const_cast<BaseRule*>(&rule)->accept(&rpv);
  return out;
}


DLVHEX_NAMESPACE_END


// Local Variables:
// mode: C++
// End:
