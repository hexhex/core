/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schller
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
 * @file   PlainAuxPrinter.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Helper for printing auxiliary objects for the user.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#include "dlvhex2/PlainAuxPrinter.h"
#include "dlvhex2/Printer.h"

DLVHEX_NAMESPACE_BEGIN

PlainAuxPrinter::PlainAuxPrinter(RegistryPtr reg):
reg(reg)
{
}


// print an ID and return true,
// or do not print it and return false
bool PlainAuxPrinter::print(
std::ostream& out, ID id, const std::string& prefix) const
{
    // simply print all IDs
    assert(id.isOrdinaryGroundAtom() && id.isAuxiliary());
    out << prefix << reg->ogatoms.getByAddress(id.address).text;
    return true;
}


DLVHEX_NAMESPACE_END
