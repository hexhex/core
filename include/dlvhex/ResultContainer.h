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
#include "dlvhex/AnswerSet.h"
#include "dlvhex/OutputBuilder.h"


/**
 * @brief
 * 
 */
class ResultContainer
{
public:

    /**
     * @brief Custom compare operator for AnswerSets.
     */
    struct AnswerSetPtrCompare
    {
        bool 
        operator() (const AnswerSetPtr& a, const AnswerSetPtr& b)
        {
            return *a < *b;
        }
    };

    typedef std::set<AnswerSetPtr, ResultContainer::AnswerSetPtrCompare> result_t;

    /**
     * @brief Constructor.
     *
     * If a string is passed to the constructor, weak constraint-mode is
     * switched on. The string then identifies auxiliary predicates in each
     * answer set that determine the set's cost.
     */
    ResultContainer(std::string = "");

    void
    addSet(AtomSet&);

    void
    filterOut(const NamesTable<std::string>&);

    void
    filterIn(const std::vector<std::string>&);

    void
    print(std::ostream&, OutputBuilder*) const;

private:

    result_t sets;

    std::string wcprefix;

    AnswerSet::weights_t lowestWeights;
};

#endif /* _RESULTCONTAINER_H */
