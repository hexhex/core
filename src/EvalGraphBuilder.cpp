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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/config_values.h"
#include "dlvhex2/EvalGraphBuilder.h"
#include "dlvhex2/PlainModelGenerator.h"
#include "dlvhex2/WellfoundedModelGenerator.h"
#include "dlvhex2/GuessAndCheckModelGenerator.h"
#include "dlvhex2/GenuinePlainModelGenerator.h"
#include "dlvhex2/GenuineWellfoundedModelGenerator.h"
#include "dlvhex2/GenuineGuessAndCheckModelGenerator.h"
#include "dlvhex2/GenuineGuessAndCheckModelGeneratorAsync.h"
#include "dlvhex2/HTPlainModelGenerator.h"
#include "dlvhex2/SEQPlainModelGenerator.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/PluginInterface.h"

#include <boost/range/iterator_range.hpp>

#include <sstream>

DLVHEX_NAMESPACE_BEGIN

namespace
{
  typedef ComponentGraph::ComponentSet ComponentSet;
  typedef ComponentGraph::ComponentInfo ComponentInfo;
  typedef ComponentGraph::DependencyInfo DependencyInfo;
  typedef ComponentGraph::DepMap DepMap;
}

template<typename EvalGraphT>
EvalGraphBuilder<EvalGraphT>::EvalGraphBuilder(
    ProgramCtx& ctx, 
		ComponentGraph& cg,
    EvalGraphT& eg,
    ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig):
  ctx(ctx),
  clonedcgptr(cg.clone()),
  cg(*clonedcgptr),
  eg(eg),
  externalEvalConfig(externalEvalConfig),
	mapping(),
  unusedEdgeFilter(&cg, &mapping),
  unusedVertexFilter(&mapping),
  cgrest(cg.getInternalGraph(), unusedEdgeFilter, unusedVertexFilter)
{
}

template<typename EvalGraphT>
EvalGraphBuilder<EvalGraphT>::~EvalGraphBuilder()
{
}

template<typename EvalGraphT>
RegistryPtr EvalGraphBuilder<EvalGraphT>::registry()
{
  return ctx.registry();
}

template<typename EvalGraphT>
typename EvalGraphBuilder<EvalGraphT>::Component
EvalGraphBuilder<EvalGraphT>::getComponentForUnit(EvalGraphBuilder::EvalUnit u) const
{
  typename ComponentEvalUnitMapping::right_map::const_iterator it =
    mapping.right.find(u);
  if( it == mapping.right.end() )
    throw std::runtime_error("tried to get component for unit not created here!");
  return it->second;
}

template<typename EvalGraphT>
void EvalGraphBuilder<EvalGraphT>::setFactory(EvalUnit& u, const ComponentInfo& ci)
{
}

template<>
void EvalGraphBuilder<HTEvalGraph>::setFactory(EvalUnit& u, const ComponentInfo& ci)
{
  // TODO: check ComponentInfo
  // configure unit
  EvalUnitProperties& uprops = eg.propsOf(u);
  if (ctx.config.getOption(CFG_SEQ_MODELS)){
    uprops.mgf.reset(new SEQPlainModelGeneratorFactory(ctx, ci, externalEvalConfig));
  }else{
    uprops.mgf.reset(new HTPlainModelGeneratorFactory(ctx, ci, externalEvalConfig));
  }
}

template<>
void EvalGraphBuilder<FinalEvalGraph>::setFactory(EvalUnit& u, const ComponentInfo& ci)
{
  // configure unit
  EvalUnitProperties& uprops = eg.propsOf(u);
  if( ci.innerEatoms.empty() )
  {
    // no inner external atoms -> plain model generator factory
    LOG(DBG,"configuring plain model generator factory for eval unit " << u);
    if (ctx.config.getOption("GenuineSolver") > 0){
      uprops.mgf.reset(new GenuinePlainModelGeneratorFactory(
            ctx, ci, externalEvalConfig));
    }else{
      uprops.mgf.reset(new PlainModelGeneratorFactory(
            ctx, ci, externalEvalConfig));
    }
  }
  else
  {
    if( !ci.innerEatomsNonmonotonic && !ci.negativeDependencyBetweenRules && !ci.disjunctiveHeads )
    {
      // inner external atoms and only in positive cycles and monotonic and no disjunctive rules
		// -> wellfounded/fixpoint model generator factory
      LOG(DBG,"configuring wellfounded model generator factory for eval unit " << u);
      if (ctx.config.getOption("GenuineSolver") > 0){
        uprops.mgf.reset(new GenuineWellfoundedModelGeneratorFactory(
              ctx, ci, externalEvalConfig));
      }else{
        uprops.mgf.reset(new WellfoundedModelGeneratorFactory(
              ctx, ci, externalEvalConfig));
      }
    }
    else
    {
      // everything else -> guess and check model generator factory
      LOG(DBG,"configuring guess and check model generator factory for eval unit " << u);
      if (ctx.config.getOption("GenuineSolver") > 0){
        if (ctx.config.getOption("MultiThreading")){
          uprops.mgf.reset(new GenuineGuessAndCheckModelGeneratorAsyncFactory(
                ctx, ci, externalEvalConfig));
        }else{
          uprops.mgf.reset(new GenuineGuessAndCheckModelGeneratorFactory(
                ctx, ci, externalEvalConfig));
        }
      }else{
        uprops.mgf.reset(new GuessAndCheckModelGeneratorFactory(
              ctx, ci, externalEvalConfig));
      }
    }
  }
}

