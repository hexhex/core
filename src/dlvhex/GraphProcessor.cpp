/* -*- C++ -*- */

/**
 * @file GraphProcessor.cpp
 * @author Roman Schindlauer
 * @date Fri Sep  9 14:40:26 CEST 2005
 *
 * @brief Control center for traversing and evaluating the program graph.
 *
 *
 */

#include "dlvhex/GraphProcessor.h"
#include "dlvhex/ModelGenerator.h"
#include "dlvhex/errorHandling.h"

GraphProcessor::GraphProcessor(DependencyGraph *dg)
    : depGraph(dg)
{
}


void
GraphProcessor::run(const GAtomSet &in)
{
    
//    std::vector<Subgraph> subgraphs(depGraph->getSubgraphs());

/*    Subgraph tmpgraph;

    std::vector<Component> weakcomponents;

    Component* ptrComponent;
*/

    Subgraph* sg;

    Component* leafComponent;

    //
    // go thorugh all weak components of the entire program
    //
    //for (std::vector<Subgraph>::const_iterator si = subgraphs.begin();
    //     si != subgraphs.end();
    //     ++si)
    while (sg = depGraph->getNextSubgraph())
    {
        
        std::vector<Component*> leaves;
        
        sg->getUnsolvedLeaves(leaves);

        //
        // solve the leaves first
        //
        for (std::vector<Component*>::iterator ci = leaves.begin();
             ci != leaves.end();
             ++ci)
        {
            leafComponent->evaluate();
        }

        //
        // make copy of subgraph
        //
        Subgraph tmpsg(*sg);

        //
        // remove components from temp subgraph
        // the result is a subgraph without any SCCs
        //
        tmpsg.pruneComponents();

        //
        // from the nodes left in the subgraph, find all WCCs
        //
        std::vector<std::vector<AtomNode*> > wccs;

        depGraph->getWeakComponents(tmpsg.getNodes(), wccs);

        //
        // go through all found WCCs and create component-object for them
        //
        for (std::vector<std::vector<AtomNode*> >::iterator wcc = wccs.begin();
            wcc != wccs.end();
            ++wcc)
        {
            Component* weakComponent;

            //
            // make a component from the node-set
            //
            depGraph->createComponent(*wcc, weakComponent);

            //
            // add the weak component to the subgraph
            //
            sg->addComponent(weakComponent);

            //
            // and solve it
            //
            weakComponent->evaluate();
        }

    }

/*    {
        //
        // repeat while this subgraph has nodes left
        //
        while (sg->countNodes() > 0)
        {
            //
            // go through all Components of the subgraph
            //
//            for (std::vector<Component*>::const_iterator ci = sg->getComponents().begin();
//                 ci != sg->getComponents().end();
//                 ++ci)

            //
            // solve all leaf components until none is left
            //
            while (ptrComponent = sg->getNextLeafComponent())
            {
//                if (sg->isLeaf(*ci))
                ptrComponent->solve();
            }

            tmpgraph = *sg;

            //
            // remove all other components and everything 'above' them
            //
//            for (std::vector<Component*>::const_iterator ci = sg->getComponents().begin();
//                 ci != sg->getComponents().end();
//                 ++ci)
            while (ptrComponent = tmpgraph->getNextComponent())
            {
                tmpgraph.pruneComponent(ptrComponent);
            }

            //
            // from what is left, we compute all weak components
            //
            depGraph->getWeakComponents(tmpgraph, weakComponents);

            for (std::vector<Component>::const_iterator wi = weakComponents.begin();
                wi != weakComponents.end();
                ++wi)
            {
                //
                // and evaluate them
                //
                wi->solve();

                //
                // and remove them from the subgraph's tree
                //
                sg->removeComponent(wi);
            }
        }
    }*/
}


GAtomSet*
GraphProcessor::getNextModel()
{
    if (resultSetIndex != resultModels.end())
        return &(*(resultSetIndex++));

    return NULL;
}
