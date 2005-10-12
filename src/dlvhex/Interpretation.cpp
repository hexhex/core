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


Interpretation::Interpretation(const GAtomList &facts)
    : positive(facts)
{
}


void Interpretation::replaceBy(const GAtomList &atomset)
{
    positive = atomset;
}

unsigned Interpretation::getSize() const
{
    return positive.size();
}

const GAtomList* Interpretation::getPositive() const
{
    return &positive;
}

void Interpretation::addPositive(const GAtom &gatom)
{
    GAtomList::iterator a = find(positive.begin(), positive.end(), gatom);

    //
    // add only new atoms
    //
    if (a == positive.end())
        positive.push_back(gatom);
}

void Interpretation::addPositive(const GAtomList &gatomset)
{
    for (GAtomList::const_iterator a = gatomset.begin(); a != gatomset.end(); a++)
        addPositive(*a);
}

bool Interpretation::isTrue(const GAtom &gatom)
{
    GAtomList::iterator a = find(positive.begin(), positive.end(), gatom);

    return (a != positive.end());
}

void Interpretation::matchAtom(const Atom atom, GAtomList &atomset) const
{
    for (GAtomList::const_iterator a = positive.begin(); a != positive.end(); a++)
    {
        if (a->unifiesWith(atom))
            atomset.push_back(*a);
    }
}

void Interpretation::matchPredicate(const std::string pred, GAtomList &atomset) const
{
    for (GAtomList::const_iterator a = positive.begin(); a != positive.end(); a++)
    {
        if (a->getPredicate() == pred)
            atomset.push_back(*a);
    }
}

void Interpretation::removePredicate(const std::string pred)
{
    //
    // attn: erase destroys the iterator, but also returns the next
    // element!
    //
    for (GAtomList::iterator a = positive.begin(); a != positive.end(); )
    {
        if (a->getPredicate() == pred)
            a = positive.erase(a);
        else
            a++;
    }
}

//#include <sstream>

/*
std::string Interpretation::printFacts() const
{
    ostringstream facts;
    
    for (GAtomList::const_iterator a = positive.begin(); a != positive.end(); a++)
    {
        facts << *a << ".\n";
    }
    
    return facts.str();
}
*/


std::ostream&
Interpretation::printSet(std::ostream& stream, const bool ho) const
{
    stream << "{";

    for (GAtomList::const_iterator a = positive.begin(); a != positive.end(); a++)
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



