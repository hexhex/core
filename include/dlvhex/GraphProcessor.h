/* -*- C++ -*- */

/**
 * @file GraphProcessor.h
 * @author Roman Schindlauer
 * @date Tue Sep 13 19:21:37 CEST 2005
 *
 * @brief Control class for traversing and evaluating the program graph.
 *
 *
 */

#ifndef _GRAPHPROCESSOR_H
#define _GRAPHPROCESSOR_H


#include "dlvhex/DependencyGraph.h"
#include "dlvhex/Atom.h"

/**
 * @brief Control center for traversing and evaluating the program graph.
 */
class GraphProcessor
{
public:

    GraphProcessor(DependencyGraph *depgraph);

    void
    run(const GAtomSet &in);

    GAtomSet*
    getNextModel();

    std::vector<GAtomSet>
    resultModels;

    unsigned returnPointer;

private:

    /**
     * @brief 
     */
    void
    evaluateSubgraph(const std::vector<Component*> &components,
                     GAtomSet &result) const;

    DependencyGraph *depGraph;
};


#endif /* _GRAPHPROCESSOR_H */
