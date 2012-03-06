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
 * @file PlainModelGenerator.cpp
 * @author Peter Schüller
 *
 * @brief Implementation of the model generator for "Plain" components.
 */

#define DLVHEX_BENCHMARK

#include "dlvhex2/GenuinePlainModelGenerator.h"
#include "dlvhex2/InternalGrounder.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/ASPSolver.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/Benchmarking.h"

#include "dlvhex2/InconsistencyAnalyzer.h"

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

GenuinePlainModelGeneratorFactory::GenuinePlainModelGeneratorFactory(
    ProgramCtx& ctx,
    const ComponentInfo& ci,
    ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig):
  BaseModelGeneratorFactory(),
  externalEvalConfig(externalEvalConfig),
  ctx(ctx),
  ci(ci),
  eatoms(ci.outerEatoms),
  idb(),
  xidb()
{
  RegistryPtr reg = ctx.registry();

  // this model generator can handle:
  // components with outer eatoms
  // components with inner rules
  // components with inner constraints
  // this model generator CANNOT handle:
  // components with inner eatoms

  assert(ci.innerEatoms.empty());

  // copy rules and constraints to idb
  // TODO we do not need this except for debugging
  idb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
  idb.insert(idb.end(), ci.innerRules.begin(), ci.innerRules.end());
  idb.insert(idb.end(), ci.innerConstraints.begin(), ci.innerConstraints.end());

  // transform original innerRules and innerConstraints
  // to xidb with only auxiliaries
  xidb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
  std::back_insert_iterator<std::vector<ID> > inserter(xidb);
  std::transform(ci.innerRules.begin(), ci.innerRules.end(),
      inserter, boost::bind(
        &GenuinePlainModelGeneratorFactory::convertRule, this, reg, _1));
  std::transform(ci.innerConstraints.begin(), ci.innerConstraints.end(),
      inserter, boost::bind(
        &GenuinePlainModelGeneratorFactory::convertRule, this, reg, _1));

  #ifndef NDEBUG
  {
    {
      std::ostringstream s;
      RawPrinter printer(s,ctx.registry());
      printer.printmany(idb," ");
      DBGLOG(DBG,"GenuinePlainModelGeneratorFactory got idb " << s.str());
    }
    {
      std::ostringstream s;
      RawPrinter printer(s,ctx.registry());
      printer.printmany(xidb," ");
      DBGLOG(DBG,"GenuinePlainModelGeneratorFactory got xidb " << s.str());
    }
  }
  #endif
}

std::ostream& GenuinePlainModelGeneratorFactory::print(
    std::ostream& o) const
{
  RawPrinter printer(o, ctx.registry());
  if( !eatoms.empty() )
  {
    printer.printmany(eatoms,",");
  }
  if( !xidb.empty() )
  {
    printer.printmany(xidb,",");
  }
  return o;
}

GenuinePlainModelGenerator::GenuinePlainModelGenerator(
    Factory& factory,
    InterpretationConstPtr input):
  BaseModelGenerator(input),
  factory(factory)
{
	RegistryPtr reg = factory.ctx.registry();

	// create new interpretation as copy
	Interpretation::Ptr newint;
	if( input == 0 )
	{
		// empty construction
		newint.reset(new Interpretation(reg));
	}
	else
	{
		// copy construction
		newint.reset(new Interpretation(*input));
	}

	// augment input with edb
	newint->add(*factory.ctx.edb);

	// remember facts so far (we have to remove these from any output)
	InterpretationConstPtr mask(new Interpretation(*newint));

	// manage outer external atoms
	if( !factory.eatoms.empty() )
	{
		// augment input with result of external atom evaluation
		// use newint as input and as output interpretation
		IntegrateExternalAnswerIntoInterpretationCB cb(newint);
		evaluateExternalAtoms(reg, factory.eatoms, newint, cb);
		DLVHEX_BENCHMARK_REGISTER(sidcountexternalanswersets,
		    "outer external atom computations");
		DLVHEX_BENCHMARK_COUNT(sidcountexternalanswersets,1);
	}

	// store in model generator and store as const
	postprocessedInput = newint;

	OrdinaryASPProgram program(reg, factory.xidb, postprocessedInput, factory.ctx.maxint, mask);

	solver = GenuineSolver::getInstance(factory.ctx, program);
	factory.ctx.globalNogoods.addNogoodListener(solver);
	firstModel = true;

//Nogood ng1;
//ng1.insert(solver->createLiteral(29));
//factory.ctx.globalNogoods.addNogood(ng1);

//	grounder = InternalGrounderPtr(new InternalGrounder(factory.ctx, program));
//	if (factory.ctx.config.getOption("Instantiate")){
//		std::cout << "% Component " << &(factory.ci) << std::endl;
//		std::cout << "% Nonground Program " << &(factory.ci) << std::endl;
//		std::cout << grounder->getNongroundProgramString();
//		std::cout << "% Ground Program " << &(factory.ci) << std::endl;
//		std::cout << grounder->getGroundProgramString();
//	}

//	OrdinaryASPProgram gprogram = grounder->getGroundProgram();
//	igas = InternalGroundDASPSolverPtr(new InternalGroundDASPSolver(factory.ctx, gprogram));
//	currentanswer = 0;
}

GenuinePlainModelGenerator::~GenuinePlainModelGenerator(){
	factory.ctx.globalNogoods.removeNogoodListener(solver);
	DBGLOG(DBG, "Final Statistics:" << std::endl << solver->getStatistics());
}

GenuinePlainModelGenerator::InterpretationPtr
GenuinePlainModelGenerator::generateNextModel()
{
	if (solver == GenuineSolverPtr()){
		return InterpretationPtr();
	}

	RegistryPtr reg = factory.ctx.registry();

	// remove edb from result
	InterpretationPtr modelCandidate = solver->projectToOrdinaryAtoms(solver->getNextModel());

	// learn global nogoods
	if (modelCandidate == InterpretationPtr()){
		globalConflictAnalysis(factory.ctx, factory.idb, solver, factory.ci.componentIsMonotonic);






if (firstModel){

/*
HittingSetDetector<int>::Hypergraph hg;
HittingSetDetector<int>::Hyperedge e1; e1.push_back(0); e1.push_back(2); hg.push_back(e1);
HittingSetDetector<int>::Hyperedge e2; e2.push_back(0); e2.push_back(7); hg.push_back(e2);
HittingSetDetector<int>::Hyperedge e3; e3.push_back(6); e3.push_back(2); hg.push_back(e3);
HittingSetDetector<int>::Hyperedge e4; e4.push_back(6); e4.push_back(7); hg.push_back(e4);
std::vector<int> hs = HittingSetDetector<int>::getHittingSet(hg);
*/

/*
std::cout << "Hitting set: ";
BOOST_FOREACH (int x, hs){
	 std::cout << " " << x;
}
std::cout << std::endl;
*/






/*
static bool an = true;
if (an){
DBGLOG(DBG, "Conflict on first model: Analyzing inconsistency");

OrdinaryASPProgram program(reg, factory.xidb, postprocessedInput, factory.ctx.maxint);
InconsistencyAnalyzer ia(factory.ctx);
Nogood ng = ia.explainInconsistency(program, postprocessedInput);
factory.ctx.globalNogoods.addNogood(ng);
}
an = false;
*/


}


	}
	firstModel = false;

	DBGLOG(DBG, "Statistics:" << std::endl << solver->getStatistics());
	return modelCandidate;
}

DLVHEX_NAMESPACE_END
