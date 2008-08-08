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


DLVHEX_NAMESPACE_BEGIN

// forward declaration
class BaseASPSolver;


/**
 * @brief A wrapper process for the DLV ASP engine.
 */
class DLVHEX_EXPORT DLVProcess : public Process
{
 protected:
  /// communication buffer
  ProcessBuf proc;
  
  // iostreams to the dlv process
  std::istream* ipipe;
  std::ostream* opipe;
  
  /// dlv command line options
  std::vector<std::string> argv;
  
 public:
  DLVProcess();

  virtual
  ~DLVProcess();

  /**
   * @return a new BaseASPSolver
   */
  virtual BaseASPSolver*
  createSolver();

  virtual void
  addOption(const std::string&);

  virtual std::string
  path() const;

  virtual std::vector<std::string>
  commandline() const;

  virtual void
  spawn();

  virtual void
  endoffile();

  virtual int
  close();

  virtual std::ostream&
  getOutput();

  virtual std::istream&
  getInput();
};


/**
 * @brief A wrapper process for the DLVDB ASP engine.
 */
class DLVHEX_EXPORT DLVDBProcess : public DLVProcess
{
 public:
  DLVDBProcess();

  virtual std::string
  path() const;

  virtual std::vector<std::string>
  commandline() const;

};


DLVHEX_NAMESPACE_END

#endif // _DLVHEX_DLVPROCESS_H


// Local Variables:
// mode: C++
// End:
