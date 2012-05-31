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
 * @file GenuineSolver.cpp
 * @author Thomas Krennwallner
 * @author Christoph Redl
 * @author Peter Schueller
 *
 * @brief  Interface to genuine nonground disjunctive ASP Solvers
 *         powered by clingo or internal solver
 * 
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "dlvhex2/GenuineSolver.h"
#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/AnswerSet.h"

#include "dlvhex2/InternalGrounder.h"
#include "dlvhex2/InternalGroundDASPSolver.h"
#include "dlvhex2/GringoGrounder.h"
#include "dlvhex2/ClaspSolver.h"

#include <boost/tokenizer.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <list>

#ifdef HAVE_LIBCLINGO
#include <gringo/gringo_app.h>
#endif


#include "dlvhex2/GringoGrounder.h"

DLVHEX_NAMESPACE_BEGIN

GenuineGrounderPtr GenuineGrounder::getInstance(ProgramCtx& ctx, const OrdinaryASPProgram& p){

	switch(ctx.config.getOption("GenuineSolver")){
	case 1: case 3:	// internal grounder + internal solver or clasp
		{
		DBGLOG(DBG, "Instantiating genuine grounder with internal grounder");
		GenuineGrounderPtr ptr(new InternalGrounder(ctx, p));
		return ptr;
		}
		break;
	case 2: case 4:	// Gringo + internal solver or clasp
#ifdef HAVE_LIBGRINGO
		{
		DBGLOG(DBG, "Instantiating genuine grounder with gringo");
		GenuineGrounderPtr ptr(new GringoGrounder(ctx, p));
		return ptr;
		}
#else
		throw GeneralError("No support for gringo compiled into this binary");
#endif // HAVE_LIBCLINGO
		break;
	}
}

GenuineGroundSolverPtr GenuineGroundSolver::getInstance(ProgramCtx& ctx, const AnnotatedGroundProgram& p, bool interleavedThreading, bool minCheck){

	switch (ctx.config.getOption("GenuineSolver")){
	case 1: case 2:	// internal grounder or Gringo + internal solver
		{
		DBGLOG(DBG, "Instantiating genuine solver with internal solver (min-check: " << minCheck << ")");
		GenuineGroundSolverPtr ptr(minCheck ? new InternalGroundDASPSolver(ctx, p) : new InternalGroundASPSolver(ctx, p));
		return ptr;
		}
		break;
	case 3: case 4:	// internal grounder or Gringo + clasp
#ifdef HAVE_LIBCLASP
		{
		DBGLOG(DBG, "Instantiating genuine solver with clasp (min-check: " << minCheck << ")");
		GenuineGroundSolverPtr ptr(minCheck ? new DisjunctiveClaspSolver(ctx, p, interleavedThreading) : new ClaspSolver(ctx, p, interleavedThreading, ClaspSolver::ChoiceRules));
		return ptr;
		}
#else
		throw GeneralError("No support for clasp compiled into this binary");
#endif // HAVE_LIBCLINGO
		break;
	}
}

GenuineGroundSolverPtr GenuineGroundSolver::getInstance(ProgramCtx& ctx, const OrdinaryASPProgram& p, bool interleavedThreading, bool minCheck){

	switch (ctx.config.getOption("GenuineSolver")){
	case 1: case 2:	// internal grounder or Gringo + internal solver
		{
		DBGLOG(DBG, "Instantiating genuine solver with internal solver (min-check: " << minCheck << ")");
		GenuineGroundSolverPtr ptr(minCheck ? new InternalGroundDASPSolver(ctx, AnnotatedGroundProgram(ctx.registry(), p)) : new InternalGroundASPSolver(ctx, AnnotatedGroundProgram(ctx.registry(), p)));
		return ptr;
		}
		break;
	case 3: case 4:	// internal grounder or Gringo + clasp
#ifdef HAVE_LIBCLASP
		{
		DBGLOG(DBG, "Instantiating genuine solver with clasp (min-check: " << minCheck << ")");
		GenuineGroundSolverPtr ptr(minCheck ? new DisjunctiveClaspSolver(ctx, AnnotatedGroundProgram(ctx.registry(), p), interleavedThreading) : new ClaspSolver(ctx, AnnotatedGroundProgram(ctx.registry(), p), interleavedThreading, ClaspSolver::ChoiceRules));
		return ptr;
		}
#else
		throw GeneralError("No support for clasp compiled into this binary");
#endif // HAVE_LIBCLINGO
		break;
	}
}

GenuineSolverPtr GenuineSolver::getInstance(ProgramCtx& ctx, const OrdinaryASPProgram& p, bool interleavedThreading, bool minCheck){
	GenuineGrounderPtr grounder = GenuineGrounder::getInstance(ctx, p);
	const OrdinaryASPProgram& gprog = grounder->getGroundProgram();

	GenuineGroundSolverPtr gsolver = GenuineGroundSolver::getInstance(ctx, gprog, interleavedThreading, minCheck);
	return GenuineSolverPtr(new GenuineSolver(grounder, gsolver, grounder->getGroundProgram()));
}

std::string GenuineSolver::getStatistics(){
	return solver->getStatistics();
}

const OrdinaryASPProgram& GenuineSolver::getGroundProgram(){
	return gprog;
}

InterpretationConstPtr GenuineSolver::getNextModel(){
	return solver->getNextModel();
}

int GenuineSolver::getModelCount(){
	return solver->getModelCount();
}

InterpretationPtr GenuineSolver::projectToOrdinaryAtoms(InterpretationConstPtr inter){
	return solver->projectToOrdinaryAtoms(inter);
}

int GenuineSolver::addNogood(Nogood ng){
	return solver->addNogood(ng);
}

Nogood GenuineSolver::getNogood(int index){
	return solver->getNogood(index);
}

void GenuineSolver::removeNogood(int index){
	solver->removeNogood(index);
}

int GenuineSolver::getNogoodCount(){
	return solver->getNogoodCount();
}

void GenuineSolver::addExternalLearner(LearningCallback* lb){
	solver->addExternalLearner(lb);
}

void GenuineSolver::removeExternalLearner(LearningCallback* lb){
	solver->removeExternalLearner(lb);
}

DLVHEX_NAMESPACE_END

/* vim: set noet sw=2 ts=8 tw=80: */
// Local Variables:
// mode: C++
// End:
