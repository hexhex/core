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
#include "dlvhex/Error.h"
#include "dlvhex/globals.h"
#include "dlvhex/ProgramBuilder.h"



/*
DependencyGraph::DependencyGraph()
{
}
*/


DependencyGraph::DependencyGraph(const NodeGraph& ng,
                                 ComponentFinder* cf)
    : nodegraph(ng), componentFinder(cf)
{
    std::vector<std::vector<AtomNodePtr> > weakComponents;

    const std::vector<AtomNodePtr> allnodes = nodegraph.getNodes();

    Subgraph* subgraph = new Subgraph;

    std::vector<std::vector<AtomNodePtr> > strongComponents;

    //
    // keep track of the nodes that belong to a SCC
    //
    std::vector<AtomNodePtr> visited;

    //
    // find all strong components
    //
    getStrongComponents(allnodes, strongComponents);

    //
    // go through strong components
    //
    for (std::vector<std::vector<AtomNodePtr> >::const_iterator scc = strongComponents.begin();
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
            subgraph->addComponent(comp);

            //
            // mark these scc nodes as visited
            // TODO: this is not so nice here
            //
            for (std::vector<AtomNodePtr>::const_iterator ni = (*scc).begin();
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
    for (std::vector<AtomNodePtr>::const_iterator weaknode = allnodes.begin();
            weaknode != allnodes.end();
            ++weaknode)
    {
        //
        // add atomnodes to subgraph!
        //
        subgraph->addNode(*weaknode);

        if (find(visited.begin(), visited.end(), *weaknode) == visited.end())
        {
            if (typeid(*((*weaknode)->getAtom())) == typeid(ExternalAtom))
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
                subgraph->addComponent(comp);

            }
        }
    }
    

    if (Globals::Instance()->doVerbose(Globals::DUMP_DEPENDENCY_GRAPH))
    {
        subgraph->dump(Globals::Instance()->getVerboseStream());
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
    for (std::vector<Subgraph*>::const_iterator si = subgraphs.begin();
         si != subgraphs.end();
         ++si)
    {
        delete *si;
    }
}





//
// TODO: this function is actually meant for sccs - otherwise we would have to
// go through succeeding as well!
//
bool
DependencyGraph::hasNegEdge(const std::vector<AtomNodePtr>& nodes)
{
    for (std::vector<AtomNodePtr>::const_iterator ni = nodes.begin();
         ni != nodes.end();
         ++ni)
    {
        //
        // since an SCC is always cyclic, we only have to consider preceding,
        // not preceding AND succeeding!
        //
        for (std::set<Dependency>::const_iterator di = (*ni)->getPreceding().begin();
                di != (*ni)->getPreceding().end();
                ++di)
        {
            if (((*di).getType() == Dependency::NEG_PRECEDING) ||
               ((*di).getType() == Dependency::DISJUNCTIVE))
                //
                // a scc has a negated edge only if the "target" of the edge is also in the cycle!
                //
                if (find(nodes.begin(), nodes.end(), (*di).getAtomNode()) != nodes.end())
                    return true;
        }
    }

    return false;
}


bool
DependencyGraph::isExternal(const std::vector<AtomNodePtr>& nodes)
{
    for (std::vector<AtomNodePtr>::const_iterator ni = nodes.begin();
         ni != nodes.end();
         ++ni)
    {
        if (typeid(*(*ni)->getAtom()) == typeid(ExternalAtom))
            return true;
    }

    return false;
}



/*
void
DependencyGraph::getWeakComponents(const std::vector<AtomNodePtr>& nodes,
                      std::vector<std::vector<AtomNodePtr> >& wccs)
{
//    componentFinder->findWeakComponents(nodes, wccs);
}
*/


void
DependencyGraph::getStrongComponents(const std::vector<AtomNodePtr>& nodes,
                        std::vector<std::vector<AtomNodePtr> >& sccs)
{
    componentFinder->findStrongComponents(nodes, sccs);
}


std::vector<Component*>
DependencyGraph::getComponents() const
{
    return components;
}


Subgraph*
DependencyGraph::getNextSubgraph()
{
    if (currentSubgraph != subgraphs.end())
        return *(currentSubgraph++);

    return NULL;
}


