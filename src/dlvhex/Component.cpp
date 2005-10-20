/* -*- C++ -*- */

/**
 * @file Conponent.cpp
 * @author Roman Schindlauer
 * @date Mon Sep 19 13:14:56 CEST 2005
 *
 * @brief Conponent and Node Class.
 *
 *
 */

#include "dlvhex/Component.h"
#include "dlvhex/ProgramBuilder.h"

Node::Node()
{
}

Node::Node(Rule *r)
    : rule(r)
{
}

const Rule*
Node::getRule() const
{
    return rule;
}



Component::Component()
{
}

void
Component::addNode(Node *n)
{
    nodes.push_back(n);
    
    //
    // find external atoms
    //
    const std::vector<Literal>* b = n->getRule()->getBody();
    
    for (std::vector<Literal>::const_iterator l = b->begin(); l != b->end(); l++)
        if (l->getAtom()->getType() == Atom::EXTERNAL)
            externalAtoms.push_back((ExternalAtom*)l->getAtom());
}


void
Component::buildProgram(ProgramBuilder *builder) const
{
    for (std::vector<Node*>::const_iterator n = nodes.begin();
         n != nodes.end();
         n++)
    {
        (*builder).buildRule(*((**n).getRule()));
    }
}


void
Component::getExtAtoms(std::vector<ExternalAtom*> &extatoms) const
{
    extatoms = externalAtoms;
}

