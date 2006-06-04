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
    // the names container type maps an index to a name (e.g., std::string)
    // index is l.h.s. to ensure that names are not reordered and iterators to
    // map-entries can be used.
    //
    typedef std::map<size_t, NameType> names_t;

    //
    // lookup table that provides fast access to names (name is l.h.s.) and
    // their iterators in the names_t table
    //
    typedef std::map<NameType, typename names_t::const_iterator> lookup_t;

    /**
     * Actual storage of names.
     */
    names_t names;

    lookup_t lookup;

    /**
     * Size of the container.
     */
    size_t indexcount;

public:

    
    class const_iterator
    {
        typename names_t::const_iterator it;

    public:

        const_iterator()
        {
            //assert(0);
        }

        const_iterator(const typename names_t::const_iterator &it1)
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
    insert(NameType name)
    {
        //
        // see if we already have this name
        //
        typename lookup_t::const_iterator it = this->lookup.find(name);

        //
        // yes - then return its iterator in the names-map
        //
        if (it != this->lookup.end())
            return const_iterator((*it).second);

        //
        // no - insert it into names table with current indexcount
        //
        std::pair<size_t, NameType> ins(this->indexcount++, name);
        std::pair<typename names_t::const_iterator, bool> res;
        res = this->names.insert(ins);

        //
        // and also update lookup, i.e., insert name and iterator we just
        // created:
        //
        std::pair<NameType, typename names_t::const_iterator> inslk(name, res.first);
        this->lookup.insert(inslk);

        return const_iterator(res.first);
    }

    void
    modify(const_iterator i, NameType name)
    {
        names[i.getIndex()] = name;
    }

    const_iterator
    begin() const
    {
        return const_iterator(names.begin());
    }

    const_iterator
    end() const
    {
        return const_iterator(names.end());
    }
};

#endif /* _NAMESTABLE_H */
