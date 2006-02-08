/* -*- C++ -*- */

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
//#include "dlvhex/AtomFactory.h"


/**
 * @brief An AtomSet is a set of Atoms.
 */
class AtomSet
{
public:

    /**
     * @brief We actually use pointers here.
     *
     * @todo Using some kind of artificial reference would make it easier to use
     * std::set in the AtomFactory, saving space for big atomsets with a lot of
     * duplicates. On the other hand, indexing of atoms would probably take
     * longer than though a normal pointer.
     */
    typedef std::set<Atom*> atomset_t;

    /**
     * @brief Iterator to traverse the atomset.
     */
    class const_iterator
    {
        atomset_t::const_iterator it;

    public:

        const_iterator()
        {
            //assert(0);
        }

        const_iterator(const atomset_t::const_iterator &it1)
            : it(it1)
        { }

        const Atom&
        operator *() const
        {
            return *(*it);
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
     * @brief Insert an atom from the specified address.
     */
    void
    insert(Atom*);

    /**
     * @brief Insert an Atom.
     */
    void
    insert(Atom);

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
     * @brief Prints the atomset to the specified stream.
     */
    void
    print(std::ostream&, const bool) const;

    /**
     * @brief Removes all Atoms from the atomset whose predicate match the specified string.
     */
    void
    remove(const std::string&);

    /**
     * @brief Keep only thos Atoms in the atomset whose predicates are contained
     * in the specified vector of strings.
     */
    void
    keep(const std::vector<std::string>&);

    bool
    operator== (const AtomSet&) const;

    bool
    operator!= (const AtomSet&) const;

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
