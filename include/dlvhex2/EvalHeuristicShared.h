/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Thomas Krennwallner
 * Copyright (C) 2009, 2010, 2011 Peter Schüller
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
 * @file EvalHeuristicShared.h
 * @author Peter Schüller
 *
 * @brief Code used in multiple evaluation heuristics.
 */

#ifndef EVAL_HEURISTIC_SHARED_HPP_INCLUDED__30112011
#define EVAL_HEURISTIC_SHARED_HPP_INCLUDED__30112011

#include "dlvhex2/EvalHeuristicBase.h"
#include "dlvhex2/EvalGraphBuilder.h"

#include <boost/graph/topological_sort.hpp>
#include <boost/property_map/property_map.hpp>

DLVHEX_NAMESPACE_BEGIN

namespace evalheur
{

typedef ComponentGraph::Component Component;
typedef ComponentGraph::ComponentIterator ComponentIterator;
typedef std::vector<Component> ComponentContainer;
typedef std::set<Component> ComponentSet;

// topological sort of all components in graph into vector
//
// takes either internal component graph or component graph rest
template<typename ComponentGraphIntOrRest>
void topologicalSortComponents(const ComponentGraphIntOrRest& cg, ComponentContainer& out);



struct BuildCommand
{
	// components to collapse to unit
	ComponentContainer collapse;
	// components to share into unit (constraint components)
	ComponentContainer share;
};
typedef std::vector<BuildCommand> CommandVector;

void executeBuildCommands(const CommandVector& commands, EvalGraphBuilder& builder);

}

namespace evalheur
{

void executeBuildCommands(const CommandVector& commands, EvalGraphBuilder& builder)
{
  const ComponentGraph& compgraph = builder.getComponentGraph();

  // collapse according to commands
  BOOST_FOREACH(const BuildCommand& cmd, commands)
  {
    LOG(ANALYZE,"BuildCommand collapses components " <<
				printvector(cmd.collapse) <<
				" and shared components " << printvector(cmd.share));
		std::list<Component> comps(cmd.collapse.begin(), cmd.collapse.end());
		std::list<Component> ccomps(cmd.share.begin(), cmd.share.end());
		EvalGraphBuilder::EvalUnit u = builder.createEvalUnit(comps, ccomps);
		LOG(ANALYZE,"yields eval unit " << u);
  }
}

// we need a hash map, as component graph is no graph with vecS-storage
typedef boost::unordered_map<Component, boost::default_color_type> CompColorHashMap;
typedef boost::associative_property_map<CompColorHashMap> CompColorMap;

template<typename ComponentGraphIntOrRest>
void topologicalSortComponents(const ComponentGraphIntOrRest& cg, ComponentContainer& out)
{
	// create white hash map for topological sort
	CompColorHashMap ccWhiteHashMap;
	{
    typename boost::graph_traits<ComponentGraphIntOrRest>::vertex_iterator cit, cit_end;
    for(boost::tie(cit, cit_end) = boost::vertices(cg);
        cit != cit_end; ++cit)
    {
      ccWhiteHashMap[*cit] = boost::white_color;
		}
	}

  assert(out.empty());
  std::back_insert_iterator<ComponentContainer> compinserter(out);
  boost::topological_sort(
      cg,
      compinserter,
      boost::color_map(CompColorMap(ccWhiteHashMap)));
}

}

DLVHEX_NAMESPACE_END

#endif // EVAL_HEURISTIC_SHARED_HPP_INCLUDED__30112011
