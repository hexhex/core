/* -*- C++ -*- */

/**
 * @file Component.h
 * @author Roman Schindlauer
 * @date Mon Sep 19 13:14:56 CEST 2005
 *
 * @brief Component and Subgraph classes.
 *
 *
 */


#ifndef _COMPONENT_H
#define _COMPONENT_H


#include <utility>

#include "dlvhex/AtomNode.h"
#include "dlvhex/ExternalAtom.h"
#include "dlvhex/ModelGenerator.h"



/**
 * @brief Component class.
 *
 * A component consists a set of nodes in the dependency graph of the
 * program and thus corresponds to a subprogram. 
 * The base class is pure vurtual.
 */
class Component
{
public:

    /// Dtor.
    virtual
    ~Component();

    /**
     * @brief Computes the Model(s) of the component, based on a set of inputs.
     */
    virtual void
    evaluate(std::vector<AtomSet>&) = 0;

    /**
     * @brief Returns true if this component was already evaluated.
     */
    bool
    isSolved() const;

    /**
     * @brief Adds an AtomNode to the comonent.
     */
    void
    addAtomNode(const AtomNode*);

    /**
     * @brief Returns all AtomNodes of this component.
     */
    const std::vector<const AtomNode*>&
    getNodes() const;
    
    /**
     * @brief Returns the rules that belong to this component.
     */
    Program
    getBottom() const;

//    const std::vector<Rule*>&
//    getBottom() const;

    /**
     * Serialize component to stream out for verbose and debugging.
     */
    virtual void
    dump(std::ostream& out) const = 0;

    /**
     * @brief Returns the result of the component's evaluation.
     */
    void
    getResult(std::vector<AtomSet>&);

    /**
     * @brief Checks if the spcified Atom occurs in the component.
     */
    bool
    isInComponent(const Atom*) const;

protected:
    
    /// Ctor.
//    Component(const std::vector<AtomNode*>&);

    /// Ctor.
    Component();

    /**
     * @brief AtomNodes that belong to this component.
     */
    std::vector<const AtomNode*> atomnodes;

    bool evaluated;

    std::vector<AtomSet> result;

private:

    std::vector<AtomNode*> incomingNodes;

//    std::vector<Rule*> bottom;

};



/**
 * @brief A ProgramComponent is a subprogram consisting of a set of strongly
 * connected hex-rules.
 *
 * A ProgramComponent can be (i) a SCC containing only ordinary (internal) atoms,
 * (ii) a stratified SCC with external atoms or (iii) an unstratified SCC with
 * external atoms. Each of these component types uses a different model generator.
 * Type (i) needs to call the ASP solver only once and can have 0..n answer sets.
 * Type (ii) uses an iterative model generator (which, if clever implemented, can
 * also be used for (i) without loss of efficiency).
 * Type (iii) needs to use a guess & check algorithm
 */
class ProgramComponent : public Component
{
public:

    /// Ctor.
    //ProgramComponent();

    ProgramComponent(const std::vector<AtomNode*>&,
                     ModelGenerator*);

 //   ProgramComponent(Program&, ModelGenerator*);
    //ProgramComponent(ModelGenerator*);

    /// Dtor.
    ~ProgramComponent();

//    void
//    setProgram(Program&);

    /**
     * @brief Computes the model(s) of the subprogram of this component.
     */
    virtual void
    evaluate(std::vector<AtomSet>&);


    /**
     * @brief Adds a node pointer to the component.
     */
//    void
//    addRuleNode(RuleNode *rn);

    /**
     * Serialize component to stream out for verbose and debugging.
     */
    virtual void
    dump(std::ostream& out) const;


protected:
    
    /**
     * @brief Model Generator that suits this particular component type
     */
    ModelGenerator* modelGenerator;
    
    /**
     * @brief Nodes of this component.
     */
  //  std::vector<RuleNode*> ruleNodes;
//    Program program;
};




/**
 * @brief An external component is a single external atom.
 */
class ExternalComponent : public Component
{
public:

    /// Ctor.
    ExternalComponent(AtomNode*);

    /**
     * @brief Computes the result of the external computation.
     */
    virtual void
    evaluate(std::vector<AtomSet>&);

    /**
     * Serialize component to stream out for verbose and debugging.
     */
    virtual void
    dump(std::ostream& out) const;

private:

    /**
     * @brief External atom of this component.
     */
    ExternalAtom* externalAtom;
};



/**
 * @brief A Subgraph represents a weakly connected component.
 *
 * It contains zero or more component-objects, which represent strongly connected
 * components withing this WCC as well as single external atoms.
 */
class Subgraph
{
public:

    /// Ctor.
    Subgraph();

    /**
     * Copy constructor.
     */
    Subgraph(const Subgraph&);

    /// Dtor.
    ~Subgraph();

    void
    addComponent(Component*);

    void
    addNode(AtomNode*);

    /**
     * @brief Removes components from the subgraph.
     *
     * pruneComponents removes every unsolved components and all its succeeding
     * nodes and all solved components. The remaining nodes are not included
     * in any component.
     */
    void
    pruneComponents();

    /**
     * @brief Returns all AtomNodes that belong to this subgraph.
     */
    const std::vector<AtomNode*>&
    getNodes() const;


    /**
     * @brief Returns a list of unsolved components with only solved predecessors.
     */
    void
    getUnsolvedLeaves(std::vector<Component*>&);

    /**
     * @brief Returns preceding components of the specified one.
     *
     * Cannot be const because of map[] operator!
     */
    std::vector<Component*>
    getPredecessors(Component*);

    /**
     * @brief Returns succeeding components of the specified one.
     *
     * Cannot be const because of map[] operator!
     */
    std::vector<Component*>
    getSuccessors(Component*);

    bool
    unsolvedComponentsLeft();

    /**
     * @brief Evaluates a component and stores the result as the last
     * result in the subgraph.
     */
//    void
//    evaluateComponent(Component*,
//                      std::vector<const GAtomSet>&);

//    std::vector<Component*>
//    getComponents() const;

//    Edges
//    getEdges() const;

    /**
     * Doesn't remove the component from the subgraph, but its nodes from the
     * subgraph's tree,
     */
//    void
//    removeComponentNodes(Component);


    /**
     * @brief Remove a node pointer from the subgraph.
     */
    void
    removeNode(const AtomNode*);


    std::vector<AtomSet*>&
    getLastResult();
    
    /**
     * Serialize subgraph to stream out for verbose and debugging.
     */
    virtual void
    dump(std::ostream& out) const;

private:
    
    /**
     * @brief Collects all Nodes that depend on the specified one.
     *
     * Recursive function.
     */
    void
    collectUp(const AtomNode*,
              std::vector<const AtomNode*>&);

    /**
     * @brief Nodes in this subgraph.
     */
    std::vector<AtomNode*> atomnodes;

    /**
     * @brief Components in this subgraph.
     */
    std::vector<Component*> components;

    /**
     * @brief associating each node with a component
     *
     * For faster acces of components a node belongs to.
     */
    std::map<const AtomNode*, Component*> nodeComponentMap;


    /**
     * @brief Most recent component result.
     */
    std::vector<AtomSet*> lastResult;

};


#endif /* _COMPONENT_H_ */
