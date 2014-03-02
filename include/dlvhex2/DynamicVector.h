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
 * @file   DynamicVector.hpp
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Dynamically extended vector with index access.
 */

#ifndef DYNAMICVECTOR_HPP_INCLUDED__23122011
#define DYNAMICVECTOR_HPP_INCLUDED__23122011

#include <iterator>
#include <boost/foreach.hpp>
#include <utility>
#include <iostream>
#include <bm/bm.h>
#include <boost/numeric/ublas/vector.hpp>

template<typename K, typename T>
class DynamicVector : public std::vector<T>{
private:
	bm::bvector<> stored;
public:
	typename std::vector<T>::iterator find(K index){
		if (stored.get_bit(index))	return std::vector<T>::begin() + index;
		else				return std::vector<T>::end();
	}

	void erase(K index){
		stored.clear_bit(index);
	}

	inline T& operator[](K index){
		if (index >= this->size()){
			this->resize(index + 1);
		}
		stored.set_bit(index);
		return std::vector<T>::operator[](index);
	}
};

/*
template<typename KeyType, typename ValueType>
class DynArray;

template<typename KeyType, typename ValueType>
class const_dynarray_iterator : public std::iterator<std::input_iterator_tag, std::pair<KeyType, ValueType>, ptrdiff_t, const std::pair<KeyType, ValueType>*, const std::pair<KeyType, ValueType>&>{
protected:
	const DynArray<KeyType, ValueType>& da;
	KeyType loc;
public:
	const_dynarray_iterator(const DynArray<KeyType, ValueType>& da, int l = 0) : da(da), loc(l){
	}

	inline const std::pair<KeyType, ValueType>& operator*() const{
		return std::pair<KeyType, ValueType>(loc, da[loc]);
	}

	inline const_dynarray_iterator& operator++(){
		++loc;
		return *this;
	}

	inline const_dynarray_iterator operator++(int){
		const_dynarray_iterator<KeyType, ValueType> old(*this);
		operator++();
		return old;
	}
	
	inline const_dynarray_iterator& operator--(){
		--loc;
		return *this;
	}

	inline const_dynarray_iterator operator--(int){
		const_dynarray_iterator<KeyType, ValueType> old(*this);
		operator--();
		return old;
	}

	inline const_dynarray_iterator operator+(int i) const{
		return const_dynarray_iterator<KeyType, ValueType>(da, loc + 1);
	}
	
	inline const_dynarray_iterator operator+(const_dynarray_iterator& it) const{
		return const_dynarray_iterator<KeyType, ValueType>(da, loc + it.loc);
	}

	inline const_dynarray_iterator operator-(int i) const{
		return const_dynarray_iterator<KeyType, ValueType>(da, loc - i);
	}
	
	inline const_dynarray_iterator operator-(const_dynarray_iterator& it) const{
		return const_dynarray_iterator<KeyType, ValueType>(da, loc - it.loc);
	}

	inline operator const KeyType() const{
		return loc;
	}

	inline bool operator==(const_dynarray_iterator const& sit2) const{
		return loc == sit2.loc;
	}

	inline bool operator!=(const_dynarray_iterator const& sit2) const{
		return loc != sit2.loc;
	}
};

template<typename KeyType, typename ValueType>
class dynarray_iterator : public std::iterator<std::input_iterator_tag, std::pair<KeyType, ValueType>, ptrdiff_t, std::pair<KeyType, ValueType>*, std::pair<KeyType, ValueType>&>{
protected:
	DynArray<KeyType, ValueType>& da;
	KeyType loc;
public:
	dynarray_iterator(DynArray<KeyType, ValueType>& da, int l = 0) : da(da), loc(l){
	}

	inline std::pair<KeyType, ValueType>& operator*(){
		return std::pair<KeyType, ValueType>(loc, da[loc]);
	}

	inline dynarray_iterator& operator++(){
		++loc;
		return *this;
	}

	inline dynarray_iterator operator++(int){
		dynarray_iterator<KeyType, ValueType> old(*this);
		operator++();
		return old;
	}
	
	inline dynarray_iterator& operator--(){
		--loc;
		return *this;
	}

	inline dynarray_iterator operator--(int){
		dynarray_iterator<KeyType, ValueType> old(*this);
		operator--();
		return old;
	}

	inline dynarray_iterator operator+(int i){
		return dynarray_iterator<KeyType, ValueType>(da, loc + 1);
	}
	
	inline dynarray_iterator operator+(dynarray_iterator& it){
		return dynarray_iterator<KeyType, ValueType>(da, loc + it.loc);
	}

	inline dynarray_iterator operator-(int i){
		return dynarray_iterator<KeyType, ValueType>(da, loc - i);
	}
	
	inline dynarray_iterator operator-(dynarray_iterator& it){
		return dynarray_iterator<KeyType, ValueType>(da, loc - it.loc);
	}

	inline operator const KeyType() const{
		return loc;
	}

	inline bool operator==(dynarray_iterator const& sit2) const{
		return loc == sit2.loc;
	}

	inline bool operator!=(dynarray_iterator const& sit2) const{
		return loc != sit2.loc;
	}
};

template<typename KeyType, typename ValueType>
class DynArray{
protected:
	ValueType* data;
	bm::bvector<> stored;
	long allocSize;
	
	inline void grow(KeyType index){
		if (index >= allocSize){
			allocSize = index + 1;
			data = (ValueType*)realloc(data, sizeof(ValueType) * allocSize);
		}
	}
public:
	DynArray(int initialSize = 1){
		data = (ValueType*)realloc(0, sizeof(ValueType) * initialSize);
		allocSize = initialSize;
	}

	virtual ~DynArray(){
		if (data) free(data);
		data = 0;
	}

	inline void erase(KeyType index){
		stored.clear_bit(index);
	}

	inline ValueType& operator[](KeyType index){
		grow(index);
		stored.set_bit(index);
		return data[index];
	}

	inline const ValueType& operator[](KeyType index) const{
		grow(index);
		stored.set_bit(index);
		return data[index];
	}

	const_dynarray_iterator<KeyType, ValueType> find(KeyType k) const{
		if (stored.get_bit(k)){
			return const_dynarray_iterator<KeyType, ValueType>(*this, k);
		}else{
			return ((const DynArray<KeyType, ValueType>*)this)->end();
		}
	}

	dynarray_iterator<KeyType, ValueType> begin(){
		return dynarray_iterator<KeyType, ValueType>(*this, 0);
	}

	dynarray_iterator<KeyType, ValueType> end(){
		return dynarray_iterator<KeyType, ValueType>(*this, allocSize);
	}

	const_dynarray_iterator<KeyType, ValueType> begin() const{
		return const_dynarray_iterator<KeyType, ValueType>(*this, 0);
	}

	const_dynarray_iterator<KeyType, ValueType> end() const{
		return const_dynarray_iterator<KeyType, ValueType>(*this, allocSize);
	}
};


// compatibility with BOOST_FOREACH
namespace boost{
	template<typename KeyType, typename ValueType>
	struct range_mutable_iterator< DynArray<KeyType, ValueType> >{
		typedef dynarray_iterator<KeyType, ValueType> type;
	};

	template<typename KeyType, typename ValueType>
	struct range_const_iterator< DynArray<KeyType, ValueType> >{
		typedef dynarray_iterator<KeyType, ValueType> type;
	};
}
*/
#endif

