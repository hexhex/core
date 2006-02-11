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


void
Component::getResult(std::vector<AtomSet>& r)
{
    r = result;
    /*
    r.clear();

    for (std::vector<GAtomSet>::iterator gi = result.begin();
         gi != result.end();
         ++gi)
    {
        r.push_back(&(*gi));
    }
    */

/*    if (input.size() == 0)
    {
        input.push_back(in);
    }
    else
    {
        std::vector<GAtomSet> newinput;

        GAtomSet un;

        for (std::vector<GAtomSet>::const_iterator gi = input.begin();
             gi != input.end();
             ++gi)
        {
//            for (std::vector<GAtomSet>::const_iterator ngi = in.begin();
//                 ngi != in.end();
//                 ++ngi)
//            {
                un.clear();

                set_union(gi->begin(), gi->end(),
                          in.begin(), in.end(),
                          un.begin());

                newinput.push_back(un);
//            }
        }

        input = newinput;
    }
    */
}



ProgramComponent::ProgramComponent(const std::vector<AtomNode*>& nodes,
                                   ModelGenerator* mg)
    : modelGenerator(mg)
{
    //
    // go thorugh all nodes
    //
    for (std::vector<AtomNode*>::const_iterator ni = nodes.begin();
         ni != nodes.end();
         ++ni)
    {
        //
        // add all rules from this node to the component
        //
        for (std::vector<const Rule*>::const_iterator ri = (*ni)->getRules().begin();
                ri != (*ni)->getRules().end();
                ++ri)
        {
            program.addRule(*ri);

        }

        //
        // add this node to the component-object
        //
        addAtomNode(*ni);
    }
}


ProgramComponent::ProgramComponent(Program& p, ModelGenerator* mg)
//ProgramComponent::ProgramComponent(ModelGenerator* mg)
    : program(p),
      modelGenerator(mg),
      Component()
{
    mg->initialize(program);
}


ProgramComponent::~ProgramComponent()
{
    if (modelGenerator != NULL)
        delete modelGenerator;
}


void
ProgramComponent::setProgram(Program& p)
{
    program = p;
}


void
ProgramComponent::evaluate(std::vector<AtomSet>& input)
{
    if (global::optionVerbose)
    {
        std::cout << "Evaluating program component:" << std::endl;
        program.dump(std::cout);
    }

    std::vector<AtomSet> res, previous;

    //
    // compute model for each input factset
    //
    for (std::vector<AtomSet>::const_iterator in = input.begin();
         in != input.end();
         ++in)
    {
        if (global::optionVerbose)
        {
            std::cout << "Input set: ";
            //printGAtomSet(*in, std::cout, 0);
            (*in).print(std::cout, 0);
            std::cout << std::endl;
        }

        res.clear();

        try
        {
            modelGenerator->compute(program, *in, res);
        }
        catch (GeneralError&)
        {
            throw;
        }

        result.insert(result.end(), res.begin(), res.end());
    }

    if (global::optionVerbose)
    {
        std::cout << "Result set(s): ";

        for (std::vector<AtomSet>::const_iterator out = result.begin();
            out != result.end();
            ++out)
        {
            //printGAtomSet(*out, std::cout, 0);
            (*out).print(std::cout, 0);
            std::cout << std::endl;
        }

        std::cout << std::endl;
    }

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

    out << "Program:" << std::endl;

    program.dump(out);

    out << "ProgramComponent-object end ----------------------------" << std::endl;

//    out << std::endl;
}



ExternalComponent::ExternalComponent(AtomNode* node)
    : externalAtom((ExternalAtom*)node->getAtom())
{
}

void
ExternalComponent::evaluate(std::vector<AtomSet>& input)
{
    if (global::optionVerbose)
    {
        std::cout << "Evaluating external component" << std::endl;
    }

    //
    // compute model for each input factset
    //
    for (std::vector<AtomSet>::const_iterator in = input.begin();
         in != input.end();
         ++in)
    {
        if (global::optionVerbose)
        {
            std::cout << "Input set: ";
            //printGAtomSet(*in, std::cout, 0);
            (*in).print(std::cout, 0);
            std::cout << std::endl;
        }

        AtomSet res;

        //res.clear();

        AtomSet i(*in);

        std::vector<Tuple> inputArguments;

        //
        // first, we assume that we only take the original input list
        //
        inputArguments.push_back(externalAtom->getInputTerms());

        //
        // if we had an auxiliary rule because of variable input arguments, we
        // have to build the ground input list(s) now:
        //

        std::vector<Dependency> pred((*atomnodes.begin())->getPreceding());

        for (std::vector<Dependency>::const_iterator di = pred.begin();
             di != pred.end();
             ++di)
        {
            if ((*di).getType() == Dependency::EXTERNAL_AUX)
            {
                //
                // now that we know there are variable input arguments
                // (otherwise there wouldn't be such a dependency), we can start
                // over with the input list again and construct it from the
                // result of the auxiliary rules
                //
                inputArguments.clear();

                //
                // find out the name of the aux-rule head atom
                //
                Term auxname = (*di).getAtomNode()->getAtom()->getPredicate();

                AtomSet arglist;

                //
                // get all the facts from i that match the auxiliary head atom
                //
                i.matchPredicate(auxname.getString(), arglist);

                for (AtomSet::const_iterator argi = arglist.begin();
                     argi != arglist.end();
                     ++argi)
                {
                    inputArguments.push_back((*argi).getArguments());
                }
            }
        }

        try
        {
            //
            // evaluate external atom for each input list we have now
            //
            for (std::vector<Tuple>::const_iterator inputi = inputArguments.begin();
                 inputi != inputArguments.end();
                 ++inputi)
            {
                AtomSet r;
                
//                std::cout << "calling ext reasoner with params " << *inputi << std::endl;

                externalAtom->evaluate(i, *inputi, r);

//                std::cout << "result:" << std::endl;
//                printGAtomSet(r, std::cout, 0);

                res.insert(r);

                //
                // important: the component result must include also its input
                // (like the EDB, that is always included in the result). This
                // is due to our graphprocessor algorithm.
                /// @TODO: think about this!
                //
                res.insert(*in);
                //res.insert((*in).begin(), (*in).end());
            }

        }
        catch (GeneralError&)
        {
            throw;
        }

        result.push_back(res);
    }

    if (global::optionVerbose)
    {
        std::cout << "Result set(s): ";

        for (std::vector<AtomSet>::const_iterator out = result.begin();
            out != result.end();
            ++out)
        {
            //printGAtomSet(*out, std::cout, 0);
            (*out).print(std::cout, 0);
            std::cout << std::endl;
        }

        std::cout << std::endl;
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
        //std::cout << "adding node " << n->getId() << " to the collectup list" << std::endl;

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

        //std::cout << "prune: removing node " << **ni << std::endl;
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
    std::cout << "looking for predecessors of" << std::endl;
    comp->dump(std::cout);

    Component* c;

    std::vector<Component*> pred;

    std::vector<const AtomNode*> compnodes = comp->getNodes();

    for (std::vector<const AtomNode*>::const_iterator ni = compnodes.begin();
         ni != compnodes.end();
         ++ni)
    {
        std::cout << "looking at " << **ni <<std::endl;
        //
        // go through all predecessors of this node
        //
        for (std::vector<Dependency>::const_iterator d = (*ni)->getPreceding().begin();
            d != (*ni)->getPreceding().end();
            ++d)
        {
            std::cout << "has pred " << *((*d).getAtomNode()) << std::endl;
            //
            // for each preceding node:
            // if the node belongs to a component, get the component this node belongs to
            //
            if (nodeComponentMap.find((*d).getAtomNode()) != nodeComponentMap.end())
            {

                c = nodeComponentMap[(*d).getAtomNode()];
                //std::cout << "belongs to comp" << std::endl; c->dump(std::cout);

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
                //std::cout << "looking at " << **ni <<std::endl;
                //
                // go through all predecessors of this node
                //
                for (std::vector<Dependency>::const_iterator d = (*ni)->getPreceding().begin();
                    d != (*ni)->getPreceding().end();
                    ++d)
                {
                    //std::cout << "has pred " << *((*d).getAtomNode()) << std::endl;
                    //
                    // for each preceding node:
                    // if the node belongs to a component, get the component this node belongs to
                    //
                    if (nodeComponentMap.find((*d).getAtomNode()) != nodeComponentMap.end())
                    {

                        c = nodeComponentMap[(*d).getAtomNode()];
                        //std::cout << "belongs to comp" << std::endl; c->dump(std::cout);

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





/*
            for (std::vector<Component*>::const_iterator pi = pred.begin();
                 pi != pred.end();
                 ++pi)
            {
                if (!(*pi)->isSolved())
                    allsolved = false;
            }
*/
            //
            // so *ci is unsolved, but all preceding are solved - then it is a leaf!
            //
            if (isLeaf)
                leaves.push_back(*ci);
        }
    }
}


/*
void
Subgraph::evaluateComponent(Component* comp,
                            std::vector<const GAtomSet>& input)
{
    comp->evaluate(input);

    comp->getResult(lastResult);
}
*/

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
    std::cout << "Subgraph components:" << std::endl;

    for (std::vector<Component*>::const_iterator ci = components.begin();
         ci != components.end();
         ci++)
    {
        (*ci)->dump(out);
    }

    std::cout << std::endl;

    std::cout << "Subgraph nodes:" << std::endl;

    for (std::vector<AtomNode*>::const_iterator ni = atomnodes.begin();
         ni != atomnodes.end();
         ni++)
    {
        out << **ni << std::endl;;
    }

    std::cout << std::endl;
}


