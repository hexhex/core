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
GraphProcessor::run(const GAtomSet& in)
{
    //
    // start with empty set
    //
    resultModels.push_back(GAtomSet());

    Subgraph* sg;

    Component* leafComponent;

    //
    // go through all subgraphs
    //
    while (sg = depGraph->getNextSubgraph())
    {
        std::vector<GAtomSet> current;

        //
        // sgresult is the result of the entire subgraph
        //
        std::vector<GAtomSet> sgresult;

        //
        // each subgraph starts with input set
        //
        sgresult.push_back(in);

        do
        {
            std::vector<Component*> leaves;
            
            sg->getUnsolvedLeaves(leaves);

            current.clear();

            //
            // start with empty set
            //
            current.push_back(GAtomSet());

            std::vector<GAtomSet> compresult;

            //
            // solve the leaves first
            // if we didn't have any leaves - current = input
            //
            if (leaves.size() == 0)
            {
                current = sgresult;
            }
            else
            {
                for (std::vector<Component*>::iterator ci = leaves.begin();
                    ci != leaves.end();
                    ++ci)
                {
                    //
                    // evaluate each component based on the last result
                    //
                    (*ci)->evaluate(sgresult);

                    (*ci)->getResult(compresult);

                    if (compresult.size() == 0)
                    {
                        //
                        // inconsistent!
                        //
                        current.clear();

                        break;
                    }

                    multiplySets(current, compresult, current);
                }
            }

            if (current.size() == 0)
            {
                //
                // inconsistent!
                //
                sgresult.clear();

                break;
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

    //        std::cout << "after pruning, tpsg has nodes: " << tmpsg.getNodes().size() << std::endl;
            //
            // make a component from the node-set
            //
            Component* weakComponent = depGraph->createWeakComponent(tmpsg.getNodes());

            //
            // add the weak component to the subgraph
            //
            sg->addComponent(weakComponent);

            weakComponent->evaluate(current);

            weakComponent->getResult(sgresult);

            if (sgresult.size() == 0)
            {
                //
                // inconsistent!
                //
                break;
            }

        } while (sg->unsolvedComponentsLeft());

        if (sgresult.size() == 0)
        {
            //
            // inconsistent!
            //
            resultModels.clear();

            break;
        }

        multiplySets(resultModels, sgresult, resultModels);
    }

    resultSetIndex = resultModels.begin();
}


GAtomSet*
GraphProcessor::getNextModel()
{
    if (resultSetIndex != resultModels.end())
        return &(*(resultSetIndex++));

    return NULL;
}
