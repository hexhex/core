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
 * @file   Set.hpp
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 *
 * @brief  Set data structures implemented as dynamically allocated arrays.
 */

#ifndef SET_HPP_INCLUDED__09122011
#define SET_HPP_INCLUDED__09122011

#include <iterator>
#include <boost/foreach.hpp>
#include <boost/unordered_map.hpp>
#include "dlvhex2/DynamicVector.h"

template<typename T>
class Set;

/** \brief This class implements an insertion iterator for Set according to public std::iterator.
 *
 * See public std::iterator for descriptions of the methods and operators in this class. */
template<typename T>
class insert_set_iterator : public std::iterator<std::output_iterator_tag, void, void, void, void>
{
    protected:
        Set<T>& set;
    public:
        insert_set_iterator(Set<T>& s) : set(s) {
        }

        insert_set_iterator& operator=(const typename Set<T>::value_type& v) {
            set.insert(v);
            return *this;
        }

        insert_set_iterator& operator*() {
            return *this;
        }

        insert_set_iterator& operator++() {
            return *this;
        }

        insert_set_iterator operator++(int) {
            return *this;
        }
};

/** \brief This class implements a const_iterator for Set according to public std::iterator.
 *
 * See public std::iterator for descriptions of the methods and operators in this class. */
template<typename T>
class const_set_iterator : public std::iterator<std::input_iterator_tag, T, ptrdiff_t, const T*, const T&>
{
    protected:
        const Set<T>& set;
        int loc;
    public:
        const_set_iterator(const Set<T>& s, int l = 0) : set(s), loc(l) {
        }

        inline const T& operator*() const
        {
            return set.getData()[loc];
        }

        inline const T* operator->() const
        {
            return &(set.getData()[loc]);
        }

        inline const_set_iterator& operator++() {
            ++loc;
            return *this;
        }

        inline const_set_iterator operator++(int) {
            const_set_iterator<T> old(*this);
            operator++();
            return old;
        }

        inline const_set_iterator& operator--() {
            --loc;
            return *this;
        }

        inline const_set_iterator operator--(int) {
            const_set_iterator<T> old(*this);
            operator--();
            return old;
        }

        inline const_set_iterator operator+(int i) const
        {
            return const_set_iterator<T>(set, loc + 1);
        }

        inline const_set_iterator operator+(const_set_iterator& it) const
        {
            return const_set_iterator<T>(set, loc + it.loc);
        }

        inline const_set_iterator operator-(int i) const
        {
            return const_set_iterator<T>(set, loc - i);
        }

        inline const_set_iterator operator-(const_set_iterator& it) const
        {
            return const_set_iterator<T>(set, loc - it.loc);
        }

        inline operator const int() const
        {
            return loc;
        }

        inline bool operator==(const_set_iterator const& sit2) const
        {
            return loc == sit2.loc;
        }

        inline bool operator!=(const_set_iterator const& sit2) const
        {
            return loc != sit2.loc;
        }
};

/** \brief This class implements an iterator for Set according to public std::iterator.
 *
 * See public std::iterator for descriptions of the methods and operators in this class. */
template<typename T>
class set_iterator : public std::iterator<std::input_iterator_tag, T, ptrdiff_t, T*, T&>
{
    protected:
        Set<T>& set;
        int loc;
    public:
        set_iterator(Set<T>& s, int l = 0) : set(s), loc(l) {
        }

        inline T& operator*() {
            return set.getData()[loc];
        }

        inline T* operator->() {
            return &(set.getData()[loc]);
        }

        inline set_iterator& operator++() {
            ++loc;
            return *this;
        }

        inline set_iterator operator++(int) {
            set_iterator<T> old(*this);
            operator++();
            return old;
        }

        inline set_iterator& operator--() {
            --loc;
            return *this;
        }

        inline set_iterator operator--(int) {
            set_iterator<T> old(*this);
            operator--();
            return old;
        }

        inline set_iterator operator+(int i) const
        {
            return set_iterator<T>(set, loc + i);
        }

