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
}


DependencyGraph::DependencyGraph(Rules &program,
                                 GraphBuilder *gb)
{
    for (Rules::iterator r = program.begin();
         r != program.end();
         r++)
    {
        nodes.push_back(Node(&(*r)));
    }
    
    // FindDependencies();
    
    (*gb).findComponents(nodes, components);
    //FindComponentsFromNodes();
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


std::vector<Cluster>*
DependencyGraph::getClusters()
{
    return &clusters;
}

