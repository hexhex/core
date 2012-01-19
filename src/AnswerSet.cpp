/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
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
 * @file AnswerSet.cpp
 * @author Roman Schindlauer, Peter Schüller
 *
 * @brief AnswerSet class.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/AnswerSet.h"


DLVHEX_NAMESPACE_BEGIN

std::ostream& AnswerSet::print(std::ostream& o) const
{
  // use ", " with space here! (compatibility)
  interpretation->print(o, "{", ", ", "}");
  if( costWeight != -1 || costLevel != -1 )
    o << ",weight=" << costWeight << ",level=" << costLevel;
  return o;
}

#if 0
unsigned AnswerSet::maxLevel = 1;
unsigned AnswerSet::maxWeight = 0;


AnswerSet::AnswerSet(const std::string& wcpr)
    : WCprefix(wcpr)
{
}


void
AnswerSet::setSet(const AtomSet& atomset)
{
  // set atoms
  this->atoms = atomset.atoms;

  // check if we have a prefix for weak constraints
  if (!this->WCprefix.empty())
    {
      AtomSet wcatoms;
    
      for (AtomSet::const_iterator asit = this->atoms.begin();
	   asit != this->atoms.end();
	   ++asit)
	{
	  if (asit->getPredicate().getString().substr(0, WCprefix.length()) == WCprefix)
	    {
	      AtomPtr wca(new Atom(*asit));
	      wcatoms.insert(wca);
	    }
	}

      //
      // this answer set has no weight atoms
      //
      if (wcatoms.size() == 0)
	{
	  this->weights.push_back(0);
	}
      else
	{
	  for (AtomSet::const_iterator asit = wcatoms.begin(); asit != wcatoms.end(); ++asit)
	    {
	      Tuple args = asit->getArguments();
	      
	      Term tlevel(*(args.end() - 1));
	      Term tweight(*(args.end() - 2));
	      
	      if (!tlevel.isInt())
		throw GeneralError("Weak constraint level instantiated with non-integer!");

	      unsigned l = tlevel.getInt();
	      
	      if (!tweight.isInt())
		throw GeneralError("Weak constraint weight instantiated with non-integer!");
	      
	      unsigned w = tweight.getInt();
	      
	      this->addWeight(w, l);
	    }
	}
    }
}


bool
AnswerSet::hasWeights() const
{
  return !this->WCprefix.empty();
}


unsigned
AnswerSet::getWeightLevels() const
{
  return this->weights.size();
}


void
AnswerSet::addWeight(unsigned weight,
                     unsigned level)
{
    assert(level > 0);

    setMaxLevelWeight(level, weight);

    //
    // create new levels if necessary
    //
    if (this->weights.size() < level)
    {
        for (unsigned ws = this->weights.size(); ws < level; ++ws)
            this->weights.push_back(0);
    }

    //
    // levels start from one, the std::vector starts from 0, hence level - 1
    //
    this->weights[level - 1] += weight;
}


unsigned
AnswerSet::getWeight(unsigned level) const
{
    //
    // unspecified levels have weight 0
    //
    if (level > this->weights.size())
        return 0;

    return this->weights.at(level - 1);
}


bool
AnswerSet::cheaperThan(const AnswerSet& answerset2) const
{
    if (WCprefix.empty())
        return 0;

    int ret = 0;

    unsigned maxlevel = this->weights.size();
    
    if (answerset2.weights.size() > maxlevel)
        maxlevel = answerset2.weights.size();

    //
    // higher levels have higher priority
    //
    //for (unsigned currlevel = 1; currlevel <= maxlevel; ++currlevel)
    for (unsigned currlevel = maxlevel; currlevel >= 1; --currlevel)
    {
        if (this->getWeight(currlevel) < answerset2.getWeight(currlevel))
        {
            ret = !Globals::Instance()->getOption("ReverseOrder");
            break;
        }

        if (this->getWeight(currlevel) > answerset2.getWeight(currlevel))
		{
            ret = Globals::Instance()->getOption("ReverseOrder");
            break;
		}
    }

    return ret;
}


bool
AnswerSet::moreExpensiveThan(const weights_t& weights) const
{
    if (WCprefix.empty())
        return 0;

    int ret = 0;
    //int ret = Globals::Instance()->getOption("ReverseOrder");

    unsigned maxlevel = this->weights.size();
    
    //
    // find out the maximum of both levels
    //
    if (weights.size() > maxlevel)
        maxlevel = weights.size();

    //
    // go through all levels
    //
    //for (unsigned currlevel = 1; currlevel <= maxlevel; ++currlevel)
    for (unsigned currlevel = maxlevel; currlevel >= 1; --currlevel)
    {
        unsigned w = 0;

        //
        // only take weight from existing levels of the specified weight
        // vector - otherwise w is still 0 (nonspecified levels have 0 weight)
        //
        if (currlevel <= weights.size())
            w = weights.at(currlevel - 1);

        //
        // compare with weight from *this - getWeight ensures that we don't read
        // from unspecified levels (see getWeight)
        // if *this weighs more than the specified vector, it is more expensive,
        // return 1
        //
        if (this->getWeight(currlevel) > w)
        {
            ret = !Globals::Instance()->getOption("ReverseOrder");
            break;
        }

        //
        // if this weighs less than the specified vector, it is cheaper, return
        // 0
        //
        if (this->getWeight(currlevel) < w)
		{
            ret = Globals::Instance()->getOption("ReverseOrder");
            break;
		}
    }

    //
    // if weights at all levels were equal, *this is not more expensive than the
    // specified vector; ret is still at 0
    //

    return ret;
}


int
AnswerSet::operator< (const AnswerSet& answerset2) const
{
    //
    // with weak constraints, we order the AnswerSets according to their
    // weights
    //
    if (!WCprefix.empty())
    {
		bool isCheaper(1);

		//
		// if we use reverse order, we simply toggle the return value:
		//
		//if (Globals::Instance()->getOption("ReverseOrder"))
		//	isCheaper = 0;

        if (this->cheaperThan(answerset2))
            return isCheaper;

        if (answerset2.cheaperThan(*this))
            return !isCheaper;
    }

    //
    // if the answersets have equal costs in weak constraint-mode, we have to
    // check for normal equality, otherwise two equal-cost answer sets would be
    // already considered as equal and only one of them could be inserted in a
    // container<AnswerSet>!
    //

    //
    // in normal mode, we use the AtomSet::< comparison
    //
    return AtomSet::operator< (answerset2);
}


void
AnswerSet::setMaxLevelWeight(unsigned l, unsigned w)
{
    if (l > maxLevel)
        maxLevel = l;
    if (w > maxWeight)
        maxWeight = w;
}


unsigned
AnswerSet::getMaxLevel()
{
    return maxLevel;
}


std::ostream&
operator<< (std::ostream& out, const AnswerSet& atomset)
{
    ///@todo maybe we could add the weight handling to a visiting method
    RawPrintVisitor rpv(out);
    const_cast<AnswerSet*>(&atomset)->accept(rpv);
    return out;
}
#endif


DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
