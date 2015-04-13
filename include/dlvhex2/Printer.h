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
 * @file Printer.h
 * @author Peter Schueller
 * @date
 *
 * @brief Printer classes for printing objects stored in registry given registry and ID.
 */

#ifndef PRINTER_HPP_INCLUDED_14012011
#define PRINTER_HPP_INCLUDED_14012011

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/ID.h"

#include <vector>
#include <string>
#include <iosfwd>

DLVHEX_NAMESPACE_BEGIN

/** \brief Prints dlvhex IDs in differnt formats depending on the implementation in a subclass. */
class DLVHEX_EXPORT Printer
{
    protected:
        /** \brief Output stream to print to. */
        std::ostream& out;
        /** \brief Registry used for resolving IDs. */
        Registry* registry;

    public:
        /** \brief Constructor.
         * @param out See Printer::out.
         * @param registry See Printer::registry. */
        Printer(std::ostream& out, Registry* registry):
        out(out), registry(registry) {}
        /** \brief Constructor.
         * @param out See Printer::out.
         * @param registry See Printer::registry. */
        Printer(std::ostream& out, RegistryPtr registry):
        out(out), registry(registry.get()) {}
        /** \brief Destructor. */
        virtual ~Printer() {}
        /** \brief Prints multiple IDs.
         * @param ids Vector of IDs.
         * @param separator String to print between two IDs. */
        void printmany(const Tuple& ids, const std::string& separator);
        /** \brief Prints a single ID.
         * @param id ID to print. */
        virtual void print(ID id) = 0;
};

/** \print Prints IDs in human-readable format. */
class DLVHEX_EXPORT RawPrinter:
public Printer
{
    private:
        std::string removeModulePrefix(const std::string& text);
    public:
        /** \brief Constructor.
         * @param out See Printer::out.
         * @param registry See Printer::registry. */
        RawPrinter(std::ostream& out, Registry* registry):
        Printer(out, registry) {}
        /** \brief Constructor.
         * @param out See Printer::out.
         * @param registry See Printer::registry. */
        RawPrinter(std::ostream& out, RegistryPtr registry):
        Printer(out, registry) {}
        virtual void print(ID id);
        /** \brief Prints a single ID without module prefix (cf. modular HEX).
         * @param id ID to print. */
        void printWithoutPrefix(ID id);
        /** \brief Prints an ID to a string.
         * @param reg Registry used for resolving IDs.
         * @param id ID to print.
         * @return String representation of \p id. */
        static std::string toString(RegistryPtr reg, ID id);
};

/** \brief Prints an ID to a string.
 * @param reg Registry used for resolving IDs.
 * @param id ID to print.
 * @return String representation of \p id. */
template<typename PrinterT>
std::string printToString(ID id, RegistryPtr reg)
{
    std::ostringstream s;
    PrinterT p(s, reg);
    p.print(id);
    return s.str();
}


/** \brief Prints multiple IDs to a string.
 * @param ids Vector of IDs to print.
 * @param separator String to print between two IDs.
 * @param reg Registry used for resolving IDs.
 * @return String representation of \p ids. */
template<typename PrinterT>
std::string printManyToString(
const Tuple& ids, const std::string& separator, RegistryPtr reg)
{
    std::ostringstream s;
    PrinterT p(s, reg);
    p.printmany(ids, separator);
    return s.str();
}


DLVHEX_NAMESPACE_END
#endif                           // PRINTER_HPP_INCLUDED_14012011
