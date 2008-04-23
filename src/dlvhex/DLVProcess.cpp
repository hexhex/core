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
 * @file   DLVProcess.cpp
 * @author Thomas Krennwallner
 * @date   
 * 
 * @brief  
 * 
 * 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex/DLVProcess.h"
#include "dlvhex/DLVresultParserDriver.h"
#include "dlvhex/ASPSolver.h"
#include "dlvhex/globals.h"
#include "dlvhex/PrintVisitor.h"


DLVHEX_NAMESPACE_BEGIN


DLVProcess::DLVProcess()
  : proc(), iopipe(&proc)
{
  // let ProcessBuf throw std::ios_base::failure
  iopipe.exceptions(std::ios_base::badbit);
}


DLVProcess::~DLVProcess()
{
  proc.close();
}


BaseASPSolver*
DLVProcess::createSolver()
{
  if (Globals::Instance()->getOption("NoPredicate"))
    {
      return new ASPSolver<HOPrintVisitor, DLVresultParserDriver>(*this);
    }
  else
    {
      return new ASPSolver<DLVPrintVisitor, DLVresultParserDriver>(*this);
    }
}


void
DLVProcess::addOption(const std::string& option)
{
  argv.push_back(option);
}


void
DLVProcess::spawn()
{
  std::vector<std::string> tmp;

  tmp.push_back(DLVPATH);
  // never include the set of initial facts in the answer sets
  tmp.push_back("-nofacts");
  tmp.push_back("-silent");
  tmp.insert(tmp.end(), argv.begin(), argv.end());
  tmp.push_back("--"); // request stdin as last parameter!

  proc.open(tmp);
}


void
DLVProcess::endoffile()
{
  proc.endoffile();
}


int
DLVProcess::close()
{
  // first reset the state of the iostream in order to re-use it
  iopipe.clear();
  // exit code of process
  return proc.close();
}


std::ostream&
DLVProcess::getOutput()
{
  return iopipe;
}


std::istream&
DLVProcess::getInput()
{
  return iopipe;
}



DLVDBProcess::DLVDBProcess()
  : DLVProcess()
{ }


void
DLVDBProcess::spawn()
{
  std::vector<std::string> tmp;

  tmp.push_back(DLVDBPATH);
  tmp.push_back("-DBSupport"); // turn on database support
  tmp.push_back("-ORdr"); // turn on rewriting of false body rules
  // never include the set of initial facts in the answer sets
  tmp.push_back("-nofacts");
  tmp.push_back("-silent");
  tmp.insert(tmp.end(), argv.begin(), argv.end());
  tmp.push_back("--"); // request stdin as last parameter!

  proc.open(tmp);
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
