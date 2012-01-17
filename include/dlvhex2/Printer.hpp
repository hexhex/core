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
 * @file Printer.hpp
 * @author Peter Schueller
 * @date
 *
 * @brief Printer classes for printing objects stored in registry given registry and ID.
 */

#ifndef PRINTER_HPP_INCLUDED_14012011
#define PRINTER_HPP_INCLUDED_14012011

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/fwd.hpp"
#include "dlvhex/ID.hpp"

#include <vector>
#include <string>
#include <iosfwd>

DLVHEX_NAMESPACE_BEGIN

class Printer
{
protected:
  std::ostream& out;
  Registry* registry;

public:
  Printer(std::ostream& out, Registry* registry):
    out(out), registry(registry) {}
  Printer(std::ostream& out, RegistryPtr registry):
    out(out), registry(registry.get()) {}
  virtual ~Printer() {}
  void printmany(const Tuple& ids, const std::string& separator);
  virtual void print(ID id) = 0;
};

class RawPrinter:
  public Printer
{
private: 
  std::string removeModulePrefix(const std::string& text);
public:
  RawPrinter(std::ostream& out, Registry* registry):
    Printer(out, registry) {}
  RawPrinter(std::ostream& out, RegistryPtr registry):
    Printer(out, registry) {}
  virtual void print(ID id);
  void printWithoutPrefix(ID id);
};

template<typename PrinterT>
std::string printToString(ID id, RegistryPtr reg)
{
  std::ostringstream s;
  PrinterT p(s, reg);
  p.print(id);
  return s.str();
}

#warning we should make this a const Range with concept check value_type = ID (simplifies a lot of code)
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

#endif // PRINTER_HPP_INCLUDED_14012011
