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


/**
 * @brief Interpretation class.
 *
 * An interpretation is a set of facts. The class provides auxiliary functions
 * for dealing with interpretations and their subsets.
 */
class Interpretation
{
public:
   
    /// Ctor.
    Interpretation();


    /**
     * @brief Construct an Interpretation from a set of ground atoms.
     */
    Interpretation(const GAtomSet&);


    /**
     * @brief Removes all facts from the interpretation.
     */
    void
    clear();

    
    /**
     * @brief Fill atomset with all groundatoms whose predicate unifies with
     * pred. atomset is not cleared before!
     */
    void
    matchPredicate(const std::string, GAtomSet&) const;
    
    
    /**
     * @brief Adds a set of facts to the interpretation.
     */
    void
    add(const GAtomSet&);

    
    /**
     * @brief Replaces the interpretation's facts by the specified set.
     */
    void
    replaceBy(const GAtomSet&);

    
    /**
     * @brief Returns the interpretation's facts.
     */
    const GAtomSet&
    getAtomSet() const;

    
    /**
     * @brief Returns an iterator pointing to the first fact of the interpretation.
     */
    const GAtomSet::const_iterator
    begin() const;

    
    /**
     * @brief Returns an iterator pointing after the last element of the
     * interpretation.
     */
    const GAtomSet::const_iterator
    end() const;


private:
    
    GAtomSet positive;
};


/**
 * @brief Dumps the interpretation in first-order notation; should only
 * be used for testing purposes.
 */
std::ostream&
operator<< (std::ostream&, const Interpretation&);


#endif /* _INTERPRETATION_H */

