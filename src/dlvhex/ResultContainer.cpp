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


#include "dlvhex/ResultContainer.h"
#include "dlvhex/Interpretation.h"


void
ResultContainer::addSet(GAtomSet& res)
{
    sets.push_back(res);
}

void
ResultContainer::filterOut(const std::vector<Term>& predicates)
{
}

void
ResultContainer::filterIn(const std::vector<Term>& predicates)
{
    /*
    Interpretation i;

    for (std::vector<GAtomSet>::iterator ri = sets.begin();
         ri != sets.end();
         ++ri)
    {
        //
        // initialize iout to the result model
        //
        i.replaceBy(*ri);
       
        GAtomSet g;
        
        //
        // take each filter predicate
        //
        for (std::vector<Term>::const_iterator pi = predicates.begin();
                pi != predicates.end();
                pi++)
        {
            //
            // extract matching facts from the current result set
            //
            i.matchPredicate((*pi).getString(), g);
            
            filtered.insert(g.begin(), g.end());
        }
    }
    */
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

