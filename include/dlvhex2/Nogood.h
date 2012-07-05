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
#include "dlvhex2/Registry.h"
#include <boost/unordered_map.hpp>

DLVHEX_NAMESPACE_BEGIN

class Nogood : public Set<ID>, public ostream_printable<Nogood>{
private:
	std::size_t hashValue;
	bool ground;
public:
	Nogood();
	void recomputeHash();
	size_t getHash();
	bool operator==(const Nogood& ng2);
	bool operator!=(const Nogood& ng2);
	std::ostream& print(std::ostream& o) const;
	std::string getStringRepresentation(RegistryPtr reg) const;
	Nogood resolve(Nogood& ng2, IDAddress litadr);
	void insert(ID lit);
	template <class InputIterator> void insert(InputIterator begin, InputIterator end){
		for (InputIterator it = begin; it != end; ++it){
			insert(*it);
		}
	}
	bool isGround() const;
	bool match(RegistryPtr reg, ID atomID, Nogood& instance) const;
};

class NogoodSet : private ostream_printable<NogoodSet>{
private:
	std::vector<Nogood> nogoods;
	Set<int> freeIndices;
	boost::unordered_map<size_t, Set<int> > nogoodsWithHash;

	// reorders the nogoods such that there are no free indices in the range 0-(getNogoodCount()-1)
	void defragment();
public:
	int addNogood(Nogood ng);
	void removeNogood(int nogoodIndex);
	void removeNogood(Nogood ng);
	Nogood& getNogood(int index);
	const Nogood& getNogood(int index) const;
	int getNogoodCount() const;

	std::ostream& print(std::ostream& o) const;
	std::string getStringRepresentation(RegistryPtr reg) const;
};

class NogoodContainer{
public:
	virtual void addNogood(Nogood ng) = 0;

	static inline ID createLiteral(ID lit){
		if (lit.isOrdinaryGroundAtom()){
			return ID(ID::MAINKIND_LITERAL | ID::SUBKIND_ATOM_ORDINARYG | (lit.isNaf() ? ID::NAF_MASK : 0), lit.address);
		}else{
			return ID(ID::MAINKIND_LITERAL | ID::SUBKIND_ATOM_ORDINARYN | (lit.isNaf() ? ID::NAF_MASK : 0), lit.address);
		}
	}
	static inline ID createLiteral(IDAddress litadr, bool truthValue = true, bool ground = true){
		if (ground){
			return ID(ID::MAINKIND_LITERAL | ID::SUBKIND_ATOM_ORDINARYG | (truthValue ? 0 : ID::NAF_MASK), litadr);
		}else{
			return ID(ID::MAINKIND_LITERAL | ID::SUBKIND_ATOM_ORDINARYN | (truthValue ? 0 : ID::NAF_MASK), litadr);
		}
	}

	typedef boost::shared_ptr<NogoodContainer> Ptr;
	typedef boost::shared_ptr<const NogoodContainer> ConstPtr;
};

typedef NogoodContainer::Ptr NogoodContainerPtr;
typedef NogoodContainer::ConstPtr NogoodContainerConstPtr;

class SimpleNogoodContainer : public NogoodContainer{
private:
	boost::mutex mutex;	// exclusive access
	NogoodSet ngg;
public:
	void addNogood(Nogood ng);
	void removeNogood(Nogood ng);
	Nogood& getNogood(int index);
	int getNogoodCount();
	void clear();

	typedef boost::shared_ptr<SimpleNogoodContainer> Ptr;
	typedef boost::shared_ptr<const SimpleNogoodContainer> ConstPtr;
};

typedef SimpleNogoodContainer::Ptr SimpleNogoodContainerPtr;
typedef SimpleNogoodContainer::ConstPtr SimpleNogoodContainerConstPtr;

DLVHEX_NAMESPACE_END

#endif
