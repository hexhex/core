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
#include "dlvhex/Registry.h"


/*
void
GraphBuilder::addDep(AtomNode* from, AtomNode* to, Dependency::Type type)
{
    Dependency dep1(from, type);
    Dependency dep2(to, type);

    from->addSucceeding(dep2);
    to->addPreceding(dep1);
}
*/


void
GraphBuilder::run(const Program& program, NodeGraph& nodegraph)
{
    //
    // in this multimap, we will store the input arguments of type PREDICATE
    // of the external atoms. see below.
    //
    std::multimap<Term, AtomNode*> extinputs;

    //
    // go through all rules of the given program
    //
    for (Program::const_iterator r = program.begin();
         r != program.end();
         ++r)
    {
        //
        // all nodes of the current rule's head
        //
        std::vector<AtomNode*> currentHeadNodes;

        //
        // go through entire head disjunction
        //
        RuleHead_t head = (*r)->getHead();

        for (RuleHead_t::const_iterator hi = head.begin();
             hi != head.end();
             ++hi)
        {
            //
            // add a head atom node. This function will take care of also adding
            // the appropriate unifying dependency for all existing nodes.
            //
            AtomNode* hn = nodegraph.addUniqueHeadNode(*hi);

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
                Dependency::addDep(hn, *currhead, Dependency::DISJUNCTIVE);
                Dependency::addDep(*currhead, hn, Dependency::DISJUNCTIVE);
            }

            //
            // add this atom to current head
            //
            currentHeadNodes.push_back(hn);
        }

        //
        // constraint: create virtual head node to keep the rule
        //
        if (head.size() == 0)
        {
            AtomPtr va = Registry::Instance()->storeAtom(new falseAtom);
            
            AtomNode* vn = nodegraph.addUniqueHeadNode(va);
//            AtomNode* vn = nodegraph.addNode();

            currentHeadNodes.push_back(vn);
        }


        std::vector<AtomNode*> currentOrdinaryBodyNodes;
        std::vector<AtomNode*> currentExternalBodyNodes;

        //
        // go through rule body
        //

        RuleBody_t body = (*r)->getBody();

        for (RuleBody_t::const_iterator li = body.begin();
                li != body.end();
                ++li)
        {

            //
            // builtins do not contribute to dependencies (yet)
            //
            if (typeid(*(*li)->getAtom()) == typeid(BuiltinPredicate))
                continue;

            //
            // add a body atom node. This function will take care of also adding the appropriate
            // unifying dependency for all existing nodes.
            //
            AtomNode* bn = nodegraph.addUniqueBodyNode((*li)->getAtom());

            //
            // save normal and external atoms of this body - after we are through the entire
            // body, we might have to update EXTERNAL dependencies inside the
            // rule and build auxiliary rules!
            //
            if ((typeid(*((*li)->getAtom())) == typeid(Atom)) &&
                (!(*li)->isNAF()))
                currentOrdinaryBodyNodes.push_back(bn);

            if (typeid(*((*li)->getAtom())) == typeid(ExternalAtom))
                currentExternalBodyNodes.push_back(bn);

            //
            // add dependency from this body atom to each head atom
            //
            for (std::vector<AtomNode*>::iterator currhead = currentHeadNodes.begin();
                 currhead != currentHeadNodes.end();
                 ++currhead)
            {
                if ((*li)->isNAF())
                    Dependency::addDep(bn, *currhead, Dependency::NEG_PRECEDING);
                else
                    Dependency::addDep(bn, *currhead, Dependency::PRECEDING);

                //
                // if an external atom is in the body, we have to take care of the
                // external dependencies - between its arguments (but only those of type
                // PREDICATE) and any other atom in the program that matches this argument.
                //
                // What we will do here is to build a multimap, which stores each input
                // predicate symbol together with the AtomNode of this external atom.
                // If we are through all rules, we will go through the complete set
                // of AtomNodes and search for matches with this multimap.
                //
                if (typeid(*((*li)->getAtom())) == typeid(ExternalAtom))
                {
                    ExternalAtom* ext = dynamic_cast<ExternalAtom*>((*li)->getAtom().get());

                    //
                    // go through all input terms of this external atom
                    //
                    for (unsigned s = 0; s < ext->getInputTerms().size(); s++)
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
        } // body finished

        //
        // now we go through the ordinary and external atoms of the body again
        // and see if we have to add any EXTERNAL_AUX dependencies.
        // An EXTERNAL_AUX dependency arises, if an external atom has variable
        // input arguments, which makes it necessary to create an auxiliary
        // rule.
        //
        for (std::vector<AtomNode*>::iterator currextbody = currentExternalBodyNodes.begin();
             currextbody != currentExternalBodyNodes.end();
             ++currextbody)
        {
            ExternalAtom* ext = dynamic_cast<ExternalAtom*>((*currextbody)->getAtom().get());

            //
            // does this external atom have any variable input parameters?
            //
            if (!ext->pureGroundInput())
            {

                //
                // ok, get the parameters
                //
                Tuple extinput = ext->getInputTerms();

                //
                // make a new atom with the ext-parameters as arguments, will be
                // the head of the auxiliary rule
                //
//                Atom* auxheadatom = new Atom(ext->getAuxPredicate(), extinput);

                //
                // add this atom to the global atom store
                //
                AtomPtr auxheadatom = Registry::Instance()->storeAtom(new Atom(ext->getAuxPredicate(), extinput));

                //
                // and add the atom name to the store of auxiliary names (which
                // we save separately because we don't want to have them in any
                // output)
                //
                Term::auxnames.insert("aux_" + ext->getReplacementName());

                //
                // add a new head node with this atom
                //
                AtomNode* auxheadnode = nodegraph.addUniqueHeadNode(auxheadatom);

                //
                // add aux dependency from this new head to the external atom
                // node
                //
                Dependency::addDep(auxheadnode, *currextbody, Dependency::EXTERNAL_AUX);

                RuleBody_t auxbody;

                //
                // the body of the auxiliary rule are all ordinary body literals
                // that have variables with the aux_head in common
                // and that are not weakly negated!
                //
                for (std::vector<AtomNode*>::iterator currbody = currentOrdinaryBodyNodes.begin();
                    currbody != currentOrdinaryBodyNodes.end();
                    ++currbody)
                {
                    bool thisAtomIsRelevant = false;

                    Tuple currentAtomArguments = (*currbody)->getAtom()->getArguments();

                    //
                    // go through all variables of the external atom
                    //
                    Tuple::const_iterator inb = extinput.begin(), ine = extinput.end();

                    while (inb != ine)
                    {
                        //
                        // now see if any of the current ordinary body atom
                        // arguments has a common variable with the external
                        // atom
                        //
                        Tuple::const_iterator bodb = currentAtomArguments.begin();
                        Tuple::const_iterator bode = currentAtomArguments.end();

                        while (bodb != bode)
                        {
                            if (*(bodb++) == *inb)
                            {
                                thisAtomIsRelevant = true;
                            }
                        }

                        inb++;
                    }

                    //
                    // should this atom be in the auxiliary rule body?
                    //
                    if (thisAtomIsRelevant)
                    {
                        //
                        // make new literals with the (ordinary) body atoms of the current rule
                        //
                        //auxbody.push_back(Literal((*currbody)->getAtom()));
                        Literal* l = new Literal((*currbody)->getAtom());

                        Registry::Instance()->storeObject(l);

                        auxbody.push_back(l);
                    
                        //
                        // make a node for each of these new atoms
                        //
                        AtomNode* auxbodynode = nodegraph.addUniqueBodyNode(l->getAtom());
                        
                        //
                        // add the usual body->head dependency
                        //
                        Dependency::addDep(auxbodynode, auxheadnode, Dependency::PRECEDING);
                    }
                }

                //
                // finally, make an auxiliary rule object to add to the head node
                //
                RuleHead_t auxhead;

                auxhead.push_back(auxheadatom);

                Rule* auxrule = new Rule(auxhead, auxbody);

                Registry::Instance()->storeObject(auxrule);

                auxheadnode->addRule(auxrule);
            }
        }

        //
        // finally add this rule to each head node:
        //
        for (std::vector<AtomNode*>::iterator currhead = currentHeadNodes.begin();
                currhead != currentHeadNodes.end();
                ++currhead)
        {
            (*currhead)->addRule(*r);
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
        // do this only for ordinary atoms, external atoms can't be in the input
        // list!
        //
        if (typeid(*(*node)->getAtom()) != typeid(ExternalAtom))
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
                Dependency::addDep(*node, i->second, Dependency::EXTERNAL);
            }
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