        inline set_iterator operator+(set_iterator& it) const
        {
            return set_iterator<T>(set, loc + it.loc);
        }

        inline set_iterator operator-(int i) const
        {
            return set_iterator<T>(set, loc - i);
        }

        inline set_iterator operator-(set_iterator& it) const
        {
            return set_iterator<T>(set, loc - it.loc);
        }

        inline operator const int() const
        {
            return loc;
        }

        inline bool operator==(set_iterator const& sit2) const
        {
            return loc == sit2.loc;
        }

        inline bool operator!=(set_iterator const& sit2) const
        {
            return loc != sit2.loc;
        }
};

/** \brief Data structure for storing sets based on a sorted array. */
template<typename T>
class Set
{
    private:
        T* data;

        int allocSize;
        int rsize;
        int increase;

        /** \brief Grows Set::data such that it covers Set::allocSize. */
        void grow() {
            allocSize += increase;
            data = (T*)realloc(data, sizeof(T) * allocSize);
        }

        /** \brief Grows Set::data such that it covers a given minimum size.
         *
         * @param minSize Minimum size of the Set after this method retuns. */
        void grow(int minSize) {
            allocSize = minSize % increase == 0
                ? minSize
                : (minSize / increase + 1) * increase;
            data = (T*)realloc(data, sizeof(T) * allocSize);
        }

        /** \brief Implements binary search for the set.
         * @param e Element to search for.
         * @return Index of \p e in the Set or 0 if it is not contained. */
        int binarySearch(T e) const
        {
            if (rsize == 0) return 0;

            int lower = 0;
            int upper = rsize - 1;
            int pos = (lower + upper) / 2;

            while (data[pos] != e) {
                if (e < data[pos]) {
                    if (pos > lower) {
                        upper = pos - 1;
                    }
                    else {
                        // element is not contained: return insert location
                        return pos;
                    }
                }
                else {
                    if (pos < upper) {
                        lower = pos + 1;
                    }
                    else {
                        // element is not contained: return insert location
                        return pos + 1;
                    }
                }
                pos = (lower + upper) / 2;
            }
            // element is contained
            return pos;
        }

    public:
        typedef set_iterator<T> iterator;
        typedef const_set_iterator<T> const_iterator;
        typedef T value_type;
        typedef const T& const_reference;

        /** \brief Constructor.
         * @param initialSize Internal size of the internal array.
         * @param inc Number of elements to add when the internal array needs to be resized. */
        Set(int initialSize = 0, int inc = 10) : increase(inc), rsize(0) {
            if (initialSize == 0) {
                data = 0;
            }
            else {
                data = (T*)realloc(0, sizeof(T) * initialSize);
            }
            allocSize = initialSize;
        }

        /** \brief Copy-constructor.
         * @param s2 Second set. */
        Set(const Set<T>& s2) {
            data = (T*)realloc(0, sizeof(T) * s2.allocSize);
            allocSize = s2.allocSize;
            increase = s2.increase;
            rsize = s2.rsize;
            memcpy(data, s2.data, sizeof(T) * rsize);
        }

        /** \brief Destructor. */
        virtual ~Set() {
            if (data) free(data);
            data = 0;
        }

        /** \brief Checks if an element is contained in the set.
         * @param e Element to search for.
         * @return True if \p e is contained in the Set and false otherwise. */
        inline bool contains(T e) const
        {
            int p =  binarySearch(e);
            return (p < rsize) && (data[p] == e);
        }

        /** \brief Retrieves the size of the Set.
         *
         * For compatibility with std::set.
         * @return Size of the Set. */
        inline int count(T e) const
        {
            return contains(e) ? 1 : 0;
        }

        /** \brief Adds an element to the Set.
         * @param e Element to add. */
        inline void insert(T e) {
            // find location for insertion
            int p = binarySearch(e);
                                 // already contained
            if (p < rsize && data[p] == e) return;

            if (rsize == allocSize) {
                grow();
            }

            // shift
            memmove(&(data[p + 1]), &(data[p]), sizeof(T) * (rsize - p));

            // insert
            data[p] = e;
            rsize++;
        }

