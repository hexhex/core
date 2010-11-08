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

namespace
{
  struct OriginalsDFSVisitor:
    public boost::default_dfs_visitor
  {
    ComponentSet& origs;
    OriginalsDFSVisitor(ComponentSet& origs): boost::default_dfs_visitor(), origs(origs) {}
    OriginalsDFSVisitor(const OriginalsDFSVisitor& other): boost::default_dfs_visitor(), origs(other.origs) {}
    void discover_vertex(Component comp, const ComponentGraph::Graph&)
    {
      LOG("discover " << comp);
      origs.insert(comp);
    }
  };

  struct OriginalsDFSTerminator
  {
    const ComponentGraph& cg;
    const ComponentSet& origs;
    OriginalsDFSTerminator(const ComponentGraph& cg, const ComponentSet& origs):
      cg(cg), origs(origs) {}
    bool operator()(Component comp, const ComponentGraph::Graph&)
    {
      // return true if vertex shall not be expanded
      // -> return true for eatoms and for members of origs
      bool hasEAtoms = (!cg.propsOf(comp).outerEatoms.empty());
      bool isOrig = (origs.find(comp) != origs.end());
      LOG("terminate?=" << hasEAtoms << "/" << isOrig << " @ " << comp);
      return hasEAtoms || isOrig;
    }
  };
}

// old dlvhex approach:
// 1) start at all roots (= components not depending on another component)
// 2) mark from there using dfs/bfs until eatom hit (including eatom)
// 3) collapse this into new component c
// 4) take all nodes c depends on as new roots
// 5) goto 2)
void EvalHeuristicOldDlvhex::build()
{
  ComponentGraph& compgraph = builder.getComponentGraph();

	typedef std::list<Component> RootContainer;
	RootContainer roots;
	ComponentIterator cit, cit_end;
	// find roots
	for(boost::tie(cit, cit_end) = compgraph.getComponents();
			cit != cit_end; ++cit)
	{
		SuccessorIterator sit, sit_end;
		boost::tie(sit, sit_end) = compgraph.getProvides(*cit);
		if( sit == sit_end )
		{
			roots.push_back(*cit);
		}
	}

	// prepare dfs

	// TODO think about how this can be made more efficient (below is the failsafe and slow version)
	// (we could use two_bit_vector_property_map with custom IndexMap,
	//  the index map knows the graph and can use properties of vertices to get some
	//  index stored in the properties, the index is monotonically increasing and
	//  managed by ComponentGraph.
	//  we can preallocate a big property map (1.5*maxindex) -> no reallocations)

	// we need a hash map, as component graph is no graph with vecS-storage
	typedef boost::unordered_map<Component, boost::default_color_type> CompColorHashMap;
	typedef boost::associative_property_map<CompColorHashMap> CompColorMap;
	CompColorHashMap ccWhiteHashMap;

	// fill white hash map
	for(boost::tie(cit, cit_end) = compgraph.getComponents();
			cit != cit_end; ++cit)
	{
		//boost::put(ccWhiteHashMap, *cit, boost::white_color);
		ccWhiteHashMap[*cit] = boost::white_color;
	}

	do
	{
		// step 2
		LOG("starting step 2 of collapsing with roots " << printrange(roots));

		// components to collapse
		typedef std::set<Component> ComponentSet;
		ComponentSet originals;

		// do dfs:
		// from each root
		// up to and including first eatom
		// add nodes to originals
		// if hitting an original, do not continue (pointless)

		for(RootContainer::const_iterator itr = roots.begin();
				itr != roots.end(); ++itr)
		{
      ComponentSet neworiginals;
      OriginalsDFSVisitor dfs_vis(neworiginals);
			CompColorHashMap ccHashMap(ccWhiteHashMap);
			//CompColorMap ccMap(ccHashMap);
			LOG("doing dfs visit for root " << *itr);
			boost::depth_first_visit(
					compgraph.getInternalGraph(),
					*itr, 
					dfs_vis,
					CompColorMap(ccHashMap),
					OriginalsDFSTerminator(compgraph, originals));
      originals.insert(neworiginals.begin(), neworiginals.end());
      neworiginals.clear();
			LOG("dfs visit terminated: originals = " << printrange(originals));
		}

    // collapse originals into new component
		Component newcomp = compgraph.collapseComponents(originals);
    LOG("collapsing " << printrange(originals) << " yielded component " << newcomp);

    // calculate new roots (= all successors of new component)
    roots.clear();
    PredecessorIterator pit, pit_end;
    for(boost::tie(pit, pit_end) = compgraph.getDependencies(newcomp);
        pit != pit_end; ++pit)
    {
			roots.push_back(compgraph.targetOf(*pit));
		}
	}
	while( !roots.empty() );
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
