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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef DLVHEX_DEBUG
#include <boost/date_time/posix_time/posix_time.hpp>
#endif // DLVHEX_DEBUG

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
    //
    // start with empty result set
    //
    resultModels.push_back(AtomSet());

    bool firstsubgraph = 1;

    Subgraph* sg;

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

        //        if (Globals::Instance()->getOption("Verbose"))
        //            std::cerr << "No leaf components" << std::endl;
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

                    //    if (Globals::Instance()->getOption("Verbose"))
                    //        std::cerr << "Leaf Component was inconsistent!" << std::endl;

                        break;
                    }

#ifdef DLVHEX_DEBUG
                    DEBUG_START_TIMER
#endif // DLVHEX_DEBUG

                    //
                    // build the product of the other leaves' result and this
                    // one
                    //
                    multiplySets(current, compresult, current);

#ifdef DLVHEX_DEBUG
	                //                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
                    DEBUG_STOP_TIMER("Combining intra-subgraph results       ")
#endif // DLVHEX_DEBUG

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

                //
                // add the weak component to the subgraph
                //
                sg->addComponent(weakComponent);

                try
                {
                    weakComponent->evaluate(current);
                }
                catch (GeneralError&)
                {
                    throw;
                }

                weakComponent->getResult(sgresult);

                if (sgresult.size() == 0)
                {
                    //
                    // inconsistent!
                    //
              //      if (Globals::Instance()->getOption("Verbose"))
              //          std::cerr << "Program Component was inconsistent!" << std::endl;

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

#ifdef DLVHEX_DEBUG
        DEBUG_START_TIMER
#endif // DLVHEX_DEBUG

        //
        // speed up things for first subgraph:
        //
        if (firstsubgraph)
            resultModels = sgresult;
        else
            multiplySets(resultModels, sgresult, resultModels);

        firstsubgraph = 0;

#ifdef DLVHEX_DEBUG
	    //                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
        DEBUG_STOP_TIMER("Combining inter-subgraph results       ")
#endif // DLVHEX_DEBUG
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