        /** \brief Inserts a range of elements given by a pair of begin and end iterator.
         * @param begin Begin iterator.
         * @param end End iterator. */
        template<typename _Iter>
        inline void insert(_Iter begin, _Iter end) {
            grow(size() + (end - begin));
            for (_Iter it = begin; it != end; ++it) {
                insert(*it);
            }
        }

        /** \brief Removes an element from the Set if contained.
         * @param e Element to remove. */
        inline void erase(T e) {
            // find element
            int p = binarySearch(e);
                                 // not contained
            if (p >= rsize) return;
                                 // not contained
            if (data[p] != e) return;

            // shift
            memmove(&(data[p]), &(data[p + 1]), sizeof(T) * (rsize - (p + 1)));

            rsize--;
        }

        /** \brief Searches for an element in the Set.
         * @param e Element to search for.
         * @return Iterator pointing to \p e if contained and the end() iterator otherwise. */
        inline set_iterator<T> find(T e) {
            // find element
            int p = binarySearch(e);
                                 // not contained
            if (p >= rsize) return end();
                                 // not contained
            if (data[p] != e) return end();
            return set_iterator<T>(*this, p);
        }

        /** \brief Searches for an element in the Set.
         * @param e Element to search for.
         * @return Const iterator pointing to \p e if contained and the end() iterator otherwise. */
        inline const_set_iterator<T> find(T e) const
        {
            // find element
            int p = binarySearch(e);
                                 // not contained
            if (p >= rsize) return ((const Set<T>*)this)->end();
                                 // not contained
            if (data[p] != e) return ((const Set<T>*)this)->end();
            return const_set_iterator<T>(*this, p);
        }

        /** \brief Checks if the Set is empty.
         * @return True if the Set is empty and false otherwise. */
        bool empty() const
        {
            return rsize == 0;
        }

        /** \brief Empties the Set. */
        void clear() {
            rsize = 0;
        }

        /** \brief Retruns the begin iterator of the Set.
         * @return Begin iterator of the Set. */
        set_iterator<T> begin() {
            return set_iterator<T>(*this, 0);
        }

        /** \brief Retruns the end iterator of the Set.
         * @return End iterator of the Set. */
        set_iterator<T> end() {
            return set_iterator<T>(*this, rsize);
        }

        /** \brief Retruns the const begin iterator of the Set.
         * @return Const begin iterator of the Set. */
        const_set_iterator<T> begin() const
        {
            return const_set_iterator<T>(*this, 0);
        }

        /** \brief Retruns the const end iterator of the Set.
         * @return Const end iterator of the Set. */
        const_set_iterator<T> end() const
        {
            return const_set_iterator<T>(*this, rsize);
        }

        /** \brief Accesses an element given by its index.
         * @param i Index of the element to access.
         * @return Reference to element \p i in the Set. */
        inline T& operator[](int i) {
            return data[i];
        }

        /** \brief Accesses an element given by its index.
         * @param i Index of the element to access.
         * @return Reference to element \p i in the Set. */
        inline const T& operator[](int i) const
        {
            return data[i];
        }

        /** \brief Retrieves the size of the Set.
         *
         * For compatibility with std::set.
         * @return Size of the Set. */
        inline int size() const
        {
            return rsize;
        }

        /** \brief Retrieves the internal data (array).
         * @return Pointer to the begin of the internal array. */
        T* getData() {
            return data;
        }

        /** \brief Retrieves the internal data (array).
         * @return Pointer to the begin of the internal array. */
        const T* getData() const
        {
            return data;
        }

        /** \brief Assigns another Set to this one.
         * @param s2 Second Set.
         * @return Reference to this Set. */
        Set<T>& operator=(const Set<T>& s2) {
            clear();
            grow(s2.size());
            rsize = s2.rsize;
            memcpy(data, s2.data, sizeof(T) * rsize);
            return *this;
        }
};

