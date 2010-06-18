
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
 * @file   ProcessBuf.h
 * @author Thomas Krennwallner
 * @date   Sun May 21 13:22:36 2006
 * 
 * @brief  iostreams interface to external programs.
 * 
 * 
 */


#if !defined(_DLVHEX_PROCESSBUF_H)
#define _DLVHEX_PROCESSBUF_H

#include "dlvhex/PlatformDefinitions.h"

#include <iostream>
#include <streambuf>
#include <vector>
#include <string>


DLVHEX_NAMESPACE_BEGIN

/**
 * @brief A std::streambuf interface for creating child processes and
 * writing/reading data via stdin/stdout.
 */
class DLVHEX_EXPORT ProcessBuf : public std::streambuf
{
public:
  ProcessBuf();

  ProcessBuf(const ProcessBuf&);

  virtual
  ~ProcessBuf();

  virtual void
  endoffile();

  virtual pid_t
  open(const std::vector<std::string>&);

  virtual int
  close();

private:
  pid_t process;

  int status;

  int outpipes[2];
  int inpipes[2];

  unsigned bufsize;

  std::streambuf::char_type* obuf;
  std::streambuf::char_type* ibuf;

  void
  initBuffers();

protected:
  virtual std::streambuf::int_type
  overflow(std::streambuf::int_type c);

  virtual std::streambuf::int_type
  underflow();

  virtual std::streambuf::int_type
  sync();
};

DLVHEX_NAMESPACE_END

#endif // _DLVHEX_PROCESSBUF_H


// Local Variables:
// mode: C++
// End:
