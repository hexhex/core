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
 * @file   PlainAuxPrinter.hpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Helper for printing auxiliary objects for the user.
 */

#ifndef PLAIN_AUX_PRINTER_HPP_INCLUDED__18012011
#define PLAIN_AUX_PRINTER_HPP_INCLUDED__18012011

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/Registry.hpp"

DLVHEX_NAMESPACE_BEGIN

class PlainAuxPrinter:
  public AuxPrinter
{
public:
  PlainAuxPrinter(RegistryPtr reg);

  // print an ID and return true,
  // or do not print it and return false
  virtual bool print(std::ostream& out, ID id, const std::string& prefix) const;
protected:
  RegistryPtr reg;
};

DLVHEX_NAMESPACE_END

#endif // PLAIN_AUX_PRINTER_HPP_INCLUDED__18012011

