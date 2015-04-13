/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schüller
 * Copyright (C) 2011-2015 Christoph Redl
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
 * @author Roman Schindlauer, Peter Schller
 *
 * @brief AnswerSet class.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#include "dlvhex2/AnswerSet.h"
#include "dlvhex2/Benchmarking.h"

DLVHEX_NAMESPACE_BEGIN

void AnswerSet::computeWeightVector()
{
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"AnswerSet::computeWeightVector");

    weightVector = std::vector<int>();
    weightVector.push_back(0);

    RegistryPtr reg = interpretation->getRegistry();

    // go through all atoms
    bm::bvector<>::enumerator en = interpretation->getStorage().first();
    bm::bvector<>::enumerator en_end = interpretation->getStorage().end();

    while (en < en_end) {
        ID id = reg->ogatoms.getIDByAddress(*en);
        const OrdinaryAtom oatom = reg->ogatoms.getByAddress(*en);
        if (id.isAuxiliary()) {
            if (reg->getTypeByAuxiliaryConstantSymbol(oatom.tuple[0]) == 'w') {
                // tuple[1] and tuple[2] encode weight and level
                assert (oatom.tuple[1].isIntegerTerm());
                assert (oatom.tuple[2].isIntegerTerm());

                // make sure that the weight vector is long enough
                while (weightVector.size() < oatom.tuple[2].address + 1 || weightVector.size() == 0) weightVector.push_back(0);

                weightVector[oatom.tuple[2].address] += oatom.tuple[1].address;
            }
        }
        en++;
    }
}


std::vector<int>& AnswerSet::getWeightVector()
{
    return weightVector;
}


bool AnswerSet::betterThan(std::vector<int>& cwv)
{
    return weightVector == cwv || strictlyBetterThan(cwv);
}


bool AnswerSet::strictlyBetterThan(std::vector<int>& cwv)
{
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"AnswerSet::strictlyBetterThan");

    // check if one of the vectors has cost values on higher levels
    if (weightVector.size() < cwv.size()) {
        for (uint32_t j = weightVector.size(); j < cwv.size(); ++j)
            if (cwv[j] > 0) return true;
    }
    if (cwv.size() < weightVector.size()) {
        for (uint32_t j = cwv.size(); j < weightVector.size(); ++j)
            if (weightVector[j] > 0) return false;
    }

    // compare the costs on all levels which are present in both weight vectors
    int i = (weightVector.size() < cwv.size() ? weightVector.size() : cwv.size()) - 1;
    while (i >= 0) {
        if (weightVector[i] < cwv[i]) return true;
        if (cwv[i] < weightVector[i]) return false;
        i--;
    }

    // same solution quality
    return false;
}


std::ostream& AnswerSet::printWeightVector(std::ostream& o) const
{
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"AnswerSet::printWeightVector");

    if (weightVector.size() > 0) {
        bool first = true;
        for (uint32_t level = 0; level < weightVector.size(); ++level) {
            if (weightVector[level] > 0) {
                o << (first ? " <" : ",");
                o << "[" << weightVector[level] << ":" << level << "]";
                first = false;
            }
        }
        if (!first) o << ">";
    }
    return o;
}


std::ostream& AnswerSet::print(std::ostream& o) const
{
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"AnswerSet::print");

    // use ", " with space here! (compatibility)
    interpretation->print(o, "{", ", ", "}");
    if (weightVector.size() > 0) {
        printWeightVector(o);
    }
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
    if (!this->WCprefix.empty()) {
        AtomSet wcatoms;

        for (AtomSet::const_iterator asit = this->atoms.begin();
            asit != this->atoms.end();
        ++asit) {
            if (asit->getPredicate().getString().substr(0, WCprefix.length()) == WCprefix) {
                AtomPtr wca(new Atom(*asit));
                wcatoms.insert(wca);
            }
        }

        //
        // this answer set has no weight atoms
        //
        if (wcatoms.size() == 0) {
            this->weights.push_back(0);
        }
        else {
            for (AtomSet::const_iterator asit = wcatoms.begin(); asit != wcatoms.end(); ++asit) {
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
    if (this->weights.size() < level) {
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
    for (unsigned currlevel = maxlevel; currlevel >= 1; --currlevel) {
        if (this->getWeight(currlevel) < answerset2.getWeight(currlevel)) {
            ret = !Globals::Instance()->getOption("ReverseOrder");
            break;
        }

        if (this->getWeight(currlevel) > answerset2.getWeight(currlevel)) {
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
    for (unsigned currlevel = maxlevel; currlevel >= 1; --currlevel) {
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
        if (this->getWeight(currlevel) > w) {
            ret = !Globals::Instance()->getOption("ReverseOrder");
            break;
        }

        //
        // if this weighs less than the specified vector, it is cheaper, return
        // 0
        //
        if (this->getWeight(currlevel) < w) {
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
    if (!WCprefix.empty()) {
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
