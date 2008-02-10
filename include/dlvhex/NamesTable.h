/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
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
 * @file NamesTable.h
 * @author Roman Schindlauer
 * @date Sat Feb  4 14:32:14 CET 2006
 *
 * @brief Names container class.
 *
 *
 */


#if !defined(_DLVHEX_NAMESTABLE_H)
#define _DLVHEX_NAMESTABLE_H

#include "dlvhex/PlatformDefinitions.h"

#include <map>

DLVHEX_NAMESPACE_BEGIN

/**
 * @brief Container class for names.
 *
 * A Name is an identifier - for predicate symbols and constants. This class
 * provides functions for storing names in a table, retrieve and modify them.
 */
template <class NameType>
class DLVHEX_EXPORT NamesTable
{
    struct n_t {
        NameType n;
        unsigned ix;
    };

    //
    // the names container type maps an index to a name (e.g., std::string)
    // index is l.h.s. to ensure that names are not reordered and iterators to
    // map-entries can be used.
    //
    typedef std::map<size_t, n_t> names_t;

    //
    // lookup table that provides fast access to names (name is l.h.s.) and
    // their iterators in the names_t table
    //
    typedef std::map<NameType, typename names_t::const_iterator> lookup_t;

public:
    /**
     * Actual storage of names.
     */
    names_t names;

    lookup_t lookup;

    /**
     * Size of the container.
     */
    size_t indexcount;

    
    /**
     * @brief Custom const_iterator class, such that we can treat the
     * class NamesTable<T> similar to a container.
     */
    class const_iterator
    {
    public:
        typename names_t::const_iterator it;


        const_iterator()
        {
            //assert(0);
        }

        const_iterator(const typename names_t::const_iterator& it1)
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
            return (*it).second.n;
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

        int
        cmp(const const_iterator& i2) const
        {
            if (this->it->second.ix < i2.it->second.ix)
                return 1;
            else if (this->it->second.ix > i2.it->second.ix)
                return -1;
            
            return 0;
        }
    };
    
    NamesTable()
        : indexcount(0)
    {
    }


    const_iterator
    insert(const NameType& name)
    {
        //
        // see if we already have this name
        //
        typename lookup_t::const_iterator it = this->lookup.find(name);

        //
        // yes - then return its iterator in the names-map
        //
        if (it != this->lookup.end())
            return const_iterator(it->second);

        //
        // no - insert it into names table with current indexcount
        //
        n_t newname;
        newname.n = name;
        newname.ix = this->indexcount;

        std::pair<size_t, n_t> ins(this->indexcount++, newname);
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
        typename names_t::iterator ni = names.begin();

        bool wasinlookup = 0;

        while (ni != names.end())
        {
            //
            // already exists:
            //
            if (ni->second.n == name)
            {
                ni->second.ix = names[i.getIndex()].ix;

                wasinlookup = 1;
            }

            ++ni;
        }

        names[i.getIndex()].n = name;

        if (!wasinlookup)
        {
            //
            // update also lookup table:
            // 
            std::pair<NameType, typename names_t::const_iterator> inslk(name, i.it);
            this->lookup.insert(inslk);
        }
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

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_NAMESTABLE_H */


// Local Variables:
// mode: C++
// End:
