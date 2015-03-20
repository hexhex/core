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
 * @file Nogood.cpp
 * @author Christoph Redl
 *
 * @brief Data structures for CDNLSolver.
 */


#include "dlvhex2/Nogood.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include "dlvhex2/Logger.h"
#include "dlvhex2/Printer.h"
#include <boost/functional/hash.hpp>

DLVHEX_NAMESPACE_BEGIN


// ---------- Class Nogood ----------

//#define DBGLOGD(X,Y) DBGLOG(X,Y)
#define DBGLOGD(X,Y) do{}while(false);

namespace{
	struct VariableSorter{
		typedef std::pair<ID, std::vector<int> > VarType;
		bool operator() (VarType p1, VarType p2){
			for (uint32_t i = 0; i < p1.second.size(); i++){
				if (p1.second[i] < p2.second[i]) return true;
				if (p1.second[i] > p2.second[i]) return false;
			}
			if (p2.second.size() > p1.second.size()) return true;

			// they are considered equal
			return false;
		}
	};
}

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

bool Nogood::operator==(const Nogood& ng2) const{

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

bool Nogood::operator!=(const Nogood& ng2) const{
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

Nogood Nogood::resolve(const Nogood& ng2, IDAddress groundlitadr){
	// resolvent = union of this and ng2 minus both polarities of the resolved literal
	Nogood resolvent = *this;
	resolvent.insert(ng2.begin(), ng2.end());
	resolvent.erase(NogoodContainer::createLiteral(groundlitadr, true));
	resolvent.erase(NogoodContainer::createLiteral(groundlitadr, false));
	DBGLOG(DBG, "Resolution " << *this << " with " << ng2 << ": " << resolvent);
	assert(resolvent.size() < this->size() + ng2.size() && "resolvent is not smaller than the union of the two nogoods; ensure that the resolved literal is chosen correctly");
	return resolvent;
}

Nogood Nogood::resolve(const Nogood& ng2, ID lit){
	// resolvent = union of this and ng2 minus both polarities of the resolved literal
	Nogood resolvent = *this;
	resolvent.insert(ng2.begin(), ng2.end());
	resolvent.erase(NogoodContainer::createLiteral(lit.address, true, lit.isOrdinaryGroundAtom()));
	resolvent.erase(NogoodContainer::createLiteral(lit.address, false, lit.isOrdinaryGroundAtom()));
	DBGLOG(DBG, "Resolution " << *this << " with " << ng2 << ": " << resolvent);
	assert(resolvent.size() < this->size() + ng2.size() && "resolvent is not smaller than the union of the two nogoods; ensure that the resolved literal is chosen correctly");
	return resolvent;
}

void Nogood::applyVariableSubstitution(RegistryPtr reg, const std::map<ID, ID>& subst){

	DBGLOG(DBG, "Applying variable substitution to " << getStringRepresentation(reg));
	Nogood newng;
	BOOST_FOREACH (ID lit, *this){
		OrdinaryAtom oatom = reg->lookupOrdinaryAtom(lit);
		bool changed = false;
		for (uint32_t t = 1; t < oatom.tuple.size(); t++){
			if (subst.count(oatom.tuple[t]) > 0){
				oatom.tuple[t] = subst.at(oatom.tuple[t]);
				changed = true;
			}
		}
		ID newlit = lit;
		if (changed) newlit.address = reg->storeOrdinaryAtom(oatom).address;
		newng.insert(NogoodContainer::createLiteral(newlit));
	}
	DBGLOG(DBG, "New nogood is " << newng.getStringRepresentation(reg));
	*this = newng;
}

void Nogood::heuristicNormalization(RegistryPtr reg){

	// This method renames the variables in a nonground nogood
	// such that multiple nogoods which differ only in the variable naming
	// are likely to become equivalent.
	// This is useful for reducing redundancy in resolution.

	// For this we sort the variables such that for variables X, Y
	// we have X < Y
	// iff Y occurs more often than X
	//     or Y occurs as often as X but more often at first argument position
	//     or Y occurs as often as X also at first argument position but more often at the second,
	//     etc.
	// and X = Y otherwise.

	if (isGround()) return;
	DBGLOG(DBG, "Normalizing " << getStringRepresentation(reg));

	// prepare statistics
	std::map<ID, std::vector<int> > vars;
	BOOST_FOREACH (ID id, *this){
		const OrdinaryAtom& oatom = reg->lookupOrdinaryAtom(id);
		for (uint32_t i = 1; i < oatom.tuple.size(); ++i){
			if (oatom.tuple[i].isVariableTerm()){
				std::vector<int>& pos = vars[oatom.tuple[i]];
				while (pos.size() < i + 1) pos.push_back(0);
				pos[0]++;
				pos[i]++;
			}
		}
	}
	std::vector<VariableSorter::VarType> sortedVars;
	BOOST_FOREACH (VariableSorter::VarType v, vars){
		sortedVars.push_back(v);
	}

	// sort
	VariableSorter sorter;
	std::sort(sortedVars.begin(), sortedVars.end(), sorter);

	// assign new variable names
	std::map<ID, ID> renaming;
	int i = 0;
	BOOST_FOREACH (VariableSorter::VarType v, sortedVars){
		std::stringstream ss;
		ss << "X" << i++;
		renaming[v.first] = reg->storeVariableTerm(ss.str());
	}
	applyVariableSubstitution(reg, renaming);
	DBGLOG(DBG, "Normalized " << getStringRepresentation(reg));
}

void Nogood::insert(ID lit){
	// strip off property flags
	lit = NogoodContainer::createLiteral(lit);

	// actual insertion
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

		if (atom.unifiesWith(nat, reg)){
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
					for (uint32_t i = 0; i < nat2.tuple.size(); ++i){
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

#ifndef NDEBUG

std::string Nogood::dbgsave() const{
	std::stringstream ss;
	BOOST_FOREACH (ID id, *this){
		ss << (id.isNaf() ? "-" : "+") << "/" << id.address << ";";
	}
	return ss.str();
}

void Nogood::dbgload(std::string str){

	while (str.find_first_of("/") != std::string::npos){
		std::string sign = str.substr(0, str.find_first_of("/"));
		std::string adr = str.substr(str.find_first_of("/") + 1, str.find_first_of(";"));
		str = str.substr(str.find_first_of(";") + 1);
		insert(NogoodContainer::createLiteral(atoi(str.c_str()), sign[0] == '+'));
	}
}

#endif

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
		while (free < (int)nogoods.size() - 1 && freeIndices.count(free) == 0) free++;
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
				assert (i < (int)nogoods.size());
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
		index = *freeIndices.begin();
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
	for (uint32_t i = 0; i < nogoods.size(); i++){
		mac = mac > addCount[i] ? mac : addCount[i];
	}
	// delete those with an add count of less than 5% of the maximum add count
	for (uint32_t i = 0; i < nogoods.size(); i++){
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

void SimpleNogoodContainer::addAllResolvents(RegistryPtr reg, int maxSize){

	std::vector<Nogood> nogoodList;
	for (int i = 0; i < getNogoodCount(); i++){
		nogoodList.push_back(getNogood(i));
		nogoodList[nogoodList.size() - 1].heuristicNormalization(reg);
	}

	// for all nogoods
	std::vector<Nogood> addList;
	int ng1i = 0;
	while (ng1i < (int)nogoodList.size()){
		Nogood ng1 = nogoodList[ng1i];
		DBGLOG(DBG, "Trying to resolve " << ng1.getStringRepresentation(reg));

		// rename all variables in ng1 to avoid name clashes
		DBGLOG(DBG, "Renaming all variables");
		std::set<ID> vars;
		std::map<ID, ID> renaming;
		BOOST_FOREACH (ID id1, ng1) reg->getVariablesInID(id1, vars);
		BOOST_FOREACH (ID v, vars){
			assert(!v.isAnonymousVariable() && "do not support anonymous variables in nogoods");
			// just mark the variables as anonymous to ensure that they have a different ID
			renaming[v] = ID(v.kind | ID::PROPERTY_VAR_ANONYMOUS,  v.address);
		}
		ng1.applyVariableSubstitution(reg, renaming);

		// for all other nogoods
		for (uint32_t ng2i = 0; ng2i < nogoodList.size(); ng2i++){
			const Nogood& ng2 = nogoodList[ng2i];

			// check if they unify
			DBGLOG(DBG, "Checking if " << ng1.getStringRepresentation(reg) << " unifies with " << ng2.getStringRepresentation(reg));
			BOOST_FOREACH (ID id1, ng1){
				BOOST_FOREACH (ID id2, ng2){
					if (id1.isNaf() != id2.isNaf() && reg->lookupOrdinaryAtom(id1).unifiesWith(reg->lookupOrdinaryAtom(id2))){
						// match id1 with id2
						std::map<ID, ID> match;
						const OrdinaryAtom& at1 = reg->lookupOrdinaryAtom(id1);
						const OrdinaryAtom& at2 = reg->lookupOrdinaryAtom(id2);
						for (uint32_t i = 1; i < at1.tuple.size(); i++) match[at1.tuple[i]] = at2.tuple[i];
						Nogood ng1matched = ng1;
						ng1matched.applyVariableSubstitution(reg, match);

						DBGLOG(DBG, "Resolving " << ng1matched.getStringRepresentation(reg) << "(" << ng1matched << ") with " << ng2.getStringRepresentation(reg) << " (" << ng2 << ")" << " on " << id2);
						Nogood resolvent = ng1matched.resolve(ng2, id2);
						// now assign new variable names to the renamed variables (names which occur neither in ng1 nor in ng2)
						std::set<ID> vars;
						std::map<ID, ID> backrenaming;
						BOOST_FOREACH (ID id1, ng1matched) reg->getVariablesInID(id1, vars);
						BOOST_FOREACH (ID id1, ng2) reg->getVariablesInID(id1, vars);
						typedef std::pair<ID, ID> KVPair;
						BOOST_FOREACH (KVPair kv, renaming){
							ID newVar = ID_FAIL;
							int cnt = 0;
							while (newVar == ID_FAIL || vars.count(newVar) > 0){
								std::stringstream ss;
								ss << reg->terms.getByID(kv.first).getUnquotedString();
								if (cnt++ > 0) ss << cnt;
								newVar = reg->storeVariableTerm(ss.str());
							}
							backrenaming[renaming[kv.first]] = newVar;
						}
						resolvent.applyVariableSubstitution(reg, backrenaming);
						resolvent.heuristicNormalization(reg);
#ifdef DEBUG
						std::stringstream ss;
						RawPrinter printer(ss, reg);
						printer.print(id1);
						DBGLOG(DBG, "Computed resolvent " << resolvent.getStringRepresentation(reg) << " by resolving " << ss.str());
#endif
						// finally add the resolvent if its size is within the limit
						if (maxSize == -1 || resolvent.size() <= maxSize){
							DBGLOG(DBG, "Adding the resolvent");
							addNogood(resolvent);
							// if the nogood is not already present, then we also need to resolve it
							if (getNogoodCount() > (int)(nogoodList.size() + addList.size())){
								DBGLOG(DBG, "Adding the resolvent " << resolvent.getStringRepresentation(reg) << " for further resolution because there were " << (getNogoodCount() - (nogoodList.size() + addList.size())) << " new nogoods");
								addList.push_back(resolvent);
							}
						}
					}
				}
			}
			DBGLOG(DBG, "Finished checking " << ng2.getStringRepresentation(reg));
		}

		DBGLOG(DBG, "Finished checking " << ng1.getStringRepresentation(reg));
		nogoodList.insert(nogoodList.end(), addList.begin(), addList.end());
		addList.clear();
		ng1i++;
	}
}

void SimpleNogoodContainer::forgetLeastFrequentlyAdded(){
	boost::mutex::scoped_lock lock(mutex);
	DBGLOG(DBG, "Nogood count before forgetting " << ngg.getNogoodCount());
	ngg.forgetLeastFrequentlyAdded();
	ngg.defragment();
	DBGLOG(DBG, "Nogood count after forgetting " << ngg.getNogoodCount());
}

void SimpleNogoodContainer::defragment(){
	ngg.defragment();
}

DLVHEX_NAMESPACE_END
