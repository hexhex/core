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
#include "dlvhex/ExternalAtom.h"
#include "dlvhex/ModelGenerator.h"


/**
 * @brief Abstract Node class.
 *
 * A node is the smallest entity of the dependency graph.
 */
class Node
{
public:
    
    typedef enum { RULE, EXTATOM } NodeType;

protected:
        
    /// Ctor.
    Node();

    unsigned id;

    NodeType type;

public:
    
    /**
     * @brief Returns the node's type.
     */
    NodeType
    getType() const;

    /**
     * @brief Returns the node's index.
     */
    unsigned
    getId() const;

private:

    static unsigned nodecount;

};

unsigned Node::nodecount = 0;

/**
 * @brief Node that represents a rule.
 *
 * A rule node consists of a single (head) literal and contains also
 * a reference to the respective rule body.
 */
class RuleNode : public Node
{
public:
    
    /// Ctor.
    RuleNode();

    /**
     * @brief Initializing the node with a rule.
     */
    RuleNode(Rule *rule);

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
 * @brief Node that represents an external atom.
 */
class ExternalNode : public Node
{
public:
    
    /// Ctor
    ExternalNode();

    /**
     * @brief Initializing the node with an external atom.
     */
    ExternalNode(ExternalAtom *atom);

    /**
     * @brief Returns the external atom.
     */
    const ExternalAtom*
    getExternalAtom() const;

private:

    ExternalAtom* externalAtom;
};


/**
 * @brief Component class.
 *
 * A component consists a set of nodes in the dependency graph of the
 * program and thus corresponds to a subprogram.
 */
class Component
{
public:

    /// Dtor.
    virtual
    ~Component();

    /**
     * @brief Computes the Model(s) of the component.
     */
    virtual void
    evaluate(std::vector<GAtomSet>&) = 0;

    unsigned
    numResults() const;

    const std::vector<GAtomSet>&
    getResult() const;

    bool
    wasEvaluated() const;
    
protected:
    
    /// Ctor.
    Component();

    bool evaluated;

    std::vector<GAtomSet> result;
};


/**
 * @brief A ProgramComponent is a subprogram consisting of a set of hex-rules.
 */
class ProgramComponent : public Component
{
public:

    /// Ctor.
    //ProgramComponent();

    ProgramComponent(Program&, ModelGenerator*);

    /// Dtor.
    ~ProgramComponent();

    /**
     * @brief Computes the model(s) of the subprogram of this component.
     */
    virtual void
    evaluate(std::vector<GAtomSet>&);


    /**
     * @brief Adds a node pointer to the component.
     */
//    void
//    addRuleNode(RuleNode *rn);

protected:
    
    /**
     * @brief Model Generator that suits this particular component type
     */
    ModelGenerator* modelGenerator;
    
    /**
     * @brief Nodes of this component.
     */
  //  std::vector<RuleNode*> ruleNodes;
    Program program;
};


/**
 * @brief An external component is a single external atom.
 */
class ExternalComponent : public Component
{
public:

    /**
     * @brief Computes the result of the external computation.
     */
    virtual void
    evaluate(std::vector<GAtomSet>&);

private:

    /**
     * @brief External atom of this component.
     */
    ExternalAtom* externalAtom;
};



class Subgraph
{
public:

    /// Ctor.
    Subgraph();


    /// Dtor.
    ~Subgraph();

    void
    addComponent(Component*);

    std::vector<Component*>
    getComponents() const;

private:
    
    /**
     * @brief Components in this subgraph.
     */
    std::vector<Component*> components;

};



#endif /* _COMPONENT_H_ */
