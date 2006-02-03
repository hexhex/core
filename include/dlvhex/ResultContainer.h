/* -*- C++ -*- */

/**
 * @file ResultContainer.h
 * @author Roman Schindlauer
 * @date Fri Feb  3 15:51:37 CET 2006
 *
 * @brief ResultContainer class
 *
 *
 */


#ifndef _RESULTCONTAINER_H
#define _RESULTCONTAINER_H


#include <iostream>
#include <vector>

#include "dlvhex/Atom.h"


/**
 * @brief
 * 
 */
class ResultContainer
{
public:

    void
    addSet(GAtomSet&);

    void
    filterOut(const std::vector<Term>&);

    void
    filterIn(const std::vector<Term>&);

    void
    print(std::ostream&) const;

private:

    std::vector<GAtomSet> sets;

};

#endif /* _RESULTCONTAINER_H */
