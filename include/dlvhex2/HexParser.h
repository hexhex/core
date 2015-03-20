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
 * @file   HexParser.h
 * @author Peter Schüller
 * 
 * @brief  HEX parser interface and the basic HEX parser
 */

#ifndef HEXPARSER_HPP_INCLUDED__14102010
#define HEXPARSER_HPP_INCLUDED__14102010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/fwd.h"
#include "dlvhex2/Error.h"

#include <vector>
#include <iosfwd>

DLVHEX_NAMESPACE_BEGIN

/** \brief Implements the parser for HEX-programs. */
class DLVHEX_EXPORT HexParser
{
public:
  /** \brief Destructor. */
  virtual ~HexParser();
  /** \brief Parses \p in into \p out.
    * @param in InputProvider to parse from.
    * @param out ProgramCtx to receive EDB and IDB of the parsed program. */
  virtual void parse(InputProviderPtr in, ProgramCtx& out) = 0;
};
typedef boost::shared_ptr<HexParser> HexParserPtr;

/**
 * @brief Parses HEX-programs, extendable by parser modules.
 */
class DLVHEX_EXPORT ModuleHexParser:
  public HexParser
{
public:
  /** \brief Adds an additional module the the parser.
    *
    * Modules are used to parse special non-standard HEX-syntax as
    * supported e.g. by plugins.
    * @param module ParserModule to add. */
  virtual void registerModule(HexParserModulePtr module);

public:
  /** \brief Parses \p in into \p out using the registered modules.
    * @param in InputProvider to parse from.
    * @param out ProgramCtx to receive EDB and IDB of the parsed program. */
  virtual void parse(InputProviderPtr in, ProgramCtx& out);

protected:
  /** \brief Currently registered modules. */
  std::vector<HexParserModulePtr> modules;
};
typedef boost::shared_ptr<ModuleHexParser> ModuleHexParserPtr;

DLVHEX_NAMESPACE_END

#endif // HEXPARSER_HPP_INCLUDED__14102010

// Local Variables:
// mode: C++
// End:
