/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
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
 * @author Peter Schüller
 *
 * @brief Implementation of an evaluation heuristic corresponding to old dlvhex.
 */

#include "dlvhex/EvalHeuristicOldDlvhex.hpp"
#include "dlvhex/Logger.hpp"

#include <boost/unordered_map.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/graph/topological_sort.hpp>

DLVHEX_NAMESPACE_BEGIN

EvalHeuristicOldDlvhex::EvalHeuristicOldDlvhex(EvalGraphBuilder& builder):
  Base(builder)
{
}

EvalHeuristicOldDlvhex::~EvalHeuristicOldDlvhex()
{
}

// as we are in a CPP file, we can do this
typedef ComponentGraph::Component Component;
typedef ComponentGraph::ComponentIterator ComponentIterator;
typedef ComponentGraph::SuccessorIterator SuccessorIterator;
typedef ComponentGraph::PredecessorIterator PredecessorIterator;
typedef std::set<Component> ComponentSet;
typedef std::list<Component> ComponentList;
typedef std::vector<Component> ComponentVector;

// old dlvhex approach:
// "calculate all that is calculateable" then go to next set of components and continue
//
// 1) do a topological sort of components not yet put into eval units
// 2) go through components in order and mark "take" if
//    * is an external component and depends only on prior eval units, or
//    * is no external component and depends only on prior eval units or "take" components
// 3) build eval unit from all marked as "take"
// 4) restart
void EvalHeuristicOldDlvhex::build()
{
  ComponentGraph& compgraph = builder.getComponentGraph();

  // get internal compgraph
  const ComponentGraph::Graph& igraph = compgraph.getInternalGraph();

  //
  // do topological sort
	// (this sorting will remain stable, though elements may be removed)
  //

  // prepare colormap
  //
	// we need a hash map, as component graph is no graph with vecS-storage
	typedef boost::unordered_map<Component, boost::default_color_type> CompColorHashMap;
	typedef boost::associative_property_map<CompColorHashMap> CompColorMap;
	CompColorHashMap ccWhiteHashMap;
	// fill white hash map
  ComponentGraph::ComponentIterator cit, cit_end;
	for(boost::tie(cit, cit_end) = compgraph.getComponents();
			cit != cit_end; ++cit)
	{
		ccWhiteHashMap[*cit] = boost::white_color;
	}

  // do it
  ComponentList opencomps;
  std::back_insert_iterator<ComponentList> compinserter(opencomps);
  CompColorHashMap ccHashMap(ccWhiteHashMap);
  boost::topological_sort(
      compgraph.getInternalGraph(),
      compinserter,
      boost::color_map(CompColorMap(ccHashMap)));

  ComponentList finishedcomps;
  ComponentSet finishedcompsSet;

  do
  {
		LOG("creating new eval unit:");
    LOG("open =     " << printrange(opencomps));
    LOG("finished = " << printrange(finishedcomps));

    ComponentSet markedcomps;
    for(ComponentList::const_iterator it = opencomps.begin();
        it != opencomps.end(); ++it)
    {
      Component comp = *it;
      bool extcomp = !compgraph.propsOf(comp).outerEatoms.empty();
      LOG("comp " << comp << " is " << (extcomp?"":"not ") << "external");

      // check dependencies
      bool mark = true;
      ComponentGraph::PredecessorIterator pit, pit_end;
      for(boost::tie(pit, pit_end) = compgraph.getDependencies(comp);
          pit != pit_end; ++pit)
      {
        Component dependsOn = compgraph.targetOf(*pit);
        if( extcomp )
        {
          if( finishedcompsSet.find(dependsOn) == finishedcompsSet.end() )
          {
            // this is an external component and it depends on something not yet finished
            mark = false;
            break;
          }
        }
        else
        {
          if( finishedcompsSet.find(dependsOn) == finishedcompsSet.end() &&
              markedcomps.find(dependsOn) == markedcomps.end() )
          {
            // this is not an external component and it depends on something not yet finished or marked
            mark = false;
            break;
          }
        }
      } // go through dependencies of each component
      LOG("comp " << comp << " is " << (mark?"":"not ") << "marked for this eval unit");

      if( mark )
        markedcomps.insert(comp);
    } // go through all components in order and determine marking for each of them

    LOG("marked = " << printrange(markedcomps));

    // collapse marked into new component
		Component newcomp = compgraph.collapseComponents(markedcomps);
    LOG("collapsing marked yielded component " << newcomp);

    // remove marked from opencomps
    for(ComponentList::iterator it = opencomps.begin();
        it != opencomps.end();)
    {
      if( markedcomps.find(*it) != markedcomps.end() )
      {
        ComponentList::iterator backup = it;
        it++; // may now be end() !
        opencomps.erase(backup);
        continue; // does not increment, first checks if end
      }
      // do it here, so that continue; does not increment
      it++;
    }

    // add newcomp to finished set and list
    finishedcomps.push_back(newcomp);
    finishedcompsSet.insert(newcomp);
  }
  while(!opencomps.empty());

  for(ComponentList::const_iterator it = finishedcomps.begin();
      it != finishedcomps.end(); ++it)
  {
    // build eval unit
    EvalGraphBuilder::EvalUnit u = builder.createEvalUnit(*it);
    LOG("component " << *it << " became eval unit " << u);
  }
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
