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
#include "dlvhex/PlainModelGenerator.hpp"
#include "dlvhex/WellfoundedModelGenerator.hpp"
#include "dlvhex/GuessAndCheckModelGenerator.hpp"
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

//template<typename EvalGraphT>
EvalGraphBuilder::~EvalGraphBuilder()
{
}

#if 0
template<typename NodeRange>
EvalGraphBuilder<EvalGraphT>::createEvalUnit(NodeRange nodes)
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

namespace
{

typedef ComponentGraph::ComponentInfo ComponentInfo;
struct DependencyInfo:
	public ComponentGraph::DependencyInfo
{
	// this is not a property of a graph,
	// we use this when we calculate the dependencies of a new unit
	// and for that we need to know on which units it depends
	EvalUnit dependsOn;
};
typedef ComponentGraph::Component Component;
typedef ComponentGraph::ComponentSet ComponentSet;

// collapse components given in range into one new component
// collapse incoming and outgoing dependencies
// update properties of dependencies
// update properties of component
// asserts that this operation does not make the DAG cyclic
void collapseComponents(
		const ComponentSet& originals)
{
	DBGLOG_SCOPE(DBG,"cC", false);
	DBGLOG(DBG,"= collapseComponents(" << printrange(originals) << ")");

	typedef std::map<Component, DependencyInfo> DepMap;

	// set of dependencies from the new component to other components
	DepMap outgoing;
  // set of original components that depend on original components
  ComponentSet internallyDepends;

	// iterate over all originals and over outgoing dependencies
	ComponentSet::const_iterator ito;
	for(ito = originals.begin(); ito != originals.end(); ++ito)
	{
		DBGLOG(DBG,"original " << *ito << ":");
		DBGLOG_INDENT(DBG);

		PredecessorIterator itpred, itpred_end;
		for(boost::tie(itpred, itpred_end) = getDependencies(*ito);
				itpred != itpred_end; ++itpred)
		{
			Dependency outgoing_dep = *itpred;
			Component target = targetOf(outgoing_dep);
			if( originals.count(target) == 0 )
			{
				// dependency not within the new collapsed component
				DBGLOG(DBG,"outgoing dependency to " << target);
				outgoing[target] |= propsOf(outgoing_dep);
			}
      else
      {
				// dependency within the new collapsed component
				DBGLOG(DBG,"internal dependency (to " << target << ")");
				internallyDepends.insert(*ito);
      }
		} // iterate over predecessors
	} // iterate over originals

	// dependencies of other components on the new component
	DepMap incoming;

	// iterate over all originals and over incoming dependencies
  // now also check for duplicate violations
	for(ito = originals.begin(); ito != originals.end(); ++ito)
	{
		DBGLOG(DBG,"original " << *ito << ":");
		DBGLOG_INDENT(DBG);

		SuccessorIterator itsucc, itsucc_end;
		for(boost::tie(itsucc, itsucc_end) = getProvides(*ito);
				itsucc != itsucc_end; ++itsucc)
		{
			Dependency incoming_dep = *itsucc;
			Component source = sourceOf(incoming_dep);
			if( originals.count(source) == 0 )
			{
				// do not count dependencies within the new collapsed component
				DBGLOG(DBG,"incoming dependency from " << source);
				incoming[source] |= propsOf(incoming_dep);
				// ensure that we do not create cycles
        // (this check is not too costly, so this is no assertion but a real runtime check)
				DepMap::const_iterator itdm = outgoing.find(source);
				// if we have an incoming dep and an outgoing dep,
				// we create a cycle so this collapsing is invalid
        // (this is a bug in the code calling collapseComponents!)
        if( itdm != outgoing.end() )
        {
          throw std::runtime_error(
              "collapseComponents tried to create a cycle!");
        }
			}
		} // iterate over successors
	} // iterate over originals

	//
	// we prepared all dependencies, so now we create the component
	//

	Component c = boost::add_vertex(cg);
	LOG(DBG,"created component node " << c << " for collapsed component");

	// build combined component info
	ComponentInfo& ci = propsOf(c);
  assert(ci.innerEatomsMonotonicAndOnlyPositiveCycles &&
      "ComponentInfo constructor should set this to true");
	for(ito = originals.begin(); ito != originals.end(); ++ito)
	{
		ComponentInfo& cio = propsOf(*ito);
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

    // innerEatomsMonotonicAndOnlyPositiveCycles
    // is false if it is false for any component
    ci.innerEatomsMonotonicAndOnlyPositiveCycles &=
      cio.innerEatomsMonotonicAndOnlyPositiveCycles;

    // if *ito does not depend on any component in originals
    // then outer eatoms stay outer eatoms
    // otherwise they become inner eatoms
    if( internallyDepends.find(*ito) == internallyDepends.end() )
    {
      // does not depend on other components
      ci.outerEatoms.insert(ci.outerEatoms.end(),
          cio.outerEatoms.begin(), cio.outerEatoms.end());
    }
    else
    {
      // does depend on other components
      // -> former outer eatoms now become inner eatoms
      ci.innerEatoms.insert(ci.innerEatoms.end(),
          cio.outerEatoms.begin(), cio.outerEatoms.end());

      // innerEatomsMonotonicAndOnlyPositiveCycles
      // is false if any outer eatom that became an inner eatom is nonmonotonic
      if( ci.innerEatomsMonotonicAndOnlyPositiveCycles )
      {
        BOOST_FOREACH(ID innerEatomId, cio.outerEatoms)
        {
          if( !checkEatomMonotonic(reg, innerEatomId) )
          {
            ci.innerEatomsMonotonicAndOnlyPositiveCycles = false;
            break;
          }
        }
      }
    }
    // TODO i.e., if "input" component consists only of eatoms, they may be nonmonotonic, and we stil can have wellfounded model generator
    // TODO create testcase for this (how about wellfounded2.hex?)
	}

	// build incoming dependencies
	for(DepMap::const_iterator itd = incoming.begin();
			itd != incoming.end(); ++itd)
	{
		Dependency newdep;
		bool success;
		DBGLOG(DBG,"adding edge " << itd->first << " -> " << c);
		boost::tie(newdep, success) = boost::add_edge(itd->first, c, itd->second, cg);
		assert(success); // we only add new edges here, and each only once
	}

	// build outgoing dependencies
	for(DepMap::const_iterator itd = outgoing.begin();
			itd != outgoing.end(); ++itd)
	{
		Dependency newdep;
		bool success;
		DBGLOG(DBG,"adding edge " << c << " -> " << itd->first);
		boost::tie(newdep, success) = boost::add_edge(c, itd->first, itd->second, cg);
		assert(success); // we only add new edges here, and each only once
	}

	// remove all original components
	for(ito = originals.begin(); ito != originals.end(); ++ito)
	{
		boost::clear_vertex(*ito, cg);
		boost::remove_vertex(*ito, cg);
	}

	return c;
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



  // create eval unit
  EvalUnit u = eg.addUnit(eup_empty);
  LOG(DBG,"created unit " << u);

  // associate with component
  typedef ComponentEvalUnitMapping::value_type MappedPair;
  bool success = mapping.insert(MappedPair(comp, u)).second;
  assert(success); // component must not already exist here

  // configure unit
  EvalUnitProperties& uprops = eg.propsOf(u);

  // configure model generator factory, depending on type of component
  {
    const ComponentGraph::ComponentInfo& ci = cg.propsOf(comp);
    if( ci.innerEatoms.empty() )
    {
      // no inner external atoms -> plain model generator factory
      LOG(DBG,"configuring plain model generator factory for eval unit " << u);
      uprops.mgf.reset(new PlainModelGeneratorFactory(
            ctx, ci, externalEvalConfig));
    }
    else
    {
      if( ci.innerEatomsMonotonicAndOnlyPositiveCycles )
      {
        // inner external atoms and only in positive cycles and monotonic -> wellfounded/fixpoint model generator factory
        LOG(DBG,"configuring wellfounded model generator factory for eval unit " << u);
        uprops.mgf.reset(new WellfoundedModelGeneratorFactory(
              ctx, ci, externalEvalConfig));
      }
      else
      {
        // everything else -> guess and check model generator factory
        LOG(DBG,"configuring guess and check model generator factory for eval unit " << u);
        uprops.mgf.reset(new GuessAndCheckModelGeneratorFactory(
              ctx, ci, externalEvalConfig));
      }
    }
  }

  // create dependencies

  // find all dependencies and create them in eval graph
  ComponentGraph::PredecessorIterator it, it_end;
  unsigned joinOrder = 0;
  for(boost::tie(it, it_end) = cg.getDependencies(comp);
      it != it_end; ++it)
  {
    Component dcomp = cg.targetOf(*it);
    DBGLOG(DBG,"found dependency to component " << dcomp);
    ComponentEvalUnitMapping::left_const_iterator itu =
      mapping.left.find(dcomp);
    if( itu == mapping.left.end() )
      throw std::runtime_error(
          "tried to create an eval unit, "
          "where not all predecessors have previously been created!");
    EvalUnit du = itu->second;
    DBGLOG(DBG,"adding dependency to unit " << du << " with joinOrder " << joinOrder);
    eg.addDependency(u, du, EvalUnitDepProperties(joinOrder));
    joinOrder++;
  }

  return u;
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
