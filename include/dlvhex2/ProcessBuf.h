
/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * Copyright (C) 2011, 2012, 2013 Christoph Redl
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/PlatformDefinitions.h"

#include <iostream>
#include <streambuf>
#include <vector>
#include <string>

#ifdef WIN32
#include <windows.h>
#undef ERROR
typedef HANDLE pid_t;
#endif


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

  // wait for end of process
  // if kill is true, kill if not already ended
  virtual int
  close(bool kill=false);

private:

#ifdef POSIX
  pid_t process;
  int outpipes[2];
  int inpipes[2];
#else
	#ifdef WIN32
		PROCESS_INFORMATION processInformation;
		HANDLE g_hChildStd_IN_Rd;
		HANDLE g_hChildStd_IN_Wr;
		HANDLE g_hChildStd_OUT_Rd;
		HANDLE g_hChildStd_OUT_Wr;
	#else
		#error Either POSIX or WIN32 must be defined
	#endif
#endif

  int status;
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
