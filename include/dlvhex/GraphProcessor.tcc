/* -*- C++ -*- */

/**
 * @file ModelGenerator.tcc
 * @author Roman Schindlauer

 * @date Fri Nov 25 00:42:38 CET 2005
 *
 * @brief Template implementation of the GraphProcessor class.
 *
 *
 */


#ifndef _GRAPHPROCESSOR_TCC
#define _GRAPHPROCESSOR_TCC


template <class SubgraphType, class ComponentType>
GraphProcessor<SubgraphType, ComponentType>::GraphProcessor(DependencyGraph *dg)
    : depGraph(dg)
{
}


template <class SubgraphType, class ComponentType>
void
GraphProcessor<SubgraphType, ComponentType>::combine(std::vector<GAtomSet>& s1, std::vector<GAtomSet>& s2)
{
    if (s1.size() == 0)
    {
        s1 = s2;
    }
    else
    {
    }
}

template <class SubgraphType, class ComponentType>
void
GraphProcessor<SubgraphType, ComponentType>::solve(ComponentType* c)
{
    if (c->wasEvaluated())
        return;

    std::vector<GAtomSet> input;
    std::vector<GAtomSet> predResult;

    //
    // to solve this component, we first need to solve all its
    // predecessors
    //
    std::vector<ComponentType*> predecessors(depGraph->getPredecessors(c));

    for (typename std::vector<ComponentType*>::const_iterator pi = predecessors.begin();
         pi != predecessors.end();
         pi++)
    {
        try
        {
            solve(*pi);
        }
        catch (GeneralError&)
        {
            throw;
        }

        //
        // if the predecessor had a result...
        //
        if ((**pi).numResults() > 0)
        {
            //
            // combine it with the result of the other predecessors
            //
            predResult = (**pi).getResult();
            combine(input, predResult);
        }
        else
        {
            //
            // otherwise it was inconsistent and we can stop everything
            //
            return;
        }
    }

    //
    // no previous components? then we have a single set of facts
    // as input for the current one
    //
    if (predecessors.size() == 0)
        input.push_back(startFacts);

    try
    {
        c->evaluate(input);
    }
    catch (GeneralError&)
    {
        throw;
    }

    //
    // overwrite entire subgraph result with Component's result
    // for each Component
    // the Component which was evaluated last should have the result of the
    // entire subgraph.
    //
    singleSubgraphAnswer = c->getResult();
}

template <class SubgraphType, class ComponentType>
void
GraphProcessor<SubgraphType, ComponentType>::run(const GAtomSet &in)
{
    returnPointer = 0;

    startFacts = in;

    SubgraphType* sg;

    typedef std::vector<ComponentType*> ComponentVector;

    ComponentVector Components;

    //
    // go though all connected components of the program ("subgraphs")
    //
    while (sg = (*depGraph).getNextSubgraph())
    {
        //
        // get all Components from this CC
        //
        Components = (*depGraph).getComponents(sg);

        //
        // solve each Component with recursive function
        //
        for (typename ComponentVector::iterator ci = Components.begin();
             ci != Components.end();
             ci++)
        {
            try
            {
                solve(*ci);
            }
            catch (GeneralError&)
            {
                throw;
            }

            //
            // something belom *com was inconsistent?
            //
            if ((**ci).numResults() == 0)
                break;
        }

        //
        // if one component was inconsistent - bail out
        //
        if (singleSubgraphAnswer.size() == 0)
        {
            resultModels.clear();
            break;
        }

        //
        // after all Compact Components of one connected
        // component are evaluated,
        // the resulting answer sets are in singleCCAnswer
        //
        combine(resultModels, singleSubgraphAnswer);
    }
}


template <class SubgraphType, class ComponentType>
GAtomSet*
GraphProcessor<SubgraphType, ComponentType>::getNextModel()
{
    if (returnPointer < resultModels.size())
        return &resultModels[returnPointer++];
    
    return NULL;
}


#endif /* _GRAPHPROCESSOR_TCC */
