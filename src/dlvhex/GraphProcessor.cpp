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
    resultModels.clear();
}


void
GraphProcessor::run(const GAtomSet &in)
{
    returnPointer = 0;

    //
    // for now: everything with iteration:
    //
    FixpointModelGenerator mg;

    try
    {
        mg.computeModels((*depGraph).components, in, resultModels);
    }
    catch (generalError&)
    {
        throw;
    }

}


GAtomSet*
GraphProcessor::getNextModel()
{
    GAtomSet* ret;
    
    if (returnPointer < resultModels.size())
        ret = &resultModels[returnPointer];
    else
        ret = NULL;
    
    returnPointer++;
    
    return ret;
}

 void
 GraphProcessor::evaluateSubgraph(const std::vector<Component*> &components,
                                  GAtomSet &result) const
{
}
