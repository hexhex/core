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
 * @file   PlainAuxPrinter.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Helper for printing auxiliary objects for the user.
 */

#ifndef PLAIN_AUX_PRINTER_HPP_INCLUDED__18012011
#define PLAIN_AUX_PRINTER_HPP_INCLUDED__18012011

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/Registry.h"

DLVHEX_NAMESPACE_BEGIN

/** \brief Prints auxiliary atoms in a generic fashion of kind aux_XXX(...). */
class PlainAuxPrinter:
public AuxPrinter
{
    public:
        /** \brief Constructor.
         * @param reg Registry to use for resolving IDs. */
        PlainAuxPrinter(RegistryPtr reg);

        /** \brief Print an ID.
         * @param out Stream to print \p id to.
         * @param id ID of an auxiliary atom.
         * @param prefix String to print before \p id.
         * @return True. */
        virtual bool print(std::ostream& out, ID id, const std::string& prefix) const;
    protected:
        /** \brief reg Registry to use for resolving IDs. */
        RegistryPtr reg;
};

DLVHEX_NAMESPACE_END
#endif                           // PLAIN_AUX_PRINTER_HPP_INCLUDED__18012011
