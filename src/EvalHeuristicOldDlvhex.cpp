/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schüller
 * Copyright (C) 2011-2015 Christoph Redl
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
 * @file EvalHeuristicOldDlvhex.cpp
 * @author Peter Schller
 *
 * @brief Implementation of an evaluation heuristic corresponding to old dlvhex.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#include "dlvhex2/EvalHeuristicOldDlvhex.h"
#include "dlvhex2/EvalHeuristicShared.h"
#include "dlvhex2/Logger.h"

#include <boost/unordered_map.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/reverse_graph.hpp>

DLVHEX_NAMESPACE_BEGIN

EvalHeuristicOldDlvhex::EvalHeuristicOldDlvhex():
Base()
{
}


EvalHeuristicOldDlvhex::~EvalHeuristicOldDlvhex()
{
}


namespace
{
    typedef ComponentGraph::Component Component;
    typedef ComponentGraph::ComponentIterator ComponentIterator;
    typedef ComponentGraph::SuccessorIterator SuccessorIterator;
    typedef ComponentGraph::PredecessorIterator PredecessorIterator;
    typedef std::set<Component> ComponentSet;
    typedef std::list<Component> ComponentList;
    typedef std::vector<Component> ComponentVector;
}


// old dlvhex approach:
// "calculate all that is calculateable" then go to next set of components and continue
//
// 1) do a topological sort of components not yet put into eval units
// 2) go through components in order and mark "take" if
//    * is an external component and depends only on prior eval units, or
//    * is no external component and depends only on prior eval units or "take" components
// 3) build eval unit from all marked as "take"
// 4) restart
void EvalHeuristicOldDlvhex::build(EvalGraphBuilder& builder)
{
    const ComponentGraph& compgraph = builder.getComponentGraph();

    // get internal compgraph
    const ComponentGraph::Graph& igraph = compgraph.getInternalGraph();

    ComponentList opencomps;
    evalheur::topologicalSortComponents(igraph, opencomps);

    ComponentSet finishedcompsSet;

    do {
        DBGLOG(DBG,"creating new eval unit:");
        DBGLOG(DBG,"open =     " << printrange(opencomps));
        DBGLOG(DBG,"finished = " << printrange(finishedcompsSet));

        ComponentSet markedcomps;
        for(ComponentList::const_iterator it = opencomps.begin();
        it != opencomps.end(); ++it) {
            Component comp = *it;
            bool extcomp = !compgraph.propsOf(comp).outerEatoms.empty();
            DBGLOG(DBG,"comp " << comp << " is " << (extcomp?"":"not ") << "external");

            // check dependencies
            bool mark = true;
            ComponentGraph::PredecessorIterator pit, pit_end;
            for(boost::tie(pit, pit_end) = compgraph.getDependencies(comp);
            pit != pit_end; ++pit) {
                Component dependsOn = compgraph.targetOf(*pit);
                if( extcomp ) {
                    if( finishedcompsSet.find(dependsOn) == finishedcompsSet.end() ) {
                        // this is an external component and it depends on something not yet finished
                        mark = false;
                        break;
                    }
                }
                else {
                    if( finishedcompsSet.find(dependsOn) == finishedcompsSet.end() &&
                    markedcomps.find(dependsOn) == markedcomps.end() ) {
                        // this is not an external component and it depends on something not yet finished or marked
                        mark = false;
                        break;
                    }
                }
            }                    // go through dependencies of each component
            DBGLOG(DBG,"comp " << comp << " is " << (mark?"":"not ") << "marked for this eval unit");

            if( mark )
                markedcomps.insert(comp);
        }                        // go through all components in order and determine marking for each of them

        LOG(ANALYZE,"marked = " << printrange(markedcomps));

        // create new component
        {
            std::list<Component> comps(markedcomps.begin(), markedcomps.end());
            std::list<Component> ccomps;
            EvalGraphBuilder::EvalUnit u = builder.createEvalUnit(comps, ccomps);
            Component c = builder.getComponentForUnit(u);
            LOG(ANALYZE,"components " << printrange(comps) << " became eval unit " << u << " and component " << c);
            finishedcompsSet.insert(c);
        }

        // remove marked from opencomps
        ComponentList::iterator it = opencomps.begin();
        while( it != opencomps.end() ) {
            if( markedcomps.find(*it) != markedcomps.end() ) {
                // found marked component

                // add to finished set
                finishedcompsSet.insert(*it);

                // remove from open components list
                it = opencomps.erase(it);
                continue;        // does not increment, first checks if end
            }
            // do it here, so that continue; does not increment
            it++;
        }
    }
    while(!opencomps.empty());
}


DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
