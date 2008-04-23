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
 * @file DependencyGraph.h
 * @author Roman Schindlauer
 * @date Mon Sep 19 12:19:38 CEST 2005
 *
 * @brief Classes for the dependency graph class and its subparts.
 *
 *
 */


#if !defined(_DLVHEX_DEPENDENCYGRAPH_H)
#define _DLVHEX_DEPENDENCYGRAPH_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/Rule.h"
#include "dlvhex/AtomNode.h"
#include "dlvhex/Component.h"
#include "dlvhex/GraphBuilder.h"
#include "dlvhex/ComponentFinder.h"
#include "dlvhex/NodeGraph.h"

#include <vector>


DLVHEX_NAMESPACE_BEGIN

// forward declarations
class ProgramCtx;

/**
 * @brief Holds information for the components in a dependency graph.
 */
class DLVHEX_EXPORT DependencyGraph
{
public:
    
    /// Dtor.
    ~DependencyGraph();

    /**
     * @brief Constructor that builds the dependency graph.
     *
     */
    DependencyGraph(ComponentFinder*, const ProgramCtx&);


    /**
     * @brief Creates strongly connected components from AtomNodes.
     */
    void
    getStrongComponents(const std::vector<AtomNodePtr>&,
                        std::vector<std::vector<AtomNodePtr> >&);

    /**
     * @brief Returns all Components.
     */
    std::vector<Component*>
    getComponents() const;

    Subgraph*
    getNextSubgraph();

    
private:

    bool
    hasNegEdge(const std::vector<AtomNodePtr>&) const;

    bool
    isExternal(const std::vector<AtomNodePtr>&) const;
    
    /**
     * @brief All nodes.
     */
    NodeGraph nodegraph;
    

    /**
     * @brief All components (strongly connected components).
     */
    std::vector<Component*> components;

    /**
     * @brief All subgraphs (connected components).
     */
    std::vector<Subgraph*> subgraphs;

    /**
     * @brief Current subgraph pointer.
     */
    std::vector<Subgraph*>::iterator
    currentSubgraph;


    ComponentFinder* componentFinder;
};

DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_DEPENDENCYGRAPH_H */


// Local Variables:
// mode: C++
// End:
