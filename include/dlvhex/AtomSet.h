/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


/**
 * @file   AtomSet.h
 * @author Roman Schindlauer
 * @date   Tue Feb  7 17:20:32 CET 2006
 * 
 * @brief  AtomSet class.
 * 
 * 
 */

#ifndef _ATOMSET_H
#define _ATOMSET_H

#include "dlvhex/Atom.h"
#include "dlvhex/BaseVisitor.h"

/**
 * @brief An AtomSet is a set of Atoms.
 */
class AtomSet
{
public:
    /**
     * @brief Custom compare operator.
     *
     * In order to treat the internal atom storage as a set of Atoms instead of
     * a set of AtomPtr, we define a custom compare operator that dereferences
     * the AtomPtrs.
     */
    struct AtomCompare
    {
        bool 
        operator() (const AtomPtr& a, const AtomPtr& b)
        {
            return *a < *b;
        }
    };

    /**
     * @brief Internal atom storage.
     *
     * The atom storage is a set of AtomPtrs, using std::set with a custom
     * compare operator that dereferences the AtomPtrs. This ensures that not
     * the pointers are uniquely inserted, but the Atoms themselves
     * (std::set::insert() uses the compare operator for determining element
     * existence).
     */
    typedef std::set<AtomPtr, AtomSet::AtomCompare> atomset_t;

public:

    /**
     * @brief Iterator to traverse the atomset.
     */
    class const_iterator : public std::iterator<atomset_t::const_iterator::iterator_category,
                                                Atom,
                                                atomset_t::difference_type>
    {
        atomset_t::const_iterator it;

    public:

        const_iterator()
        { }

        const_iterator(const atomset_t::const_iterator &it1)
            : it(it1)
        { }

        const Atom&
        operator *() const
        {
            return *(*it);
        }

        const Atom*
        operator ->() const
        {
            return &(operator*());
        }

        const_iterator&
        operator ++()
        {
            it++;

            return *this;
        }

        const_iterator
        operator ++(int)
        {
            const_iterator tmp = *this;

            ++*this;

            return tmp;
        }

        const_iterator&
        operator --()
        {
            it--;

            return *this;
        }

        const_iterator
        operator --(int)
        {
            const_iterator tmp = *this;

            --*this;

            return tmp;
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

    const_iterator
    begin() const;

    const_iterator
    end() const;

    /**
     * @brief Load the atomset with a given set of atoms.
     */
//    void
//    initialize(const GAtomSet&);

    /**
     * @brief Empties the atomset.
     */
    void
    clear();

    /**
     * @brief Returns true if the atomset is empty, false otherwise.
     */
    bool
    empty() const;

    /**
     * @brief Returns number of atoms contained in the AtomSet.
     */
    size_t
    size() const;

    /**
     * @brief Insert an atom pointer.
     */
    void
    insert(AtomPtr&);

    /**
     * @brief Insert an atom from the specified address.
     */
//    void
//    insert(Atom*);

    /**
     * @brief Insert an Atom.
     */
//    void
//    insert(Atom);

    /**
     * @brief Insert all Atoms from the specifed atomset.
     */
    void
    insert(const AtomSet&);

    /**
     * @brief Fill all atoms that match the specified predicate into the
     * specified atomset.
     */
    void
    matchPredicate(const std::string, AtomSet&) const;

    /**
     * @brief Fill all atoms that unify with the given atom into the
     * specified atomset.
     */
    void
    matchAtom(const AtomPtr&, AtomSet&) const;

    /**
     * @brief Returns the set difference: *this \ specified atomset.
     */
    AtomSet
    difference(AtomSet&) const;

    /**
     * @brief accepts a visitor.
     */
    void
    accept(BaseVisitor&) const;

    /**
     * @brief Removes all Atoms from the atomset whose predicate match the specified string.
     */
    void
    remove(const std::string&);

    void
    remove(const std::vector<std::string>&);

    /**
     * @brief Keep only those Atoms in the atomset whose predicates are contained
     * in the specified vector of strings.
     */
    void
    keep(const std::vector<std::string>&);

    /**
     * @brief Remove negative atoms from the set.
     */
    void
    keepPos();

	/**
	 * @brief Test if AtomSet is consistent.
	 */
	bool
	isConsistent() const;

    bool
    operator== (const AtomSet&) const;

    bool
    operator!= (const AtomSet&) const;

    int
    operator< (const AtomSet&) const;
    
//private:

    atomset_t atoms;
};


//
// for compatibility reasons
//
typedef AtomSet Interpretation;

//
// could this become a member function?
//
void
multiplySets(std::vector<AtomSet>& s1,
             std::vector<AtomSet>& s2,
             std::vector<AtomSet>& result);


#endif /* _ATOMSET_H */


// Local Variables:
// mode: C++
// End:
