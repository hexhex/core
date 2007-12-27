/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
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
 * @file SafetyChecker.h
 * @author Roman Schindlauer
 * @date Mon Feb 27 15:09:49 CET 2006
 *
 * @brief Class for checking rule and program safety.
 *
 *
 */


#ifndef _DLVHEX_SAFETYCHECKER_H
#define _DLVHEX_SAFETYCHECKER_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/Rule.h"
#include "dlvhex/DependencyGraph.h"
#include "dlvhex/Error.h"

DLVHEX_NAMESPACE_BEGIN

/**
 * @brief Abstract bae class
 */
class DLVHEX_EXPORT SafetyCheckerBase
{
public:

protected:

    /// Ctor.
    SafetyCheckerBase();

public:

};


/**
 * @brief Safety checker class.
 */
class DLVHEX_EXPORT SafetyChecker : public SafetyCheckerBase
{
public:

    SafetyChecker(const Program&);

    void
    testRules(const Program&) const throw (SyntaxError);
};


/**
 * @brief Strong safety checker class.
 */
class DLVHEX_EXPORT StrongSafetyChecker : public SafetyChecker
{
public:

    StrongSafetyChecker(const Program&, const DependencyGraph*);

    void
    testStrongSafety(const DependencyGraph*) const throw (SyntaxError);
};

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_SAFETYCHECKER_H_ */


// Local Variables:
// mode: C++
// End:
