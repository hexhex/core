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
 * @file   Error.cpp
 * @author Roman Schindlauer
 * @date   Fri Mar  3 11:40:24 CET 2006
 *
 * @brief  Exception classes.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#include "dlvhex2/Error.h"

#include <sstream>

DLVHEX_NAMESPACE_BEGIN

GeneralError::GeneralError(const std::string& msg)
: std::runtime_error(msg)
{
}


SyntaxError::SyntaxError(const std::string& msg,
const unsigned l,
const std::string& f)
: GeneralError(msg),
line(l),
file(f)
{
}


std::string
SyntaxError::getErrorMsg() const
{
    std::ostringstream err;

    err << "Syntax Error";

    if (!file.empty())
        err << " in " << file;

    if (line != 0)
        err << ", line " << line;

    err << ": " << this->what();

    return err.str();
}


void
SyntaxError::setLine(unsigned l)
{
    this->line = l;
}


void
SyntaxError::setFile(const std::string& f)
{
    this->file = f;
}


FatalError::FatalError(const std::string& msg)
: GeneralError("Fatal: " + msg)
{
}


PluginError::PluginError(const std::string& msg)
: GeneralError(msg)
{
}


void
PluginError::setContext(const std::string& c)
{
    context = c;
}


std::string
PluginError::getErrorMsg() const
{
    std::ostringstream err;

    err << "Plugin Error";

    if (!context.empty())
        err << " in " << context;

    err << ": " << this->what();

    return err.str();
}


DLVHEX_NAMESPACE_END


// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
