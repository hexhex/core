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
 * @file   ProcessBuf.cpp
 * @author Thomas Krennwallner
 * @date   Sun May 21 13:22:23 2006
 * 
 * @brief  
 * 
 * 
 */

#include "dlvhex/ProcessBuf.h"

#include <sstream>
#include <cerrno>
#include <cstdio>
#include <csignal>
#include <cstring>
#include <cstdlib>
#include <sys/types.h>
#include <sys/wait.h>


DLVHEX_NAMESPACE_BEGIN

ProcessBuf::ProcessBuf()
  : std::streambuf(),
    process(-1),
    bufsize(256)
{
  // ignore SIGPIPE
  struct sigaction sa;
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);

  if (::sigaction(SIGPIPE, &sa, 0))
    {
      ::perror("sigaction");
      ::exit(1);
    }

  initBuffers(); // don't call virtual methods in the ctor
}


ProcessBuf::ProcessBuf(const ProcessBuf& sb)
  : std::streambuf(),
    process(sb.process),
    status(sb.status),
    bufsize(sb.bufsize)
{
  ::memcpy(inpipes, sb.inpipes, 2);
  ::memcpy(outpipes, sb.outpipes, 2);
  initBuffers(); // don't call virtual methods in the ctor
}




ProcessBuf::~ProcessBuf()
{
  close();

  if (ibuf)
    {
      delete[] ibuf;
      ibuf = 0;
    }

  if (obuf)
    {
      delete[] obuf;
      obuf = 0;
    }
}


void
ProcessBuf::initBuffers()
{
  obuf = new std::streambuf::char_type[bufsize];
  ibuf = new std::streambuf::char_type[bufsize];
  setp(obuf, obuf + bufsize);
  setg(ibuf, ibuf, ibuf);
}



pid_t
ProcessBuf::open(const std::vector<std::string>& av)
{
  // close before re-open it
  if (process != -1)
    {
      int ret = close();
      if (ret != 0)
	{
	  return ret < 0 ? ret : -ret;
	}
    }

  outpipes[0] = 0;
  outpipes[1] = 0;
  inpipes[0] = 0;
  inpipes[1] = 0;

  // we want a full-duplex stream -> create two pairs of pipes

  if (::pipe(outpipes) < 0)
    {
      ::perror("pipes");
      return -1;
    }

  if (::pipe(inpipes) < 0)
    {
      ::perror("pipes");
      return -1;
    }

  // create a new process 
  process = ::fork();

  switch (process)
    {
    case -1: // error
      ::perror("fork");
      ::exit(process);
      break;

    case 0: // child
      {
	// setup argv

	char* argv[av.size() + 1];
	int i = 0;

	for (std::vector<std::string>::const_iterator it = av.begin();
	     it != av.end(); it++)
	  {
	    std::string::size_type size = it->size();
	    argv[i] = new char[size + 1];
	    it->copy(argv[i], size);
	    argv[i][size] = '\0';
	    i++;
	  }
	
	argv[i] = '\0';

	// redirect stdin and stdout and stderr

	if (::dup2(outpipes[1], STDOUT_FILENO) < 0)
	  {
	    ::perror("dup2");
	    ::exit(1);
	  }
	
	if (::dup2(outpipes[1], STDERR_FILENO) < 0)
	  {
	    ::perror("dup2");
	    ::exit(1);
	  }
	
	if (::dup2(inpipes[0], STDIN_FILENO) < 0)
	  {
	    ::perror("dup2");
	    ::exit(1);
	  }
	
	// stdout and stdin is redirected, close unneeded filedescr.
	::close(outpipes[0]);
 	::close(outpipes[1]);
 	::close(inpipes[0]);
	::close(inpipes[1]);
	
	// execute command, should not return
	::execvp(*argv, argv);
	
	// just in case we couldn't execute the command
	::exit(127);
      }
      break;


    default: // parent

      // close writing end of the output pipe
      ::close(outpipes[1]);
      outpipes[1] = -1;
      // close reading end of the input pipe
      ::close(inpipes[0]);
      inpipes[0] = -1;
      
      break;
    }

  return process;
}


void
ProcessBuf::endoffile()
{
  // reset output buffer
  setp(obuf, obuf + bufsize);

  if (inpipes[1] != -1)
    {
      ::close(inpipes[1]); // send EOF to stdin of child process
      inpipes[1] = -1;
    }
}

int
ProcessBuf::close()
{
  // we're done writing
  endoffile();

  // reset input buffer
  setg(ibuf, ibuf, ibuf);

  // we're done reading
  if (outpipes[0] != -1)
    {
      ::close(outpipes[0]);
      outpipes[0] = -1;
    }

  // obviously we do not want to leave zombies around, so get status
  // code of the process
  ::waitpid(process, &status, 0);
  process = -1;

  // exit code of process
  return WEXITSTATUS(status);
}


std::streambuf::int_type
ProcessBuf::overflow(std::streambuf::int_type c)
{
  if (pptr() >= epptr()) // full obuf -> write buffer
    {
      if (sync() == -1)
	{
	  return traits_type::eof();
	}
    }

  // if c != EOF, put c into output buffer so next call to sync() will
  // write it
  if (!traits_type::eq_int_type(c, traits_type::eof()))
    {
      *pptr() = traits_type::to_char_type(c);
      pbump(1); // increase put pointer by one
    }
  
  return traits_type::not_eof(c);
}


std::streambuf::int_type
ProcessBuf::underflow()
{
  if (gptr() >= egptr()) // empty ibuf -> get data
    {
      errno = 0;

      // try to receive at most bufsize bytes
      ssize_t n = ::read(outpipes[0], ibuf, bufsize);

      if (n == 0) // EOF
	{
	  return traits_type::eof();
	}
      else if (n < 0) // a failure occured while receiving from the stream
	{
	  std::ostringstream oss;
	  oss << "Process prematurely closed pipe before I could read (errno = " << errno << ").";
	  throw std::ios_base::failure(oss.str());
	}

      setg(ibuf, ibuf, ibuf + n); // set new input buffer boundaries
    }

  return traits_type::to_int_type(*gptr());
}



std::streambuf::int_type
ProcessBuf::sync()
{
  // reset input buffer
  setg(ibuf, ibuf, ibuf);

  const ssize_t len = pptr() - pbase();

  if (len) // non-empty obuf -> send data
    {
      errno = 0;

      // loops until whole obuf is sent
      //
      // Warning: when peer disconnects during the sending we receive
      // a SIGPIPE and the default signal handler exits the program.
      // Therefore we have to ignore SIGPIPE (in ctor) and reset the
      // obuf followed by an error return value. See chapter 5.13 of
      // W.R. Stevens: Unix Network Programming Vol.1.

      ssize_t ret = 0;

      for (ssize_t written = 0; written < len; written += ret)
	{
	  ret = ::write (inpipes[1], pbase() + written, len - written);
	  if (ret == -1 || ret == 0) break;
	}

      // reset output buffer right after sending to the stream
      setp(obuf, obuf + bufsize);

      if (ret == 0) // EOF
	{
	  return -1;
	}
      else if (ret < 0 || errno == EPIPE) // failure
	{
	  std::ostringstream oss;
	  oss << "Process prematurely closed pipe before I could write (errno = " << errno << ").";
	  throw std::ios_base::failure(oss.str());
	}
    }
  
  return 0;
}


DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
