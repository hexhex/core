/* -*- C++ -*- */

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
