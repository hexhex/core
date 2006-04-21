/* -*- C++ -*- */

/**
 * @file AtomNode.h
 * @author Roman Schindlauer
 * @date Mon Jan 30 14:46:36 CET 2006
 *
 * @brief AtomNode, Dependency and NodeGraph classes.
 *
 *
 */


#ifndef _ATOMNODE_H
#define _ATOMNODE_H


#include <utility>

#include "dlvhex/Rule.h"


//
// forward declaration
//
class Dependency;


/**
 * @brief Single Node of a dependency Graph.
 *
 * An AtomNode is the representation of an Atom in a program's dependency
 * structure.
 */
class AtomNode
{
public:

    /// Ctor.
    //AtomNode();

    /**
     * @brief Constructs an AtomNode from a given Atom.
     */
    AtomNode(const AtomPtr);
    
    /**
     * @brief Sets the head-flag of the Node.
     *
     * For calculating the correct dependencies when a new AtomNode is added to
     * a collection of existing Nodes (see NodeGraph), it is vital to know for
     * each AtomNode whether it is associated with a head-atom or a body-atom.
     * This function sets the head-flag of the AtomNode.
     */
    void
    setHead();

    /**
     * @brief Sets the body-flag of the Node.
     *
     * See setHead().
     */
    void
    setBody();

    /**
     * @brief Returns the head-flag of the AtomNode.
     *
     * See setHead().
     */
    bool
    isHead();

    /**
     * @brief Returns the body-flag of the AtomNode.
     *
     * See setHead().
     */
    bool
    isBody();

    /**
     * @brief Adds a preceding dependency for this AtomNode.
     *
     * A preceding dependency means that this AtomNode depends on another one.
     */
    void
    addPreceding(const Dependency&);

    /**
     * @brief Add succeeding dependency for this AtomNode.
     *
     * A succeeding dependency means that another AtomNode depends on this one.
     */
    void
    addSucceeding(const Dependency&);

    /**
     * @brief Adds a rule pointer to this AtomNode.
     *
     * If an AtomNode stems from a rule head, it also keeps track of the
     * rule-object itself. This helps if we need to build a subprogram based on
     * a set of AtomNodes.
     */
    void
    addRule(const Rule*);

    /**
     * @brief Returns the atom-object this Node is associated with.
     */
    const AtomPtr
    getAtom() const;

    /**
     * @brief Returns all preceding dependencies of this AtomNode.
     */
    const std::vector<Dependency>&
    getPreceding() const;

    /**
     * @brief Returns all succeeding dependencies of this AtomNode.
     */
    const std::vector<Dependency>&
    getSucceeding() const;

    /**
     * @brief Returns all rules associated with this AtomNode.
     */
    const std::vector<const Rule*>&
    getRules() const;

    /**
     * @brief Returns the unique Id of this AtomNode.
     */
    unsigned
    getId() const;

private:

    /**
     * @brief This AtomNode's Atom object.
     */
    const AtomPtr atom;

    /**
     * @brief head-flag.
     */
    bool inHead;

    /**
     * @brief body-flag.
     */
    bool inBody;

    /**
     * @brief Rules that belong to this AtomNode (in case it occured in a rule's
     * head).
     */
    std::vector<const Rule*> rules;

    /**
     * @brief Preceding dependencies.
     */
    std::vector<Dependency> preceding;

    /**
     * @brief succeeding dependencies.
     */
    std::vector<Dependency> succeeding;

    /**
     * @brief Unique numerical index to facilitate interfacing of Component
     * finder algorithms.
     */
    unsigned nodeId;

    /**
     * @brief Node counter for assigning node Ids.
     */
    static unsigned nodeCount;
};


//
// verbose and debug serialization.
//
std::ostream& operator<< (std::ostream& out, const AtomNode& atomnode);


/**
 * @brief Dependency between two AtomNodes.
 *
 * A dependency contains an AtomNode, which is the "target" of the dependency,
 * and a type. A dependency object is supposed to belong to an AtomNode object,
 * which is then the "source" of the dependency.
 */
class Dependency
{
public:

    /**
     * @brief Type of Dependency.
     *
     * UNIFYING: The atoms of two nodes can be unified.
     * PRECEDING: A preceding dependency points from a body atom node to its head
     * atom node.
     * NEG_PRECEDING: Like preceding, but with a weakly negated body atom.
     * DISJUNCTIVE: Dependency between two head atom nodes of a disjunctive
     * head.
     * EXTERNAL: If an input argument of an external atom is of type
     * PluginAtom::PREDICATE, it depends on all atoms with a matching predicate.
     * EXTERNAL_AUX: If an input argument is nonground, an auxiliary atom will
     * be created, being the target of a dependency of this type.
     */
    typedef enum { UNIFYING = 0,
                   PRECEDING,
                   NEG_PRECEDING,
                   DISJUNCTIVE,
                   EXTERNAL,
                   EXTERNAL_AUX} Type;

    /// Ctor.
    Dependency();

    /**
     * Copy constructor.
     */
    Dependency(const Dependency&);

    /**
     * @brief Construct a dependency of a specific type to a given AtomNode
     * target.
     */
    Dependency(const AtomNode*, Type);

    /**
     * @brief Return the dependency type.
     */
    const Type
    getType() const;

    /**
     * @brief Return the target AtomNode of the dependency.
     */
    const AtomNode*
    getAtomNode() const;

    /**
     * @brief Add a dependency information to two AtomNodes.
     *
     */
    static void
    addDep(AtomNode*, AtomNode*, Type);

private:

    /**
     * @brief AtomNode that is the target of this dependency.
     */
    const AtomNode* atomNode;

    /**
     * Type od the dependency.
     */
    Type type;

};


//
// verbose and debug serialization
//
std::ostream& operator<< (std::ostream& out, const Dependency& dep);


/**
 * @brief Class for storing simple nodes of a dependency graph.
 *
 * A nodegraph is just a container for AtomNodes, taking care of creating and
 * deleting AtomNode objects. It provides specific functions for handling a set
 * of AtomNodes, like unique creation and finding a specific node.
 */
class NodeGraph
{
private:

    std::vector<AtomNode*> atomNodes;

public:

    /// Dtor.
    ~NodeGraph();

    /**
     * @brief Returns entire set of AtomNodes.
     */
    const std::vector<AtomNode*>&
    getNodes() const;

    /**
     * @brief Return a node with a specific AtomNode Id.
     */
    //const AtomNode*
    //getNode(unsigned);

    AtomNode*
    addNode();

    /**
     * @brief Create a new node for this head atom and return its pointer or
     * return pointer to already existing node that matches this atom.
     *
     * If a new node is created, all existing nodes are searched for atoms that
     * unify with the new one. If a node is found, whose atom occured in a
     * rule's body, a unifying dependency from the new node to the found one is
     * created.
     */
    AtomNode*
    addUniqueHeadNode(const AtomPtr);

    /**
     * @brief Create a new node for this body atom and return its pointer or
     * return pointer to already existing node that matches this atom.
     *
     * See also addUniqueHeadNode. All existing nods are searched for a node
     * that unfies with the new one. If one is found that occured in a rule's
     * head, a unifying dependency from the existing one to the new one is
     * creates.
     */
    AtomNode*
    addUniqueBodyNode(const AtomPtr);

    /**
     * @brief Finds an AtomNode that is associated with a specific Atom object.
     */
    AtomNode*
    findNode(const AtomPtr) const;
};


#endif /* _ATOMNODE_H_ */
