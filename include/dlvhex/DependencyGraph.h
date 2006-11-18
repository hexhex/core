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
#include "dlvhex/AtomNode.h"
#include "dlvhex/Component.h"
#include "dlvhex/GraphBuilder.h"
#include "dlvhex/ComponentFinder.h"


/**
 * @brief
 */
class DependencyGraph
{
public:
    
    /// Ctor.
//    DependencyGraph();


    /// Dtor.
    ~DependencyGraph();

    /**
     * @brief Constructor that builds the dependency graph.
     *
     */
    DependencyGraph(const NodeGraph&,
                    ComponentFinder*);


    /**
     * @brief Creates weakly connected components from AtomNodes.
     */
    //void
    //getWeakComponents(const std::vector<AtomNodePtr>&,
    //                  std::vector<std::vector<AtomNodePtr> >&);

    /**
     * @brief Creates strongly connected components from AtomNodes.
     */
    void
    getStrongComponents(const std::vector<AtomNodePtr>&,
                        std::vector<std::vector<AtomNodePtr> >&);

    /**
     * @brief Creates a component-object from a WCC.
     */
//    Component*
//    createWeakComponent(const std::vector<AtomNodePtr>&);

    /**
     * @brief Creates a component-object from a SCC.
     */
//    Component*
//    createStrongComponent(const std::vector<AtomNodePtr>&);

    /*
    void
    prune(Edges&, const Component&);
    */
    /*
    std::vector<Component*>
    getPredecessors(Component* c) const;
*/

    /**
     * @brief Returns all Components.
     */
    std::vector<Component*>
    getComponents() const;

    Subgraph*
    getNextSubgraph();

    
private:

    bool
    hasNegEdge(const std::vector<AtomNodePtr>&);

    bool
    isExternal(const std::vector<AtomNodePtr>&);
    
    /**
     * @brief All nodes.
     */
    NodeGraph nodegraph;
    

    /**
     * @brief All components (strongly connected components).
     */
    std::vector<Component*> components;

    /**
     * @brief All subgraphs (connected components).
     */
    std::vector<Subgraph*> subgraphs;

    /**
     * @brief Current subgraph pointer.
     */
    std::vector<Subgraph*>::iterator
    currentSubgraph;


    ComponentFinder* componentFinder;
    /**
     * @brief
     */
//    void
//    FindComponentsFromNodes();

};

#endif /* _DEPENDENCYGRAPH_H */
