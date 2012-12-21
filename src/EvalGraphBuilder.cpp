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

#include "dlvhex2/EvalGraphBuilder.h"
#include "dlvhex2/PlainModelGenerator.h"
#include "dlvhex2/WellfoundedModelGenerator.h"
#include "dlvhex2/GuessAndCheckModelGenerator.h"
#include "dlvhex2/GenuinePlainModelGenerator.h"
#include "dlvhex2/GenuineWellfoundedModelGenerator.h"
#include "dlvhex2/GenuineGuessAndCheckModelGenerator.h"
#include "dlvhex2/GenuineGuessAndCheckModelGeneratorAsync.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/PluginInterface.h"

#include <boost/range/iterator_range.hpp>

#include <sstream>

DLVHEX_NAMESPACE_BEGIN

EvalGraphBuilder::EvalGraphBuilder(
    ProgramCtx& ctx, 
		ComponentGraph& cg,
    EvalGraphT& eg,
    ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig):
  ctx(ctx),
	cg(cg),
  eg(eg),
  externalEvalConfig(externalEvalConfig),
	mapping(),
  unusedEdgeFilter(&cg, &mapping),
  unusedVertexFilter(&mapping),
  cgrest(cg.getInternalGraph(), unusedEdgeFilter, unusedVertexFilter)
{
}

EvalGraphBuilder::~EvalGraphBuilder()
{
}

RegistryPtr EvalGraphBuilder::registry()
{
  return ctx.registry();
}

#if 0
template<typename NodeRange>
EvalGraphBuilder::createEvalUnit(NodeRange nodes)
{
	typename NodeRange::iterator itn;
	for(itn = boost::begin(nodes); itn != boost::end(nodes); ++itn)
	{
		LOG("adding node " << *itn << " to new eval unit");
		// TODO
	}
#endif

typedef FinalEvalGraph::EvalUnitPropertyBundle
  EvalUnitProperties;
typedef FinalEvalGraph::EvalUnitDepPropertyBundle
  EvalUnitDepProperties;

namespace
{
  EvalUnitProperties eup_empty;
}

// build evaluation unit from components <comps> and copy into this unit also constraints in <ccomps>
//
// look at all dependencies outgoing from <comps>
// * dependencies to existing units (in this->mapping.left) are accumulated
//   into unit ependencies -> <newUnitsDependOn>
// * dependencies to <comps> are rememberd to determine whether eatoms are inner or outer
//   -> this is used to build newUnitInfo
// * all other dependencies are forbidden!
//   -> this ensures that no cycles are introduced
//
void EvalGraphBuilder::calculateNewEvalUnitInfos(
		const ComponentSet& comps, const ComponentSet& ccomps,
		std::list<DependencyInfo>& newUnitDependsOn,
		ComponentInfo& newUnitInfo)
{
	DBGLOG_SCOPE(DBG,"cEUI", false);
	DBGLOG(DBG,"= calculateEvalUnitInfos(" << printrange(comps) <<
			"," << printrange(ccomps) << ")");

	// set of dependencies from the new components to other components
	// key = other eval unit
	// value = dependency info
	typedef std::map<EvalUnit, DependencyInfo> DepMap;
	DepMap outgoing;

  // set of original components that depend on other original components
	// (we need this to find out whether an eatom in a component is an outer or
	// an inner eatom ... if it depends on stuff in the components it is inner)
  ComponentSet internallyDepends;

	// iterate over all originals and over their outgoing dependencies (what they depend on)
	ComponentSet::const_iterator ito;
	bool foundInternalNegativeRuleDependency = false;
	for(ito = comps.begin(); ito != comps.end(); ++ito)
	{
		DBGLOG(DBG,"original " << *ito << ":");
		DBGLOG_INDENT(DBG);

		ComponentGraph::PredecessorIterator itpred, itpred_end;
		for(boost::tie(itpred, itpred_end) = cg.getDependencies(*ito);
				itpred != itpred_end; ++itpred)
		{
			Dependency outgoing_dep = *itpred;
			Component target = cg.targetOf(outgoing_dep);
			if( comps.find(target) == comps.end() )
			{
				// dependency not within the new collapsed component
				// -> dependency must be to eval unit
				// -> it must be to already mapped component
				DBGLOG(DBG,"outgoing dependency to " << target);
				// map component to unit
				ComponentEvalUnitMapping::left_map::iterator itu(
						mapping.left.find(target));
				#ifndef NDEBUG
				// be nice: give a nice error message
				if( itu == mapping.left.end() )
				{
					LOG(ERROR,"outgoing dependency from component " << *ito <<
							" to component " << target << " not allowed because "
							" the target is not yet mapped to an evaluation unit");
				}
				#endif
				const ComponentGraph::DependencyInfo& comp_depinfo =
					cg.propsOf(outgoing_dep);

				// accumulate dependency info into <outgoing>
				assert(itu != mapping.left.end());
				EvalUnit dependsOn = itu->second;
				DepMap::iterator itdo = outgoing.find(dependsOn);
				if( itdo == outgoing.end() )
				{
					outgoing.insert(std::make_pair(
								dependsOn, DependencyInfo(dependsOn, comp_depinfo)));
				}
				else
				{
					itdo->second |= comp_depinfo;
				}
			}
      else
      {
				// dependency to other component within the new collapsed component

				const ComponentGraph::DependencyInfo& comp_depinfo =
					cg.propsOf(outgoing_dep);

				DBGLOG(DBG,"internal dependency (to " << target << ")");
				internallyDepends.insert(*ito);

				if( comp_depinfo.negativeRule )
					foundInternalNegativeRuleDependency = true;
      }
		} // iterate over predecessors
	} // iterate over originals

	//
	// build newUnitInfo
	//
	ComponentInfo& ci = newUnitInfo;
	for(ito = comps.begin(); ito != comps.end(); ++ito)
	{
		const ComponentInfo& cio = cg.propsOf(*ito);
    #ifdef COMPGRAPH_SOURCESDEBUG
		ci.sources.insert(ci.sources.end(),
				cio.sources.begin(), cio.sources.end());
		#endif
    // inner rules stay inner rules
		ci.innerRules.insert(ci.innerRules.end(),
				cio.innerRules.begin(), cio.innerRules.end());
    // inner eatoms always stay inner eatoms, they cannot become outer eatoms
		ci.innerEatoms.insert(ci.innerEatoms.end(),
				cio.innerEatoms.begin(), cio.innerEatoms.end());
    // inner constraints stay inner constraints
		ci.innerConstraints.insert(ci.innerConstraints.end(),
				cio.innerConstraints.begin(), cio.innerConstraints.end());
    // information about strongly safe variables and stratified literals
		typedef std::pair<ID, std::set<ID> > Pair;
		BOOST_FOREACH (Pair p, cio.stronglySafeVariables){
			BOOST_FOREACH (ID id, p.second){
				ci.stronglySafeVariables[p.first].insert(id);
			}
		}
		ci.predicatesInComponent.insert(
				cio.predicatesInComponent.begin(), cio.predicatesInComponent.end());
/*
		BOOST_FOREACH (Pair p, cio.stratifiedLiterals){
			BOOST_FOREACH (ID id, p.second){
				ci.stratifiedLiterals[p.first].insert(id);
			}
		}
*/

    ci.disjunctiveHeads |= cio.disjunctiveHeads;
    ci.negationInCycles |= cio.negationInCycles;
		ci.innerEatomsNonmonotonic |= cio.innerEatomsNonmonotonic;
    ci.fixedDomain |= cio.fixedDomain;
    ci.componentIsMonotonic |= cio.componentIsMonotonic;

    // if *ito does not depend on any component in originals
    // then outer eatoms stay outer eatoms
    // otherwise they become inner eatoms
    if( internallyDepends.find(*ito) == internallyDepends.end() )
    {
      // does not depend on other components
      ci.outerEatoms.insert(ci.outerEatoms.end(),
          cio.outerEatoms.begin(), cio.outerEatoms.end());
			ci.outerEatomsNonmonotonic |= cio.outerEatomsNonmonotonic;
    }
    else
    {
      // does depend on other components
      // -> former outer eatoms now become inner eatoms
      ci.innerEatoms.insert(ci.innerEatoms.end(),
          cio.outerEatoms.begin(), cio.outerEatoms.end());

      // here, outer eatom becomes inner eatom
			ci.innerEatomsNonmonotonic |= cio.outerEatomsNonmonotonic;
    }
    #warning if "input" component consists only of eatoms, they may be nonmonotonic, and we still can have wellfounded model generator ... create testcase for this ? how about wellfounded2.hex?
	}
  ci.negationInCycles |= foundInternalNegativeRuleDependency;
	ComponentGraph::calculateStratificationInfo(registry(), ci);

	//
	// build newUnitDependsOn
	//
	// (we only build outgoing dependencies)
	for(DepMap::const_iterator itd = outgoing.begin();
			itd != outgoing.end(); ++itd)
	{
		newUnitDependsOn.push_back(itd->second);
	}
}

EvalGraphBuilder::EvalUnit
EvalGraphBuilder::createEvalUnit(
		const std::list<Component>& comps, const std::list<Component>& ccomps)
{
  LOG_SCOPE(ANALYZE,"cEU",true);
  DBGLOG(DBG,"= EvalGraphBuilder::createEvalUnit(" <<
			printrange(comps) << "," << printrange(ccomps) << ")");

	// calculate properties of new eval unit
	// (this verifies a lot of stuff in debug mode)
	std::list<DependencyInfo> newUnitDependsOn;
	ComponentInfo newUnitInfo;
	{
		// TODO perhaps directly take ComponentSet as inputs
		ComponentSet scomps(comps.begin(), comps.end());
		ComponentSet sccomps(ccomps.begin(), ccomps.end());
		calculateNewEvalUnitInfos(scomps, sccomps, newUnitDependsOn, newUnitInfo);
	}

  // create eval unit
  EvalUnit u = eg.addUnit(eup_empty);
  LOG(DBG,"created unit " << u);

  // associate comps with component
	// ignore shared ccomps here (see comments in .hpp file)
	BOOST_FOREACH(Component c, comps)
	{
		typedef ComponentEvalUnitMapping::value_type MappedPair;
		bool success = mapping.insert(MappedPair(c, u)).second;
		assert(success); // component must not already exist here
	}

  // configure unit
  EvalUnitProperties& uprops = eg.propsOf(u);

  // configure model generator factory, depending on type of component
  {
    const ComponentGraph::ComponentInfo& ci = newUnitInfo;
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
      if( !ci.innerEatomsNonmonotonic && !ci.negationInCycles && !ci.disjunctiveHeads )
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

  // create dependencies
  unsigned joinOrder = 0;
	BOOST_FOREACH(const DependencyInfo& di, newUnitDependsOn)
	{
		// TODO join order?
    DBGLOG(DBG,"adding dependency to unit " << di.dependsOn << " with joinOrder " << joinOrder);
    eg.addDependency(u, di.dependsOn, EvalUnitDepProperties(joinOrder));
    joinOrder++;
	}

  return u;
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
