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


#include "dlvhex2/Nogood.h"

#include <iostream>
#include <sstream>
#include "dlvhex2/Logger.h"
#include "dlvhex2/Printer.h"
#include <boost/functional/hash.hpp>

DLVHEX_NAMESPACE_BEGIN


// ---------- Class Nogood ----------

//#define DBGLOGD(X,Y) DBGLOG(X,Y)
#define DBGLOGD(X,Y) do{}while(false);

Nogood::Nogood() : ground(true){
}

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

const Nogood& Nogood::operator=(const Nogood& other)
{
  this->Set<ID>::operator=(other);
  hashValue = other.hashValue;
  ground = other.ground;
  return *this;
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

std::string Nogood::getStringRepresentation(RegistryPtr reg) const{

	std::stringstream ss;
	RawPrinter printer(ss, reg);
	ss << "{ ";
	bool first = true;
	BOOST_FOREACH (ID lit, *this){
		if (!first){
			ss << ", ";
		}
		first = false;
		ss << (lit.isNaf() ? "-" : "");
		if (lit.isOrdinaryGroundAtom()){
			printer.print(reg->ogatoms.getIDByAddress(lit.address));
		}else{
			printer.print(reg->onatoms.getIDByAddress(lit.address));
		}
	}
	ss << " }";
	return ss.str();
}

Nogood Nogood::resolve(Nogood& ng2, IDAddress litadr){
	// resolvent = union of this and ng2 minus both polarities of the resolved literal
	Nogood resolvent = *this;
	resolvent.insert(ng2.begin(), ng2.end());
	resolvent.erase(NogoodContainer::createLiteral(litadr, true));
	resolvent.erase(NogoodContainer::createLiteral(litadr, false));
	DBGLOG(DBG, "Resolution " << *this << " with " << ng2 << ": " << resolvent);
	return resolvent;
}

void Nogood::insert(ID lit){
	Set<ID>::insert(lit);
	ground &= lit.isOrdinaryGroundAtom();
}

bool Nogood::isGround() const{
	return ground;
}

bool Nogood::match(RegistryPtr reg, ID atomID, Nogood& instance) const{

	DBGLOG(DBG, "Matching " << *this << " with " << atomID);

	const OrdinaryAtom& atom = reg->ogatoms.getByID(atomID);

	// find an element in the nogood with unifies with atom
	BOOST_FOREACH (ID natID, *this){
		const OrdinaryAtom& nat = natID.isOrdinaryGroundAtom() ? reg->ogatoms.getByID(natID) : reg->onatoms.getByID(natID);

		if (atom.unifiesWith(nat)){
			DBGLOG(DBG, "Unifies with " << natID);

			// compute unifier
			std::map<ID, ID> unifier;
			int i = 0;
			BOOST_FOREACH (ID t, nat.tuple){
				if (t.isVariableTerm()){
					unifier[t] = atom.tuple[i];
					DBGLOG(DBG, "Unifier: " << t << " --> " << atom.tuple[i]);
				}
				i++;
			}

			// apply unifier to the overall nogood
			DBGLOG(DBG, "Applying unifier");
			BOOST_FOREACH (ID natID2, *this){
				if (natID2.isOrdinaryGroundAtom()){
					instance.insert(natID2);
				}else{
					OrdinaryAtom nat2 = reg->onatoms.getByID(natID2);
					bool ground = true;
					for (int i = 0; i < nat2.tuple.size(); ++i){
						if (unifier.find(nat2.tuple[i]) != unifier.end()){
							DBGLOG(DBG, "Substituting " << nat2.tuple[i] << " by " << unifier[nat2.tuple[i]]);
							nat2.tuple[i] = unifier[nat2.tuple[i]];
						}
						if (nat2.tuple[i].isVariableTerm()) ground = false;
					}
					if (ground){
						nat2.kind &= (ID::ALL_ONES ^ ID::SUBKIND_MASK);
						nat2.kind |= ID::SUBKIND_ATOM_ORDINARYG;
					}
					instance.insert(NogoodContainer::createLiteral(reg->storeOrdinaryAtom(nat2).address, !natID2.isNaf(), ground));
				}
			}
			DBGLOG(DBG, "Instance: " << instance);
			if (!instance.isGround()) DBGLOG(DBG, "Note: Instance is not ground!");
			return true;
		}
	}
	// no match
	return false;
}

// ---------- Class NogoodSet ----------

const NogoodSet& NogoodSet::operator=(const NogoodSet& other)
{
  nogoods = other.nogoods;
  freeIndices = other.freeIndices;
  nogoodsWithHash = other.nogoodsWithHash;
  
  return *this;
}

// reorders the nogoods such that there are no free indices in the range 0-(getNogoodCount()-1)
void NogoodSet::defragment(){
	if (freeIndices.size() == 0) return;

	int free = 0;
	int used = nogoods.size() - 1;
	while (free < used){
		// let used point to the last element which is not free
		while (used > 0 && freeIndices.count(used) > 0){
			nogoods.pop_back();
			used--;
		}
		// let free point to the next free index in the range 0-(ngg.nogoods.size()-1)
		while (free < nogoods.size() - 1 && freeIndices.count(free) == 0) free++;
		// move used to free
		if (free < used){
			nogoods[free] = nogoods[used];
			addCount[free] = addCount[used];
			nogoods.pop_back();
			addCount.pop_back();
			nogoodsWithHash[nogoods[free].getHash()].erase(used);
			nogoodsWithHash[nogoods[free].getHash()].insert(free);
			freeIndices.erase(free);
			free++;
			used--;
		}
	}
#ifndef NDEBUG
	// there must not be pointers to non-existing nogoods
	{
		typedef std::pair<size_t, Set<int> > Pair;
		BOOST_FOREACH (Pair p, nogoodsWithHash){
			BOOST_FOREACH (int i, p.second){
				assert (i < nogoods.size());
			}
		}
	}
#endif
	freeIndices.clear();
}

int NogoodSet::addNogood(Nogood ng){

	ng.recomputeHash();
	DBGLOG(DBG, "Hash of " << ng << " is " << ng.getHash());

	// check if ng is already present
	BOOST_FOREACH (int i, nogoodsWithHash[ng.getHash()]){
		if (nogoods[i] == ng){
			addCount[i]++;
			DBGLOG(DBG, "Already contained with index " << i);
			return i;
		}
	}

	// nogood is not present
	int index;
	if (freeIndices.size() == 0){
		nogoods.push_back(ng);
		addCount.push_back(1);
		index = nogoods.size() - 1;
	}else{
		int index = *freeIndices.begin();
		nogoods[index] = ng;
		addCount[index] = 1;
		freeIndices.erase(index);
	}
	DBGLOG(DBG, "Adding with index " << index);
	nogoodsWithHash[ng.getHash()].insert(index);
	return index;
}

Nogood& NogoodSet::getNogood(int index){
	return nogoods[index];
}

const Nogood& NogoodSet::getNogood(int index) const{
	return nogoods[index];
}

void NogoodSet::removeNogood(int nogoodIndex){
	addCount[nogoodIndex] = 0;
	nogoodsWithHash[nogoods[nogoodIndex].getHash()].erase(nogoodIndex);
	freeIndices.insert(nogoodIndex);
//	defragment();	// make sure that the nogood vector does not contain free slots
}

void NogoodSet::removeNogood(Nogood ng){
	ng.recomputeHash();

	// check if ng is present
	BOOST_FOREACH (int i, nogoodsWithHash[ng.getHash()]){
		if (nogoods[i] == ng){
			DBGLOG(DBG, "Deleting nogood " << ng << " (index: " << i << ")");
			// yes: delete it
			removeNogood(i);
			return;
		}
	}
	return;
}

int NogoodSet::getNogoodCount() const{
	return nogoods.size() - freeIndices.size();
}

void NogoodSet::forgetLeastFrequentlyAdded(){

	int mac = 0;
	for (int i = 0; i < nogoods.size(); i++){
		mac = mac > addCount[i] ? mac : addCount[i];
	}
	// delete those with an add count of less than 20% of the maximum add count
	for (int i = 0; i < nogoods.size(); i++){
		if (addCount[i] < mac * 0.05){
			DBGLOG(DBG, "Forgetting nogood " << nogoods[i]);
			removeNogood(i);
		}
	}
}

std::string NogoodSet::getStringRepresentation(RegistryPtr reg) const{

	std::stringstream ss;
	bool first = true;
	BOOST_FOREACH (Nogood ng, nogoods){
		if (!first){
			ss << ", ";
		}
		first = false;
		ss << ng.getStringRepresentation(reg);
	}
	return ss.str();
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

void SimpleNogoodContainer::addNogood(Nogood ng){
	boost::mutex::scoped_lock lock(mutex);
	ngg.addNogood(ng);
}

void SimpleNogoodContainer::removeNogood(Nogood ng){
	boost::mutex::scoped_lock lock(mutex);
	ngg.removeNogood(ng);
}

Nogood& SimpleNogoodContainer::getNogood(int index){
	boost::mutex::scoped_lock lock(mutex);
	return ngg.getNogood(index);
}

int SimpleNogoodContainer::getNogoodCount(){
	boost::mutex::scoped_lock lock(mutex);
	return ngg.getNogoodCount();
}

void SimpleNogoodContainer::clear(){
	boost::mutex::scoped_lock lock(mutex);
	ngg = NogoodSet();
}

void SimpleNogoodContainer::forgetLeastFrequentlyAdded(){
	DBGLOG(DBG, "Nogood count before forgetting " << ngg.getNogoodCount());
	ngg.forgetLeastFrequentlyAdded();
	ngg.defragment();
	DBGLOG(DBG, "Nogood count after forgetting " << ngg.getNogoodCount());
}

DLVHEX_NAMESPACE_END
