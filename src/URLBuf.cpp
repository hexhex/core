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
 * @file   URLBuf.cpp
 * @author Thomas Krennwallner
 * @date   Fri Jan 25 10:00:58 GMT 2008
 *
 * @brief  iostreams interface for libcurl.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#ifdef HAVE_LIBCURL

#include "dlvhex2/URLBuf.h"

#include <sstream>
#include <cstdlib>
#include <cstring>

#include <curl/curl.h>

DLVHEX_NAMESPACE_BEGIN

URLBuf::URLBuf()
: std::streambuf(),
ibuf(0),
bufsize(0),
easy_handle(0)
{ }

URLBuf::URLBuf(const URLBuf&)
: std::streambuf(),
ibuf(0),
bufsize(0),
easy_handle(0)
{ }

URLBuf::~URLBuf()
{
    if (ibuf) {
        free(ibuf);
        ibuf = 0;
    }
}


void
URLBuf::open(const std::string& url)
{
    if (easy_handle == 0) {
        easy_handle = curl_easy_init();
        curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, URLBuf::writer);
        curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, this);
        curl_easy_setopt(easy_handle, CURLOPT_URL, url.c_str());
    }
}


long
URLBuf::responsecode() const
{
    return response;
}


size_t
URLBuf::writer(void *ptr, size_t size, size_t nmemb, void *stream)
{
    URLBuf* mybuf = reinterpret_cast<URLBuf*>(stream);
    return mybuf->write(ptr, size * nmemb);
}


size_t
URLBuf::write(void* ptr, size_t size)
{
    if (ibuf == 0) {
        // allocate input buffer for the first time
        ibuf = (std::streambuf::char_type*) ::malloc(size);
        bufsize = 0;
    }
    else {
        // reallocate input buffer
        ibuf = (std::streambuf::char_type*) ::realloc(ibuf, bufsize + size);
    }

    //  copy ptr to input buffer
    ::memcpy(ibuf + bufsize, ptr, size);
    // and increase buffer size
    bufsize += size;

    // set new input buffer boundaries
    setg(ibuf, ibuf, ibuf + bufsize);

    return size;
}


std::streambuf::int_type
URLBuf::underflow()
{
    if (easy_handle == 0) {
        // we received everything
        return traits_type::eof();
    }
                                 // empty ibuf -> receive data
    else if (gptr() >= egptr()) {
        CURLcode res;

        // fetch from server
        res = curl_easy_perform(easy_handle);

        // get return code
        curl_easy_getinfo(easy_handle, CURLINFO_RESPONSE_CODE, &response);

        // shutdown connection
        curl_easy_cleanup(easy_handle);
        easy_handle = 0;

        if (res != 0) {
            std::cerr << curl_easy_strerror(res) << std::endl;
            return traits_type::eof();
        }
    }

    return traits_type::to_int_type(*gptr());
}


DLVHEX_NAMESPACE_END
#endif

// Local Variables:
// mode: C++
// End:
