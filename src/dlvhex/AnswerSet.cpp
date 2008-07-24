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
 * @file AnswerSet.cpp
 * @author Roman Schindlauer
 * @date Wed May  3 13:28:37 CEST 2006
 *
 * @brief AnswerSet class.
 *
 *
 */

#include "dlvhex/AnswerSet.h"
#include "dlvhex/PrintVisitor.h"
#include "dlvhex/globals.h"
#include "dlvhex/Error.h"
#include "dlvhex/PredefinedNames.h"

DLVHEX_NAMESPACE_BEGIN


AnswerSet::AnswerSet(bool wp)
  : weakProcessing(wp),
    maxLevel(1),
    maxWeight(0)
{ }


const AtomSet&
AnswerSet::getAtomSet() const
{
  return atoms;
}


void
AnswerSet::setAtomSet(const AtomSet& atomset)
{
  // set atoms
  this->atoms = atomset;

  // check if we have a prefix for weak constraints
  if (this->weakProcessing)
    {
      AtomSet wcatoms;
    
      // copy all atoms with a weak predicte to wcatoms
      for (AtomSet::const_iterator asit = this->atoms.begin();
	   asit != this->atoms.end();
	   ++asit)
	{
	  const Term& pred = (*asit)->getPredicate();
	  if (pred.isSymbol() || pred.isString())
	    {
	      const std::string& p = pred.getString();
	      if (p.find(PredefinedNames::WEAKHEAD) == 0)
		{
		  wcatoms.insert(*asit);
		}
	    }
	}

      //
      // this answer set has no weight atoms
      //
      if (wcatoms.empty())
	{
	  this->weights.push_back(0);
	}
      else
	{
	  for (AtomSet::const_iterator asit = wcatoms.begin();
	       asit != wcatoms.end(); ++asit)
	    {
	      unsigned n = (*asit)->getArity();

	      const Term& tlevel = (**asit)[n];
	      const Term& tweight = (**asit)[n-1]; 
	      
	      assert(tlevel.isInt());
	      assert(tweight.isInt());

	      unsigned l = tlevel.getInt();
	      unsigned w = tweight.getInt();
	      
	      this->addWeight(w, l);
	    }
	}
    }
}


bool
AnswerSet::hasWeights() const
{
  return weakProcessing;
}


unsigned
AnswerSet::getWeightLevels() const
{
  return this->weights.size();
}


void
AnswerSet::addWeight(unsigned level, unsigned weight)
{
  assert(level > 0);

  setMaxLevelWeight(level, weight);

  // create new levels if necessary, and initialize them with 0
  weights.resize(level, 0);

  // levels start from one, the std::vector starts from 0, hence level - 1
  weights[level - 1] += weight;
}


unsigned
AnswerSet::getWeight(unsigned level) const
{
  // unspecified levels have weight 0
  if (level > this->weights.size())
    {
      return 0;
    }

  return this->weights[level - 1];
}


bool
AnswerSet::cheaperThan(const AnswerSet& answerset2) const
{
  bool ret = false;
      
  if (weakProcessing)
    {
      unsigned maxlevel = this->weights.size();
    
      if (answerset2.weights.size() > maxlevel)
	{
	  maxlevel = answerset2.weights.size();
	}

      //
      // higher levels have higher priority
      //
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
    }

  return ret;
}


bool
AnswerSet::moreExpensiveThan(const LevelWeight& weights2) const
{
  bool ret = false;

  if (weakProcessing)
    {
      unsigned maxlevel = this->weights.size();
    
      // find out the maximum of both levels
      if (weights2.size() > maxlevel)
	{
	  maxlevel = weights2.size();
	}

      //
      // go through all levels
      //
      for (unsigned currlevel = maxlevel; currlevel >= 1; --currlevel)
	{
	  unsigned w = 0;
	  
	  //
	  // only take weight from existing levels of the specified
	  // weight vector - otherwise w is still 0 (nonspecified
	  // levels have 0 weight)
	  //
	  if (currlevel <= weights2.size())
	    {
	      w = weights2.at(currlevel - 1);
	    }

	  //
	  // compare with weight from *this - getWeight ensures that
	  // we don't read from unspecified levels (see getWeight) if
	  // *this weighs more than the specified vector, it is more
	  // expensive, return 1
	  //
	  if (this->getWeight(currlevel) > w)
	    {
	      ret = !Globals::Instance()->getOption("ReverseOrder");
	      break;
	    }

	  //
	  // if this weighs less than the specified vector, it is
	  // cheaper, return 0
	  //
	  if (this->getWeight(currlevel) < w)
	    {
	      ret = Globals::Instance()->getOption("ReverseOrder");
	      break;
	    }
	}
    }

  //
  // if weights at all levels were equal, *this is not more expensive
  // than the specified vector; ret is still at 0
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
  if (weakProcessing)
    {
      //
      // if we use reverse order, we simply toggle the return value:
      //
      //if (Globals::Instance()->getOption("ReverseOrder"))
      //	isCheaper = 0;

      if (this->cheaperThan(answerset2))
	{
	  return true;
	}
      else if (answerset2.cheaperThan(*this))
	{
	  return false;
	}
    }

  //
  // if the answersets have equal costs in weak constraint-mode, we
  // have to check for normal equality, otherwise two equal-cost
  // answer sets would be already considered as equal and only one of
  // them could be inserted in a container<AnswerSet>!
  //

  return this->atoms < answerset2.atoms;
}


void
AnswerSet::setMaxLevelWeight(unsigned l, unsigned w)
{
  maxLevel = std::max(l, maxLevel);
  maxWeight = std::max(w, maxWeight);
}


unsigned
AnswerSet::getMaxLevel() const
{
  return maxLevel;
}


std::ostream&
operator<< (std::ostream& out, const AnswerSet& atomset)
{
  ///@todo maybe we could add the weight handling to a visiting method
  RawPrintVisitor rpv(out);
  rpv << atomset.getAtomSet();
  return out;
}


DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
