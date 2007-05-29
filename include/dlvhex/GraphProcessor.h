/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


/**
 * @file GraphProcessor.h
 * @author Roman Schindlauer
 * @date Tue Sep 13 19:21:37 CEST 2005
 *
 * @brief Control class for traversing and evaluating the program graph.
 *
 *
 */

#ifndef _GRAPHPROCESSOR_H
#define _GRAPHPROCESSOR_H


#include "dlvhex/DependencyGraph.h"
#include "dlvhex/Atom.h"
#include "dlvhex/Component.h"


/**
 * @brief Control center for traversing and evaluating the program graph.
 */
class GraphProcessor
{
public:

    /// Ctor.
    GraphProcessor(DependencyGraph*);

    void
    run(const AtomSet&);

    AtomSet*
    getNextModel();

private:

    /**
     * @brief Internal result retrieval pointer.
     */
    std::vector<AtomSet>::iterator resultSetIndex;

//    void
//    combine(std::vector<GAtomSet>&, std::vector<GAtomSet>&);

    /**
     * @brief Result of all connected components (= the entire program).
     */
    std::vector<AtomSet> resultModels;

    /**
     * @brief input EDB.
     */
//    GAtomSet
//    startFacts;

    /**
     * @brief Result of all CompactCs in a single connected component.
     */
//    std::vector<GAtomSet>
//    singleSubgraphAnswer;


    /**
     * @brief 
     */
//    void
//    evaluateSubgraph(const std::vector<Component*> &components,
//                     GAtomSet &result) const;

    DependencyGraph *depGraph;
};


//
// include implementation of templates
//
//#include "dlvhex/GraphProcessor.tcc"


#endif /* _GRAPHPROCESSOR_H */


// Local Variables:
// mode: C++
// End:
