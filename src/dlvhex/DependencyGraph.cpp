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
    gb->run(program.getRules(), nodegraph);

    if (global::optionVerbose)
    {
        gb->dumpGraph(nodegraph, std::cout);
    }


    std::vector<std::vector<AtomNode*> > weakComponents;

    //
    // get WCCs form the entire nodegraph
    //
    getWeakComponents(nodegraph.getNodes(), weakComponents);

    //
    // creating subgraphs from weak components
    //
    for (std::vector<std::vector<AtomNode*> >::iterator wcc = weakComponents.begin();
         wcc != weakComponents.end();
         ++wcc)
    {
        Subgraph subgraph;

        std::vector<std::vector<AtomNode*> > strongComponents;

        //
        // keep track of the nodes that belong to a SCC
        //
        std::vector<AtomNode*> visited;

        //
        // find all strong components within this WCC
        //
        getStrongComponents(*wcc, strongComponents);

        for (std::vector<std::vector<AtomNode*> >::const_iterator scc = strongComponents.begin();
             scc != strongComponents.end();
             ++scc)
        {
            //
            // for each found SCC we create a component-object
            //

            Component* comp;

            createComponent(*scc, comp);

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

        //
        // now, after processing all SCCs of this WCC, let's see if there is any
        // external atom left that was not in any SCC
        //
        for (std::vector<AtomNode*>::const_iterator weaknode = (*wcc).begin();
             weaknode != (*wcc).end();
             ++weaknode)
        {
            if (find(visited.begin(), visited.end(), *weaknode) == visited.end())
            {
                if ((*weaknode)->getAtom()->getType() == Atom::EXTERNAL)
                {
                    //std::cout << "single node external atom!" << std::endl;
                    Component* comp = new ExternalComponent();

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
    }

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

    for (std::vector<Component*>::const_iterator ci = components.begin();
         ci != components.end();
         ++ci)
    {
        delete *ci;
    }
}



void
DependencyGraph::createComponent(const std::vector<AtomNode*>& nodes,
                                 Component* component)
{
    Program p;


    bool isStratified = true;

    //
    // go through all nodes of this SCC
    //
    for (std::vector<AtomNode*>::const_iterator ni = nodes.begin();
         ni != nodes.end();
         ++ni)
    {

        //
        // add all rules from this node to a temporary program-object
        //
        for (std::vector<const Rule*>::const_iterator ri = (*ni)->getRules().begin();
                ri != (*ni)->getRules().end();
                ++ri)
        {
            if (find(p.getRules().begin(), p.getRules().end(), **ri) == 
                p.getRules().end())
                p.addRule(**ri);

        }

        //
        // look through all dependencies of this Vertex - if we find a
        // NEG_PRECEDING, this is an unstratified SCC!
        //
        // since an SCC is always cyclid, we only have to consider preceding,
        // not preceding AND succeeding!
        //
        for (std::vector<Dependency>::const_iterator di = (*ni)->getPreceding().begin();
                di != (*ni)->getPreceding().end();
                ++di)
        {
            if ((*di).getType() == Dependency::NEG_PRECEDING)
                isStratified = false;
        }
    }

    //
    // now decide which model generator we need:
    //
    // pure ordinary SCC (no ext atoms): call-dlv-once-mg (fixpoint should work)
    // strat, with ext atoms: fixpoint
    // unstrat, with ext atoms: guess&check
    //

    ModelGenerator* mg;

    if (isStratified)
    {
        if (p.getExternalAtoms().size() > 0)
            mg = new FixpointModelGenerator();
        else
            mg = new OrdinaryModelGenerator();
    }
    else
    {
        mg = new FixpointModelGenerator();
        std::cout << "*** guess and check not implemented yet! ***" << std::endl;
    }

    //
    // now that we have the subprogram and the suitable model
    // generator, we can create the component-object
    //
    component = new ProgramComponent(p, mg);
    
    //
    // now that we have created the component-object, go through the nodes
    // again and add them
    //
    for (std::vector<AtomNode*>::const_iterator ni = nodes.begin();
            ni != nodes.end();
            ++ni)
    {
        //
        // add this node to the component-object
        //
        component->addAtomNode(*ni);
    }
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


