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
     * @param aggtype Type of the aggregate (this is basically just the
     * aggregate's name that will be passed on to dlv).
     * @param vars Variables to aggregate on.
     * @param conj Atom conjunction of the aggregate.
     */
    AggregateAtom(const std::string& aggtype,
                  const Tuple& vars,
                  const RuleBody_t& conj);

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


    virtual void
    accept(BaseVisitor&) const;


    const RuleBody_t&
    getBody() const
    {
      return body;
    }


    const Tuple&
    getVars() const
    {
      return aggVars;
    }


    const std::string&
    getType() const
    {
      return type;
    }

    const Term&
    getLeft() const
    {
      return left;
    }

    const Term&
    getRight() const
    {
      return right;
    }

    const std::string&
    getCmpLeft() const
    {
      return cmpLeft;
    }

    const std::string&
    getCmpRight() const
    {
      return cmpRight;
    }

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

