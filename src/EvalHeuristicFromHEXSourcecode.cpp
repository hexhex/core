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
 * @file ManualEvalHeuristicsPlugin.cpp
 * @author Peter Schueller
 *
 * @brief Plugin for specifying evaluation units in HEX input.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/config_values.h"

#include "dlvhex2/EvalHeuristicFromHEXSourcecode.h"
#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Printhelpers.h"
#include "dlvhex2/Logger.h"

DLVHEX_NAMESPACE_BEGIN

namespace spirit = boost::spirit;
namespace qi = boost::spirit::qi;

// collapse certain combinations of rules that belong to one unit:
// components that consist of one external atom
template<typename EvalGraphT>
void EvalHeuristicFromHEXSourcecode<EvalGraphT>::preprocessComponents(EvalGraphBuilder<EvalGraphT>& builder) {
	RegistryPtr reg = builder.registry();
	ComponentGraph& compgraph = builder.getComponentGraph();

	// for all components with only outer external atoms:
	// merge with components that depend on them

	for(ComponentIterator cit = compgraph.getComponents().first; // do not use boost::tie here! the container is modified in the loop!
	    cit != compgraph.getComponents().second; ++cit)
	{
		Component comp = *cit;
		const ComponentInfo& ci = compgraph.propsOf(comp);
		if( !ci.innerRules.empty() || !ci.innerConstraints.empty() )
			continue;

		DBGLOG(DBG,"preprocessing non-rule component " << comp << " " << ci);

		ComponentSet collapse;
		ComponentGraph::SuccessorIterator sit, sit_end;
		for(boost::tie(sit, sit_end) = compgraph.getProvides(comp);
				sit != sit_end; ++sit)
		{
			Component succ = compgraph.sourceOf(*sit);
			const ComponentInfo& sci = compgraph.propsOf(succ);
			DBGLOG(DBG," collapsing with " << succ << " " << sci);
			collapse.insert(succ);
		}
		// put into the set this very component
		collapse.insert(comp);
		assert(!collapse.empty());

		Component c = compgraph.collapseComponents(collapse);
		LOG(DBG,"collapse of " << printrange(collapse) << " yielded new component " << c);

		// restart loop after collapse
		cit = compgraph.getComponents().first;
	}
}

#if 0
// collapse certain combinations of rules that belong to one unit:
// components that consist of one auxiliary external atom input rule
void EvalHeuristicFromHEXSourcecode::postprocessComponents(EvalGraphBuilder& builder) {
	RegistryPtr reg = builder.registry();
	ComponentGraph& compgraph = builder.getComponentGraph();

	// for all components with auxiliary input atom in their head:
	// merge with topologically last component that they depend on

	////ComponentContainer sortedcomps;
	for(ComponentIterator cit = compgraph.getComponents().first; // do not use boost::tie here! the container is modified in the loop!
	    cit != compgraph.getComponents().second; ++cit)
	{
		////if( cit == compgraph.getComponents().first ) {
		////	// sort components topologically
		////	// (each time we restart the loop)
		////	// XXX this could be made more efficient, which is necessary only for many different nonground external atoms in a program
		////	evalheur::topologicalSortComponents(compgraph.getInternalGraph(), sortedcomps);
		////}

		Component comp = *cit;
		const ComponentInfo& ci = compgraph.propsOf(comp);
		// the components we are looking for always contain just one rule
		if( ci.innerRules.size() != 1 )
			continue;
		ID ruleid = ci.innerRules.front();
		const Rule& rule = reg->rules.getByID(ruleid);
		// the rules we are looking for always contain just one head which is a nonground atom
		if( rule.head.size() != 1 )
			continue;
		ID ruleheadid = rule.head.front();
		if( !ruleheadid.isOrdinaryNongroundAtom() )
			continue;
		const OrdinaryAtom& rulehead = reg->onatoms.getByID(ruleheadid);
		ID headpredicate = rulehead.tuple.front();
		if( !headpredicate.isExternalInputAuxiliary() )
			continue;

		DBGLOG(DBG,"preprocessing component with external input auxilary rule " << comp << " " << printToString<RawPrinter>(ruleid, reg));

		// gather list of predecessors
		ComponentSet collapse;
		ComponentGraph::PredecessorIterator sit, sit_end;
		for(boost::tie(sit, sit_end) = compgraph.getDependencies(comp); sit != sit_end; ++sit) {
			Component pred = compgraph.targetOf(*sit);
			collapse.insert(pred);
		}

		// put into the set this very component
		collapse.insert(comp);
		Component c = compgraph.collapseComponents(collapse);
		LOG(DBG,"collapse of " << printrange(collapse) << " yielded new component " << c);

		// restart loop after collapse
		cit = compgraph.getComponents().first;
	}
}
#endif

