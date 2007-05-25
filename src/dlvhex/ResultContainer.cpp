/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

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
#include "dlvhex/globals.h"



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
    if (predicates.size() == 0)
        return;

    //
    // go through all atom sets we have
    //
    for (result_t::iterator ri = sets.begin();
         ri != sets.end();
         ++ri)
    {
        (*ri)->keep(predicates);
        (*ri)->keepPos();
    } 
}


void
ResultContainer::print(std::ostream& stream, OutputBuilder* builder) const
{
    builder->buildPre();

    AnswerSet::weights_t lowestWeights;

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

            //
            // record the weight of the first model - the set is already
            // ordered, so this must be the best one.
            //
            if (ri == sets.begin())
                for (unsigned i = 0; i < AnswerSet::getMaxLevel(); ++i)
                    lowestWeights.push_back((*ri)->getWeight(i + 1));
            
			/*
            for (AnswerSet::weights_t::const_iterator wi = lowestWeights.begin();
                 wi != lowestWeights.end();
                 ++wi)
                std::cout << " w: " << *wi;
			*/
            

            //
            // any model that is more expensive than the best one(s) will not be
            // displayed
            //
            if (!Globals::Instance()->getOption("AllModels"))
                if ((*ri)->moreExpensiveThan(lowestWeights))
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

