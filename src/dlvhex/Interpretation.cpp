/* -*- C++ -*- */

/**
 * @file Interpretation.cpp
 * @author Roman Schindlauer
 * @date Mon Sep  5 17:09:33 CEST 2005
 *
 * @brief Interpretation class.
 *
 *
 */


#include "dlvhex/Interpretation.h"
#include "dlvhex/Atom.h"

 
Interpretation::Interpretation()
{ }


Interpretation::Interpretation(const GAtomSet& facts)
    : positive(facts)
{
}


int
Interpretation::operator== (const Interpretation& i2)
{
    return (positive == i2.positive);
}


int
Interpretation::operator!= (const Interpretation& i2)
{
    return !(positive == i2.positive);
}


void
Interpretation::clear()
{
    positive.clear();
}


void
Interpretation::add(const GAtomSet& atomset)
{
    positive.insert(atomset.begin(), atomset.end());
}


void
Interpretation::replaceBy(const GAtomSet& atomset)
{
    positive = atomset;
}


const GAtomSet&
Interpretation::getAtomSet() const
{
    return positive;
}


void
Interpretation::matchPredicate(const std::string pred, GAtomSet& atomset) const
{
    for (GAtomSet::const_iterator a = positive.begin(); a != positive.end(); a++)
    {
        if (a->getPredicate() == pred)
            atomset.insert(*a);
    }
}

const GAtomSet::const_iterator
Interpretation::begin() const
{
    return positive.begin();
}


const GAtomSet::const_iterator
Interpretation::end() const
{
    return positive.end();
}



