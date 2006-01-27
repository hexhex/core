/* -*- C++ -*- */

/**
 * @file GraphBuilder.cpp
 * @author Roman Schindlauer
 * @date Wed Jan 18 17:43:14 CET 2006
 *
 * @brief Abstract strategy class for finding the dependency edges of a program.
 *
 *
 */

#include "dlvhex/GraphBuilder.h"
#include "dlvhex/Component.h"
#include "dlvhex/globals.h"




void
GraphBuilder::addDep(AtomNode* from, AtomNode* to, Dependency::Type type)
{
    Dependency dep1(from, type);
    Dependency dep2(to, type);

    from->addSucceeding(dep2);
    to->addPreceding(dep1);
}



void
GraphBuilder::run(const Rules& rules, NodeGraph& nodegraph)
{
    //
    // in this multimap, we will store the input arguments of type PREDICATE
    // of the external atoms. see below.
    //
    std::multimap<Term, AtomNode*> extinputs;

    for (Rules::const_iterator r = rules.begin();
         r != rules.end();
         r++)
    {
        //
        // all nodes of the current rule's head
        //
        std::vector<AtomNode*> currentHeadNodes;

        //
        // go through entire head disjunction
        //
        for (std::vector<Atom>::const_iterator hi = r->getHead().begin();
             hi != r->getHead().end();
             ++hi)
        {
            //
            // add a head atom node. This function will take care of also adding the appropriate
            // unifying dependency for all existing nodes.
            //
            AtomNode* hn = nodegraph.addUniqueHeadNode(&(*hi));

            //
            // go through all head atoms that were alreay created for this rule
            //
            for (std::vector<AtomNode*>::iterator currhead = currentHeadNodes.begin();
                 currhead != currentHeadNodes.end();
                 ++currhead)
            {
                //
                // and add disjunctive dependency
                //
                addDep(hn, *currhead, Dependency::DISJUNCTIVE);
                addDep(*currhead, hn, Dependency::DISJUNCTIVE);
            }

            //
            // add this atom to current head
            //
            currentHeadNodes.push_back(hn);
        }


        //
        // go through rule body
        //
        for (std::vector<Literal>::const_iterator li = r->getBody().begin();
                li != r->getBody().end();
                ++li)
        {

            //
            // add a body atom node. This function will take care of also adding the appropriate
            // unifying dependency for all existing nodes.
            //
            AtomNode* bn = nodegraph.addUniqueBodyNode(li->getAtom());

            //
            // add dependency from this body atom to each head atom
            //
            for (std::vector<AtomNode*>::iterator currhead = currentHeadNodes.begin();
                 currhead != currentHeadNodes.end();
                 ++currhead)
            {
                if (li->isNAF())
                    addDep(bn, *currhead, Dependency::NEG_PRECEDING);
                else
                    addDep(bn, *currhead, Dependency::PRECEDING);

                //
                // if an external atom is in the body, we have to take care of the
                // external dependencies - between its arguments (but only those of type
                // PREDICATE) and any other atom in the program that matches this argument.
                //
                // What we iwll do here is to build a multimap, which stores each input
                // predicate symbol together with the AtomNode of this external atom.
                // If we are through all rules, we will go through the complete set
                // of AtomNodes and search for matches with this multimap.
                //
                if (li->getAtom()->getType() == Atom::EXTERNAL)
                {
                    ExternalAtom* ext = (ExternalAtom*)li->getAtom();

                    //
                    // go through all input terms of this external atom
                    //
                    for (int s = 0; s < ext->getInputTerms().size(); s++)
                    {
                        //
                        // consider only PREDICATE input terms (naturally, for constant
                        // input terms we won't have any dependencies!)
                        //
                        if (ext->getInputType(s) == PluginAtom::PREDICATE)
                        {
                            //
                            // store the AtomNode of this external atom together will
                            // all the predicate input terms
                            //
                            // e.g., if we had an external atom '&ext[a,b](X)', where
                            // 'a' is of type PREDICATE, and the atom was store in Node n1,
                            // then the map will get an entry <'a', n1>. Below, we will
                            // then search for those AtomNodes with a Predicate 'a' - those
                            // will be assigned a dependency relation with n1!
                            //
                            extinputs.insert(std::pair<Term, AtomNode*>(ext->getInputTerms()[s], bn));
                        }
                    }
                }
            }
        }

        //
        // finally add this rule to each head node:
        //
        for (std::vector<AtomNode*>::iterator currhead = currentHeadNodes.begin();
                currhead != currentHeadNodes.end();
                ++currhead)
        {
            (*currhead)->addRule(&(*r));
        }

    }
    
    //
    // Now we will build the EXTERNAL dependencies:
    //
    typedef std::multimap<Term, AtomNode*>::iterator mi;

    //
    // Go through all AtomNodes
    //
    for (std::vector<AtomNode*>::const_iterator node = nodegraph.getNodes().begin();
         node != nodegraph.getNodes().end();
         ++node)
    {
        //
        // For this AtomNode: take the predicate term of its atom and extract all
        // entries in the multimap that match this predicate. Those entries contain
        // now the AtomNodes of the external atoms that have such an input predicate.
        //
        std::pair<mi, mi> range = extinputs.equal_range((*node)->getAtom()->getPredicate());

        //
        // add dependency: from this node to the external atom (second in the pair of the
        // multimap)
        //
        for (mi i = range.first; i != range.second; ++i)
        {
            addDep(*node, i->second, Dependency::EXTERNAL);
        }
    }

}


void
GraphBuilder::dumpGraph(const NodeGraph& nodegraph, std::ostream& out) const
{
    out << "Dependency graph - Program Nodes:" << std::endl;

    for (std::vector<AtomNode*>::const_iterator node = nodegraph.getNodes().begin();
         node != nodegraph.getNodes().end();
         ++node)
    {
        out << **node << std::endl;
    }

    out << std::endl;
}


