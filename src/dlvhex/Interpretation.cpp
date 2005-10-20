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


Interpretation::Interpretation(const GAtomSet &facts)
    : positive(facts)
{
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
Interpretation::replaceBy(const GAtomSet &atomset)
{
    positive = atomset;
}


GAtomSet*
Interpretation::getAtomSet()
{
    return &positive;
}


void
Interpretation::matchPredicate(const std::string pred, GAtomSet &atomset) const
{
    for (GAtomSet::const_iterator a = positive.begin(); a != positive.end(); a++)
    {
        if (a->getPredicate() == pred)
            atomset.insert(*a);
    }
}

GAtomSet::const_iterator
Interpretation::begin() const
{
    return positive.begin();
}


GAtomSet::const_iterator
Interpretation::end() const
{
    return positive.end();
}


/*
unsigned Interpretation::getSize() const
{
    return positive.size();
}

const GAtomSet* Interpretation::getPositive() const
{
    return &positive;
}

void Interpretation::addPositive(const GAtom &gatom)
{
    GAtomSet::iterator a = find(positive.begin(), positive.end(), gatom);

    //
    // add only new atoms
    //
    if (a == positive.end())
        positive.push_back(gatom);
}

void Interpretation::addPositive(const GAtomSet &gatomset)
{
    for (GAtomSet::const_iterator a = gatomset.begin(); a != gatomset.end(); a++)
        addPositive(*a);
}

bool Interpretation::isTrue(const GAtom &gatom)
{
    GAtomSet::iterator a = find(positive.begin(), positive.end(), gatom);

    return (a != positive.end());
}

void Interpretation::matchAtom(const Atom atom, GAtomSet &atomset) const
{
    for (GAtomSet::const_iterator a = positive.begin(); a != positive.end(); a++)
    {
        if (a->unifiesWith(atom))
            atomset.push_back(*a);
    }
}

void Interpretation::removePredicate(const std::string pred)
{
    //
    // attn: erase destroys the iterator, but also returns the next
    // element!
    //
    for (GAtomSet::iterator a = positive.begin(); a != positive.end(); )
    {
        if (a->getPredicate() == pred)
            a = positive.erase(a);
        else
            a++;
    }
}
*/
//#include <sstream>

/*
std::string Interpretation::printFacts() const
{
    ostringstream facts;
    
    for (GAtomSet::const_iterator a = positive.begin(); a != positive.end(); a++)
    {
        facts << *a << ".\n";
    }
    
    return facts.str();
}
*/

/*
std::ostream&
Interpretation::printSet(std::ostream& stream, const bool ho) const
{
    stream << "{";

    for (GAtomSet::const_iterator a = positive.begin(); a != positive.end(); a++)
    {
        if (a != positive.begin())
            stream << ", ";

        (*a).print(stream, ho);
    }

    stream << "}";

    return stream;
}

std::ostream&
operator<< (std::ostream &out, const Interpretation &i)
{
    return i.printSet(out, false);
}
*/


