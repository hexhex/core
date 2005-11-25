/* -*- C++ -*- */

/**
 * @file DependencyGraph.cpp
 * @author Roman Schindlauer
 * @date Mon Sep 19 12:19:38 CEST 2005
 *
 * @brief Classes for the dependency graph class and its subparts.
 *
 *
 */

#include <sstream>

#include "dlvhex/DependencyGraph.h"

#include "dlvhex/ASPsolver.h"
#include "dlvhex/errorHandling.h"
#include "dlvhex/globals.h"
#include "dlvhex/ProgramBuilder.h"



DependencyGraph::DependencyGraph()
{
    currentSubgraph = subgraphs.begin();
}


DependencyGraph::DependencyGraph(Program& program,
                                 GraphBuilder* gb)
{
    gb->build(program, subgraphs);

    currentSubgraph = subgraphs.begin();

    
//    (*gb).findComponents(nodes, components);
/*    
    StratifiedComponent* c = new StratifiedComponent;
    
    for (std::vector<Node>::iterator n = nodes.begin();
         n != nodes.end();
         n++)
        (*c).addRuleNode((RuleNode*)&(*n));
    
    components.push_back(c);
    */
}


/*
void DependencyGraph::FindComponentsFromNodes()
{
    //
    // for now: add all nodes to one single component
    //
    
    Component c;
    
    for (std::vector<Node>::iterator n = nodes.begin();
         n != nodes.end();
         n++)
        c.addNode(&(*n));
    
    components.push_back(c);
}
*/

Subgraph*
DependencyGraph::getNextSubgraph()
{
    if (currentSubgraph != subgraphs.end())
        return *(currentSubgraph++);

    return NULL;
}


std::vector<Component*>
DependencyGraph::getPredecessors(Component* c) const
{
    std::vector<Component*> foo;

    return foo;
}


std::vector<Component*>
DependencyGraph::getComponents(const Subgraph* sg) const
{
    return sg->getComponents();
}


DependencyGraph::~DependencyGraph()
{
    for (std::vector<Subgraph*>::const_iterator si = subgraphs.begin();
         si != subgraphs.end();
         ++si)
    {
        delete *si;
    }
}

/*
DependencyGraph::
{
}


DependencyGraph::
{
}


DependencyGraph::
{
}
*/
