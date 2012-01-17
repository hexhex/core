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
 * @file   Module.hpp
 * @author Tri Kurniawan Wijaya <trikurniawanwijaya@gmail.com>
 * 
 * @brief  Module structure: stores module name, the associated input list, edb and idb
 *
 */ 

#ifndef MODULE_HPP_INCLUDED__20122010
#define MODULE_HPP_INCLUDED__20122010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ID.hpp"
#include "dlvhex2/Logger.hpp"

DLVHEX_NAMESPACE_BEGIN

// Module structure, used in ModuleTable.hpp
struct Module:
  private ostream_printable<Module>
{
  // the kind part of the ID of this symbol
  IDKind kind; // not used in module
  std::string moduleName;
  int inputList;
  int edb;
  int idb;

//  Module(IDKind kind, const std::string& symbol, const int& arity): kind(kind), symbol(symbol), arity(arity)
//		{ assert(ID(kind,0).isTerm()); }

  Module(): moduleName(""), inputList(-1), edb(-1), idb(-1) { }
  Module(const std::string& moduleName, const int& inputList, const int& edb, const int& idb): 
                moduleName(moduleName), inputList(inputList), edb(edb), idb(idb)
		{  }
  inline bool operator==(const Module& mod2) const 
         { return moduleName == mod2.moduleName && inputList == mod2.inputList && edb == mod2.edb && idb == mod2.idb; }
  inline bool operator!=(const Module& mod2) const 
         { return moduleName != mod2.moduleName || inputList || mod2.inputList || edb != mod2.edb || idb != mod2.idb; }
  std::ostream& print(std::ostream& o) const
    { return o << "Module(" << moduleName << ", inputList: " << inputList << ", edb: " << edb << ", idb: " << idb << ")"; }
};

const Module MODULE_FAIL("", -1, -1, -1);

DLVHEX_NAMESPACE_END

#endif // MODULE_HPP_INCLUDED__20122010
