/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * Copyright (C) 2011, 2012, 2013 Christoph Redl
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
 * @file   GraphvizHelpers.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Helpers for creating graphviz output.
 */

#ifndef GRAPHVIZHELPERS_HPP_INCLUDED__20082011
#define GRAPHVIZHELPERS_HPP_INCLUDED__20082011

#include "dlvhex2/PlatformDefinitions.h"

#include <string>
#include <iosfwd>

DLVHEX_NAMESPACE_BEGIN

namespace graphviz
{
/** \brief Writes \p s to \p o end escapes special symbols.
  * @param o Output stream.
  * @param s String. */
void escape(std::ostream& o, const std::string& s);

}

DLVHEX_NAMESPACE_END

#endif // GRAPHVIZHELPERS_HPP_INCLUDED__20082011
