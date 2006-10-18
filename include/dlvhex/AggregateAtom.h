/* -*- C++ -*- */

/**
 * @file AggregateAtom.h
 * @author Roman Schindlauer
 * @date Wed Oct 18 16:03:31 CEST 2006
 *
 * @brief Aggregate Atom class.
 *
 */

#ifndef _AGGREGATEATOM_H
#define _AGGREGATEATOM_H


#include "dlvhex/Atom.h"

// we need also literal, because the rule body type is declared there:
#include "dlvhex/Literal.h"



/**
 * @brief Aggregate atom.
 *
 * Aggregates are special types of atoms. In principle, we can just pass them on
 * to dlv, but we have to recognize their internal structure because of
 * dependencies. An aggregate includes a conjunction of atoms (like a rule body)
 * which specify the values to aggregate on. These atoms of course stem from
 * other parts of the program, hence we have to consider this when building the
 * dependency graph
 */
class AggregateAtom : public Atom
{
public:

    /**
     * Constructing an aggregate predicate.
     *
     * The constructor only builds the aggregate itself, the actual comparison
     * operator and the other term(s) of the comparison are parsed later.
     *
     * @params:
     * Type of the aggregate.
     * Variables to aggregate on.
     * Atom conjunction of the aggregate.
     */
    AggregateAtom(const std::string&,
                  const Tuple&,
                  const RuleBody_t&);

    /**
     * Sets the comparison operator(s).
     *
     * There can be two comparison operators:
     *   2 <= AGG < 5
     * Or only one:
     *   0 < AGG
     *   AGG > 5
     *
     * The first string is the left one, the second the right. If only one of
     * them exists, the other one stays empty.
     */
    void
    setComp(const std::string, const std::string);

    /**
     * Sets the left term.
     *
     * See setComp(). In the atom
     *   2 <= AGG < 5
     * the left term would be 2.
     */
    void
    setLeftTerm(const Term&);

    /**
     * Sets the right term.
     *
     * See setComp(). In the atom
     *   2 <= AGG < 5
     * the right term would be 5.
     */
    void
    setRightTerm(const Term&);

    /**
     * @brief Tests for unification with another atom.
     *
     * Here, we abuse the notion of unification a bit. Unification is used in
     * dlvhex to find out which atoms depend from each other. An aggregate
     * depends from another atom, if this atom unifies with one of the atoms in
     * the aggregate's body. So we use the aggregate's unifiesWith method to
     * determine whether an aggregate expression depends on an atom - though, of
     * course, an aggregate itself cannot unify with anything.
     */
    virtual bool
    unifiesWith(const AtomPtr) const;

    /**
     * @brief Prints the aggregate.
     *
     * The aggregate is printed in its normal syntax to be understood by dlv.
     * The higher-order switch is also used, because we have atoms in the
     * aggregate's conjunction.
     */
    virtual std::ostream&
    print(std::ostream&, const bool) const;


private:

    /**
     * The atom conjunction.
     */
    RuleBody_t body;

    /**
     * The actual variables to aggregate on.
     */
    Tuple aggVars;

    /**
     * The type of aggregate.
     *
     * We just take the string as it was parsed and pass it on to dlv.
     */
    std::string type;

    /**
     * Term(s) to be compared with.
     */
    Term left, right;

    /**
     * Comparison operator.
     */
    std::string cmpLeft, cmpRight;
};

#endif /* _AGGREGATEATOM_H */

