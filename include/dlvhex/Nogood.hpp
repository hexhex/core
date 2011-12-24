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
 * @file   Nogood.hpp
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Data structures for CDNLSolver.
 */

#ifndef NOGOOD_HPP_INCLUDED__09122011
#define NOGOOD_HPP_INCLUDED__09122011

#include "dlvhex/ID.hpp"
#include <vector>
#include <set>
#include <map>
#include <boost/foreach.hpp>
#include "dlvhex/Printhelpers.hpp"
#include <boost/foreach.hpp>
#include "Set.hpp"
#include <boost/unordered_map.hpp>

DLVHEX_NAMESPACE_BEGIN

class Nogood : public Set<ID>, public ostream_printable<Nogood>{
private:
	std::size_t hashValue;
public:
	void recomputeHash();
	size_t getHash();
	bool operator==(const Nogood& ng2);
	bool operator!=(const Nogood& ng2);
	std::ostream& print(std::ostream& o) const;
};

class NogoodSet : private ostream_printable<NogoodSet>{
public:
	std::vector<Nogood> nogoods;
	std::vector<int> freeIndices;
	boost::unordered_map<size_t, Set<int> > nogoodsWithHash;

	int addNogood(Nogood ng);
	void removeNogood(int nogoodIndex);

	std::ostream& print(std::ostream& o) const;
};

DLVHEX_NAMESPACE_END

#endif
