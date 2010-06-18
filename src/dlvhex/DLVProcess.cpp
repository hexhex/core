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

#include <iostream>
#include <boost/iostreams/tee.hpp>
#include <boost/iostreams/filtering_stream.hpp>


DLVHEX_NAMESPACE_BEGIN

DLVProcess::DLVProcess()
  : proc(), ipipe(0), opipe(0)
{ }


DLVProcess::~DLVProcess()
{
  proc.close();
  delete opipe;
  delete ipipe;
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


std::string
DLVProcess::path() const
{
  return DLVPATH;
}


std::vector<std::string>
DLVProcess::commandline() const
{
  std::vector<std::string> tmp;

  tmp.push_back(path());
  // never include the set of initial facts in the answer sets
  tmp.push_back("-nofacts");
  tmp.push_back("-silent");
  tmp.insert(tmp.end(), argv.begin(), argv.end());
  tmp.push_back("--"); // request stdin as last parameter!

  return tmp;
}


void
DLVProcess::setupStreams()
{
  if (ipipe == 0 && opipe == 0)
    {
      // first, setup the iostreams
      if (Globals::Instance()->doVerbose(Globals::GRAPH_PROCESSOR))
	{
	  Globals::Instance()->getVerboseStream() << "Setting up DLVProcess opipe to be verbose" << std::endl;

	  boost::iostreams::filtering_ostream* tmpopipe = new boost::iostreams::filtering_ostream;

	  tmpopipe->push(boost::iostreams::tee_filter<std::streambuf>(proc));
	  tmpopipe->push(Globals::Instance()->getVerboseStream());
	  
	  opipe = tmpopipe;
	}
      else
	{
	  opipe = new std::iostream(&proc);
	}
      
      ipipe = new std::iostream(&proc);
      
      // let ProcessBuf throw std::ios_base::failure
      opipe->exceptions(std::ios_base::badbit);
      ipipe->exceptions(std::ios_base::badbit);
    }
}


void
DLVProcess::spawn()
{
  setupStreams();
  proc.open(commandline());
}


void
DLVProcess::spawn(const std::vector<std::string>& opt)
{
  setupStreams();
  std::vector<std::string> tmp(opt);
  tmp.insert(tmp.begin(), path());
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
  assert(ipipe != 0 && opipe != 0);

  // first reset the state of the iostreams in order to re-use them
  opipe->clear();
  ipipe->clear();
  // exit code of process
  return proc.close();
}


std::ostream&
DLVProcess::getOutput()
{
  assert(opipe != 0);
  return *opipe;
}


std::istream&
DLVProcess::getInput()
{
  assert(ipipe != 0);
  return *ipipe;
}



DLVDBProcess::DLVDBProcess()
  : DLVProcess()
{ }


std::string
DLVDBProcess::path() const
{
#if defined(HAVE_DLVDB)
  return DLVDBPATH;
#else
  return DLVProcess::path();
#endif
}


std::vector<std::string>
DLVDBProcess::commandline() const
{
#if defined(HAVE_DLVDB)
  std::vector<std::string> tmp;

  tmp.push_back(path());
  tmp.push_back("-DBSupport"); // turn on database support
  tmp.push_back("-ORdr-"); // turn on rewriting of false body rules
  // never include the set of initial facts in the answer sets
  tmp.push_back("-nofacts");
  tmp.push_back("-silent");
  tmp.insert(tmp.end(), argv.begin(), argv.end());
  tmp.push_back("--"); // request stdin as last parameter!

  return tmp;
#else
  return DLVProcess::commandline();
#endif /* HAVE_DLVDB */
}


DLVHEX_NAMESPACE_END

// vim:se ts=8:
// Local Variables:
// mode: C++
// End:
