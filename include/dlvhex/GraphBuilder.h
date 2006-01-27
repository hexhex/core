/* -*- C++ -*- */

/**
 * @file GraphBuilder.h
 * @author Roman Schindlauer
 * @date Wed Jan 18 17:43:21 CET 2006
 *
 * @brief Abstract strategy class for finding the dependency edges of a program.
 *
 *
 */

#ifndef _GRAPHBUILDER_H
#define _GRAPHBUILDER_H

#include <iostream>

#include "dlvhex/Component.h"
#include "dlvhex/Rule.h"

/**
 * @brief
 *
 */
class GraphBuilder
{
public:

    /**
     * @brief Takes a set of rules and builds the according node graph.
     *
     */
    void
    run(const Rules&, NodeGraph&);

    /**
     * @brief Debug dump.
     */
    void
    dumpGraph(const NodeGraph&, std::ostream&) const;



private:

    /**
     * @brief Update two AtomNodes with a dependency.
     *
     */
    void
    addDep(AtomNode*, AtomNode*, Dependency::Type);
};


#endif /* _GRAPHBUILDER_H */
