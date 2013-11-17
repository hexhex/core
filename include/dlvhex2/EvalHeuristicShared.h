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
template<typename ComponentGraphIntOrRest, typename Sequence>
void topologicalSortComponents(const ComponentGraphIntOrRest& cg, Sequence& out);

struct BuildCommand
{
	// components to collapse to unit
	ComponentContainer collapse;
	// components to share into unit (constraint components)
	ComponentContainer share;
};
typedef std::vector<BuildCommand> CommandVector;

template<typename EvalGraphT>
void executeBuildCommands(const CommandVector& commands, EvalGraphBuilder<EvalGraphT>& builder);

// template implementation
template<typename ComponentGraphIntOrRest, typename Sequence>
void topologicalSortComponents(const ComponentGraphIntOrRest& cg, Sequence& out)
{
	// we need a hash map, as component graph is no graph with vecS-storage
	typedef boost::unordered_map<Component, boost::default_color_type> CompColorHashMap;
	typedef boost::associative_property_map<CompColorHashMap> CompColorMap;

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
	typename std::back_insert_iterator<Sequence> compinserter(out);
	boost::topological_sort(
		cg,
		compinserter,
		boost::color_map(CompColorMap(ccWhiteHashMap)));
}

}

DLVHEX_NAMESPACE_END

#endif // EVAL_HEURISTIC_SHARED_HPP_INCLUDED__30112011
