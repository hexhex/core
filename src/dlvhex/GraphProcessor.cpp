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

#include "dlvhex/globals.h"
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
    if (global::optionVerbose)
        std::cout << "Starting Graph Processor" << std::endl;

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

        do // while unsolved components left
        {
            std::vector<Component*> leaves;
            
            sg->getUnsolvedLeaves(leaves);

            current.clear();

            //
            // start with empty set as result of the leaves
            //
            current.push_back(GAtomSet());

            //
            // result of a single leaf component
            //
            std::vector<GAtomSet> compresult;

            //
            // solve the leaves first
            // if we didn't have any leaves - current = input
            //
            if (leaves.size() == 0)
            {
                current = sgresult;

                if (global::optionVerbose)
                    std::cout << "No leaf components" << std::endl;
            }
            else
            {
                for (std::vector<Component*>::iterator ci = leaves.begin();
                    ci != leaves.end();
                    ++ci)
                {
                    if (global::optionVerbose)
                    {
                        std::cout << "Evaluating leaf component:" << std::endl;

                        (*ci)->dump(std::cout);
                    }

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

                    //
                    // build the product of the other leaves' result and this
                    // one
                    //
                    multiplySets(current, compresult, current);
                }

//                std::cout << "Leaf Result: ";
//                printGAtomSet(current[0], std::cout, 0);
//                std::cout << std::endl;
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

            //std::cout << "after pruning: " << std::endl;
            //tmpsg.dump(std::cout);
            //std::cout << std::endl;

            //
            // make a component from the node-set
            //
//            Component* weakComponent = depGraph->createWeakComponent(tmpsg.getNodes());
            ModelGenerator* mg = new OrdinaryModelGenerator();

            Component* weakComponent = new ProgramComponent(tmpsg.getNodes(), mg);

            //std::cout << "wcc created." << std::endl;

            //
            // add the weak component to the subgraph
            //
            sg->addComponent(weakComponent);

            //std::cout << "wcc added to sg:" << std::endl;
            //weakComponent->dump(std::cout);

            weakComponent->evaluate(current);

            //std::cout << "wcc evaluated." << std::endl;

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
