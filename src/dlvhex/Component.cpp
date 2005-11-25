/* -*- C++ -*- */

/**
 * @file Component.cpp
 * @author Roman Schindlauer
 * @date Mon Sep 19 13:14:56 CEST 2005
 *
 * @brief Conponent and Node Class.
 *
 *
 */

#include "dlvhex/Component.h"
#include "dlvhex/ProgramBuilder.h"
#include "dlvhex/ASPsolver.h"
#include "dlvhex/globals.h"

Node::Node()
{
    id = nodecount++;
}

Node::NodeType
Node::getType() const
{
    return type;
}

unsigned
Node::getId() const
{
    return id;
}

RuleNode::RuleNode(Rule *r)
    : rule(r),
      Node()
{
    type = RULE;
}

const Rule*
RuleNode::getRule() const
{
    return rule;
}

ExternalNode::ExternalNode(ExternalAtom *atom)
    : externalAtom(atom),
      Node()
{
    type = EXTATOM;
}

const ExternalAtom*
ExternalNode::getExternalAtom() const
{
    return externalAtom;
}


Component::Component()
    : evaluated(false)
{
}


Component::~Component()
{
}


unsigned
Component::numResults() const
{
    return result.size();
}


const std::vector<GAtomSet>&
Component::getResult() const
{
    return result;
}

bool

Component::wasEvaluated() const
{
    return evaluated;
}


ProgramComponent::ProgramComponent(Program& p, ModelGenerator* mg)
    : program(p),
      modelGenerator(mg),
      Component()
{
    mg->initialize(program);
}

    
void
ProgramComponent::evaluate(std::vector<GAtomSet>& input)
{
    std::vector<GAtomSet> res;

    //
    // compute model for each input factset
    //
    for (std::vector<GAtomSet>::const_iterator ii = input.begin();
         ii != input.end();
         ii++)
    {
        res.clear();

        try
        {
            modelGenerator->compute(program, *ii, res);
        }
        catch (generalError&)
        {
            throw;
        }

        //
        // collect all results:
        //
        result.insert(result.end(), res.begin(), res.end());
    }

    evaluated = true;
}


ProgramComponent::~ProgramComponent()
{
    if (modelGenerator != NULL)
        delete modelGenerator;
}



void
ExternalComponent::evaluate(std::vector<GAtomSet>& input)
{
}



Subgraph::Subgraph()
{
}


Subgraph::~Subgraph()
{
    for (std::vector<Component*>::const_iterator ci = components.begin();
         ci != components.end();
         ci++)
    {
        delete *ci;
    }
    
}


void
Subgraph::addComponent(Component* c)
{
    components.push_back(c);
}


std::vector<Component*>
Subgraph::getComponents() const
{
    return components;
}
