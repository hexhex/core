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
 * @file   TextOutputBuilder.h
 * @author Thomas Krennwallner
 * @date   Mon Dec 22 20:52:26 CET 2007
 * 
 * @brief  Builder for standard text output.
 * 
 * 
 */

#if !defined(_DLVHEX_TEXTOUTPUTBUILDER_H)
#define _DLVHEX_TEXTOUTPUTBUILDER_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/AnswerSet.h"
#include "dlvhex/OutputBuilder.h"

#include <iosfwd>

DLVHEX_NAMESPACE_BEGIN

class ResultContainer;

/**
 * @brief Simple textual output.
 */
class DLVHEX_EXPORT TextOutputBuilder : public OutputBuilder
{
public:

    /// Dtor
    virtual
    ~TextOutputBuilder();

    /// Ctor
    TextOutputBuilder();

    /**
     * @brief Build answer set.
     */
    virtual void
    buildResult(std::ostream&, const ResultContainer&);

};

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_TEXTOUTPUTBUILDER_H */


// Local Variables:
// mode: C++
// End:
