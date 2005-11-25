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
#include "dlvhex/Component.h"

/**
 * @brief Control center for traversing and evaluating the program graph.
 */
template <class SubgraphType, class ComponentType>
class GraphProcessor
{
public:

    GraphProcessor(DependencyGraph *depgraph);

    void
    run(const GAtomSet &in);

    void
    solve(ComponentType* c);

    GAtomSet*
    getNextModel();

private:

    unsigned returnPointer;

    void
    combine(std::vector<GAtomSet>&, std::vector<GAtomSet>&);

    /**
     * @brief Result of all connected components (= the entire program).
     */
    std::vector<GAtomSet>
    resultModels;

    /**
     * @brief input EDB.
     */
    GAtomSet
    startFacts;

    /**
     * @brief Result of all CompactCs in a single connected component.
     */
    std::vector<GAtomSet>
    singleSubgraphAnswer;


    /**
     * @brief 
     */
//    void
//    evaluateSubgraph(const std::vector<Component*> &components,
//                     GAtomSet &result) const;

    DependencyGraph *depGraph;
};


//
// include implementation of templates
//
#include "dlvhex/GraphProcessor.tcc"


#endif /* _GRAPHPROCESSOR_H */
