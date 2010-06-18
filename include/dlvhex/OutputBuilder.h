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
 * @file   OutputBuilder.h
 * @author Roman Schindlauer
 * @date   Mon Feb 20 14:32:29 CET 2006
 * 
 * @brief  Builders for solver result.
 * 
 * 
 */

#if !defined(_DLVHEX_OUTPUTBUILDER_H)
#define _DLVHEX_OUTPUTBUILDER_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/ResultContainer.h"

#include <iosfwd>

DLVHEX_NAMESPACE_BEGIN

// forward declaration
class ResultContainer;

/**
 * @brief Base Builder for building solver output.
 */
class DLVHEX_EXPORT OutputBuilder
{
protected:
    
    /// Ctor
    OutputBuilder() {};

public:

    /// Dtor
    virtual
    ~OutputBuilder() {};

    /**
     * @brief Build answer set.
     */
    virtual void
    buildResult(std::ostream&, const ResultContainer&) = 0;

};


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_OUTPUTBUILDER_H */


// Local Variables:
// mode: C++
// End:
