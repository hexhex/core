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
#include <utility>



class Dependency;

/**
 * @brief Single Node of a dependency Graph.
 *
 */
class AtomNode
{
public:

    /// Ctor.
    AtomNode();

    AtomNode(const Atom*);
    
    void
    setHead();

    void
    setBody();

    bool
    isHead();

    bool
    isBody();

    void
    addPreceding(const Dependency&);

    void
    addSucceeding(const Dependency&);

    void
    addRule(const Rule*);

    const Atom*
    getAtom() const;

    const std::vector<Dependency>&
    getPreceding() const;

    const std::vector<Dependency>&
    getSucceeding() const;

    const std::vector<const Rule*>&
    getRules() const;

    unsigned
    getId() const;

private:

    const Atom* atom;

    bool inHead;

    bool inBody;

    std::vector<const Rule*> rules;

    std::vector<Dependency> preceding;

    std::vector<Dependency> succeeding;

    /**
     * @brief Mantaining also a numerical index to facilitate interfacing of
     * Component finder algorithms.
     */
    unsigned nodeId;

    static unsigned
    nodeCount;
};


std::ostream& operator<< (std::ostream& out, const AtomNode& atomnode);


/**
 * @brief Dependency between two AtomNodes.
 *
 */
class Dependency
{
public:

    typedef enum { UNIFYING = 0,
                   PRECEDING,
                   NEG_PRECEDING,
                   DISJUNCTIVE,
                   EXTERNAL } Type;

    /// Ctor.
    Dependency();

    /**
     * Copy constructor.
     */
    Dependency(const Dependency&);

    Dependency(const AtomNode*, Type);

    const Type
    getType() const;

    const AtomNode*
    getAtomNode() const;

private:

    const AtomNode* atomNode;

    Type type;

};

std::ostream& operator<< (std::ostream& out, const Dependency& dep);


/**
 * @brief Class for storing simple nodes of a dependency graph.
 *
 */
class NodeGraph
{
private:

    std::vector<AtomNode*> atomNodes;

public:

    /// Dtor.
    ~NodeGraph();

    const std::vector<AtomNode*>&
    getNodes() const;

    /**
     * @brief Get node from node id.
     */
    const AtomNode*
    getNode(unsigned);

    void
    addNode(AtomNode*);

    /**
     * @brief Create a new node for this head atom and return its pointer or
     * return pointer to already existing node that matches this atom.
     *
     * If a new node is created, all existing nodes are searched for atoms
     * that unify with the new one. If a node is found, whose atom occured
     * in a rule's body, a unifying dependency from the new node to the
     * found one is created.
     */
    AtomNode*
    addUniqueHeadNode(const Atom*);

    /**
     * @brief Create a new node for this body atom and return its pointer or
     * return pointer to already existing node that matches this atom.
     *
     * See also addUniqueHeadNode. All exisitng nods are searched for a node that
     * unfies with the new one. If one is found that occured in a rule's head,
     * a unifying dependency from the existing one to the new one is creates.
     */
    AtomNode*
    addUniqueBodyNode(const Atom*);

    AtomNode*
    findNode(const Atom*) const;

    void
    removeNode();
};


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
    evaluate(std::vector<GAtomSet>&) = 0;

    /**
     * @brief Returns true if this component was already evaluated.
     */
    bool
    isSolved() const;

    /**
     * @brief Adds an AtomNode to the component.
     */
    void
    addAtomNode(const AtomNode*);

    /**
     * @brief Returns all AtomNodes of this component.
     */
    const std::vector<const AtomNode*>&
    getNodes() const;
    
    /**
     * Serialize component to stream out for verbose and debugging.
     */
    virtual void
    dump(std::ostream& out) const = 0;

    /**
     * @brief Returns the result of the component's evaluation.
     */
    void
    getResult(std::vector<GAtomSet>&);

protected:
    
    /// Ctor.
    Component();

    /**
     * @brief AtomNodes that belong to this component.
     */
    std::vector<const AtomNode*> atomnodes;

    bool evaluated;

    std::vector<GAtomSet> result;

private:

    std::vector<AtomNode*> incomingNodes;

    std::vector<Rule*> bottom;

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

    ProgramComponent(Program&, ModelGenerator*);
    //ProgramComponent(ModelGenerator*);

    /// Dtor.
    ~ProgramComponent();

    void
    setProgram(Program&);

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
    Program program;
};




/**
 * @brief An external component is a single external atom.
 */
class ExternalComponent : public Component
{
public:

    /// Ctor.
//    ExternalComponent(ExternalAtom*);

    /**
     * @brief Computes the result of the external computation.
     */
    virtual void
    evaluate(std::vector<GAtomSet>&);

    /**
     * Serialize component to stream out for verbose and debugging.
     */
    virtual void
    dump(std::ostream& out) const;

private:

    /**
     * @brief External atom of this component.
     */
 //   ExternalAtom* externalAtom;
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


    std::vector<GAtomSet*>&
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
    std::vector<GAtomSet*> lastResult;

};












/////////////////////////////////////////////////////////////////////////////////////////
//
//
// OLD CLASSES
//
//
/////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Abstract Node class.
 *
 * A node is the smallest entity of the dependency graph.
 */
class Node
{
public:
    
    typedef enum { RULE, EXTATOM } NodeType;

    /**
     * Copy constructor.
     */
    Node(const Node&);

    Node(const Atom*);

    const Atom*
    getAtom() const;

    int
    operator< (const Node& node2) const;

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

    const Atom* atom;

};


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


typedef std::vector<Node> Nodes;

typedef std::vector<std::pair<Node*, Node*> > Edges;




class Graph
{
private:

public:

    /// Dtor.
    ~Graph();

    typedef std::vector<Node*> Nodes;

//    typedef std::pair<const Node*, const Node*> Edge;

//    typedef std::set<Edge> Edges;
    typedef std::multimap<const Node*, const Node*> Edges;

    void
    insertNode(Node*);

    void
    insertEdge(const Node* from, const Node* to);

    const Nodes&
    getNodes() const;

//    void
//    removeNode(Node*);
    
//    void
//    removeComponent(Component*);

//    void
//    pruneNode(Node*);

//    void
//    pruneComponent(Component*);

    void
    dump(std::ostream&);

private:

    /*
    class Vertex
    {
    public:

        std::vector<Vertex*>
        getSucc()
        {
            return succ;
        }

        std::vector<Vertex*>
        getPred()
        {
            return pred;
        }

    private:

        std::vector<Vertex*> pred, succ;

        Node* node;
    };

*/
    Nodes nodes;

    Edges edges;

/*
    std::vector<Vertex> nodes;

    std::map<Node*, std::vector<Vertex>::iterator> nodemap;
*/
};



#endif /* _COMPONENT_H_ */
