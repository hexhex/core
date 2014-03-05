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
 * @file InputProvider.h
 * @author Peter Schueller
 * @date
 *
 * @brief Input stream provider (collects input sources)
 */

#ifndef INPUT_PROVIDER_HPP_INCLUDED_14012011
#define INPUT_PROVIDER_HPP_INCLUDED_14012011

#include "dlvhex2/PlatformDefinitions.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <vector>
#include <string>
#include <iosfwd>

DLVHEX_NAMESPACE_BEGIN

class DLVHEX_EXPORT InputProvider
{
public:
  InputProvider();
  ~InputProvider();

	void addStreamInput(std::istream& i, const std::string& contentname);
	void addStringInput(const std::string& content, const std::string& contentname);
	void addFileInput(const std::string& filename);
#ifdef HAVE_CURL
	void addURLInput(const std::string& url);
#endif

	bool hasContent() const;
	const std::vector<std::string>& contentNames() const;

	std::istream& getAsStream();

private:
  class Impl;
  boost::scoped_ptr<Impl> pimpl;
};
typedef boost::shared_ptr<InputProvider> InputProviderPtr;

DLVHEX_NAMESPACE_END

#endif // INPUT_PROVIDER_HPP_INCLUDED_14012011
