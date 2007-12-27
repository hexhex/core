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

#if !defined(_DLVHEX_GRAPHPROCESSOR_H)
#define _DLVHEX_GRAPHPROCESSOR_H

#include "dlvhex/PlatformDefinitions.h"

#include "dlvhex/DependencyGraph.h"
#include "dlvhex/Atom.h"
#include "dlvhex/Component.h"


DLVHEX_NAMESPACE_BEGIN

/**
 * @brief Control center for traversing and evaluating the program graph.
 */
class DLVHEX_EXPORT GraphProcessor
{
public:

    /// Ctor.
    explicit
    GraphProcessor(DependencyGraph*);

    /// evaluate graph with supplied EDB
    void
    run(const AtomSet&);

    /// @return the models one by one
    AtomSet*
    getNextModel();

private:

    /**
     * evaluate this dependency graph
     */
    DependencyGraph *depGraph;

    /**
     * @brief Internal result retrieval pointer.
     */
    std::vector<AtomSet>::iterator resultSetIndex;

    /**
     * @brief Result of all connected components (= the entire program).
     */
    std::vector<AtomSet> resultModels;
};


DLVHEX_NAMESPACE_END

#endif /* _DLVHEX_GRAPHPROCESSOR_H */


// Local Variables:
// mode: C++
// End:
