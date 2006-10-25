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

GraphProcessor::GraphProcessor(DependencyGraph *dg)
    : depGraph(dg)
{
}


void
GraphProcessor::run(const AtomSet& in)
{
    if (Globals::Instance()->getOption("Verbose"))
        std::cerr << std::endl << "@@@ running graph processor @@@" << std::endl << std::endl;

    //
    // start with empty result set
    //
    resultModels.push_back(AtomSet());

    Subgraph* sg;

//    std::cout << "input set:" << std::endl;
//    in.print(std::cout, 0);
//    std::cout << "input set end." << std::endl;

    //Component* leafComponent;

    //
    // go through all subgraphs
    //
    while ((sg = depGraph->getNextSubgraph()) != NULL)
    {
        std::vector<AtomSet> current;

        //
        // sgresult is the result of the entire subgraph
        //
        std::vector<AtomSet> sgresult;

        //
        // each subgraph starts with input set
        //
        sgresult.push_back(in);

        do // while unsolved components left
        {
            //
            // result of a single leaf component
            //
            std::vector<AtomSet> compresult;

            std::vector<Component*> leaves;
            
            sg->getUnsolvedLeaves(leaves);

            //
            // solve the leaves first
            // if we didn't have any leaves, then current = lastresult
            //
            if (leaves.size() == 0)
            {
                current = sgresult;

                if (Globals::Instance()->getOption("Verbose"))
                    std::cerr << "No leaf components" << std::endl;
            }
            else
            {
                //
                // before going through all leaves - start with a single empty
                // answer set as a base for the set multiplying
                //
                current.clear();
                current.push_back(AtomSet());

                //
                // we have leaves - so evaluate each:
                //
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

                        if (Globals::Instance()->getOption("Verbose"))
                            std::cerr << "Leaf Component was inconsistent!" << std::endl;

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

            //std::cout << "Subgraph after pruning: " << std::endl;
            //tmpsg.dump(std::cout);
            //std::cout << std::endl;

            //
            // anything left?
            //
            if (tmpsg.getNodes().size() > 0)
            {
                //
                // make a component from the node-set
                //
                ModelGenerator* mg = new OrdinaryModelGenerator();

                Component* weakComponent = new ProgramComponent(tmpsg.getNodes(), mg);

                //std::cout << "wcc created." << std::endl;

                //
                // add the weak component to the subgraph
                //
                sg->addComponent(weakComponent);

                //std::cout << "wcc added to sg:" << std::endl;
                //weakComponent->dump(std::cout);
                //

                try
                {
                    weakComponent->evaluate(current);
                }
                catch (GeneralError&)
                {
                    throw;
                }

                //std::cout << "wcc evaluated." << std::endl;

                weakComponent->getResult(sgresult);

                if (sgresult.size() == 0)
                {
                    //
                    // inconsistent!
                    //
                    if (Globals::Instance()->getOption("Verbose"))
                        std::cerr << "Program Component was inconsistent!" << std::endl;

                    break;
                }
            }
            else
            {
                sgresult = current;
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


AtomSet*
GraphProcessor::getNextModel()
{
    if (resultSetIndex != resultModels.end())
        return &(*(resultSetIndex++));

    return NULL;
}
