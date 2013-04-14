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
 * @file InternalGroundDASPSolver.cpp
 * @author Christoph Redl
 *
 * @brief Extension of InternalGroundASPSolver to disjunctive programs.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dlvhex2/PlainModelGenerator.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/ASPSolver.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/InternalGroundDASPSolver.h"

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

InternalGroundDASPSolver::InternalGroundDASPSolver(ProgramCtx& ctx, const AnnotatedGroundProgram& p) : InternalGroundASPSolver(ctx, p), ufscm(ctx, p){
}

InterpretationPtr InternalGroundDASPSolver::getNextModel(){

	InterpretationPtr model = InternalGroundASPSolver::getNextModel();
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidsolvertime, "Solver time");
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidhexsolve, "HEX solver time");

	bool ufsFound = true;
	while (model && ufsFound){
		ufsFound = false;

		std::vector<IDAddress> ufs = ufscm.getUnfoundedSet(model);

		if (ufs.size() > 0){
			Nogood ng = ufscm.getLastUFSNogood();
			addNogood(ng);
			ufsFound = true;
			model = InternalGroundASPSolver::getNextModel();
		}
	}
	return model;
}

DLVHEX_NAMESPACE_END

