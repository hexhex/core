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
 * @file   DLVProcess.h
 * @author Thomas Krennwallner
 * @date   
 * 
 * @brief  Process interface to DLV programs.
 * 
 * 
 */


#if !defined(_DLVHEX_DLVPROCESS_H)
#define _DLVHEX_DLVPROCESS_H

#include "dlvhex/Process.h"
#include "dlvhex/ProcessBuf.h"

#include "dlvhex/PlatformDefinitions.h"

#include <iosfwd>
#include <vector>
#include <string>
#include <cassert>


DLVHEX_NAMESPACE_BEGIN

/**
 * @brief A wrapper process for the DLV/DLVDB ASP engine.
 * @todo this class should have a name similar to DebuggingSupportChildProcess to really capture its purpose
 */
class DLVHEX_EXPORT DLVProcess : public Process
{
 protected:
  /// communication buffer
  ProcessBuf proc;
  
  /// iostreams to the dlv process
  std::istream* ipipe;
  std::ostream* opipe;
  
  /// executable path/name
  std::string executable;

  /// command line options
  std::vector<std::string> argv;

  /// initialize in/out streams
  void
  setupStreams();
  
 public:
  DLVProcess();

  virtual
  ~DLVProcess();

  virtual void
  addOption(const std::string&);

  virtual void
  setPath(const std::string&);

  virtual std::string
  path() const;

  virtual std::vector<std::string>
  commandline() const;

  virtual void
  spawn();

  virtual void
  spawn(const std::vector<std::string>&);

  virtual void
  endoffile();

  virtual int
  close();

  virtual std::ostream&
  getOutput();

  virtual std::istream&
  getInput();
};

DLVHEX_NAMESPACE_END

#endif // _DLVHEX_DLVPROCESS_H


// Local Variables:
// mode: C++
// End:
