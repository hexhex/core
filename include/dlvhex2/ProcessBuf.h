
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
  /** \brief Constructor. */
  ProcessBuf();

  /** \brief Copy-constructor.
    * @param b Other ProcessBuf. */
  ProcessBuf(const ProcessBuf& b);

  /** \brief Destructor. */
  virtual
  ~ProcessBuf();

  /** \brief Send EOF to the process. */
  virtual void
  endoffile();

  /** \brief Open a commandline in a seprate process.
    * @param c Commandline to open.
    * @return Process ID if the new process. */
  virtual pid_t
  open(const std::vector<std::string>& c);

  /** \brief Wait for end of process.
    * @param If true the process will be killed, otherwise the method waits.
    * @return Return code of the process. */
  virtual int
  close(bool kill=false);

private:

#ifdef POSIX
  /* \brief Child process ID (POSIX). */
  pid_t process;
  /** \brief Bidirectional output pipe (POSIX, this and child process). */
  int outpipes[2];
  /** \brief Bidirectional input pipe (POSIX, this and child process). */
  int inpipes[2];
#else
	#ifdef WIN32
		/** Process handle (WIN32). */
		PROCESS_INFORMATION processInformation;
		/** \brief Bidirectional output pipe (WIN32, child process input read). */
		HANDLE g_hChildStd_IN_Rd;
		/** \brief Bidirectional output pipe (WIN32, child process input write). */
		HANDLE g_hChildStd_IN_Wr;
		/** \brief Bidirectional output pipe (WIN32, child process output read). */
		HANDLE g_hChildStd_OUT_Rd;
		/** \brief Bidirectional output pipe (WIN32, child process output write). */
		HANDLE g_hChildStd_OUT_Wr;
	#else
		#error Either POSIX or WIN32 must be defined
	#endif
#endif
  /** \brief Status of the child process. */
  int status;
  /** \brief Buffer size. */
  unsigned bufsize;

  /** \brief Data of the output buffer. */
  std::streambuf::char_type* obuf;
  /** \brief Data of the input buffer. */
  std::streambuf::char_type* ibuf;

  /** \brief Initializes obuf and ibuf. */
  void
  initBuffers();

protected:
  /** \brief Called when an overflow occurred.
    * @param c See std::streambuf.
    * @return std::streambuf. */
  virtual std::streambuf::int_type
  overflow(std::streambuf::int_type c);

  /** \brief Called when an underflow occurred.
    * @param c See std::streambuf.
    * @return std::streambuf. */
  virtual std::streambuf::int_type
  underflow();

  /** \brief Synchronization.
    * @param c See std::streambuf.
    * @return std::streambuf. */
  virtual std::streambuf::int_type
  sync();
};

DLVHEX_NAMESPACE_END

#endif // _DLVHEX_PROCESSBUF_H

// Local Variables:
// mode: C++
// End:
