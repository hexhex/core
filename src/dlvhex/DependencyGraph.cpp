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



/*
DependencyGraph::DependencyGraph()
{
}
*/


DependencyGraph::DependencyGraph(Program& program,
                                GraphBuilder* gb,
                                ComponentFinder* cf)
    : componentFinder(cf)
{
    //
    // the graphbuilder creates nodes from the program and puts it into
    // the nodegraph
    //

    try
    {
        gb->run(program, nodegraph);
    }
    catch (GeneralError&)
    {
        throw;
    }

    /*
    if (global::optionVerbose)
    {
        gb->dumpGraph(nodegraph, std::cout);
    }
    */


    std::vector<std::vector<AtomNode*> > weakComponents;

    const std::vector<AtomNode*> allnodes = nodegraph.getNodes();

    Subgraph subgraph;

    std::vector<std::vector<AtomNode*> > strongComponents;

    //
    // keep track of the nodes that belong to a SCC
    //
    std::vector<AtomNode*> visited;

    //
    // find all strong components
    //
    getStrongComponents(allnodes, strongComponents);

    //
    // go through strong components
    //
    for (std::vector<std::vector<AtomNode*> >::const_iterator scc = strongComponents.begin();
        scc != strongComponents.end();
        ++scc)
    {
        //
        // we need a component object for each component that needs a special
        // evaluation procedure:
        // (i) stratified SCC with external atoms: fixpoint iteration
        // (ii) unstratified SCC with external atoms: guess&check
        //
        if (isExternal(*scc))
        {
            ModelGenerator* mg;

            //
            // if we have a negated edge in this nodeset, we have an unstratifed
            // component (because the nodeset is already a SCC!)
            //
            if (hasNegEdge(*scc))
            {
                mg = new GuessCheckModelGenerator();
            }
            else
            {
                mg = new FixpointModelGenerator();
            }

            Component* comp = new ProgramComponent(*scc, mg);

            //
            // component-object is finished, add it to the dependency graph
            //
            components.push_back(comp);

            //
            // add it also to the current subgraph
            //
            subgraph.addComponent(comp);

            //
            // mark these scc nodes as visited
            // TODO: this is not so nice here
            //
            for (std::vector<AtomNode*>::const_iterator ni = (*scc).begin();
                ni != (*scc).end();
                ++ni)
            {
                visited.push_back(*ni);
            }
        }
    }

    //
    // now, after processing all SCCs of this WCC, let's see if there is any
    // external atom left that was not in any SCC
    //
    for (std::vector<AtomNode*>::const_iterator weaknode = allnodes.begin();
            weaknode != allnodes.end();
            ++weaknode)
    {
        //
        // add atomnodes to subgraph!
        //
        subgraph.addNode(*weaknode);

        if (find(visited.begin(), visited.end(), *weaknode) == visited.end())
        {
            if ((*weaknode)->getAtom()->getType() == Atom::EXTERNAL)
            {
                //std::cout << "single node external atom!" << std::endl;
                Component* comp = new ExternalComponent(*weaknode);

                //
                // the ExternalComponent-object only consists of a single
                // node
                //
                comp->addAtomNode(*weaknode);

                //
                // keep track of the component-objects
                //
                components.push_back(comp);

                //
                // add it also to the current subgraph
                //
                subgraph.addComponent(comp);
            }
        }
    }
    

    if (global::optionVerbose)
    {
        subgraph.dump(std::cout);
    }

    //
    // this WCC is through, so the corresponding subgraph is finished!
    //
    subgraphs.push_back(subgraph);

    //
    // reset subgraph iterator
    //
    currentSubgraph = subgraphs.begin();
}


DependencyGraph::~DependencyGraph()
{
    /*
    for (std::vector<Subgraph*>::const_iterator si = subgraphs.begin();
         si != subgraphs.end();
         ++si)
    {
        delete *si;
    }
    */

}





//
// TODO: this function is actually meant for sccs - otherwise we would have to
// go thorugh succeeding as well!
//
bool
DependencyGraph::hasNegEdge(const std::vector<AtomNode*>& nodes)
{
    for (std::vector<AtomNode*>::const_iterator ni = nodes.begin();
         ni != nodes.end();
         ++ni)
    {
        //
        // since an SCC is always cyclic, we only have to consider preceding,
        // not preceding AND succeeding!
        //
        for (std::vector<Dependency>::const_iterator di = (*ni)->getPreceding().begin();
                di != (*ni)->getPreceding().end();
                ++di)
        {
            if ((*di).getType() == Dependency::NEG_PRECEDING)
                return true;
        }
    }

    return false;
}


bool
DependencyGraph::isExternal(const std::vector<AtomNode*>& nodes)
{
    for (std::vector<AtomNode*>::const_iterator ni = nodes.begin();
         ni != nodes.end();
         ++ni)
    {
        if ((*ni)->getAtom()->getType() == Atom::EXTERNAL)
            return true;
    }

    return false;
}


void
DependencyGraph::getWeakComponents(const std::vector<AtomNode*>& nodes,
                      std::vector<std::vector<AtomNode*> >& wccs)
{
    componentFinder->findWeakComponents(nodes, wccs);
}


void
DependencyGraph::getStrongComponents(const std::vector<AtomNode*>& nodes,
                        std::vector<std::vector<AtomNode*> >& sccs)
{
    componentFinder->findStrongComponents(nodes, sccs);
}


Subgraph*
DependencyGraph::getNextSubgraph()
{
    if (currentSubgraph != subgraphs.end())
        return &(*(currentSubgraph++));

    return NULL;
}


