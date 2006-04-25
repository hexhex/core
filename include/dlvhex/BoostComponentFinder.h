/* -*- C++ -*- */

/**
 * @file BoostComponentFinder.h
 * @author Roman Schindlauer
 * @date Wed Jan 25 14:31:33 CET 2006
 *
 * @brief Strategy class for finding SCCs and WCCs from a given program graph, using
 * the Boost Graph Library
 *
 *
 */


#ifndef _BOOSTCOMPONENTFINDER_H
#define _BOOSTCOMPONENTFINDER_H


#include <vector>
#include <iostream>

#include "dlvhex/ComponentFinder.h"


/**
 * @brief Component Finder using the Boost Graph Library.
 *
 */
class BoostComponentFinder : public ComponentFinder
{
public:

    /// Ctor.
    BoostComponentFinder()
    { }

    /**
     * @brief Finds Weakly Connected Components from a list of AtomNodes.
     *
     * A single WCC is represented by a vector of AtomNodes. All WCCs are again
     * contained in a vector.
     */
    virtual void
    findWeakComponents(const std::vector<AtomNode*>&,
                       std::vector<std::vector<AtomNode*> >&);

    /**
     * @brief Finds Strongly Connected Components from a list of AtomNodes.
     */
    virtual void
    findStrongComponents(const std::vector<AtomNode*>&,
                         std::vector<std::vector<AtomNode*> >&);
    
private:

    /**
     * @brief Converts the AtomNode dependency structure into Edges.
     *
     * The Boost Graph Library works with integers as vertex-identifier and
     * pairs of integers as edges. This function converts the dependency
     * information of the given AtomNodes and convets it into the Edges type.
     */
    void
    makeEdges(const std::vector<AtomNode*>&, Edges&) const;

    /**
     * @brief Filters a set of AtomNodes based on given Vertices.
     *
     * The Boost functions return sets of vertices as result. This function
     * selects those AtomNodes from a given set that correspond to these
     * Vertices. The correspondence between AtomNodes and Vertices is based on
     * the unique ID each AtomNode has.
     */
    void
    selectNodes(const Vertices&,
                const std::vector<AtomNode*>&,
                std::vector<AtomNode*>&) const;
};


#endif /* _BOOSTCOMPONENTFINDER_H_ */
