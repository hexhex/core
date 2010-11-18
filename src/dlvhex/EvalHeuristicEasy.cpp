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
 * @file EvalHeuristicEasy.cpp
 * @author Peter Schüller
 *
 * @brief Implementation of a nontrivial but simple evaluation heuristic.
 */

#include "dlvhex/EvalHeuristicEasy.hpp"
#include "dlvhex/Logger.hpp"

#include <boost/unordered_map.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/properties.hpp>

DLVHEX_NAMESPACE_BEGIN

EvalHeuristicEasy::EvalHeuristicEasy(EvalGraphBuilder& builder):
  Base(builder)
{
}

EvalHeuristicEasy::~EvalHeuristicEasy()
{
}

typedef ComponentGraph::Component Component;
typedef ComponentGraph::ComponentIterator ComponentIterator;
typedef std::vector<Component> ComponentContainer;

// trivial strategy:
// do a topological sort of the tree
// build eval units in that order
void EvalHeuristicEasy::build()
{
  ComponentGraph& compgraph = builder.getComponentGraph();
  typedef ComponentGraph::Component Component;
  typedef ComponentGraph::ComponentSet ComponentSet;

  //
  // forall external atoms:
  // merge with rules that contain them if they don't depend on something else
  // TODO relax that condition
  //
  {
    ComponentIterator cit;
    for(cit = compgraph.getComponents().first; // do not use boost::tie here! the container is modified in the loop!
        cit != compgraph.getComponents().second; ++cit)
    {
      Component comp = *cit;
      if( compgraph.propsOf(comp).outerEatoms.empty() )
        continue;

      LOG("checking whether to collapse external component " << comp);

      // get successors
      ComponentSet collapse;
      ComponentGraph::SuccessorIterator sit, sit_end;
      for(boost::tie(sit, sit_end) = compgraph.getProvides(comp);
          sit != sit_end;
          ++sit)
      {
        Component succ = compgraph.sourceOf(*sit);

        // skip successors with eatoms
        if( !compgraph.propsOf(succ).outerEatoms.empty() )
          continue;

        LOG("found successor " << succ);

        ComponentGraph::PredecessorIterator pit, pit_end;
        boost::tie(pit, pit_end) = compgraph.getDependencies(succ);
        bool good = true;
        assert(pit != pit_end);
        if( compgraph.targetOf(*pit) != comp )
        {
          LOG("successor bad as it depends on other node " << compgraph.targetOf(*pit));
          good = false;
        }
        pit++;
        if( pit != pit_end )
        {
          good = false;
          LOG("successor bad as it depends on more nodes");
        }
        if( good )
          collapse.insert(succ);
      }

      // collapse if not nonempty
      if( !collapse.empty() )
      {
        collapse.insert(comp);
        Component c = compgraph.collapseComponents(collapse);
        LOG("collapse of " << printrange(collapse) << " yielded new component " << c);

        // restart loop after collapse
        cit = compgraph.getComponents().first;
      }
    }
  }

  //
  // forall components with only inner rules:
  // if they contain a guess, merge with children that do not contain a guess and no eatoms and do not depend on anything else
  // TODO relax that condition
  //
  {
    ComponentIterator cit = compgraph.getComponents().first;
    do
    {
      Component comp = *cit;
      if( !compgraph.propsOf(comp).outerEatoms.empty() )
      {
        cit++;
        continue;
      }

      LOG("checking whether to collapse internal-only component " << comp);

      // get successors
      ComponentSet collapse;
      ComponentGraph::SuccessorIterator sit, sit_end;
      for(boost::tie(sit, sit_end) = compgraph.getProvides(comp);
          sit != sit_end;
          ++sit)
      {
        Component succ = compgraph.sourceOf(*sit);

        // skip successors with eatoms
        if( !compgraph.propsOf(succ).outerEatoms.empty() )
          continue;

        LOG("found successor " << succ);

        ComponentGraph::PredecessorIterator pit, pit_end;
        boost::tie(pit, pit_end) = compgraph.getDependencies(succ);
        bool good = true;
        assert(pit != pit_end);
        if( compgraph.targetOf(*pit) != comp )
        {
          LOG("successor bad as it depends on other node " << compgraph.targetOf(*pit));
          good = false;
        }
        pit++;
        if( pit != pit_end )
        {
          good = false;
          LOG("successor bad as it depends on more nodes");
        }
        if( good )
          collapse.insert(succ);
      }

      if( !collapse.empty() )
      {
        // collapse! (decreases graph size)
        collapse.insert(comp);
        assert(collapse.size() > 1);
        Component c = compgraph.collapseComponents(collapse);
        LOG("collapse of " << printrange(collapse) << " yielded new component " << c);

        // restart loop after collapse
        cit = compgraph.getComponents().first;
      }
      else
      {
        // advance
        ++cit;
      }
    }
    while(cit != compgraph.getComponents().second);
  }

  //
  // create eval units using topological sort
  //

  {
    // we need a hash map, as component graph is no graph with vecS-storage
    //
    typedef boost::unordered_map<Component, boost::default_color_type> CompColorHashMap;
    typedef boost::associative_property_map<CompColorHashMap> CompColorMap;
    CompColorHashMap ccWhiteHashMap;
    // fill white hash map
    ComponentIterator cit, cit_end;
    for(boost::tie(cit, cit_end) = compgraph.getComponents();
        cit != cit_end; ++cit)
    {
      //boost::put(ccWhiteHashMap, *cit, boost::white_color);
      ccWhiteHashMap[*cit] = boost::white_color;
    }
    CompColorHashMap ccHashMap(ccWhiteHashMap);

    //
    // do topological sort
    //
    ComponentContainer comps;
    std::back_insert_iterator<ComponentContainer> compinserter(comps);
    boost::topological_sort(
        compgraph.getInternalGraph(),
        compinserter,
        boost::color_map(CompColorMap(ccHashMap)));

    for(ComponentContainer::const_iterator it = comps.begin();
        it != comps.end(); ++it)
    {
      EvalGraphBuilder::EvalUnit u = builder.createEvalUnit(*it);
      LOG("component " << *it << " became eval unit " << u);
    }
  }
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
