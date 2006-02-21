/* -*- C++ -*- */

/**
 * @file ComponentFinder.h
 * @author Roman Schindlauer
 * @date Wed Jan 18 17:44:48 CET 2006
 *
 * @brief Strategy class for finding SCCs and WCCs from a given program graph.
 *
 *
 */


#ifndef _COMPONENTFINDER_H
#define _COMPONENTFINDER_H


//#include "dlvhex/Rule.h"
//#include "dlvhex/ExternalAtom.h"
//#include "dlvhex/ModelGenerator.h"
//#include <utility>

#include <map>
#include <vector>
#include <iostream>

#include "dlvhex/AtomNode.h"


/**
 * @brief
 *
 *
 */
class ComponentFinder
{
public:

    /**
     * Single Vertex Type.
     */
    typedef unsigned Vertex;

    /**
     * List of Vertices.
     */
    typedef std::vector<Vertex> Vertices;

    /**
     * An Edge is a pair of Vertices.
     */
    typedef std::pair<Vertex, Vertex> Edge;

    /**
     * List of Edges.
     */
    typedef std::vector<Edge> Edges;

    /**
     * A ComponentIdx uniquely identifies a Component.
     */
//    typedef unsigned ComponentIdx;

    /**
     * A ComponentAssignment associates a Vertex with a Component.
     */
//    typedef std::map<Vertex, ComponentIdx> ComponentAssignment;

    /**
     * A ComponentList is a set of Vertices that belong to a single
     * Component.
     */
    typedef std::vector<Vertices> ComponentList;

protected:

    /// Ctor.
    ComponentFinder()
    { }

public:

    /**
     * @brief Method for finding Weakly Connected Components.
     */
    virtual void
    //findWeakComponents(const Edges, ComponentList&)
    findWeakComponents(const std::vector<AtomNode*>&,
                         std::vector<std::vector<AtomNode*> >&)
    { }

    /**
     * @brief Method for finding Strongly Connected Components.
     */
    virtual void
//    findStrongComponents(const Edges, ComponentList&)
    findStrongComponents(const std::vector<AtomNode*>&,
                         std::vector<std::vector<AtomNode*> >&)
    { }

    /**
     * @brief For verbose and debugging.
     */
    void
    dumpAssignment(const ComponentList&, std::ostream&) const;
};


/**
 * @brief Simple Component finder for testing purposes.
 *
 * This Component Finder puts every Vertex in a single WCC and finds no SCC.
 *
 */
class SimpleComponentFinder : public ComponentFinder
{
public:

    /// Ctor.
    SimpleComponentFinder()
    { }

//    virtual void
//    findWeakComponents(const Edges, ComponentList&);

//    virtual void
//    findStrongComponents(const Edges, ComponentList&);
};


#endif /* _COMPONENTFINDER_H_ */
