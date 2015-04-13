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

/** \brief Dynamically extended vector using custom index and value types. */
template<typename K, typename T>
class DynamicVector : public std::vector<T>
{
    private:
        /** \brief Storage of all valid indexes; K must be convertible to integer. */
        bm::bvector<> stored;
    public:
        /** \brief Returns an iterator to an element of the DynamicVector.
         * @param index Index to check.
         * @return Iterator to the element \p index if valid, and the end() iterator otherwise. */
        typename std::vector<T>::iterator find(K index) {
            if (stored.get_bit(index))  return std::vector<T>::begin() + index;
            else        return std::vector<T>::end();
        }

        /** \brief Erases an element from the DynamicVector.
         * @param index Index of the element to erase. */
        void erase(K index) {
            stored.clear_bit(index);
        }

        /** \brief Accesses and element of the DynamicVector.
         * @param index Index of the element to access.
         * @return Reference to the accessed element. */
        inline T& operator[](K index) {
            if (index >= (K)this->size()) {
                this->resize(index + 1);
            }
            stored.set_bit(index);
            return std::vector<T>::operator[](index);
        }
};
#endif
