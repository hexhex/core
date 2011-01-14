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
 * @file InputProvider.hpp
 * @author Peter Schueller
 * @date 
 *
 * @brief Input stream provider (collects input sources)
 */

#include "dlvhex/InputProvider.hpp"
#include "dlvhex/URLBuf.h"
#include "dlvhex/Error.h"

#include <cassert>
#include <fstream>
#include <sstream>

DLVHEX_NAMESPACE_BEGIN

#warning TODO use boost::iostream::chain or something similar to create real streaming/incremental input provider

class InputProvider::Impl
{
public:
  std::stringstream stream;
  std::vector<std::string> contentNames;

public:
  Impl()
  {
  }
};

InputProvider::InputProvider():
  pimpl(new Impl)
{
}

InputProvider::~InputProvider()
{
}

void InputProvider::addStreamInput(std::istream& i, const std::string& contentname)
{
  pimpl->stream << i.rdbuf();
  pimpl->contentNames.push_back(contentname);
}

void InputProvider::addStringInput(const std::string& content, const std::string& contentname)
{
  pimpl->stream << content;
  pimpl->contentNames.push_back(contentname);
}

void InputProvider::addFileInput(const std::string& filename)
{
  std::ifstream ifs;
  ifs.open(filename.c_str());

  if (!ifs.is_open())
    {
      throw GeneralError("File " + filename + " not found");
    }

  pimpl->stream << ifs.rdbuf();
  ifs.close();
  pimpl->contentNames.push_back(filename);
}

void InputProvider::addURLInput(const std::string& url)
{
  assert(url.find("http://") == 0 && "currently only processing http:// URLs");

  URLBuf ubuf;
  ubuf.open(url);
  std::istream is(&ubuf);

  pimpl->stream << is.rdbuf();

  if (ubuf.responsecode() == 404)
    {
      throw GeneralError("Requested URL " + url + " was not found");
    }

  pimpl->contentNames.push_back(url);
}

bool InputProvider::hasContent() const
{
  return !pimpl->contentNames.empty();
}

const std::vector<std::string>& InputProvider::contentNames() const
{
  return pimpl->contentNames;
}

std::istream& InputProvider::getAsStream()
{
  assert(hasContent() && "should have gotten some content before using content");
  return pimpl->stream;
}

DLVHEX_NAMESPACE_END

