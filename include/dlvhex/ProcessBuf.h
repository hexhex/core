/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


/**
 * @file   ProcessBuf.h
 * @author Thomas Krennwallner
 * @date   Sun May 21 13:22:36 2006
 * 
 * @brief  
 * 
 * 
 */


#ifndef _PROCESSBUF_H
#define _PROCESSBUF_H

#include <iostream>
#include <streambuf>
#include <vector>
#include <string>

class ProcessBuf : public std::streambuf
{
public:
  ProcessBuf();

  ProcessBuf(const ProcessBuf&);

  virtual
  ~ProcessBuf();

  virtual void
  endoffile();

  virtual void
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

#endif // _PROCESSBUF_H


// Local Variables:
// mode: C++
// End:
