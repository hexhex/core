/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schüller
 * Copyright (C) 2011-2015 Christoph Redl
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
 * @file   Module.h
 * @author Tri Kurniawan Wijaya <trikurniawanwijaya@gmail.com>
 *
 * @brief  Module structure: stores module name, the associated input list, edb and idb
 *
 */

#ifndef MODULE_HPP_INCLUDED__20122010
#define MODULE_HPP_INCLUDED__20122010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/Logger.h"

DLVHEX_NAMESPACE_BEGIN

/** \brief Module structure, used in ModuleTable.h. */
struct Module:
private ostream_printable<Module>
{
    /** \brief The kind part of the ID of this symbol.
     *
     * Not used in module. */
    IDKind kind;
    /** \brief Name of the module. */
    std::string moduleName;
    /** \brief Input to the module. */
    int inputList;
    /** \brief EDB of the module. */
    int edb;
    /** \brief IDB of the module. */
    int idb;

    /** \brief Constructor. */
    Module(): moduleName(""), inputList(-1), edb(-1), idb(-1) { }
    /** \brief Constructor.
     * @param moduleName See Module::moduleName.
     * @param inputList See Module::inputList.
     * @param edb See Module::edb.
     * @param idb See Module::idb. */
    Module(const std::string& moduleName, const int& inputList, const int& edb, const int& idb):
    moduleName(moduleName), inputList(inputList), edb(edb), idb(idb)
        {  }
    /** \brief Comparison of modules.
     * @param mod2 The module to compare this one to.
     * @return True if this module is equal to \p mod2 and false otherwise. */
    inline bool operator==(const Module& mod2) const
        { return moduleName == mod2.moduleName && inputList == mod2.inputList && edb == mod2.edb && idb == mod2.idb; }
    /** \brief Comparison of modules.
     * @param mod2 The module to compare this one to.
     * @return True if this module is not equal to \p mod2 and false otherwise. */
    inline bool operator!=(const Module& mod2) const
        { return moduleName != mod2.moduleName || inputList || mod2.inputList || edb != mod2.edb || idb != mod2.idb; }
    std::ostream& print(std::ostream& o) const
        { return o << "Module(" << moduleName << ", inputList: " << inputList << ", edb: " << edb << ", idb: " << idb << ")"; }
};

const Module MODULE_FAIL("", -1, -1, -1);

DLVHEX_NAMESPACE_END
#endif                           // MODULE_HPP_INCLUDED__20122010

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
