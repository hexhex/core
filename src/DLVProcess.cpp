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

#include "dlvhex2/DLVProcess.h"
#include "dlvhex2/Logger.h"

#include <iostream>
#include <boost/iostreams/tee.hpp>
#include <boost/iostreams/filtering_stream.hpp>


DLVHEX_NAMESPACE_BEGIN

DLVProcess::DLVProcess()
  : proc(), ipipe(0), opipe(0), executable(), argv()
{ }


DLVProcess::~DLVProcess()
{
  proc.close();
  delete opipe;
  delete ipipe;
}

void
DLVProcess::addOption(const std::string& option)
{
  argv.push_back(option);
}


void
DLVProcess::setPath(const std::string& path)
{
  executable = path;
}

std::string
DLVProcess::path() const
{
  return executable;
}


std::vector<std::string>
DLVProcess::commandline() const
{
  std::vector<std::string> tmp;

  assert(!path().empty());
  tmp.push_back(path());
  tmp.insert(tmp.end(), argv.begin(), argv.end());

  return tmp;
}


void
DLVProcess::setupStreams()
{
  if (ipipe == 0 && opipe == 0)
    {
      #ifndef NDEBUG
      if( Logger::Instance().shallPrint(Logger::DBG) )
      {
	// setup iostreams to copy to log stream
	DBGLOG(DBG,"Setting up DLVProcess opipe to be verbose");

	boost::iostreams::filtering_ostream* tmpopipe = new boost::iostreams::filtering_ostream;

	tmpopipe->push(boost::iostreams::tee_filter<std::streambuf>(proc));
	tmpopipe->push(Logger::Instance().stream());
	  
	opipe = tmpopipe;
      }
      else
      #endif
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
  assert(!path().empty());
  tmp.insert(tmp.begin(), path());
  proc.open(tmp);
}


void
DLVProcess::endoffile()
{
  proc.endoffile();
}


int
DLVProcess::close(bool kill)
{
  assert(ipipe != 0 && opipe != 0);

  // first reset the state of the iostreams in order to re-use them
  opipe->clear();
  ipipe->clear();
  // exit code of process
  return proc.close(kill);
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

DLVHEX_NAMESPACE_END

// vim:se ts=8:
// Local Variables:
// mode: C++
// End:
