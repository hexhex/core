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
 * @file   HexParser.hpp
 * @author Peter Schüller
 * 
 * @brief  HEX parser interface and the basic HEX parser
 */

#ifndef HEXPARSER_HPP_INCLUDED__14102010
#define HEXPARSER_HPP_INCLUDED__14102010

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/fwd.hpp"
#include "dlvhex/Error.h"

#include <iosfwd>

DLVHEX_NAMESPACE_BEGIN

class ProgramCtx;

class DLVHEX_EXPORT HexParser
{
public:
  virtual ~HexParser();
  virtual void parse(InputProviderPtr in, ProgramCtx& out) = 0;
};
typedef boost::shared_ptr<HexParser> HexParserPtr;

/**
 * @brief Parses HEX-programs.
 */
class DLVHEX_EXPORT BasicHexParser:
  public HexParser
{
public:
  virtual void parse(InputProviderPtr in, ProgramCtx& out);
};

/**
 * @brief Parses HEX-programs, extendable by parser modules.
 */
class DLVHEX_EXPORT ModuleHexParser:
  public HexParser
{
public:
  virtual void registerModule(HexParserModulePtr module);

public:
  virtual void parse(InputProviderPtr in, ProgramCtx& out);
};

#warning TODO move this into separate header "HexParserModule.hpp", only used by parser and extending parsers
# if 0
class HexParserModule
{
// TODO here we can do stuff including complicated module type containing spirit iterator/grammar types
};
#endif

DLVHEX_NAMESPACE_END

#endif // HEXPARSER_HPP_INCLUDED__14102010

// Local Variables:
// mode: C++
// End:
