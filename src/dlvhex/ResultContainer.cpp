/* -*- C++ -*- */

/**
 * @file ResultContainer.cpp
 * @author Roman Schindlauer
 * @date Fri Feb  3 15:51:37 CET 2006
 *
 * @brief ResultContainer class
 *
 *
 */

#include <algorithm>
#include <functional>

#include "dlvhex/ResultContainer.h"
#include "dlvhex/Interpretation.h"


void
ResultContainer::addSet(GAtomSet& res)
{
    sets.push_back(res);
}


struct predicateMatches : public std::binary_function<GAtom, Term, bool>
{
    bool operator()(const GAtom& g, const Term& pred) const
    {
        return (g.getPredicate() == pred);
    }
};

/*
struct pMatches : public std::binary_function<GAtom, std::string, bool>
{
    bool operator()(const GAtom& g, const std::string& pred) const
    {
        return (0 == 0);
    }
};
*/

#include <vector>

void
ResultContainer::filterOut(const NamesTable<std::string>& predicates)
{
    //
    // go through all atom sets we have
    //
    for (std::vector<GAtomSet>::iterator ri = sets.begin();
         ri != sets.end();
         ++ri)
    {
        //
        // for this atom set: go through all predicates we want to remove
        //
        for (NamesTable<std::string>::const_iterator pi = predicates.begin();
             pi != predicates.end();
             ++pi)
        {
            GAtomSet::iterator cur = (*ri).begin();

            const GAtomSet::iterator last = (*ri).end();

            //
            // does this predicate occur in the atom set?
            //
            while ((cur = std::find_if(cur, last, bind2nd(predicateMatches(), *pi))) != last)
            {
                GAtomSet::iterator tmp = cur++;

                (*ri).erase(tmp);
            }
        }
    } 
}

void
ResultContainer::filterIn(const std::vector<std::string>& predicates)
{
    //
    // go through all atom sets we have
    //
    for (std::vector<GAtomSet>::iterator ri = sets.begin();
         ri != sets.end();
         ++ri)
    {
        GAtomSet::iterator cur = (*ri).begin();

        const GAtomSet::iterator last = (*ri).end();

        //
        // go through all facts of this set
        //
        while (cur != last)
        {
            //
            // look if the current fact is in the filter set
            //
            if ((std::find_if(predicates.begin(),
                              predicates.end(),
                              bind1st(predicateMatches(), *cur))) == predicates.end())
            {
                //
                // if not, delete this facte
                //
                GAtomSet::iterator tmp = cur++;

                (*ri).erase(tmp);
            }
            else
            {
                cur++;
            }
        }

    } 
}

void
ResultContainer::print(std::ostream& stream) const
{
    for (std::vector<GAtomSet>::const_iterator ri = sets.begin();
         ri != sets.end();
         ++ri)
    {
        //
        // stringify the result set
        //
        printGAtomSet(*ri, stream, 0);

        //
        // build output stream
        //
        stream << std::endl;
    }
}

