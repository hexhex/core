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
 * @file   Process.h
 * @author Thomas Krennwallner
 * @date   
 * 
 * @brief  
 * 
 * 
 */


#if !defined(_DLVHEX_PROCESS_H)
#define _DLVHEX_PROCESS_H

#include "dlvhex/PlatformDefinitions.h"

#include <string>

DLVHEX_NAMESPACE_BEGIN


// forward declaration
class BaseASPSolver;


/**
 * @brief Base class for solver processes
 */
class DLVHEX_EXPORT Process
{
public:
  virtual
  ~Process() { }

  virtual BaseASPSolver*
  createSolver() = 0;

  virtual void
  addOption(const std::string&) = 0;

  virtual void
  spawn() = 0;

  virtual void
  endoffile() = 0;

  virtual int
  close() = 0;

  virtual std::ostream&
  getOutput() = 0;

  virtual std::istream&
  getInput() = 0;
};

DLVHEX_NAMESPACE_END

#endif // _DLVHEX_PROCESS_H


// Local Variables:
// mode: C++
// End:
