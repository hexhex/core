/* -*- C++ -*- */

/**
 * @file Component.h
 * @author Roman Schindlauer
 * @date Mon Sep 19 13:14:56 CEST 2005
 *
 * @brief Component and Node class.
 *
 *
 */


#ifndef _COMPONENT_H
#define _COMPONENT_H


#include "dlvhex/Rule.h"
#include "dlvhex/ProgramBuilder.h"


/**
 * @brief Node class.
 *
 * A node is the smallest entity of the dependency graph. It consists of
 * a single (head) literal and contains also a reference to the respective
 * rule body.
 */
class Node
{
public:
    
    /// Ctor
    Node();

    /**
     * @brief Initializing the node with a rule.
     */
    Node(Rule *rule);

    /**
     * @brief Returns the body of the rule that belongs to this node.
     */
    const Rule*
    getRule() const;

private:

    /**
     * @brief Rule belonging to this node.
     */
    Rule* rule;
};


/**
 * @brief Component class.
 *
 * A component contains a set of nodes.
 */
class Component
{
public:

    /// Ctor.
    Component();

    /**
     * @brief Adds a node pointer to the component.
     */
    void
    addNode(Node *n);

    /**
     * @brief Converts the component's rules into a textual representation
     * by using a specific subclass of ProgramBuilder.
     */
    void
    buildProgram(ProgramBuilder *builder) const;

    /**
     * @brief Returns the external Atoms that occur in this component.
     */
    void
    getExtAtoms(std::vector<ExternalAtom*> &extatoms) const;
    
private:
    
    /**
     * @brief Nodes of this component.
     */
    std::vector<Node*> nodes;

    /**
     * @brief External Atoms of this component.
     */
    std::vector<ExternalAtom*> externalAtoms;

};

#endif /* _COMPONENT_H_ */