template<typename EvalGraphT>
void EvalHeuristicFromHEXSourcecode<EvalGraphT>::build(EvalGraphBuilder<EvalGraphT>& builder) {
	RegistryPtr reg = builder.registry();
	ManualEvalHeuristicsPlugin::CtxData& ctxdata = builder.getProgramCtx().template getPluginData<ManualEvalHeuristicsPlugin>();

	// preprocess ctxdata.instructions: make sure first element is ID_FAIL
	// (defaults to eval unit 0 if not given)
	if( ctxdata.instructions.empty() || ctxdata.instructions.begin()->first != ID_FAIL )
	{
		ctxdata.instructions.push_front(std::make_pair(ID_FAIL,0));
	}

	// first build up each unit's list of components
	UnitMap unitmap;
	UnitBackMap unitbackmap;

	ComponentGraph& cg = builder.getComponentGraph();

	preprocessComponents(builder);

	ComponentContainer auxiliaryComponents;
	ComponentGraph::ComponentIterator cit, cit_end;
	for(boost::tie(cit, cit_end) = cg.getComponents();
	    cit != cit_end; ++cit) {
		Component c = *cit;
		const ComponentInfo& ci = cg.getComponentInfo(c);

		// rules plus constraints (XXX this could be made more efficient)
		Tuple rc(ci.innerRules);
		rc.insert(rc.end(),ci.innerConstraints.begin(), ci.innerConstraints.end());

		DBGLOG(DBG,"component " << static_cast<void*>(c) << " " << ci);

		// look through all rules and gather unit assignments
		std::set<unsigned> assignments;

		// XXX if we have many items in "instructions" the following search will be very inefficient and should be solved using a map with an intelligent wrapper around
		for(Tuple::const_iterator itr = rc.begin();
		    itr != rc.end(); ++itr) {
			// rule id
			ID rid = *itr;
			if( rid.address > ctxdata.lastUserRuleID.address) {
				// we do not get assignments for auxiliary rules
				DBGLOG(DBG,"  skipping unit assignment for auxiliary rule " << printToString<RawPrinter>(rid, reg));
				continue;
			}

			// find instruction for this rule id
			InstructionList::const_iterator iti = ctxdata.instructions.begin();
			assert(!ctxdata.instructions.empty());
			assert(iti->first == ID_FAIL);
			// start search at second
			iti++;
			for(;iti != ctxdata.instructions.end(); ++iti) {
				assert(iti->first != ID_FAIL);
				if( rid.address <= iti->first.address ){
					// this means: rule with ID rid was parsed after last and before this instruction
					break;
				}
			}
			// we go back to find the right instruction for this id
			iti--;
			assert(iti != ctxdata.instructions.end());
			unsigned intoUnit = iti->second;
			DBGLOG(DBG,"  unit " << intoUnit << " for rule " << printToString<RawPrinter>(rid, reg));
			assignments.insert(intoUnit);
		}
		DBGLOG(DBG,"  got assingments to units " << printset(assignments));

		if( assignments.size() > 1 ) {
			std::stringstream s;
			s << "Error: manual evaluation unit instructions put the following rules into distinct units " << printset(assignments) <<
			      " which is not possible due to these rules being a strongly connected component: \n";
			for(Tuple::const_iterator itr = rc.begin();
			    itr != rc.end(); ++itr) {
				s << printToString<RawPrinter>(*itr, reg) << "\n";
			}
			throw std::runtime_error(s.str());
		}

		if( !assignments.empty() ) {
			assert(assignments.size() == 1);
			unsigned assignedUnit = *assignments.begin();
			unitmap[assignedUnit].push_back(c);
			unitbackmap[c] = assignedUnit;
		} else {
			LOG(DBG,"component " << c << " is currently not assigned to any unit");
			auxiliaryComponents.push_back(c);
		}
	}

	// try to fix some auxiliary components:
	// if component depends on assigned component, and assigned component depends on it, and they are the same id, put into that component
	ComponentContainer::iterator pit;
       	while( !auxiliaryComponents.empty() )
	{
		pit = auxiliaryComponents.begin();
		Component c = *pit;
		const ComponentInfo& ci = cg.getComponentInfo(c);

		typedef std::set<unsigned> UISet;

		// gather list of predecessors
		ComponentSet predecessors;
		UISet predui;
		ComponentGraph::PredecessorIterator ppit, ppit_end;
		for(boost::tie(ppit, ppit_end) = cg.getDependencies(c); ppit != ppit_end; ++ppit) {
			Component pred = cg.targetOf(*ppit);
			predecessors.insert(pred);
			if( unitbackmap.find(pred) != unitbackmap.end() )
				predui.insert(unitbackmap.find(pred)->second);
		}

		// gather list of successors
		ComponentSet successors;
		UISet succui;
		ComponentGraph::SuccessorIterator sit, sit_end;
		for(boost::tie(sit, sit_end) = cg.getProvides(c); sit != sit_end; ++sit) {
			Component succ = cg.sourceOf(*sit);
			successors.insert(succ);
			if( unitbackmap.find(succ) != unitbackmap.end() )
				succui.insert(unitbackmap.find(succ)->second);
		}

		// intersect
		ComponentSet intersection;
		UISet uintersection;
		std::set_intersection(
				predecessors.begin(), predecessors.end(),
				successors.begin(), successors.end(),
				std::inserter(intersection, intersection.begin()));
		std::set_intersection(
				predui.begin(), predui.end(),
				succui.begin(), succui.end(),
				std::inserter(uintersection, uintersection.begin()));

		LOG(DBG,"trying to fix auxiliary component " << c << " " << ci << " which is "
	                "depending on " << printset(predecessors) << "/" << printset(predui) << ", "
			"providing for " << printset(successors) << "/" << printset(succui) << ", "
			"intersection is " << printset(intersection) << "/" << printset(uintersection));

		if( uintersection.size() == 1 ) {
			// if component is depending on and providing for same (single) component, put together
			unsigned assignedUnit = *uintersection.begin();
			unitmap[assignedUnit].push_back(c);
			unitbackmap[c] = assignedUnit;
			auxiliaryComponents.erase(pit);
		} else if( uintersection.empty() && predui.size() == 1 ) {
			// if component is not in cycle, depends on just one unit and provides for other units, push into first unit
			unsigned assignedUnit = *predui.begin();
			unitmap[assignedUnit].push_back(c);
			unitbackmap[c] = assignedUnit;
			auxiliaryComponents.erase(pit);
		} else {
			// TODO more cases?
			throw std::runtime_error("could not resolve auxiliary unit, perhaps more code is needed here");
			//++pit;
		}
	}

	// collapse all these units
	LOG(INFO,"collapsing according to '#evalunit(...).' instructions in source code");
	for(UnitMap::const_iterator it = unitmap.begin();
	    it != unitmap.end(); ++it) {
		cg.collapseComponents(ComponentSet(it->second.begin(), it->second.end()));
	}

#if 0
	// now try to assign units that have not been assigned so far (auxiliary input rules)
	ComponentContainer::const_iterator pit;
	for(pit = auxiliaryComponents.begin();
	    pit != auxiliaryComponents.end(); ++pit) {
		Component c = *pit;
		const ComponentInfo& ci = cg.getComponentInfo(c);

		bool processed = false;
		do {
			// the auxiliary input rule components we are looking for always contain just one rule
			if( ci.innerRules.size() != 1 )
				break;
			ID ruleid = ci.innerRules.front();
			const Rule& rule = reg->rules.getByID(ruleid);
			// the rules we are looking for always contain just one head which is a nonground atom
			if( rule.head.size() != 1 )
				break;
			ID ruleheadid = rule.head.front();
			if( !ruleheadid.isOrdinaryNongroundAtom() )
				break;
			const OrdinaryAtom& rulehead = reg->onatoms.getByID(ruleheadid);
			ID headpredicate = rulehead.tuple.front();
			if( !headpredicate.isExternalInputAuxiliary() )
				break;

			DBGLOG(DBG,"trying to assign component with external input auxilary rule " << c << " " << printToString<RawPrinter>(ruleid, reg));

			// gather list of predecessors
			ComponentGraph::PredecessorIterator sit, sit_end;
			for(boost::tie(sit, sit_end) = cg.getDependencies(c); sit != sit_end; ++sit) {
				Component pred = cg.targetOf(*sit);
				collapse.insert(pred);
			}

			// if this component depends only on one component, push it inside there (into its single predecessor)
			if( preds.size() == 1 ) {
				LOG(INFO,"putting external input auxilary rule " << printToString<RawPrinter>(ruleid, reg) << " to predecessor " << printset(preds));
			} else {
				// otherwise push it into its external atom (into its single successor)
				collapse.clear();
				ComponentGraph::SuccessorIterator sit, sit_end;
				for(boost::tie(sit, sit_end) = cg.getProvides(c);
				    sit != sit_end; ++sit)
				{
					Component succ = cg.sourceOf(*sit);
					collapse.insert(succ);
				}
				LOG(INFO,"putting external input auxilary rule " << printToString<RawPrinter>(ruleid, reg) << " to successor " << printset(collapse));
				// if this is not true, more than one external atom depends on this external atom auxiliary input
				assert(collapse.size() == 1);
			}
			// we will collapse with a unit that we create later
			Component collapsedOne = *collapse.begin();
			unsigned resultUnitToUpdate = unitbackmap(collapsedOne);

			collapse.insert(c);
			Component newc = cg.collapseComponents(collapse);
			LOG(DBG,"collapse of " << printrange(collapse) << " yielded new component " << newc);

			// replace collapsed component by new one
			unitmap[resultUnitToUpdate].erase(collapsedOne);
			unitmap[resultUnitToUpdate].push_back(newc);

			processed = true;
		} while(false);

		if( !processed ) {
			Tuple rc(ci.innerRules);
			rc.insert(rc.end(),ci.innerConstraints.begin(), ci.innerConstraints.end());
			// (auxiliary) rules do not belong to unit
			std::stringstream s;
			s << "Error: got rule(s) " << printManyToString<RawPrinter>(rc, ". ", reg) << "."
			  << " which is(are) not assigned any unit!"
			  << " perhaps you need to add code to EvalHeuristicFromHEXSourcecode::preprocessComponents(...)";
			throw std::runtime_error(s.str());
		}
	}
#endif

	// sort components topologically
	ComponentContainer sortedcomps;
	evalheur::topologicalSortComponents(cg.getInternalGraph(), sortedcomps);

	// create units from components
	for(ComponentContainer::const_iterator it = sortedcomps.begin();
	    it != sortedcomps.end(); ++it) {
		std::list<Component> comp;
		comp.push_back(*it);
		std::list<Component> empty;
		builder.createEvalUnit(comp, empty);
	}
}

DLVHEX_NAMESPACE_END

// this would be the code to use this plugin as a "real" plugin in a .so file
// but we directly use it in dlvhex.cpp
#if 0
ManualEvalHeuristicsPlugin theManualEvalHeuristicsPlugin;

// return plain C type s.t. all compilers and linkers will like this code
extern "C"
void * PLUGINIMPORTFUNCTION()
{
	return reinterpret_cast<void*>(& DLVHEX_NAMESPACE theManualEvalHeuristicsPlugin);
}

#endif

// Local Variables:
// mode: C++
// End:
