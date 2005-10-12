/* -*- C++ -*- */

/**
 * @file GraphBuilder.cpp
 * @author Roman Schindlauer
 * @date Fri Sep  9 14:40:26 CEST 2005
 *
 * @brief Abstract strategy class for building a dependency graph from rules.
 *
 *
 */

#include "dlvhex/GraphBuilder.h"


void
GraphBuilder::buildNodes(Rules &program, std::vector<Node> &nodes)
{
    for (Rules::iterator r = program.begin();
         r != program.end();
         r++)
    {
        nodes.push_back(Node(&(*r)));
    }

    //
    // TODO: dependencies!
    //
}


SimpleGraphBuilder::SimpleGraphBuilder()
{ }

void
SimpleGraphBuilder::findComponents(std::vector<Node> &nodes,
                                   std::vector<Component> &components)
{
    //
    // simple builder: add all nodes to one single component
    //
    
    Component c;
    
    for (std::vector<Node>::iterator n = nodes.begin();
         n != nodes.end();
         n++)
        c.addNode(&(*n));
    
    components.push_back(c);
}
