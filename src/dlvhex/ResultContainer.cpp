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



ResultContainer::ResultContainer(std::string wcpr)
    : wcprefix(wcpr)
{
}

void
ResultContainer::addSet(AtomSet& res)
{
    AnswerSetPtr as(new AnswerSet(wcprefix));

    as->setSet(res);

    sets.insert(as);

    //
    // update lowest-cost-vector
    //
    if (!wcprefix.empty())
    {
        //
        // all levels of new answer set
        //
        unsigned maxlevels = as->getWeightLevels();

        //
        // look through all these levels
        //
        for (unsigned i = 1; i <= maxlevels; ++i)
        {
            //
            // if we didn't have the current
            // level in the lowest-cost-vector, then
            // add it.
            //
            if (i > this->lowestWeights.size())
                this->lowestWeights.push_back(as->getWeight(i));
            else
            {
                //
                // otherwise update the exisitng level if the new costs are
                // lower
                //
                if (as->getWeight(i) < (this->lowestWeights[i - 1]))
                this->lowestWeights[i - 1] = as->getWeight(i);
            }
        }
    }
}


void
ResultContainer::filterOut(const NamesTable<std::string>& predicates)
{
    //
    // go through all atom sets we have
    //
    for (result_t::iterator ri = sets.begin();
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
            (*ri)->remove(*pi);
        }
    } 
}


void
ResultContainer::filterOutDLT()
{
    //
    // go through all atom sets we have
    //
    for (result_t::iterator ri = sets.begin();
         ri != sets.end();
         ++ri)
    {
        std::vector<std::string> toremove;

        for (AnswerSet::const_iterator ai = (*ri)->begin();
             ai != (*ri)->end();
             ++ai)
        {
            if ((*ai).getPredicate().isString())
                continue;

            std::string pred = (*ai).getPredicate().getString();

            if (pred.find('_') != std::string::npos)
            {
                std::istringstream is(pred.erase(0, pred.find('_') + 1));
                int r;
                if (!(is >> r).fail())
                    toremove.push_back((*ai).getPredicate().getString());
            }
        }

        (*ri)->remove(toremove);
    } 
}


void
ResultContainer::filterIn(const std::vector<std::string>& predicates)
{
    //
    // go through all atom sets we have
    //
    for (result_t::iterator ri = sets.begin();
         ri != sets.end();
         ++ri)
    {
        (*ri)->keep(predicates);
    } 
}


void
ResultContainer::print(std::ostream& stream, OutputBuilder* builder) const
{
    builder->buildPre();

    for (result_t::const_iterator ri = sets.begin();
            ri != sets.end();
            ++ri)
    {
        if (!wcprefix.empty())
        {
            //
            // if we are in weak constraint-mode, we stop the output after
            // the best model(s)
            //
            /*
            for (AnswerSet::weights_t::const_iterator wi = lowestWeights.begin();
                 wi != lowestWeights.end();
                 ++wi)
                std::cout << " w: " << *wi;
            */

            if ((*ri)->moreExpensiveThan(this->lowestWeights))
                break;
        }

        //
        // stringify the result set
        //
        builder->buildAnswerSet(**ri);
    }

    builder->buildPost();
        
    stream << builder->getString();
}

