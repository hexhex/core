/* -*- C++ -*- */

/**
 * @file DependencyGraph.h
 * @author Roman Schindlauer
 * @date Mon Sep 19 12:19:38 CEST 2005
 *
 * @brief Classes for the dependency graph class and its subparts.
 *
 *
 */


#ifndef _DEPENDENCYGRAPH_H
#define _DEPENDENCYGRAPH_H

#include <vector>

#include "dlvhex/Rule.h"
#include "dlvhex/Component.h"
#include "dlvhex/GraphBuilder.h"




/**
 * @brief
 */
class DependencyGraph
{
public:
    
    /// Ctor.
    DependencyGraph();


    /// Dtor.
    ~DependencyGraph();

    /**
     * @brief Constructor that builds the dependency graph.
     *
     * The dependency graph is built from
     * a set of program rules with an algorithm provided by the
     * strategy class GraphBuilder.
     *
     *TODO: if we have a possibility of getting an external atom
     *from a literal, then extract external nodes from the Rules
     * - we don't need the gloabl externalAtoms then!
     */
    DependencyGraph(Program&,
                    GraphBuilder*);

    std::vector<Component*>
    getPredecessors(Component* c) const;

    std::vector<Component*>
    getComponents(const Subgraph*) const;

    Subgraph*
    getNextSubgraph();
    
private:
    
    /**
     * @brief All nodes.
     */
    std::vector<Node>
    nodes;

    /**
     * @brief All subgraphs (connected components).
     */
    std::vector<Subgraph*>
    subgraphs;

    /**
     * @brief Current subgraph pointer.
     */
    std::vector<Subgraph*>::iterator
    currentSubgraph;


    /**
     * @brief
     */
//    void
//    FindComponentsFromNodes();

};

#endif /* _DEPENDENCYGRAPH_H */
