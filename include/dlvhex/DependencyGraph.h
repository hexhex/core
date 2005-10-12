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
#include "dlvhex/Cluster.h"
#include "dlvhex/GraphBuilder.h"




/**
 * @brief
 */
class DependencyGraph
{
public:
    
    /**
     * @brief
     */
    DependencyGraph();

    /**
     * @brief Constructor that builds the dependency graph.
     *
     * The dependency graph is built from
     * a set of program rules with an algorithm provided by the
     * strategy class GraphBuilder.
     */
    DependencyGraph(Rules &program,
                    GraphBuilder *gb);

    /**
     * @brief Returns list of clusters.
     */
    std::vector<Cluster>*
    getClusters();

private:
    
    /**
     * @brief
     */
    std::vector<Node>
    nodes;

public:

    /**
     * @brief
     */
    std::vector<Component>
    components;

    /**
     * @brief
     */
    std::vector<Cluster>
    clusters;

private:

    /**
     * @brief
     */
    void
    FindComponentsFromNodes();

};

#endif /* _DEPENDENCYGRAPH_H */
