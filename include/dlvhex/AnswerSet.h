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
 * @file   AnswerSet.h
 * @author Roman Schindlauer
 * @date   Wed May  3 13:25:35 CEST 2006
 * 
 * @brief  AnswerSet class.
 * 
 * 
 */

#if !defined(_DLVHEX_ANSWERSET_H)
#define _DLVHEX_ANSWERSET_H

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/AtomSet.h"


DLVHEX_NAMESPACE_BEGIN

/**
 * @brief An AnswerSet is an AtomSet with additional information.
 *
 * the additional information consists of the weights of this answer set. If the
 * program contains weak constraints, the weights determine the order of the
 * answer sets.
 */
class DLVHEX_EXPORT AnswerSet : public AtomSet
{
public:

    /**
     * @brief Weight vector.
     *
     * Each element of the vector corresponds to a weight-level and its value
     * is the weight of the level.
     */
    typedef std::vector<unsigned> weights_t;

    /**
     * @brief Constructor.
     *
     * the optional string specifies the prefix of the auxiliary predicate
     * within the answer set that determines the costs of the set. If the string
     * is left empty, costs are not considered at all.
     */
    explicit
    AnswerSet(const std::string& = "");

    /**
     * @brief Sets the AtomSet of the answer set.
     */
    void
    setSet(const AtomSet&);

    /**
     * @brief Returns true if the answer set contains weight information, i.e.,
     * if the program contained any weak constraints.
     */
    bool
    hasWeights() const;

    /**
     * @brief Returns the maximum level for which a weight exists in this answer
     * set.
     *
     * Note that the numbering of levels starts with 1!
     */
    unsigned
    getWeightLevels() const;

    /**
     * @brief Adds a weight:level assignment to the answer set.
     *
     * Note that the numbering of levels starts with 1!
     */
    void
    addWeight(unsigned weight, unsigned level);

    /**
     * @brief Get weight of specified level.
     *
     * Note that the numbering of levels starts with 1!
     */
    unsigned
    getWeight(unsigned) const;

    /**
     * @brief compare with other answer set regarding weights.
     *
     * This function returns true, if this answer set has less costs than the
     * specified one.
	 * The exact semantics of "less" is determined by the used ordering. Default
	 * notion of "less" is "lower numerical values", so an answer set is
	 * cheaper, if its weights on the respective level are lower.  This can be
	 * reversed by the command line switch "--reverse", then higher numerical
	 * values are considered as "cheaper".
     */
    bool
    cheaperThan(const AnswerSet&) const;

    /**
     * @brief compare with weight vector.
     *
     * This function returns true, if this answer set has higher costs than are
     * specified in the weight-vector.
	 * The exact semantics of "higher" is determined by the used ordering.
	 * Default notion of "higher" is "higher numerical values", so an answer set
	 * is more expensive, if its weights on the respective level are higher.
	 * This can be reversed by the command line switch "--reverse", then lower
	 * numerical values are considered as "more expensive".
     */
    bool
    moreExpensiveThan(const weights_t&) const;


    /**
     * @brief Comparison Operator.
     *
     * The operator returns true, if this answer set is "smaller" than the
     * specified one. If the program does not contain any weak constraints, the
     * answer sets are compared using a generic comparison, which just ensures a
     * total ordering. With weak constraints, an answer set is smaller than
     * another if it has lower costs. If the costs are equal, the original
     * comparison is used.
     */
    int
    operator< (const AnswerSet&) const;

    /**
     * @brief Store highest level and highest weight that occurs in the
     * (grounded) program.
     *
     * Each of these two values is only stored if it is higher than the previous
     * one.
     */
    static void
    setMaxLevelWeight(unsigned, unsigned);

    /**
     * @brief Get maximum level that occurs in the program.
     */
    static unsigned
    getMaxLevel();

private:

    /**
     * @brief Weight per level.
     */
    weights_t weights;

    /**
     * @brief Prefix denoting weak constraint auxiliary predicates.
     */
    std::string WCprefix;

    /**
     * @brief Highest level that occurs in the program.
     */
    static unsigned maxLevel;

    /**
     * @brief Highest weight value that occurs in the program.
     */
    static unsigned maxWeight;
};

typedef boost::shared_ptr<AnswerSet> AnswerSetPtr;

/**
 * This operator should only be used for dumping the output; it uses
 * the first-order notation.
 */
std::ostream&
operator<< (std::ostream&, const AnswerSet&);


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_ANSWERSET_H */


// Local Variables:
// mode: C++
// End:
