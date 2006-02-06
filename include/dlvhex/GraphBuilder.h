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
 * @brief Class for building a dependency graph from a given program.
 *
 */
class GraphBuilder
{
public:

    /**
     * @brief Takes a set of rules and builds the according node graph.
     *
     * This nodegraph will contain the entire dependency graph of the program,
     * including any artificial nodes that had to be created for auxiliary
     * rules, e.g., for external atoms with variable input parameters.
     */
    void
    run(const Rules&, NodeGraph&);


    /**
     * @brief Debug dump.
     */
    void
    dumpGraph(const NodeGraph&, std::ostream&) const;



private:

//    void
//    addDep(AtomNode*, AtomNode*, Dependency::Type);

};


#endif /* _GRAPHBUILDER_H */
