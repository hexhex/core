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
 * @file InternalGroundASPSolver.cpp
 * @author Christoph Redl
 *
 * @brief Interface to genuine clasp-based Solver.
 */

#include "dlvhex/ClaspSolver.hpp"

#include <iostream>
#include <sstream>
#include "dlvhex/Logger.hpp"
#include "dlvhex/GenuineSolver.hpp"
#include <boost/foreach.hpp>
#include <boost/graph/strong_components.hpp>

DLVHEX_NAMESPACE_BEGIN

ClaspSolver::ClaspSolver(ProgramCtx& c, OrdinaryASPProgram& p) : ctx(c), program(p){

	reg = ctx.registry();
}

std::string ClaspSolver::getStatistics(){
}

void ClaspSolver::addExternalLearner(LearningCallback* lb){
	learner.insert(lb);
}

void ClaspSolver::removeExternalLearner(LearningCallback* lb){
	learner.erase(lb);
}

int ClaspSolver::addNogood(Nogood ng){
}

void ClaspSolver::removeNogood(int index){
}

int ClaspSolver::getNogoodCount(){
}

InterpretationConstPtr ClaspSolver::getNextModel(){
}

InterpretationPtr ClaspSolver::projectToOrdinaryAtoms(InterpretationConstPtr intr){
}

DLVHEX_NAMESPACE_END
