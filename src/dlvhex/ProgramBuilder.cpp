/* -*- C++ -*- */

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
#include <functional>


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
  rule->accept(*pv);
}


void
ProgramDLVBuilder::buildFacts(const AtomSet& facts) // throw (???Error)
{
  facts.accept(*pv);
}


void
ProgramDLVBuilder::buildProgram(const Program& program)
{
    /// @todo: stdlib algorithm instead of loop!
    for (Program::const_iterator r = program.begin();
         r != program.end();
         ++r)
    {
        (*r)->accept(*pv);
    }
}


