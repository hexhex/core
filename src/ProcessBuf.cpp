/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schüller
 * Copyright (C) 2011-2015 Christoph Redl
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/ProcessBuf.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Printhelpers.h"

#include <boost/foreach.hpp>
#include <sstream>
#include <cerrno>
#include <cstdio>
#include <csignal>
#include <cstring>
#include <cstdlib>
#include <sys/types.h>

#ifdef POSIX
#include <sys/wait.h>
#endif


DLVHEX_NAMESPACE_BEGIN

ProcessBuf::ProcessBuf()
  : std::streambuf(),
#ifdef POSIX
    process(-1),
#endif
    bufsize(256)
{
#ifdef POSIX
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
#endif

  initBuffers(); // don't call virtual methods in the ctor
}


ProcessBuf::ProcessBuf(const ProcessBuf& sb)
  : std::streambuf(),
#ifdef POSIX
    process(sb.process),
#else
	#ifdef WIN32
		processInformation(sb.processInformation),
	#else
	  #error Either POSIX or WIN32 must be defined
	#endif
#endif
    status(sb.status),
    bufsize(sb.bufsize)
{
#ifdef POSIX
  ::memcpy(inpipes, sb.inpipes, 2);
  ::memcpy(outpipes, sb.outpipes, 2);
#endif
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
  LOG(DBG,"ProcessBuf::open" << printvector(av));

#ifdef POSIX
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
        WARNING("TODO handle signals to parent process (pass on to children s.t. child process is not reparented to init)")
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
#else
	#ifdef WIN32
		SECURITY_ATTRIBUTES saAttr;
		ZeroMemory(&saAttr, sizeof(saAttr));

		// Set the bInheritHandle flag so pipe handles are inherited. 

		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
		saAttr.bInheritHandle = TRUE; 
		saAttr.lpSecurityDescriptor = NULL; 

		// Create a pipe for the child process's STDOUT.
		if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0)) return 0;

		// Ensure the read handle to the pipe for STDOUT is not inherited.
		if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) return 0;

		// Create a pipe for the child process's STDIN. 
		if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0)) return 0;

		// Ensure the write handle to the pipe for STDIN is not inherited. 
		if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0)) return 0;

		STARTUPINFO startupInfo;
		ZeroMemory(&processInformation, sizeof(processInformation));
		ZeroMemory(&startupInfo, sizeof(startupInfo));
		startupInfo.cb = sizeof(startupInfo);
		startupInfo.hStdOutput = g_hChildStd_OUT_Wr;
		startupInfo.hStdError = g_hChildStd_OUT_Wr;
		startupInfo.hStdInput = g_hChildStd_IN_Rd;
		startupInfo.dwFlags |= STARTF_USESTDHANDLES;
   		std::stringstream args;
		for (uint32_t i = 1; i < av.size(); i++){
			args << " " << av[i];
		}
		std::string argstr = args.str();

		// get PATH variable
		DWORD bufferSize = 65535; //Limit according to http://msdn.microsoft.com/en-us/library/ms683188.aspx
		std::string buff;
		buff.resize(bufferSize);

		std::string paths = std::string(::getenv("PATH")) + ";;";

		// search for dlv in all directories listed in PATH
		bool result = false;
		while (!result && paths.find(";") != std::wstring::npos){
			std::string path = buff.substr(0, paths.find(";"));
			path += (path.size() > 0 ? "\\" : "") + av[0];
			paths = paths.substr(buff.find(";") + 1);
	
			std::string cmdstr = path;
			result = (CreateProcess((LPCSTR)cmdstr.c_str(), (LPSTR)argstr.c_str(), NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &startupInfo, &processInformation) == TRUE);
		}
	
		if (!result){
			std::cout << GetLastError() << std::endl;
		}

		return processInformation.hProcess;
	#else
		#error Either POSIX or WIN32 must be defined
	#endif
#endif
}

void
ProcessBuf::endoffile()
{
#ifdef POSIX
  // reset output buffer
  setp(obuf, obuf + bufsize);

  if (inpipes[1] != -1)
    {
      ::close(inpipes[1]); // send EOF to stdin of child process
      inpipes[1] = -1;
    }
#else
	#ifdef WIN32
		// reset output buffer
		setp(obuf, obuf + bufsize);

		if (g_hChildStd_IN_Wr != 0){
			CloseHandle(g_hChildStd_IN_Wr); // send EOF to stdin of child process
			g_hChildStd_IN_Wr = 0;
		}

		// close handles to allow child process termination
		if (g_hChildStd_IN_Rd != 0){
			CloseHandle(g_hChildStd_IN_Rd);
			g_hChildStd_IN_Rd = 0;
		}
		if (g_hChildStd_OUT_Wr != 0){
			CloseHandle(g_hChildStd_OUT_Wr);
			g_hChildStd_OUT_Wr = 0;
		}
	#else
		#error Either POSIX or WIN32 must be defined
	#endif
#endif
}

