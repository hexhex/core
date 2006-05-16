/* -*- C++ -*- */

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


AnswerSet::AnswerSet(std::string wcpr)
    : WCprefix(wcpr)
{
}


void
AnswerSet::setSet(AtomSet& atomset)
{
    this->atoms = atomset.atoms;

    if (this->WCprefix.empty())
        return;

    AtomSet wcatoms;
    
    AtomSet::const_iterator asit = this->atoms.begin(), asend = this->atoms.end();
    while (asit != asend)
    {
        if ((*asit).getPredicate().getString().substr(0, WCprefix.length()) == WCprefix)
        {
            AtomPtr wca(new Atom(*asit));
            wcatoms.insert(wca);
        }

        ++asit;
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
        AtomSet::const_iterator asit = wcatoms.begin(), asend = wcatoms.end();
        while (asit != asend)
        {
            Tuple args = (*asit).getArguments();

            Term tlevel(*(args.end() - 1));
            Term tweight(*(args.end() - 2));

            unsigned l = tlevel.getInt();
            unsigned w = tweight.getInt();

            addWeight(w, l);

            ++asit;
        }
    }
}


bool
AnswerSet::hasWeights() const
{
    return (!this->WCprefix.empty());
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

    for (unsigned currlevel = 1; currlevel <= maxlevel; ++currlevel)
    {
        if (this->getWeight(currlevel) < answerset2.getWeight(currlevel))
        {
            ret = 1;
            break;
        }

        if (this->getWeight(currlevel) > answerset2.getWeight(currlevel))
            break;
    }

    return ret;
}


bool
AnswerSet::moreExpensiveThan(const weights_t weights) const
{
    if (WCprefix.empty())
        return 0;

    int ret = 0;

    unsigned maxlevel = this->weights.size();
    
    //
    // find out the maximum of both levels
    //
    if (weights.size() > maxlevel)
        maxlevel = weights.size();

    //
    // go through all levels
    //
    for (unsigned currlevel = 1; currlevel <= maxlevel; ++currlevel)
    {
        unsigned w = 0;

        //
        // only take weight from exisitng levels of the specified weight
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
            ret = 1;
            break;
        }

        //
        // if this weighs less than the specified vector, it is cheaper, return
        // 0
        //
        if (this->getWeight(currlevel) < w)
            break;
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
        if (this->cheaperThan(answerset2))
            return true;

        if (answerset2.cheaperThan(*this))
            return false;
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
    if (this->size() < answerset2.size())
        return true;

    if (this->size() > answerset2.size())
        return false;

    return !(std::includes(this->begin(), this->end(), answerset2.begin(), answerset2.end()));
}


std::ostream&
operator<< (std::ostream& out, const AnswerSet& atomset)
{
    return atomset.print(out, false);
}
