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

#include <vector>

#include "dlvhex/ResultContainer.h"

void
ResultContainer::addSet(AtomSet& res)
{
    sets.push_back(res);
}


void
ResultContainer::filterOut(const NamesTable<std::string>& predicates)
{
    //
    // go through all atom sets we have
    //
    for (std::vector<AtomSet>::iterator ri = sets.begin();
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
            (*ri).remove(*pi);
            /*
            AtomSet::iterator cur = (*ri).begin();

            const AtomSet::iterator last = (*ri).end();

            //
            // does this predicate occur in the atom set?
            //
            while ((cur = std::find_if(cur, last, bind2nd(predicateMatches(), *pi))) != last)
            {
                AtomSet::iterator tmp = cur++;

                (*ri).erase(tmp);
            }
            */
        }
    } 
}

void
ResultContainer::filterIn(const std::vector<std::string>& predicates)
{
    //
    // go through all atom sets we have
    //
    for (std::vector<AtomSet>::iterator ri = sets.begin();
         ri != sets.end();
         ++ri)
    {

    } 
}

void
ResultContainer::print(std::ostream& stream) const
{
    for (std::vector<AtomSet>::const_iterator ri = sets.begin();
         ri != sets.end();
         ++ri)
    {
        //
        // stringify the result set
        //
        (*ri).print(stream, 0);

        //
        // newline and empty line
        //
        stream << std::endl << std::endl;
    }
}

