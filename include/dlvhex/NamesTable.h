/* -*- C++ -*- */

/**
 * @file NamesTable.h
 * @author Roman Schindlauer
 * @date Sat Feb  4 14:32:14 CET 2006
 *
 * @brief Names container class.
 *
 *
 */


#ifndef _NAMESTABLE_H
#define _NAMESTABLE_H


//#include <iostream>
//#include <string>
//#include <vector>
//#include <set>
#include <map>


/**
 * @brief Container class for names.
 *
 * A Name is an identifier - for predicate symbols and constants. This class
 * provides functions for storing names in a table, retrieve and modify them.
 */
template <class NameType>
class NamesTable
{
    //
    // the actual container type maps an index to a name (e.g., std::string)
    //
    typedef std::map<size_t, NameType> lookup_t;

    /**
     * Actual storage of names.
     */
    lookup_t lookup;

    /**
     * Size of the container.
     */
    size_t indexcount;

public:

    
    class const_iterator
    {
        typename lookup_t::const_iterator it;

    public:

        const_iterator()
        {
            //assert(0);
        }

        const_iterator(const typename lookup_t::const_iterator &it1)
            : it(it1)
        { }

        size_t
        getIndex() const
        {
            return (*it).first;
        }

        const NameType&
        operator *() const
        {
            return (*it).second;
        }

        void
        operator ++()
        {
            it++;
        }

        bool
        operator== (const const_iterator& i2) const
        {
            return it == i2.it;
        }

        bool
        operator != (const const_iterator& i2) const
        {
            return (it != i2.it);
        }
    };
    
    NamesTable()
        : indexcount(0)
    {
    }

    const_iterator
    find(NameType name) const
    {
        for (typename lookup_t::const_iterator i = lookup.begin();
             i != lookup.end();
             i++)
        {
            if ((*i).second == name)
                return const_iterator(i);
        }

        return const_iterator(lookup.end());
    }

    const_iterator
    insert(NameType name)
    {
        const_iterator i = find(name);
        
        if (i == const_iterator(lookup.end()))
        {
            indexcount++;

            std::pair<size_t, NameType> ins(indexcount, name);
            std::pair<typename lookup_t::iterator, bool> res;
            res = lookup.insert(ins);
            return const_iterator(res.first);
        }

        return i;
    }

    void
    modify(const_iterator i, NameType name)
    {
        lookup[i.getIndex()] = name;
    }

    const_iterator
    begin() const
    {
        return const_iterator(lookup.begin());
    }

    const_iterator
    end() const
    {
        return const_iterator(lookup.end());
    }
};

#endif /* _NAMESTABLE_H */
