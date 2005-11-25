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
#include "dlvhex/Component.h"


void
GraphBuilder::createNodes(Rules& program)
{
    //
    // each rule head is a node
    //
    for (Rules::iterator r = program.begin();
         r != program.end();
         r++)
    {
        nodes.push_back(RuleNode(&(*r)));

        //
        // and each external atom is a node
        // (collect them from each rule)
        //
        /*
        for (std::vector<ExternalAtom*>::const_iterator e = (*r).getExternalAtoms().begin();
            e != (*r).getExternalAtoms().end();
            e++)
        {
            nodes.push_back(ExternalNode(*e));
        }*/
    }
}


void
SimpleGraphBuilder::build(Program& program, std::vector<Subgraph*>& subgraphs)
{
    ModelGenerator* fpmg = new FixpointModelGenerator;
//    ModelGenerator* fpmg = new OrdinaryModelGenerator;

    Component* c = new ProgramComponent(program, fpmg);

    Subgraph* sg = new Subgraph;

    sg->addComponent(c);

    subgraphs.push_back(sg);
}


SimpleGraphBuilder::SimpleGraphBuilder()
{ }

/*
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
*/
