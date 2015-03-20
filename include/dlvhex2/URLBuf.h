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
 * \brief A std::streambuf for reading data from URLs.
 */
class DLVHEX_EXPORT URLBuf : public std::streambuf
{
public:
  /** \brief Constructor. */
  URLBuf();

  /** \brief Copy-constructor.
    * @param u Second URLBuf.
    */
  URLBuf(const URLBuf& u);

  /** \brief Destructor. */
  virtual
  ~URLBuf();

  /** \brief Opens a URL for reading.
   * @param url URL to open.
   */
  virtual void
  open(const std::string& url);

  /** \brief Retrieves the respondsecode from the server.
   * @return Response.
   */
  virtual long
  responsecode() const;

private:
  /** \brief Input buffer. */
  std::streambuf::char_type* ibuf;

  /** Size of input buffer. */
  unsigned bufsize;

  /** \brief CURL handle for accessing URLs. */
  CURL* easy_handle;

  /** \brief Response code from HTTP/FTP/... . */
  long response;

  /** \brief Write data to some URL stream.
    * @param ptr Data pointer.
    * @param size Size of data to write.
    * @param nmemb Multiplicator for \p size.
    * @param stream Buffer to write to.
    * @return Size of written data.
    */
  static size_t
  writer(void *ptr, size_t size, size_t nmemb, void *stream);

  /** \brief Write data to this URL stream.
    * @param size Size of data to write.
    * @return Size of written data.
    */
  size_t
  write(void* ptr, size_t size);

protected:
  virtual std::streambuf::int_type
  /** \brief Is called to fetch further data. */
  underflow();
};

DLVHEX_NAMESPACE_END

#endif // _DLVHEX_URLBUF_H

#endif

// Local Variables:
// mode: C++
// End:
