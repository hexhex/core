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
 * @file Nogood.cpp
 * @author Christoph Redl
 *
 * @brief Data structures for CDNLSolver.
 */


#include "dlvhex/Nogood.hpp"

#include <iostream>
#include <sstream>
#include "dlvhex/Logger.hpp"
#include <boost/functional/hash.hpp>

DLVHEX_NAMESPACE_BEGIN


// ---------- Class Nogood ----------

//#define DBGLOGD(X,Y) DBGLOG(X,Y)
#define DBGLOGD(X,Y) do{}while(false);

void Nogood::recomputeHash(){
	hashValue = 0;
	BOOST_FOREACH (ID lit, *this){
		boost::hash_combine(hashValue, lit.kind);
		boost::hash_combine(hashValue, lit.address);
	}
}

size_t Nogood::getHash(){
	return hashValue;
}

bool Nogood::operator==(const Nogood& ng2){

	// compare hash value
	if (hashValue != ng2.hashValue) return false;

	// compare content
	if (size() != ng2.size()) return false;

	const_set_iterator<ID> it1 = ((const Nogood*)this)->begin();
	const_set_iterator<ID> it2 = ng2.begin();
	while(it1 != end()){
		if (*it1 != *it2) return false;
		++it1;
		++it2;
	}

	return true;
}

bool Nogood::operator!=(const Nogood& ng2){
	return !(this->operator==(ng2));
}

std::ostream& Nogood::print(std::ostream& o) const{
	o << "{ ";
	bool first = true;
	BOOST_FOREACH (ID lit, *this){
		if (!first){
			o << ", ";
		}
		first = false;
		o << (lit.isNaf() ? std::string("-") : std::string("")) << lit.address;
	}
	o << " }";
	return o;
}

std::ostream& NogoodSet::print(std::ostream& o) const{
	o << "{ ";
	for (std::vector<Nogood>::const_iterator it = nogoods.begin(); it != nogoods.end(); ++it){
		if (it != nogoods.begin()) o << ", ";
		o << (*it);
	}
	o << " }";
	return o;
}


// ---------- Class NogoodSet ----------

int NogoodSet::addNogood(Nogood ng){

	ng.recomputeHash();
	DBGLOG(DBG, "Hash of " << ng << " is " << ng.getHash());

	// check if ng is already present
	BOOST_FOREACH (int i, nogoodsWithHash[ng.getHash()]){
		if (nogoods[i] == ng) return i;
	}

	// nogood is not present
	int index;
	if (freeIndices.size() == 0){
		nogoods.push_back(ng);
		index = nogoods.size() - 1;
	}else{
		int index = freeIndices[freeIndices.size() - 1];
		nogoods[index] = ng;
		freeIndices.pop_back();
	}
	nogoodsWithHash[ng.getHash()].insert(index);
	return index;
}

void NogoodSet::removeNogood(int nogoodIndex){
	nogoodsWithHash[nogoods[nogoodIndex].getHash()].erase(nogoodIndex);
	freeIndices.push_back(nogoodIndex);
}

DLVHEX_NAMESPACE_END
