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
 * @file   PlatformDefinitions.h
 * @author Thomas Krennwallner
 * @date Mon Dec 25 10:46:58 CEST 2007
 *
 * @brief Platform-specific definitions.
 *
 *
 */

#if !defined(_DLVHEX_PLATFORMDEFINITIONS_H)
#define _DLVHEX_PLATFORMDEFINITIONS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#define DLVHEX_NAMESPACE_BEGIN namespace dlvhex {
#define DLVHEX_NAMESPACE_END   }
#define DLVHEX_NAMESPACE_USE   using namespace dlvhex;
#define DLVHEX_NAMESPACE       dlvhex::

// by default we build for POSIX systems
#ifndef WIN32
#ifndef POSIX
#define POSIX 1
#endif
#endif

#ifdef DLLEXPORT
#define DLVHEX_EXPORT __declspec(dllexport)
#else
#ifdef DLLIMPORT
#define DLVHEX_EXPORT __declspec(dllimport)
#else
#define DLVHEX_EXPORT
#endif
#endif                           /* DLLEXPORT/IMPORT */

#ifdef DLVHEXPLUGIN
#define DLVHEX_PLUGINEXPORT __declspec(dllexport)
#else
#define DLVHEX_PLUGINEXPORT
#endif

#ifdef _MSC_VER
#define WARNING(msg) __pragma (message(msg))
#else
// GCC does not expand macros in #warning pragmas, thus we use messages
#define _WARNING(x) _Pragma (#x)
#define WARNING(msg) _WARNING(message (#msg))
#endif

// on Windows we need to export the relevant part of the STL instantiation
#ifdef WIN32
#include <stdexcept>
class DLVHEX_EXPORT std::runtime_error;
#endif

#include <boost/cstdint.hpp>
#endif                           /* _DLVHEX_PLATFORMDEFINITIONS_H */

/* vim: set noet sw=4 ts=4 tw=80: */

// Local Variables:
// mode: C++
// End:
