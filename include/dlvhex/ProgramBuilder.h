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
 * @file   ProgramBuilder.h
 * @author Roman Schindlauer
 * @date   Sun Sep 4 12:39:40 2005
 * 
 * @brief  Builders for logic program representations.
 * 
 * 
 */

#if !defined(_DLVHEX_PROGRAMBUILDER_H)
#define _DLVHEX_PROGRAMBUILDER_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/Program.h"

#include <string>
#include <sstream>

DLVHEX_NAMESPACE_BEGIN

//
// forward declaration
//
class PrintVisitor;


/**
* @brief Base Builder for building logic programs.
*/
class DLVHEX_EXPORT ProgramBuilder
{
protected:
    std::ostringstream stream;

    /// Ctor
    ProgramBuilder();

    /// Dtor
    virtual
    ~ProgramBuilder();

public:

    /**
     * Building method implemented by the children of ProgramBuilder.
     *
     * @param rule
     */
    virtual void
    buildRule(const Rule* rule) = 0;

    /**
     * @brief Build Facts.
     */
    virtual void
    buildFacts(const AtomSet&) = 0;

    std::string
    getString();

    void
    clearString();

};


/**
 * @brief A Builder for programs to be evaluated with DLV.
 *
 * Uses a DLVPrintVisitor (in first order mode) or a HOPrintVisitor
 * (in higher order mode) for building logic programs.
 *
 * @todo This whole builder could be effectively replaced by the
 * corresponding visitors.
 */
class DLVHEX_EXPORT ProgramDLVBuilder : public ProgramBuilder
{
public:
    /// Ctor
    explicit
    ProgramDLVBuilder(bool ho);

    /// Dtor
    virtual
    ~ProgramDLVBuilder();

    /**
     * @brief Build rule for DLV.
     */
    virtual void
    buildRule(const Rule*);

    /**
     * @brief Build facts for DLV.
     */
    virtual void
    buildFacts(const AtomSet&);

    /**
     * @brief Build program for DLV from set of rules.
     */
    virtual void
    buildProgram(const Program&);

private:
    /**
     * @brief either a DLVPrintVisitor or a HOPrintVisitor.
     */
    PrintVisitor* pv;
};


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_PROGRAMBUILDER_H */


// Local Variables:
// mode: C++
// End:
