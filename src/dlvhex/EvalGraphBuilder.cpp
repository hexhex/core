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
 * @file EvalGraphBuilder.cpp
 * @author Peter Schüller
 *
 * @brief Implementation of the eval graph builder.
 */

#include "dlvhex/EvalGraphBuilder.hpp"
#include "dlvhex/FinalModelGenerator.hpp"
#include "dlvhex/Logger.hpp"

#include <boost/range/iterator_range.hpp>

#include <sstream>

DLVHEX_NAMESPACE_BEGIN

#if 0
template<typename EvalGraphT>
bool
EvalGraphBuilder<EvalGraphT>::UnusedEdgeFilter::operator()(
    ComponentGraph::Dependency dep) const
{
  assert(cg);
  assert(ucmap);

  // edge is good (= unused) if both vertices are unused
  ComponentGraph::Node n1 = cg->sourceOf(dep);
  if( (*ucmap)[static_cast<unsigned>(n1)] == false )
    return false;
  ComponentGraph::Node n2 = cg->targetOf(dep);
  return (*ucmap)[static_cast<unsigned>(n2)];
}
#endif

//template<typename EvalGraphT>
EvalGraphBuilder::EvalGraphBuilder(
		ComponentGraph& cg, EvalGraphT& eg):
	cg(cg), eg(eg)
  #if 0
  ,
  // todo mapping
  unusedNodes(cg.countNodes(), true),
  unusedEdgeFilter(&cg, &unusedNodes),
  unusedVertexFilter(&unusedNodes),
  cgrest(cg.getInternalGraph(), unusedEdgeFilter, unusedVertexFilter),
  cgrestLeaves(cg.getLeaves())
  #endif
{
}

//template<typename EvalGraphT>
EvalGraphBuilder::~EvalGraphBuilder()
{
}

#if 0
// create eval unit
// update unusedNodes
// update cgrestLeaves
template<typename EvalGraphT>
template<typename NodeRange, typename UnitRange>
typename EvalGraphT::EvalUnit
EvalGraphBuilder<EvalGraphT>::createEvalUnit(
  NodeRange nodes, UnitRange orderedDependencies)
{
	typename NodeRange::iterator itn;
	typename UnitRange::iterator itu;
	for(itn = boost::begin(nodes); itn != boost::end(nodes); ++itn)
	{
		LOG("adding node " << *itn << " to new eval unit");
		// TODO
	}
	for(itu = boost::begin(orderedDependencies); itu != boost::end(orderedDependencies); ++itu)
	{
		LOG("adding dependency to unit " << *itu << " to eval graph");
		// TODO
	}
  //supInvalid = true;
}

#if 0
template<typename EvalGraphT>
void EvalGraphBuilder<EvalGraphT>::recalculateSupportingInformation()
{
  supInvalid = false;
}
#endif
#endif

typedef FinalEvalGraph::EvalUnitPropertyBundle
  EvalUnitProperties;
typedef FinalEvalGraph::EvalUnitDepPropertyBundle
  EvalUnitDepProperties;

namespace
{
  EvalUnitProperties eup_empty;
}

EvalGraphBuilder::EvalUnit
EvalGraphBuilder::createEvalUnit(
    Component comp)
{
  LOG_SCOPE("cEU", false);
  LOG("= EvalGraphBuilder::createEvalUnit(" << comp << ")");

  // create eval unit
  EvalUnit u = eg.addUnit(eup_empty);
  LOG("created unit " << u);

  // associate with component
  typedef ComponentEvalUnitMapping::value_type MappedPair;
  bool success = mapping.insert(MappedPair(comp, u)).second;
  assert(success); // component must not already exist here

  // configure unit
  EvalUnitProperties& uprops = eg.propsOf(u);

  // TODO configure model generator factory depending on type of component
  // TODO configure model generator factory depending on compiletime/runtime configuration
  // TODO the above matters require a refactoring, the line below is for initial tests only
  uprops.mgf.reset(new FinalModelGeneratorFactory(cg.propsOf(comp)));

  // create dependencies

  // find all dependencies and create them in eval graph
  ComponentGraph::PredecessorIterator it, it_end;
  unsigned joinOrder = 0;
  for(boost::tie(it, it_end) = cg.getDependencies(comp);
      it != it_end; ++it)
  {
    Component dcomp = cg.targetOf(*it);
    LOG("found dependency to component " << dcomp);
    ComponentEvalUnitMapping::left_const_iterator itu =
      mapping.left.find(dcomp);
    if( itu == mapping.left.end() )
      throw std::runtime_error(
          "tried to create an eval unit, "
          "where not all predecessors have previously been created!");
    EvalUnit du = itu->second;
    LOG("adding dependency to unit " << du << " with joinOrder " << joinOrder);
    eg.addDependency(u, du, EvalUnitDepProperties(joinOrder));
    joinOrder++;
  }

  return u;
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
