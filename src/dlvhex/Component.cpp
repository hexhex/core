/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* -*- C++ -*- */

/**
 * @file Component.cpp
 * @author Roman Schindlauer
 * @date Mon Sep 19 13:14:56 CEST 2005
 *
 * @brief Conponent and Subgraph Class.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef DLVHEX_DEBUG
#include <boost/date_time/posix_time/posix_time.hpp>
#endif // DLVHEX_DEBUG

#include "dlvhex/Component.h"
#include "dlvhex/ProgramBuilder.h"
#include "dlvhex/ASPsolver.h"
#include "dlvhex/globals.h"
#include "dlvhex/PrintVisitor.h"



Component::Component()
    : evaluated(false)
{
}


Component::~Component()
{
}


bool
Component::isSolved() const
{
    return evaluated;
}


void
Component::addAtomNode(const AtomNodePtr atomnode)
{
    atomnodes.push_back(atomnode);
}



const std::vector<AtomNodePtr>&
Component::getNodes() const
{
    return atomnodes;
}


Program
Component::getBottom() const
{
    Program program;

    //
    // go through all nodes
    //
    std::vector<AtomNodePtr>::const_iterator node = atomnodes.begin();

    while (node != atomnodes.end())
    {
        const std::vector<Rule*>& rules = (*node)->getRules();

        //
        // add all rules from this node to the component
        //
        for (std::vector<Rule*>::const_iterator ri = rules.begin();
                ri != rules.end();
                ++ri)
        {
            program.addRule(*ri);
        }

        ++node;
    }

    return program;
}


void
Component::getResult(std::vector<AtomSet>& r)
{
    r = result;
}


bool
Component::isInComponent(const Atom* at) const
{
    bool belongsToComp = false;

    std::vector<AtomNodePtr>::const_iterator nodeit = atomnodes.begin();

    while (nodeit != atomnodes.end())
    {
        if ((*nodeit++)->getAtom().get() == at)
        {
            belongsToComp = true;

            break;
        }
    }

    return belongsToComp;
}


ProgramComponent::ProgramComponent(const std::vector<AtomNodePtr>& nodes,
                                   ModelGenerator* mg)
    : Component(),
      modelGenerator(mg)
{
    std::vector<AtomNodePtr>::const_iterator node = nodes.begin();

    while (node != nodes.end())
        addAtomNode(*node++);

    //
    // find incoming nodes: nodes that depend on a node that does not belong to
    // the component
    //
    ///todo: this is not used yet, incoming are not enough, facts are not
    //recorded and could be relevant as well!
    //
    
    std::vector<AtomNodePtr>::const_iterator ni = atomnodes.begin();

    while (ni != atomnodes.end())
    {
        //
        // go through all nodes that this node depends on
        //
        std::set<Dependency> deps = (*ni)->getPreceding();

        std::set<Dependency>::const_iterator di = deps.begin();

        while (di != deps.end())
        {
            const AtomNodePtr other = (*di).getAtomNode();

            //
            // if this other node is not in our component, then the current node
            // is an 'incoming' node
            //
            if (find(atomnodes.begin(), atomnodes.end(), other) == atomnodes.end())
            {
                incomingNodes.push_back(other);
                //std::cerr << "incoming node: " << *other << std::endl;
            }

            ++di;
        }

        ++ni;
    }
}



ProgramComponent::~ProgramComponent()
{
    if (modelGenerator != NULL)
        delete modelGenerator;
}


void
ProgramComponent::evaluate(std::vector<AtomSet>& input)
{
    if (Globals::Instance()->doVerbose(Globals::COMPONENT_EVALUATION))
    {
        std::cerr << "Evaluating program component:" << std::endl;
        RawPrintVisitor rpv(std::cerr);
        getBottom().dump(rpv);
    }

    std::vector<AtomSet> res, previous;

    //
    // compute model for each input factset
    //
    for (std::vector<AtomSet>::const_iterator in = input.begin();
         in != input.end();
         ++in)
    {
        AtomSet filtered;

        std::list<AtomNodePtr>::const_iterator ini = incomingNodes.begin();

        res.clear();

        try
        {
            modelGenerator->compute(atomnodes, (*in), res);
        }
        catch (GeneralError&)
        {
            throw;
        }

#ifdef DLVHEX_DEBUG
        DEBUG_START_TIMER
#endif // DLVHEX_DEBUG

        //
        // add all models that came from the model generator to the result set
        // (= set of sets!)
        //
        result.insert(result.end(), res.begin(), res.end());

#ifdef DLVHEX_DEBUG
		//                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
        DEBUG_STOP_TIMER("Program-component result insert time   ")
#endif // DLVHEX_DEBUG
    }

    evaluated = true;
}


void
ProgramComponent::dump(std::ostream& out) const
{
    out << "ProgramComponent-object --------------------------------" << std::endl;
    out << "Nodes:";

    for (std::vector<AtomNodePtr>::const_iterator ni = atomnodes.begin();
         ni != atomnodes.end();
         ++ni)
    {
        out << " " << (*ni)->getId();
    }

    out << std::endl;

    out << "Bottom:" << std::endl;

    RawPrintVisitor rpv(std::cerr);
    getBottom().dump(rpv);

    out << "ProgramComponent-object end ----------------------------" << std::endl;

//    out << std::endl;
}



ExternalComponent::ExternalComponent(AtomNodePtr node)
    : externalAtom(dynamic_cast<ExternalAtom*>(node->getAtom().get()))
{
}

void
ExternalComponent::evaluate(std::vector<AtomSet>& input)
{
    //if (Globals::Instance()->doVerbose(Globals::COMPONENT_EVALUATION))
    //    std::cerr << "Evaluating external component" << std::endl;

    //
    // compute model for each input factset
    //
    for (std::vector<AtomSet>::const_iterator in = input.begin();
         in != input.end();
         ++in)
    {
        AtomSet res;

        AtomSet i(*in);

        try
        {
#ifdef DLVHEX_DEBUG
            DEBUG_START_TIMER
#endif // DLVHEX_DEBUG

            externalAtom->evaluate(i, res);

#ifdef DLVHEX_DEBUG
			//                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
		    DEBUG_STOP_TIMER("External evaluation time               ")
#endif // DLVHEX_DEBUG
        }
        catch (GeneralError&)
        {
            throw;
        }

#ifdef DLVHEX_DEBUG
            DEBUG_START_TIMER
#endif // DLVHEX_DEBUG

        //
        // important: the component result must include also its input
        // (like the EDB, that is always included in the result). This
        // is due to our graphprocessor algorithm.
        /// @todo think about this!
        //
        res.insert(*in);

        result.push_back(res);

//	    std::cerr << "got: ";
//		RawPrintVisitor rpv(Globals::Instance()->getVerboseStream());
//		res.accept(rpv);
//	    std::cerr << std::endl;

#ifdef DLVHEX_DEBUG
		//                123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
		DEBUG_STOP_TIMER("External result insert time            ")
#endif // DLVHEX_DEBUG
    }

    evaluated = true;
}


void
ExternalComponent::dump(std::ostream& out) const
{
    out << "ExternalComponent-object with node ";

    //
    // an external component can have only one node
    //
    out << (*atomnodes.begin())->getId() << std::endl;

    out << std::endl;
}


Subgraph::Subgraph()
{
    atomnodes.clear();

    components.clear();
}


Subgraph::~Subgraph()
{
    for (std::vector<Component*>::const_iterator ci = components.begin();
         ci != components.end();
         ++ci)
    {
        delete *ci;
    }
}


Subgraph::Subgraph(const Subgraph& sg2)
{
    atomnodes = sg2.atomnodes;

    components = sg2.components;

    nodeComponentMap = sg2.nodeComponentMap;

    lastResult = sg2.lastResult;
}


void
Subgraph::addComponent(Component* c)
{
    components.push_back(c);

    //
    // store also which node belongs to which component, we need that later
    //

    std::vector<AtomNodePtr> compnodes = c->getNodes();

    for (std::vector<AtomNodePtr>::const_iterator ni = compnodes.begin();
         ni != compnodes.end();
         ++ni)
    {
        nodeComponentMap[*ni] = c;
    }
}


void
Subgraph::addNode(AtomNodePtr an)
{
    atomnodes.push_back(an);
}


void
Subgraph::collectUp(const AtomNodePtr n,
                   std::vector<AtomNodePtr>& list)
{
    //
    // did we add this node to our list already?
    //
    if (find(list.begin(), list.end(), n) == list.end())
    {
        //std::cerr << "adding node " << n->getId() << " to the collectup list" << std::endl;

        list.push_back(n);

        //
        // go through all succecessors of this node
        //
        for (std::set<Dependency>::const_iterator d = n->getSucceeding().begin();
            d != n->getSucceeding().end();
            ++d)
        {
            collectUp((*d).getAtomNode(), list);
        }
    }
}


void
Subgraph::pruneComponents()
{
    std::vector<AtomNodePtr> toBeRemoved;

    //
    // go through all components of the subgraph
    //
    for (std::vector<Component*>::const_iterator ci = components.begin();
         ci != components.end();
         ++ci)
    {
        if (!(*ci)->isSolved())
        {
            //
            // remove all nodes of this component and all nodes that depend on them
            //
            for (std::vector<AtomNodePtr>::const_iterator ni = (*ci)->getNodes().begin();
                ni != (*ci)->getNodes().end();
                ++ni)
            {
                collectUp(*ni, toBeRemoved);
            }
        }
        else
        {
            //
            // if the component was solved, remove it, too
            //
            for (std::vector<AtomNodePtr>::const_iterator ni = (*ci)->getNodes().begin();
                ni != (*ci)->getNodes().end();
                ++ni)
            {
                toBeRemoved.push_back(*ni);
            }
        }
    }

    //
    // now we can remove all components from the subgraph
    //
    components.clear();

    for (std::vector<AtomNodePtr>::const_iterator ni = toBeRemoved.begin();
        ni != toBeRemoved.end();
        ++ni)
    {
        removeNode(*ni);

        //std::cerr << "prune: removing node " << **ni << std::endl;
    }

    
}


void
Subgraph::removeNode(const AtomNodePtr an)
{
    atomnodes.erase(find(atomnodes.begin(), atomnodes.end(), an));
}


const std::vector<AtomNodePtr>&
Subgraph::getNodes() const
{
    return atomnodes;
}


std::vector<Component*>
Subgraph::getPredecessors(Component* comp)
{
    std::cerr << "looking for predecessors of" << std::endl;
    comp->dump(std::cerr);

    Component* c;

    std::vector<Component*> pred;

    std::vector<AtomNodePtr> compnodes = comp->getNodes();

    for (std::vector<AtomNodePtr>::const_iterator ni = compnodes.begin();
         ni != compnodes.end();
         ++ni)
    {
        std::cerr << "looking at " << **ni <<std::endl;
        //
        // go through all predecessors of this node
        //
        for (std::set<Dependency>::const_iterator d = (*ni)->getPreceding().begin();
            d != (*ni)->getPreceding().end();
            ++d)
        {
            std::cerr << "has pred " << *((*d).getAtomNode()) << std::endl;
            //
            // for each preceding node:
            // if the node belongs to a component, get the component this node belongs to
            //
            if (nodeComponentMap.find((*d).getAtomNode()) != nodeComponentMap.end())
            {

                c = nodeComponentMap[(*d).getAtomNode()];
                //std::cerr << "belongs to comp" << std::endl; c->dump(std::cerr);

                //
                // don't add this component itself
                //
                if (comp != c)
                {
                    //
                    // did we find this component already?
                    //
                    if (find(pred.begin(), pred.end(), c) == pred.end())
                        pred.push_back(c);
                }
            }
        }

    }

    return pred;
}


std::vector<Component*>
Subgraph::getSuccessors(Component* comp)
{
    Component* c;

    std::vector<Component*> succ;

    std::vector<AtomNodePtr> compnodes = comp->getNodes();

    for (std::vector<AtomNodePtr>::const_iterator ni = compnodes.begin();
         ni != compnodes.end();
         ++ni)
    {
        //
        // go through all succecessors of this node
        //
        for (std::set<Dependency>::const_iterator d = (*ni)->getSucceeding().begin();
            d != (*ni)->getSucceeding().end();
            ++d)
        {
            //
            // for each succeeding node - get the component this node belongs to
            //
            if (nodeComponentMap.find((*d).getAtomNode()) == nodeComponentMap.end())
                assert(0);

            c = nodeComponentMap[(*d).getAtomNode()];

            //
            // did we find this component already?
            //
            if (find(succ.begin(), succ.end(), c) == succ.end())
                succ.push_back(c);
        }

    }

    return succ;
}


void
Subgraph::getUnsolvedLeaves(std::vector<Component*>& leaves)
{
    //
    // TODO: this is not very efficient! maybe we can store these leaves on the fly
    // somewhere else!
    //


    for (std::vector<Component*>::const_iterator ci = components.begin();
         ci != components.end();
         ++ci)
    {
        bool isLeaf = true;

        //
        // only look for unsolved components
        //
        if (!(*ci)->isSolved())
        {
            //
            // does it have any unsolved incoming components?
            //
            
    //        bool allsolved = true;

            //std::vector<Component*> pred = getPredecessors(*ci);

            Component* c;

            std::vector<Component*> pred;

            std::vector<AtomNodePtr> compnodes = (*ci)->getNodes();

            for (std::vector<AtomNodePtr>::const_iterator ni = compnodes.begin();
                ni != compnodes.end();
                ++ni)
            {
                //std::cerr << "looking at " << **ni <<std::endl;
                //
                // go through all predecessors of this node
                //
                for (std::set<Dependency>::const_iterator d = (*ni)->getPreceding().begin();
                    d != (*ni)->getPreceding().end();
                    ++d)
                {
                    //std::cerr << "has pred " << *((*d).getAtomNode()) << std::endl;
                    //
                    // for each preceding node:
                    // if the node belongs to a component, get the component this node belongs to
                    //
                    if (nodeComponentMap.find((*d).getAtomNode()) != nodeComponentMap.end())
                    {

                        c = nodeComponentMap[(*d).getAtomNode()];
                        //std::cerr << "belongs to comp" << std::endl; c->dump(std::cerr);

                        //
                        // don't add this component itself
                        //
                        if ((*ci) != c)
                        {
                            //
                            // did we find this component already?
                            //
                //            if (find(pred.begin(), pred.end(), c) == pred.end())
                //                pred.push_back(c);
                            
                            if (!c->isSolved())
                            {
                                isLeaf = false;

                                break;
                            }
                        }
                    }
                    else
                    {
                        isLeaf = false;

                        break;
                    }
                }

            }

            //
            // so *ci is unsolved, but all preceding are solved - then it is a leaf!
            //
            if (isLeaf)
                leaves.push_back(*ci);
        }
    }
}



bool
Subgraph::unsolvedComponentsLeft()
{
    for (std::vector<Component*>::const_iterator ci = components.begin();
         ci != components.end();
         ++ci)
    {
        if (!(*ci)->isSolved())
            return true;
    }

    return false;
}

std::vector<AtomSet*>&
Subgraph::getLastResult()
{
    return lastResult;
}


void
Subgraph::dump(std::ostream& out) const
{
    out << "Subgraph components:" << std::endl;

    for (std::vector<Component*>::const_iterator ci = components.begin();
         ci != components.end();
         ci++)
    {
        (*ci)->dump(out);
    }

    out << std::endl;

    out << "Subgraph nodes:" << std::endl;

    for (std::vector<AtomNodePtr>::const_iterator ni = atomnodes.begin();
         ni != atomnodes.end();
         ni++)
    {
        out << **ni << std::endl;;
    }

    out << std::endl;
}


