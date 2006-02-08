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
#include "dlvhex/AtomSet.h"


/**
 * @brief
 * 
 */
class ResultContainer
{
public:

    void
    addSet(AtomSet&);

    void
    filterOut(const NamesTable<std::string>&);

    void
    filterIn(const std::vector<std::string>&);

    void
    print(std::ostream&) const;

private:

    std::vector<AtomSet> sets;

};

#endif /* _RESULTCONTAINER_H */