/** \brief Implements a pair of an index and an element. */
template<typename T>
struct SortElement
{
    /** \brief Index. */
    long index;
    /** \brief Element. */
    T elem;

    /** \brief Constructor. */
    SortElement() {
    }

    /** \brief Constructor.
     * @param i See SortElement::index.
     * @param el See SortElement::elem. */
    SortElement(int i, T el) : index(i), elem(el) {
    }

    /** \brief Compares the index of \p el2 to the one in this object.
     * @param el2 Second SortElement.
     * @return True if the index of this SortElement is smaller than the one in \p el2 and false otherwise. */
    bool operator<(const SortElement<T>& el2) const
    {
        return index < el2.index;
    }

    /** \brief Compares the index of \p el2 to the one in this object.
     * @param el2 Second SortElement.
     * @return True if the index of this SortElement is larger than the one in \p el2 and false otherwise. */
    bool operator>(const SortElement<T>& el2) const
    {
        return index > el2.index;
    }

    /** \brief Compares the index of \p el2 to the one in this object.
     * @param el2 Second SortElement.
     * @return True if the index of this SortElement is equal than the one in \p el2 and false otherwise. */

    bool operator==(const SortElement<T>& el2) const
    {
        return index == el2.index;
    }

    /** \brief Compares the index of \p el2 to the one in this object.
     * @param el2 Second SortElement.
     * @return True if the index of this SortElement is not equal than the one in \p el2 and false otherwise. */
    bool operator!=(const SortElement<T>& el2) const
    {
        return index != el2.index;
    }
};

/** \brief Implementation of an ordered set based on boost::unordered_map. */
template<typename T, typename H>
class OrderedSet
{
    private:
        /** \brief Internal sorage. */
        DynamicVector<T, long> os;
        /** \brief Next element to insert. */
        long c;

        /** \brief Renumbers the elements but keeps their order; useful after elements have been deleted. */
        void renumber() {
            typedef SortElement<T> SortEl;
            std::vector<SortElement<T> > sorted;
            sorted.reserve(os.size());

            for (T i = 0; i < os.size(); ++i) {
                if (os.find(i) != os.end()) {
                    sorted.push_back(SortEl(os[i], i));
                }
            }

            std::sort(sorted.begin(), sorted.end());

            c = 0;
            BOOST_FOREACH (SortEl se, sorted) {
                os[se.elem] = c++;
            }
        }

    public:
        /** \brief Constructor. */
        OrderedSet() : c(0) {
        }

        /** \brief Inserts a new element.
         * @param el Element to insert. */
        inline void insert(T el) {
            if (c >= 1000000000) {
                renumber();
            }
            os[el] = c++;
        }

        /** \brief Removes an element.
         * @param el Element to remove. */
        inline void erase(T el) {
            os.erase(el);
        }

        /** \brief Get the insertion index of an element.
         *
         * The larger the insertion index, the later the element was added.
         * @param el Element whose insertion index shall be retrieved.
         * @return Insertion index of \p el. */
        long getInsertionIndex(T el) {
            return os[el];
        }

        /** \brief Compares two elements according to their insertion index
         *
         * @param el1 First element to be compared.
         * @param el2 Second element to be compared.
         * @return -1 if \p el1 was inserted before \p el2, 1 if \p el1 was inserted after \p el2, 0 otherwise. */
        inline int compare(T el1, T el2) {
            if (getInsertionIndex(el1) < getInsertionIndex(el2)) return -1;
            else if (getInsertionIndex(el1) > getInsertionIndex(el2)) return +1;
            else return 0;
        }

        /** \brief No operation; kept for backwards compatibility. */
        void resize(int s) {
        }
};

// compatibility with BOOST_FOREACH
namespace boost
{
    template<typename T>
        struct range_mutable_iterator< Set<T> >
    {
        typedef set_iterator<T> type;
    };

    template<typename T>
        struct range_const_iterator< Set<T> >
    {
        typedef const_set_iterator<T> type;
    };
}
#endif
