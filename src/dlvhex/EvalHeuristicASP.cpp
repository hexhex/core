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
 * @file EvalHeuristicASP.cpp
 * @author Peter Schüller
 *
 * @brief Implementation of an evaluation heuristic that uses ASP to plan hex evaluation.
 *
 * TODO documentation
 *
 * The facts given to the evaluation heuristic program describe the component graph:
 *
 * Components:
 * C is a constant term designating a unique component
 * component(C) is fact for each component
 * if innerRules is nonempty, rules(C) is fact
 * if innerConstraints is nonempty, constraints(C) is fact
 * if outerEatoms is nonempty, outerext(C) is fact
 * if innerEatoms is nonempty, innerext(C) is fact
 * if disjunctiveHeads is true, disjheads(C) is fact
 * if negationInCycles is true, negcycles(C) is fact
 * if innerEatomsNonmonotonic is true, innerextnonmon(C) is fact
 * if outerEatomsNonmonotonic is true, outerextnonmon(C) is fact
 *
 * Dependencies:
 */

#include "dlvhex/EvalHeuristicASP.hpp"
#include "dlvhex/Logger.hpp"
#include "dlvhex/Registry.hpp"
#include "dlvhex/ASPSolver.h"

//#include <boost/unordered_map.hpp>
//#include <boost/graph/topological_sort.hpp>
//#include <boost/property_map/property_map.hpp>
//#include <boost/graph/properties.hpp>
//
////#define BOOST_SPIRIT_DEBUG
//#include <boost/spirit/include/qi.hpp>

#include <fstream>
#include <sstream>

DLVHEX_NAMESPACE_BEGIN

EvalHeuristicASP::EvalHeuristicASP(const std::string& scriptname):
  Base(),
  scriptname(scriptname)
{
}

EvalHeuristicASP::~EvalHeuristicASP()
{
}

void transformComponentGraphIntoASPFacts(const ComponentGraph& cg, RegistryPtr reg, InterpretationPtr edb);

typedef ComponentGraph::Component Component;
typedef ComponentGraph::ComponentIterator ComponentIterator;
typedef std::vector<Component> ComponentContainer;
typedef std::set<Component> ComponentSet;

// manual strategy:
// get commands from file
void EvalHeuristicASP::build(EvalGraphBuilder& builder)
{
  const ComponentGraph& compgraph = builder.getComponentGraph();

	// create extra registry for this ASP!
	RegistryPtr reg2(new Registry);
	InterpretationPtr edb2(new Interpretation(reg2));

	transformComponentGraphIntoASPFacts(compgraph, reg2, edb2);
	LOG(DBG,"edb2 of component graph: " << *edb2);

	assert(false && "breakpoint TODO continue impl");
}

// see documentation at top of file
void transformComponentGraphIntoASPFacts(const ComponentGraph& cg, RegistryPtr reg, InterpretationPtr edb)
{
	// assume registry is empty
	assert(reg->terms.getSize() == 0);

	// create inputprovider, put in facts as string, put in program file, let aspsolver process it, get results in model
	throw std::runtime_error("TODO continue here");

	ComponentIterator it, end;
	for(boost::tie(it, end) = cg.getComponents(); it != end; ++it)
	{

	}
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
