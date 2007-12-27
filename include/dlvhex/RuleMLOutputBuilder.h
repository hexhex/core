/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2007 Thomas Krennwallner
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


/**
 * @file   RuleMLOutputBuilder.h
 * @author Thomas Krennwallner
 * @date   Sat Dec 22 20:52:26 CET 2007
 * 
 * @brief  Builder for RuleML output.
 * 
 * 
 */

#if !defined(_DLVHEX_RULEMLOUTPUTBUILDER_H)
#define _DLVHEX_RULEMLOUTPUTBUILDER_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/AnswerSet.h"
#include "dlvhex/OutputBuilder.h"

DLVHEX_NAMESPACE_BEGIN

/**
 * @brief XML output.
 */
class DLVHEX_EXPORT RuleMLOutputBuilder : public OutputBuilder
{
public:

    /// Dtor
    virtual
    ~RuleMLOutputBuilder();

    /// Ctor
    RuleMLOutputBuilder();

    virtual void
    buildPre();

    virtual void
    buildPost();

    /**
     * @brief Build answer set.
     */
    virtual void
    buildAnswerSet(const AnswerSet&);

};

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_RULEMLOUTPUTBUILDER_H */


// Local Variables:
// mode: C++
// End:
