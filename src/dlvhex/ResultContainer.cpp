/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with dlvhex; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */


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
#include "dlvhex/globals.h"
#include "dlvhex/OutputBuilder.h"
#include "dlvhex/AtomSetFunctions.h"

#include <vector>
#include <iostream>
#include <sstream>

DLVHEX_NAMESPACE_BEGIN

ResultContainer::ResultContainer(const std::string& wcpr)
    : wcprefix(wcpr)
{
}

void
ResultContainer::addSet(const AtomSet& res)
{
  ///@todo we can do better!
  AnswerSetPtr as(new AnswerSet(true));
  as->setAtomSet(res);
  sets.insert(as);
}


const ResultContainer::result_t&
ResultContainer::getAnswerSets() const
{
  return sets;
}


void
ResultContainer::filterOut(const NamesTable<std::string>& predicates)
{
  ///@todo ugly, fix this
  
  //
  // go through all atom sets we have
  //
  for (result_t::iterator ri = sets.begin(); ri != sets.end(); ++ri)
    {
      //
      // for this atom set: go through all predicates we want to remove
      //
      for (NamesTable<std::string>::const_iterator pi = predicates.begin();
	   pi != predicates.end();
	   ++pi)
        {
	  AtomSet as = removePredicate((*ri)->getAtomSet(), Term(*pi));
	  (*ri)->setAtomSet(as);
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
      
      const AtomSet& as = (*ri)->getAtomSet();
      
      for (AtomSet::const_iterator ai = as.begin(); ai != as.end(); ++ai)
        {
	  if ((*ai)->getPredicate().isString())
	    continue;

	  std::string pred = (*ai)->getPredicate().getString();

	  if (pred.find('_') != std::string::npos)
            {
	      std::istringstream is(pred.erase(0, pred.find('_') + 1));
	      int r;
	      if (!(is >> r).fail())
		{
		  toremove.push_back((*ai)->getPredicate().getString());
		}
            }
        }

      AtomSet newas = as;

      for (std::vector<std::string>::const_iterator r = toremove.begin();
	   r != toremove.end();
	   ++r)
	{
	  newas = removePredicate(newas, Term(*r));
	}

      (*ri)->setAtomSet(newas);
    } 
}


void
ResultContainer::filterIn(const std::vector<std::string>& predicates)
{
  ///@todo ugly, fixme

  if (predicates.empty())
    {
      return;
    }

    //
    // go through all atom sets we have
    //
    for (result_t::iterator ri = sets.begin(); ri != sets.end(); ++ri)
      {
	(*ri)->setAtomSet(keepPositive(filterPredicates((*ri)->getAtomSet(),
							predicates)
				       )
			  );
      } 
}


void
ResultContainer::print(std::ostream& stream, OutputBuilder* builder) const
{
    AnswerSet::LevelWeight lowestWeights;

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
    }


    //
    // stringify the result set
    //
    builder->buildResult(stream, *this);
}


DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
