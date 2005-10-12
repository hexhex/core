/* -*- C++ -*- */

/**
 * @file Interpretation.h
 * @author Roman Schindlauer
 * @date Mon Sep  5 16:57:17 CEST 2005
 *
 * @brief Interpretation class.
 *
 *
 */

#ifndef _INTERPRETATION_H
#define _INTERPRETATION_H

#include "dlvhex/Atom.h"

class Interpretation
{
public:
    
    Interpretation();

    Interpretation(const GAtomSet &);

    void
    replaceBy(const GAtomSet &atomset);

    unsigned
    getSize() const;

    /**
     * @brief Returns set of positive facts in interpretation.
     */
    const
    GAtomSet* getFacts() const;

    /**
     * @brief Adds a groundatom to the positive part of the interpretation,
     * if it doesn't exist already.
     */
    void
    addPositive(const GAtom &gatom);

    /**
     * @brief Adds a set of groundatoms to the positive part of the interpretation.
     */
    void
    addPositive(const GAtomSet &gatomset);

    /**
     * @brief Checks, if the groundatom is true in the interpretation.
     */
    bool
    isTrue(const GAtom &gatom);

    /**
     * @brief Fill atomset with all groundatoms that unify with atom.
     * atomset is not cleared before!
     */
    void
    matchAtom(const Atom atom, GAtomSet &atomset) const;

    /**
     * @brief Fill atomset with all groundatoms whose predicate unifies with
     * pred. atomset is not cleared before!
     */
    void
    matchPredicate(const std::string pred, GAtomSet &atomset) const;
    
    /**
     * @brief Removes all facts that have predicate pred from I.
     */
    void
    removePredicate(const std::string pred);
    
    /**
     * @brief Substracts a set of GAtoms from the Interpretation
     */

    /**
     * @brief Converts the interpretation into a textstring containing facts.
     */
    //std::string printFacts() const;
    
    std::ostream&
    printSet(std::ostream& stream, const bool ho) const;


private:
    
    GAtomSet positive;
};


/**
 * @brief Dumps the interpretation in first-order notation; should only
 * be used for testing purposes.
 */
std::ostream&
operator<< (std::ostream &out, const Interpretation &i);


#endif /* _INTERPRETATION_H */
