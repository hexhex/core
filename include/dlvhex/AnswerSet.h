/* -*- C++ -*- */

/**
 * @file   AnswerSet.h
 * @author Roman Schindlauer
 * @date   Wed May  3 13:25:35 CEST 2006
 * 
 * @brief  AnswerSet class.
 * 
 * 
 */

#ifndef _ANSWERSET_H
#define _ANSWERSET_H


#include "boost/shared_ptr.hpp"

#include "dlvhex/AtomSet.h"


/**
 * @brief An AnswerSet is an AtomSet with additional information.
 *
 * the additional information consists of the weights of this answer set. If the
 * program contains weak constraints, the weights determine the order of the
 * answer sets.
 */
class AnswerSet : public AtomSet
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
    AnswerSet(std::string = "");

    /**
     * @brief Sets the AtomSet of the answer set.
     */
    void
    setSet(AtomSet&);

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
     */
    bool
    cheaperThan(const AnswerSet&) const;

    /**
     * @brief compare with weight vector.
     *
     * this function returns true, if this answer set has more costs than are
     * specified in the weight-vector.
     */
    bool
    moreExpensiveThan(const weights_t) const;


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


#endif /* _ANSWERSET_H */
