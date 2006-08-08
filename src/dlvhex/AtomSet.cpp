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
            //
            // make union of two atomsets:
            //
            un = *i1;
            un.insert(*i2);

            //std::cout << "inserted: " << un << std::endl;

            //
            // now ensure minimality:
            //
            bool add = true;

            std::vector<AtomSet>::iterator curras = tmpset.begin();

            while (curras != tmpset.end())
            {
                //
                // is the new one a superset (or equal) than an existing one
                //
                if (std::includes(un.begin(), un.end(), (*curras).begin(), (*curras).end()))
                {
                    add = false;
                    break;
                }

                //
                // is the new one a subset of an existing one? Must be a *real* subset,
                // if we passed the previous "if"!
                //
                if (std::includes((*curras).begin(), (*curras).end(), un.begin(), un.end()))
                {
                    //
                    // remove existing one
                    //
                    tmpset.erase(curras);
                    break;
                }

                curras++;
            }

            if (add)
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



void
AtomSet::insert(AtomPtr& ap)
{
    /// @todo test if *ap really exists

    atoms.insert(ap);
}



void
AtomSet::insert(const AtomSet& add)
{
    atoms.insert(add.atoms.begin(), add.atoms.end());
}


AtomSet
AtomSet::difference(AtomSet& as) const
{
    AtomSet res;

//    std::set_difference(atoms.begin(), atoms.end(),
//                        as.atoms.begin(), as.atoms.end(),
//                        res.atoms);
    
    /// @todo: stdlib algorithm!
    for (atomset_t::const_iterator a = atoms.begin();
         a != atoms.end();
         a++)
    {
        if (as.atoms.find(*a) == as.atoms.end())
            res.atoms.insert(*a);
    }
    return res;
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
AtomSet::matchAtom(const AtomPtr& atom,
                   AtomSet& matched) const
{
    /// @todo: stdlib algorithm!
    for (atomset_t::const_iterator a = atoms.begin();
         a != atoms.end();
         a++)
    {
        if ((*a)->unifiesWith(atom))
            matched.atoms.insert(*a);
    }
}


std::ostream&
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

    return stream;
}


struct predicateMatches : public std::binary_function<AtomPtr, std::string, bool>
{
    bool operator()(const AtomPtr g, const std::string& pred) const
    {
        return (g->getPredicate() == Term(pred));
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
AtomSet::remove(const std::vector<std::string>& preds)
{
    for (std::vector<std::string>::const_iterator predit = preds.begin();
         predit != preds.end();
         ++predit)
    {
        remove(*predit);
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
    return ((this->size() == atomset2.size()) 
      && std::equal(this->begin(), this->end(), atomset2.begin()));
}


bool
AtomSet::operator!= (const AtomSet& atomset2) const
{
    return !(*this == atomset2);
}


int
AtomSet::operator< (const AtomSet& atomset2) const
{
    if (this->size() < atomset2.size())
        return true;

    if (this->size() > atomset2.size())
        return false;

    //return !(std::includes(this->begin(), this->end(), atomset2.begin(), atomset2.end()));

    // find first mismatch
    std::pair<AtomSet::const_iterator, AtomSet::const_iterator> result;
    result = std::mismatch(this->begin(), this->end(), atomset2.begin());

    //
    // no mismatch? then they are equal!
    //
    if (result.first == this->end())
        return false;

    if (*(result.first) < *(result.second))
        return true;

    return false;
}


