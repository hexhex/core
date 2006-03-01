/* -*- C++ -*- */

/**
 * @file AtomSet.cpp
 * @author Roman Schindlauer
 * @date Tue Feb  7 17:19:18 CET 2006
 *
 * @brief AtomSet class.
 *
 *
 */

#include <vector>
#include <algorithm>

#include "dlvhex/AtomSet.h"
#include "dlvhex/AtomFactory.h"


void
multiplySets(std::vector<AtomSet>& s1,
             std::vector<AtomSet>& s2,
             std::vector<AtomSet>& result)
{
    //
    // write result into temporary vector. in case the result parameter
    // is one of the two input vectors
    //
    std::vector<AtomSet> tmpset;

    AtomSet un;

    for (std::vector<AtomSet>::iterator i1 = s1.begin();
            i1 != s1.end();
            ++i1)
    {
        for (std::vector<AtomSet>::iterator i2 = s2.begin();
                i2 != s2.end();
                ++i2)
        {
            un = *i1;
            un.insert(*i2);
            //std::cout << "inserted: " << un << std::endl;

            tmpset.push_back(un);
        }
    }

    //
    // now we can write the result
    //
    swap(result, tmpset);
}


AtomSet::const_iterator
AtomSet::begin() const
{
    return const_iterator(atoms.begin());
}


AtomSet::const_iterator
AtomSet::end() const
{
    return const_iterator(atoms.end());
}


/*
void
AtomSet::initialize(const GAtomSet& in)
{
    for (GAtomSet::const_iterator i = in.begin();
         i != in.end();
         ++i)
    {
        insert(*i);
    }
}
*/


void
AtomSet::clear()
{
    atoms.clear();
}


bool
AtomSet::empty() const
{
    return atoms.empty();
}


size_t
AtomSet::size() const
{
    return atoms.size();
}


/*
void
AtomSet::insert(Atom* a)
{
    insert(*a);
}
*/


void
AtomSet::insert(AtomPtr& ap)
{
    /// @todo test if *ap really exists

    //
    // inserting the AtomPtr in the factory ensures that 
//    AtomFactory::Instance()->insert(ap);

    atoms.insert(ap);
}



/*
void
AtomSet::insert(Atom& a)
{
    atoms.insert(&a);
}
*/


/*
void
AtomSet::insert(Atom a)
{
    AtomFactory::AtomPtr pa = AtomFactory::Instance()->insert(a);

    atoms.insert(pa);
}
*/


void
AtomSet::insert(const AtomSet& add)
{
    atoms.insert(add.atoms.begin(), add.atoms.end());
}


void
AtomSet::matchPredicate(const std::string pred,
                        AtomSet& matched) const
{
    /// @todo: stdlib algorithm!
    for (atomset_t::const_iterator a = atoms.begin();
         a != atoms.end();
         a++)
    {
        if ((*a)->getPredicate() == pred)
            matched.atoms.insert(*a);
    }
}


void
AtomSet::print(std::ostream& stream, const bool ho) const
{
    stream << "{";

    for (atomset_t::const_iterator a = atoms.begin();
         a != atoms.end();
         a++)
    {
        if (a != atoms.begin())
            stream << ", ";

        (*a)->print(stream, ho);
    }

    stream << "}";
}


struct predicateMatches : public std::binary_function<AtomPtr, Term, bool>
{
    bool operator()(const AtomPtr g, const Term& pred) const
    {
        return (g->getPredicate() == pred);
    }
};


void
AtomSet::remove(const std::string& pred)
{
    atomset_t::iterator cur = atoms.begin();

    atomset_t::const_iterator last = atoms.end();

    while ((cur = std::find_if(cur, last, bind2nd(predicateMatches(), pred))) != last)
    {
        atomset_t::iterator tmp = cur++;

        atoms.erase(tmp);
    }
}


void
AtomSet::keep(const std::vector<std::string>& preds)
{
    atomset_t::iterator cur = atoms.begin();

    atomset_t::const_iterator last = atoms.end();

    //
    // go through all atoms of this set
    //
    while (cur != last)
    {
        //
        // look if the current atom is in the filter set
        //
        if ((std::find_if(preds.begin(),
                          preds.end(),
                          bind1st(predicateMatches(), *cur))) == preds.end())
        {
            //
            // if not, delete this atom
            //
            atomset_t::iterator tmp = cur++;

            atoms.erase(tmp);
        }
        else
        {
            cur++;
        }
    }
}


void
AtomSet::keepPos()
{
    atomset_t::iterator cur = atoms.begin();

    atomset_t::const_iterator last = atoms.end();

    //
    // go through all atoms of this set
    //
    while (cur != last)
    {
        if ((*cur)->isStronglyNegated())
        {
            atomset_t::iterator tmp = cur++;

            atoms.erase(tmp);
        }
        else
        {
            cur++;
        }
    }
}


bool
AtomSet::operator== (const AtomSet& atomset2) const
{
    return this->size() == atomset2.size()
      && std::equal(this->begin(), this->end(), atomset2.begin());
}


bool
AtomSet::operator!= (const AtomSet& atomset2) const
{
    return !(*this == atomset2);
}
