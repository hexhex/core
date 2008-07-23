/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with dlvhex; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */


/**
 * @file Component.h
 * @author Roman Schindlauer
 * @date Mon Sep 19 13:14:56 CEST 2005
 *
 * @brief Component and Subgraph classes.
 *
 *
 */


#if !defined(_DLVHEX_COMPONENT_H)
#define _DLVHEX_COMPONENT_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/AtomNode.h"
#include "dlvhex/ExternalAtom.h"
#include "dlvhex/ModelGenerator.h"

#include <utility>

DLVHEX_NAMESPACE_BEGIN

// forward declarations
class PluginContainer;


/**
 * @brief Component class.
 *
 * A component consists a set of nodes in the dependency graph of the
 * program and thus corresponds to a subprogram. 
 * The base class is pure vurtual.
 */
class DLVHEX_EXPORT Component
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
    addAtomNode(const AtomNodePtr);

    /**
     * @brief Returns all AtomNodes of this component.
     */
    const std::vector<AtomNodePtr>&
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
     * @brief Checks if the specified Atom occurs in the component.
     */
    bool
    isInComponent(const BaseAtom*) const;

protected:
    
    /// Ctor.
//    Component(const std::vector<AtomNodePtr>&);

    /// Ctor.
    Component();

    /**
     * @brief AtomNodes that belong to this component.
     */
    std::vector<AtomNodePtr> atomnodes;

    bool evaluated;

    std::vector<AtomSet> result;

    std::list<AtomNodePtr> incomingNodes;

private:


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
class DLVHEX_EXPORT ProgramComponent : public Component
{
public:

    /// Ctor.
    ProgramComponent(const std::vector<AtomNodePtr>&,
                     ModelGenerator*);

    /// Dtor.
    ~ProgramComponent();

    /**
     * @brief Computes the model(s) of the subprogram of this component.
     */
    virtual void
    evaluate(std::vector<AtomSet>&);


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
};




/**
 * @brief An external component is a single external atom.
 */
class DLVHEX_EXPORT ExternalComponent : public Component
{
public:

    /// Ctor.
    ExternalComponent(AtomNodePtr, PluginContainer&);

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

    PluginContainer& pluginContainer;
};



/**
 * @brief A Subgraph represents a weakly connected component.
 *
 * It contains zero or more component-objects, which represent strongly connected
 * components withing this WCC as well as single external atoms.
 */
class DLVHEX_EXPORT Subgraph
{
public:

    /// Ctor.
    Subgraph();

    /**
     * Copy constructor.
     */
    Subgraph(const Subgraph&);

    /// Dtor.
    virtual
    ~Subgraph();

    void
    addComponent(Component*);

    void
    addNode(AtomNodePtr);

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
    const std::vector<AtomNodePtr>&
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
    removeNode(const AtomNodePtr);


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
    collectUp(const AtomNodePtr,
              std::vector<AtomNodePtr>&);

    /**
     * @brief Nodes in this subgraph.
     */
    std::vector<AtomNodePtr> atomnodes;

    /**
     * @brief Components in this subgraph.
     */
    std::vector<Component*> components;

    /**
     * @brief associating each node with a component
     *
     * For faster acces of components a node belongs to.
     */
    std::map<AtomNodePtr, Component*> nodeComponentMap;


    /**
     * @brief Most recent component result.
     */
    std::vector<AtomSet*> lastResult;

};


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_COMPONENT_H_ */


// Local Variables:
// mode: C++
// End:
