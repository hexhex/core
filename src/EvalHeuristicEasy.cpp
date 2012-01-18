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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/EvalHeuristicEasy.hpp"
#include "dlvhex2/Logger.hpp"

#include <boost/unordered_map.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/properties.hpp>
#include <boost/scoped_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

EvalHeuristicEasy::EvalHeuristicEasy():
  Base()
{
}

EvalHeuristicEasy::~EvalHeuristicEasy()
{
}

typedef ComponentGraph::Component Component;
typedef ComponentGraph::ComponentIterator ComponentIterator;
typedef std::vector<Component> ComponentContainer;
typedef ComponentGraph::ComponentSet ComponentSet;

namespace internal
{

template<typename ComponentGraph, typename Sequence>
void topologicalSortOfComponents(const ComponentGraph& compgraph, Sequence& comps)
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
  std::back_insert_iterator<Sequence> compinserter(comps);
  boost::topological_sort(
      compgraph.getInternalGraph(),
      compinserter,
      boost::color_map(CompColorMap(ccHashMap)));
}

// collect all components on the way
struct DFSVisitor:
  public boost::default_dfs_visitor
{
  const ComponentGraph& cg;
  ComponentSet& comps;
  DFSVisitor(const ComponentGraph& cg, ComponentSet& comps): boost::default_dfs_visitor(), cg(cg), comps(comps) {}
  DFSVisitor(const DFSVisitor& other): boost::default_dfs_visitor(), cg(other.cg), comps(other.comps) {}
  template<typename GraphT>
  void discover_vertex(Component comp, const GraphT&)
  {
    comps.insert(comp);
  }
};

template<typename ComponentGraph, typename Set>
void transitivePredecessorComponents(const ComponentGraph& compgraph, Component from, Set& preds)
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
  // do DFS
  //
  DFSVisitor dfs_vis(compgraph, preds);
  //LOG("doing dfs visit for root " << *itr);
  boost::depth_first_visit(
      compgraph.getInternalGraph(),
      from, 
      dfs_vis,
      CompColorMap(ccHashMap));
  DBGLOG(DBG,"predecessors of " << from << " are " << printrange(preds));
}

typedef std::map<Component, std::list<Component> > Cloned2OrigMap;

// gets cloned cg
// gets components in cloned cg to collapse
// returns new component in cloned cg, 
// updates com accordingly
Component collapseHelper(Cloned2OrigMap& com, ComponentGraph& clonedcg, const ComponentSet& clonedcollapse)
{
  Component ret = clonedcg.collapseComponents(clonedcollapse);

  // insert new empty mapping into com
  com[ret] = std::list<Component>();

  // collect all targets
  // remove sources on the way there
  std::list<Component>& targets = com[ret];
  for(ComponentSet::const_iterator it = clonedcollapse.begin(); it != clonedcollapse.end(); ++it)
  {
    Cloned2OrigMap::iterator comit = com.find(*it);
    assert(comit != com.end());
    targets.insert(targets.end(), comit->second.begin(), comit->second.end());

    // erase record in com
    com.erase(*it);
  }

  return ret;
}

}

// required for some GCCs for DFSVisitor CopyConstructible Concept Check
using namespace internal;

void EvalHeuristicEasy::build(EvalGraphBuilder& builder)
{
  const ComponentGraph& constcompgraph = builder.getComponentGraph();
	boost::scoped_ptr<ComponentGraph> pcompgraph(constcompgraph.clone());
  ComponentGraph& compgraph(*pcompgraph);
  // build mapping from cloned to original component graph
  Cloned2OrigMap cloned2orig;
  {
    ComponentGraph::ComponentIterator cit, cit_end, ccit, ccit_end;
    boost::tie(cit, cit_end) = constcompgraph.getComponents();
    boost::tie(ccit, ccit_end) = compgraph.getComponents();
    for(;cit != cit_end && ccit != ccit_end; cit++, ccit++)
    {
      std::list<Component> comps;
      comps.push_back(*cit);
      cloned2orig[*ccit] = comps;
    }
  }

  bool didSomething;
  do
  {
    didSomething = false;

  //
  // forall external components e:
  // merge with all rules that 
  // * depend on e 
  // * do not contain external atoms
  // * do not depend on something e does not (transitively) depend on
  //
  {
    ComponentIterator cit;
    for(cit = compgraph.getComponents().first; // do not use boost::tie here! the container is modified in the loop!
        cit != compgraph.getComponents().second; ++cit)
    {
      Component comp = *cit;
      if( compgraph.propsOf(comp).outerEatoms.empty() )
        continue;

      LOG(ANALYZE,"checking whether to collapse external component " << comp << " with successors");

      // get predecessors
      ComponentSet preds;
      transitivePredecessorComponents(compgraph, comp, preds);

      // get successors
      ComponentSet collapse;
      bool addedToCollapse;
      // do this as long as we find new ones
      // if we do not do this loop, we might miss something
      // as PredecessorIterator not necessarily honours topological order
      // (TODO this could be made more efficient)
      do
      {
        addedToCollapse = false;

        ComponentGraph::SuccessorIterator sit, sit_end;
        for(boost::tie(sit, sit_end) = compgraph.getProvides(comp);
            sit != sit_end; ++sit)
        {
          Component succ = compgraph.sourceOf(*sit);

          // skip successors with eatoms
          if( !compgraph.propsOf(succ).outerEatoms.empty() )
            continue;
          // do not check found stuff twice
          if( collapse.find(succ) != collapse.end() )
            continue;

          DBGLOG(DBG,"found successor " << succ);

          ComponentGraph::PredecessorIterator pit, pit_end;
          bool good = true;
          for(boost::tie(pit, pit_end) = compgraph.getDependencies(succ);
              pit != pit_end; ++pit)
          {
            Component dependson = compgraph.targetOf(*pit);
            if( preds.find(dependson) == preds.end() )
            {
              LOG(DBG,"successor bad as it depends on other node " << dependson);
              good = false;
              break;
            }
          }
          if( good )
          {
            // collapse with this
            collapse.insert(succ);
            preds.insert(succ);
            addedToCollapse = true;
          }
        }
      }
      while(addedToCollapse);

      // collapse if not nonempty
      if( !collapse.empty() )
      {
        collapse.insert(comp);
        Component c = collapseHelper(cloned2orig, compgraph, collapse);
        LOG(ANALYZE,"collapse of " << printrange(collapse) << " yielded new component " << c);

        // restart loop after collapse
        cit = compgraph.getComponents().first;
        didSomething = true;
      }
    }
  }

  //
  // forall components with only inner rules or constraints:
  // merge with children that are no eatoms and do not depend on anything else
  //
  {
    ComponentIterator cit = compgraph.getComponents().first;
    while(cit != compgraph.getComponents().second)
    {
      Component comp = *cit;
      if( !compgraph.propsOf(comp).outerEatoms.empty() )
      {
        cit++;
        continue;
      }

      LOG(ANALYZE,"checking whether to collapse internal-only component " << comp << " with children");

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

        DBGLOG(DBG,"found successor " << succ);

        ComponentGraph::PredecessorIterator pit, pit_end;
        boost::tie(pit, pit_end) = compgraph.getDependencies(succ);
        bool good = true;
        assert(pit != pit_end);
        if( compgraph.targetOf(*pit) != comp )
        {
          LOG(DBG,"successor bad as it depends on other node " << compgraph.targetOf(*pit));
          good = false;
        }
        pit++;
        if( pit != pit_end )
        {
          good = false;
          LOG(DBG,"successor bad as it depends on more nodes");
        }
        if( good )
          collapse.insert(succ);
      }

      if( !collapse.empty() )
      {
        // collapse! (decreases graph size)
        collapse.insert(comp);
        assert(collapse.size() > 1);
        Component c = collapseHelper(cloned2orig, compgraph, collapse);
        LOG(ANALYZE,"collapse of " << printrange(collapse) << " yielded new component " << c);

        // restart loop after collapse
        cit = compgraph.getComponents().first;
        didSomething = true;
      }
      else
      {
        // advance
        ++cit;
      }
    }
  }

  //
  // forall components with only inner rules or constraints:
  // merge with components that depend on exactly the same predecessors
  //
  {
    ComponentIterator cit = compgraph.getComponents().first;
    while(cit != compgraph.getComponents().second)
    {
      Component comp = *cit;
      if( !compgraph.propsOf(comp).outerEatoms.empty() )
      {
        cit++;
        continue;
      }

      LOG(ANALYZE,"checking whether to collapse internal-only component " << comp << " with others");
      ComponentSet collapse;

      // get direct predecessors
      ComponentSet preds;
      {
        ComponentGraph::PredecessorIterator pit, pit_end;
        for(boost::tie(pit, pit_end) = compgraph.getDependencies(comp);
            pit != pit_end; ++pit)
        {
          preds.insert(compgraph.targetOf(*pit));
        }
      }
      if( preds.empty() )
      {
        // do not combine stuff that depends only on edb
        cit++;
        continue;
      }

      // compare all further ones (further because of symmetry breaking)
      ComponentIterator cit2 =  cit;
      cit2++;
      while( cit2 != compgraph.getComponents().second )
      {
        Component comp2 = *cit2;
        DBGLOG(DBG,"checking other component " << comp2);
        ComponentSet preds2;
        {
          ComponentGraph::PredecessorIterator pit, pit_end;
          for(boost::tie(pit, pit_end) = compgraph.getDependencies(comp2);
              pit != pit_end; ++pit)
          {
            preds2.insert(compgraph.targetOf(*pit));
          }
        }

        if( preds2 == preds )
          collapse.insert(comp2);

        cit2++;
      }

      if( !collapse.empty() )
      {
        // collapse! (decreases graph size)
        collapse.insert(comp);
        assert(collapse.size() > 1);
        Component c = collapseHelper(cloned2orig, compgraph, collapse);
        LOG(ANALYZE,"collapse of " << printrange(collapse) << " yielded new component " << c);

        // restart loop after collapse
        cit = compgraph.getComponents().first;
        didSomething = true;
      }
      else
      {
        // advance
        ++cit;
      }
    }
  }

  //
  // forall components with only inner constraints:
  // merge with all other constraint-only components
  //
  if(false){
    ComponentSet collapse;

    ComponentIterator cit;
    for(cit = compgraph.getComponents().first; // do not use boost::tie here! the container is modified in the loop!
        cit != compgraph.getComponents().second; ++cit)
    {
      Component comp = *cit;
      if( compgraph.propsOf(comp).outerEatoms.empty() &&
          compgraph.propsOf(comp).innerRules.empty() )
        collapse.insert(comp);
    }

    if( !collapse.empty() )
    {
      // collapse! (decreases graph size)
      LOG(ANALYZE,"collapsing constraint-only nodes " << printrange(collapse));
      Component c = collapseHelper(cloned2orig, compgraph, collapse);
      didSomething = true;
    }
  }

  }
  while(didSomething);

  //
  // create eval units using topological sort
  //
  ComponentContainer sortedcomps;
  topologicalSortOfComponents(compgraph, sortedcomps);
  for(ComponentContainer::const_iterator it = sortedcomps.begin();
      it != sortedcomps.end(); ++it)
  {
    // <foo>.find(<bar>)->second has an implicit assert() at the iterator dereferencing
		const std::list<Component>& comps = cloned2orig.find(*it)->second;
    std::list<Component> ccomps;
    EvalGraphBuilder::EvalUnit u = builder.createEvalUnit(comps, ccomps);
    LOG(ANALYZE,"component " << *it << " became eval unit " << u);
  }
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
