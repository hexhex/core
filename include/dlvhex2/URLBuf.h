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
 * @file   URLBuf.h
 * @author Thomas Krennwallner
 * @date   Fri Jan 25 10:00:58 GMT 2008
 * 
 * @brief  iostreams interface for libcurl.
 * 
 * 
 */


#if !defined(_DLVHEX_URLBUF_H)
#define _DLVHEX_URLBUF_H

#include "dlvhex2/PlatformDefinitions.h"

#ifdef HAVE_LIBCURL

#include <iostream>
#include <streambuf>
#include <string>

#include <curl/curl.h>

DLVHEX_NAMESPACE_BEGIN

/**
 * @brief A std::streambuf for reading data from URLs.
 */
class DLVHEX_EXPORT URLBuf : public std::streambuf
{
public:
  URLBuf();

  URLBuf(const URLBuf&);

  virtual
  ~URLBuf();

  /**
   * @param url open this URL string
   */
  virtual void
  open(const std::string& url);

  /**
   * @return #response
   */
  virtual long
  responsecode() const;

private:
  /// input buffer
  std::streambuf::char_type* ibuf;

  /// size of #ibuf
  unsigned bufsize;

  /// a CURL handle for accessing URLs
  CURL* easy_handle;

  /// response code from HTTP/FTP/...
  long response;

  static size_t
  writer(void *ptr, size_t size, size_t nmemb, void *stream);

  size_t
  write(void* ptr, size_t size);

protected:
  virtual std::streambuf::int_type
  underflow();
};

DLVHEX_NAMESPACE_END

#endif // _DLVHEX_URLBUF_H

#endif

// Local Variables:
// mode: C++
// End:
