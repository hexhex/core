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

#include "dlvhex/Component.h"
#include "dlvhex/ProgramBuilder.h"
#include "dlvhex/ASPsolver.h"
#include "dlvhex/globals.h"



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
Component::addAtomNode(const AtomNode* atomnode)
{
    atomnodes.push_back(atomnode);
}



const std::vector<const AtomNode*>&
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
    std::vector<const AtomNode*>::const_iterator node = atomnodes.begin();

    while (node != atomnodes.end())
    {
        //
        // add all rules from this node to the component
        //
        for (std::vector<const Rule*>::const_iterator ri = (*node)->getRules().begin();
                ri != (*node)->getRules().end();
                ++ri)
        {
            program.addRule(*ri);
        }

        node++;
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

    std::vector<const AtomNode*>::const_iterator nodeit = atomnodes.begin();

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


ProgramComponent::ProgramComponent(const std::vector<AtomNode*>& nodes,
                                   ModelGenerator* mg)
    : Component(),
      modelGenerator(mg)
{
    std::vector<AtomNode*>::const_iterator node = nodes.begin();

    while (node != nodes.end())
        addAtomNode(*node++);

    //
    // find incoming nodes: nodes that depend on a node that does not belong to
    // the component
    //
    ///todo: this is not used yet, incoming are not enough, facts are not
    //recorded and could be relevant as well!
    //
    
    std::vector<const AtomNode*>::const_iterator ni = atomnodes.begin();

    while (ni != atomnodes.end())
    {
        //
        // go through all nodes that this node depends on
        //
        std::vector<Dependency> deps = (*ni)->getPreceding();

        std::vector<Dependency>::const_iterator di = deps.begin();

        while (di != deps.end())
        {
            const AtomNode* other = (*di).getAtomNode();

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
        getBottom().dump(std::cerr);
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

        std::list<const AtomNode*>::const_iterator ini = incomingNodes.begin();

        //while (ini != incomingNodes.end())
        //    (*in).matchAtom((*ini++)->getAtom(), filtered);

        /*
        if (global::optionVerbose)
        {
            std::cerr << "Input set: ";
            ///not used yet: filtered.print(std::cerr, 0);
            (*in).print(std::cerr, 0);
            std::cerr << std::endl;
        }
        */

        res.clear();

        try
        {
            modelGenerator->compute(atomnodes, (*in), res);
            //modelGenerator->compute(atomnodes, filtered, res);
        }
        catch (GeneralError&)
        {
            throw;
        }

//        std::vector<AtomSet>::iterator resi = res.begin();
//        while (resi != res.end())
//            (*resi++).insert(*in);

        //
        // add all models that came from the model generator to the result set
        // (= set of sets!)
        //
        result.insert(result.end(), res.begin(), res.end());
    }

    /*
    if (global::optionVerbose)
    {
        std::cerr << "Result set(s):" << std::endl;

        for (std::vector<AtomSet>::const_iterator out = result.begin();
            out != result.end();
            ++out)
        {
            std::cerr << "  ";
            (*out).print(std::cerr, 0);
            std::cerr << std::endl;
        }

        std::cerr << std::endl;
    }
    */

    evaluated = true;
}


void
ProgramComponent::dump(std::ostream& out) const
{
    out << "ProgramComponent-object --------------------------------" << std::endl;
    out << "Nodes:";

    for (std::vector<const AtomNode*>::const_iterator ni = atomnodes.begin();
         ni != atomnodes.end();
         ++ni)
    {
        out << " " << (*ni)->getId();
    }

    out << std::endl;

    out << "Bottom:" << std::endl;

    getBottom().dump(out);

    out << "ProgramComponent-object end ----------------------------" << std::endl;

//    out << std::endl;
}



ExternalComponent::ExternalComponent(AtomNode* node)
    : externalAtom(dynamic_cast<ExternalAtom*>(node->getAtom().get()))
{
}

void
ExternalComponent::evaluate(std::vector<AtomSet>& input)
{
    if (Globals::Instance()->doVerbose(Globals::COMPONENT_EVALUATION))
    {
        std::cerr << "Evaluating external component" << std::endl;
    }

    //
    // compute model for each input factset
    //
    for (std::vector<AtomSet>::const_iterator in = input.begin();
         in != input.end();
         ++in)
    {
        /*
        if (global::optionVerbose)
        {
            std::cerr << "Input set: ";
            (*in).print(std::cerr, 0);
            std::cerr << std::endl;
        }
        */

        AtomSet res;

        AtomSet i(*in);

        try
        {
            externalAtom->evaluate(i, res);
        }
        catch (GeneralError&)
        {
            throw;
        }

        //
        // important: the component result must include also its input
        // (like the EDB, that is always included in the result). This
        // is due to our graphprocessor algorithm.
        /// @todo think about this!
        //
        res.insert(*in);

        result.push_back(res);
    }

    /*
    if (global::optionVerbose)
    {
        std::cerr << "Result set(s): ";

        for (std::vector<AtomSet>::const_iterator out = result.begin();
            out != result.end();
            ++out)
        {
            //printGAtomSet(*out, std::cerr, 0);
            (*out).print(std::cerr, 0);
            std::cerr << std::endl;
        }

        std::cerr << std::endl;
    }
    */

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

    std::vector<const AtomNode*> compnodes = c->getNodes();

    for (std::vector<const AtomNode*>::const_iterator ni = compnodes.begin();
         ni != compnodes.end();
         ++ni)
    {
        nodeComponentMap[*ni] = c;
    }
}


void
Subgraph::addNode(AtomNode* an)
{
    atomnodes.push_back(an);
}


void
Subgraph::collectUp(const AtomNode* n,
                   std::vector<const AtomNode*>& list)
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
        for (std::vector<Dependency>::const_iterator d = n->getSucceeding().begin();
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
    std::vector<const AtomNode*> toBeRemoved;

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
            for (std::vector<const AtomNode*>::const_iterator ni = (*ci)->getNodes().begin();
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
            for (std::vector<const AtomNode*>::const_iterator ni = (*ci)->getNodes().begin();
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

    for (std::vector<const AtomNode*>::const_iterator ni = toBeRemoved.begin();
        ni != toBeRemoved.end();
        ++ni)
    {
        removeNode(*ni);

        //std::cerr << "prune: removing node " << **ni << std::endl;
    }

    
}


void
Subgraph::removeNode(const AtomNode* an)
{
    atomnodes.erase(find(atomnodes.begin(), atomnodes.end(), an));
}


const std::vector<AtomNode*>&
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

    std::vector<const AtomNode*> compnodes = comp->getNodes();

    for (std::vector<const AtomNode*>::const_iterator ni = compnodes.begin();
         ni != compnodes.end();
         ++ni)
    {
        std::cerr << "looking at " << **ni <<std::endl;
        //
        // go through all predecessors of this node
        //
        for (std::vector<Dependency>::const_iterator d = (*ni)->getPreceding().begin();
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

    std::vector<const AtomNode*> compnodes = comp->getNodes();

    for (std::vector<const AtomNode*>::const_iterator ni = compnodes.begin();
         ni != compnodes.end();
         ++ni)
    {
        //
        // go through all succecessors of this node
        //
        for (std::vector<Dependency>::const_iterator d = (*ni)->getSucceeding().begin();
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

            std::vector<const AtomNode*> compnodes = (*ci)->getNodes();

            for (std::vector<const AtomNode*>::const_iterator ni = compnodes.begin();
                ni != compnodes.end();
                ++ni)
            {
                //std::cerr << "looking at " << **ni <<std::endl;
                //
                // go through all predecessors of this node
                //
                for (std::vector<Dependency>::const_iterator d = (*ni)->getPreceding().begin();
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

    for (std::vector<AtomNode*>::const_iterator ni = atomnodes.begin();
         ni != atomnodes.end();
         ni++)
    {
        out << **ni << std::endl;;
    }

    out << std::endl;
}