// wait for end of process
// if kill is true, kill if not already ended
int
ProcessBuf::close(bool kill)
{
#ifdef POSIX
  if( process == -1 )
    return -1;

  LOG(DBG,"ProcessBuf::close for process " << process << "(" << kill << ")");

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

  // try to waitpid without waiting (just see if the process is still there)
  if( ::waitpid(process, &status, WNOHANG) == 0 )
  {
    int sig = SIGTERM;
    LOG(INFO,"sending signal " << sig << " to process " << process);
    ::kill(process, sig);
  }

  // obviously we do not want to leave zombies around, so get status
  // code of the process
  // (if the process no longer exists, this will simply fail,
  // if a new process grabbed the same pid, we are doomed and will wait for that
  // unrelated process to exit)
  ::waitpid(process, &status, 0);
  int exitstatus = WEXITSTATUS(status);
  LOG(DBG,"ProcessBuf::close for process " << process << ": exit status " << exitstatus);
  process = -1;

  // exit code of process
  return exitstatus;
#else
	#ifdef WIN32
	  if (processInformation.hProcess == 0)
		  return -1;

		LOG(DBG,"ProcessBuf::close for process " << processInformation.hProcess << "(" << kill << ")");
		if (processInformation.hProcess != 0){
			if (kill){
				TerminateProcess(processInformation.hProcess, 1);
			}else{
				WaitForSingleObject(processInformation.hProcess, INFINITE);
			}
			CloseHandle(processInformation.hProcess); // send EOF to stdin of child process
			processInformation.hProcess = 0;
			if (processInformation.hThread != 0){
				CloseHandle(processInformation.hThread);
				processInformation.hThread = 0;
			}
		}

		if (g_hChildStd_IN_Rd != 0){
			CloseHandle(g_hChildStd_IN_Rd);
			g_hChildStd_IN_Rd = 0;
		}
		if (g_hChildStd_OUT_Wr != 0){
			CloseHandle(g_hChildStd_OUT_Wr);
			g_hChildStd_OUT_Wr = 0;
		}

	  return 0;
	#else
		#error Either POSIX or WIN32 must be defined
	#endif
#endif
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
#ifdef POSIX
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
#else
	#ifdef WIN32
		// try to receive at most bufsize bytes
		DWORD dwRead;
		bool bSuccess = (ReadFile(g_hChildStd_OUT_Rd, ibuf, bufsize, &dwRead, NULL) == TRUE);
		if ( ! bSuccess || dwRead == 0 ){
			return traits_type::eof();
		}
		setg(ibuf, ibuf, ibuf + dwRead); // set new input buffer boundaries
		return traits_type::to_int_type(*gptr());
	#else
		#error Either POSIX or WIN32 must be defined
	#endif
#endif
}

std::streambuf::int_type
ProcessBuf::sync()
{
#ifdef POSIX
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
#else
	#ifdef WIN32
	  // reset input buffer
	  setg(ibuf, ibuf, ibuf);

	  const int len = pptr() - pbase();

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

			DWORD bwritten = 0;
			bool ret;

			for (int written = 0; written < len; written += bwritten)
			{
				ret = (WriteFile(g_hChildStd_IN_Wr, pbase() + written, len - written, &bwritten, NULL) == TRUE);

				if (!ret || bwritten == 0) break;
			}

			// reset output buffer right after sending to the stream
			setp(obuf, obuf + bufsize);

			if (bwritten == 0) // EOF
			{
				return -1;
			}
			else if (!ret) // failure
			{
				std::ostringstream oss;
				oss << "Process prematurely closed pipe before I could write (errno = " << errno << ").";
				throw std::ios_base::failure(oss.str());
			}
		}
  
	  return 0;
	#else
		#error Either POSIX or WIN32 must be defined
	#endif
#endif
}


DLVHEX_NAMESPACE_END

// vim:se ts=8:
// Local Variables:
// mode: C++
// End:
