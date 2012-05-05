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

#include "dlvhex2/ID.h"
#include <vector>
#include <set>
#include <map>
#include <boost/foreach.hpp>
#include "dlvhex2/Printhelpers.h"
#include <boost/foreach.hpp>
#include "dlvhex2/Set.h"
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
	Nogood resolve(Nogood& ng2, IDAddress litadr);
};

class NogoodSet : private ostream_printable<NogoodSet>{
public:
	std::vector<Nogood> nogoods;
	std::vector<int> freeIndices;
	boost::unordered_map<size_t, Set<int> > nogoodsWithHash;

	int addNogood(Nogood ng);
	void removeNogood(int nogoodIndex);
	Nogood getNogood(int index);

	std::ostream& print(std::ostream& o) const;
};

class NogoodContainer{
public:
	virtual int addNogood(Nogood ng) = 0;
	virtual void removeNogood(int index) = 0;
	virtual Nogood getNogood(int index) = 0;
	virtual int getNogoodCount() = 0;

	static inline ID createLiteral(ID lit){
		return ID(ID::MAINKIND_LITERAL | ID::SUBKIND_ATOM_ORDINARYG | (lit.isNaf() ? ID::NAF_MASK : 0), lit.address); //  | (lit.kind & ID::PROPERTY_MASK)
	}
	static inline ID createLiteral(IDAddress litadr, bool truthValue = true){
		return ID(ID::MAINKIND_LITERAL | ID::SUBKIND_ATOM_ORDINARYG | (truthValue ? 0 : ID::NAF_MASK), litadr);
	}

	typedef boost::shared_ptr<NogoodContainer> Ptr;
	typedef boost::shared_ptr<const NogoodContainer> ConstPtr;
};

class SimpleNogoodContainer : public NogoodContainer{
public:
	NogoodSet ngg;

	int addNogood(Nogood ng){
		ngg.addNogood(ng);
	}

	void removeNogood(int index){
		ngg.removeNogood(index);
	}

	Nogood getNogood(int index){
		return ngg.getNogood(index);
	}

	int getNogoodCount(){
		return ngg.nogoods.size() - ngg.freeIndices.size();
	}
};

typedef NogoodContainer::Ptr NogoodContainerPtr;
typedef NogoodContainer::ConstPtr NogoodContainerConstPtr;

DLVHEX_NAMESPACE_END

#endif