template<typename EvalGraphT>
typename EvalGraphBuilder<EvalGraphT>::EvalUnit
EvalGraphBuilder<EvalGraphT>::createEvalUnit(
		const std::list<Component>& comps, const std::list<Component>& ccomps)
{
  LOG_SCOPE(ANALYZE,"cEU",true);
  if( Logger::Instance().shallPrint(Logger::DBG) ) {
	DBGLOG(DBG,"= EvalGraphBuilder::createEvalUnit(" << printrange(comps) << "," << printrange(ccomps) << ")");
	BOOST_FOREACH(Component c, comps) {
		const ComponentInfo& ci = cg.propsOf(c);
		if( !ci.innerEatoms.empty() )
			DBGLOG(DBG," compi " << printManyToString<RawPrinter>(ci.innerEatoms, ",", registry()));
		if( !ci.outerEatoms.empty() )
			DBGLOG(DBG," compo " << printManyToString<RawPrinter>(ci.outerEatoms, ",", registry()));
		if( !ci.innerRules.empty() )
			DBGLOG(DBG," compr " << printManyToString<RawPrinter>(ci.innerRules, "\n", registry()));
		if( !ci.innerConstraints.empty() )
			DBGLOG(DBG," compc " << printManyToString<RawPrinter>(ci.innerConstraints, "\n", registry()));
	}
	BOOST_FOREACH(Component c, ccomps) {
		const ComponentInfo& ci = cg.propsOf(c);
		assert( ci.innerRules.empty() && ci.innerEatoms.empty() && ci.outerEatoms.empty() );
		if( !ci.innerConstraints.empty() )
			DBGLOG(DBG," ccompc " << printManyToString<RawPrinter>(ci.innerConstraints, "\n", registry()));
	}
  }

  // collapse components into new eval unit
	// (this verifies necessary conditions and computes new dependencies)
  Component newComp;
	{
		// TODO perhaps directly take ComponentSet as inputs
		ComponentSet scomps(comps.begin(), comps.end());
		ComponentSet sccomps(ccomps.begin(), ccomps.end());
		newComp = cg.collapseComponents(scomps, sccomps);
	}
	const ComponentInfo& newUnitInfo = cg.propsOf(newComp);

  // create eval unit
  EvalUnit u = eg.addUnit(eup_empty);
  LOG(DBG,"created unit " << u << " for new comp " << newComp);

  // associate new comp with eval unit
  {
    typedef typename ComponentEvalUnitMapping::value_type MappedPair;
		bool success = mapping.insert(MappedPair(newComp, u)).second;
		assert(success); // component must not already exist here
	}

  // set model generator factory
  setFactory(u, newUnitInfo);

  // create dependencies
  unsigned joinOrder = 0; // TODO define join order in a more intelligent way?
  ComponentGraph::PredecessorIterator dit, dend;
  for(boost::tie(dit, dend) = cg.getDependencies(newComp);
      dit != dend; dit++)
  {
    Component tocomp = cg.targetOf(*dit);

    // get eval unit corresponding to tocomp
    typename ComponentEvalUnitMapping::left_map::iterator itdo =
      mapping.left.find(tocomp);
    assert(itdo != mapping.left.end());
    EvalUnit dependsOn = itdo->second;

    const DependencyInfo& di = cg.propsOf(*dit);
    DBGLOG(DBG,"adding dependency to unit " << dependsOn << " with joinOrder " << joinOrder);
    eg.addDependency(u, dependsOn, EvalUnitDepProperties(joinOrder));
    joinOrder++;
	}

  return u;
}

template class EvalGraphBuilder<FinalEvalGraph>;
template class EvalGraphBuilder<HTEvalGraph>;

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
