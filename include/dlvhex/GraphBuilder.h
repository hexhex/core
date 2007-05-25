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

/* -*- C++ -*- */

/**
 * @file GraphBuilder.h
 * @author Roman Schindlauer
 * @date Wed Jan 18 17:43:21 CET 2006
 *
 * @brief Abstract strategy class for finding the dependency edges of a program.
 *
 *
 */

#ifndef _GRAPHBUILDER_H
#define _GRAPHBUILDER_H

#include <iostream>

#include "dlvhex/Component.h"
#include "dlvhex/Rule.h"

/**
 * @brief Class for building a dependency graph from a given program.
 *
 */
class GraphBuilder
{
public:

    /**
     * @brief Takes a set of rules and builds the according node graph.
     *
     * This nodegraph will contain the entire dependency graph of the program,
     * including any artificial nodes that had to be created for auxiliary
     * rules, e.g., for external atoms with variable input parameters.
     */
    void
    run(const Program&, NodeGraph&);


    /**
     * @brief Debug dump.
     */
    void
    dumpGraph(const NodeGraph&, std::ostream&) const;



private:

//    void
//    addDep(AtomNode*, AtomNode*, Dependency::Type);

};


#endif /* _GRAPHBUILDER_H */
