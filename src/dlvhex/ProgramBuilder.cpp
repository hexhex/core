/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
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
 * @file   ProgramBuilder.cpp
 * @author Roman Schindlauer
 * @date   Sun Sep 4 12:52:05 2005
 * 
 * @brief  Builders for logic program representations.
 * 
 * 
 */

#include "dlvhex/ProgramBuilder.h"
#include "dlvhex/PrintVisitor.h"
#include "dlvhex/AtomSet.h"

#include <functional>

DLVHEX_NAMESPACE_BEGIN

ProgramBuilder::ProgramBuilder()
{ }

ProgramBuilder::~ProgramBuilder()
{ }


std::string
ProgramBuilder::getString()
{
    return stream.str();
}

void
ProgramBuilder::clearString()
{
    stream.str("");
    stream.clear();
}


ProgramDLVBuilder::ProgramDLVBuilder(bool ho)
    : ProgramBuilder(),
      pv(ho ? new HOPrintVisitor(stream) : new DLVPrintVisitor(stream))
{ }


ProgramDLVBuilder::~ProgramDLVBuilder()
{
  delete pv;
}


void
ProgramDLVBuilder::buildRule(const Rule* rule) // throw (???Error)
{
  const_cast<Rule*>(rule)->accept(*pv);
}


void
ProgramDLVBuilder::buildFacts(const AtomSet& facts) // throw (???Error)
{
  const_cast<AtomSet*>(&facts)->accept(*pv);
}


void
ProgramDLVBuilder::buildProgram(const Program& program)
{
  const_cast<Program*>(&program)->accept(*pv);
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
