/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2016 Peter Schüller
 * Copyright (C) 2011-2016 Christoph Redl
 * Copyright (C) 2015-2016 Tobias Kaminski
 * Copyright (C) 2015-2016 Antonius Weinzierl
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

GenuineGrounderPtr GenuineGrounder::getInstance(ProgramCtx& ctx, const OrdinaryASPProgram& p, InterpretationConstPtr frozen)
{

    switch(ctx.config.getOption("GenuineSolver")) {
        case 1: case 3:          // internal grounder + internal solver or clasp
        {
            if (!!frozen) {
                throw GeneralError("Internal grounder does not support frozen atoms");
            }
            DBGLOG(DBG, "Instantiating genuine grounder with internal grounder");
            GenuineGrounderPtr ptr(new InternalGrounder(ctx, p));
            return ptr;
        }
        break;
        case 2: case 4:          // Gringo + internal solver or clasp
        #ifdef HAVE_LIBGRINGO
            {
                DBGLOG(DBG, "Instantiating genuine grounder with gringo");
            #ifndef GRINGO3      // GRINGO4
                GenuineGrounderPtr ptr(new GringoGrounder(ctx, p, frozen));
            #else                // GRINGO3
                //		if (!!frozen){
                //			throw GeneralError("Gringo 3 does not support frozen atoms");
                //		}
                GenuineGrounderPtr ptr(new GringoGrounder(ctx, p, frozen));
            #endif
                return ptr;
            }
        #else
            throw GeneralError("No support for gringo compiled into this binary");
        #endif                   // HAVE_LIBGRINGO
            break;
        default:
            assert(false);
            return GenuineGrounderPtr();
    }
}


GenuineGroundSolverPtr GenuineGroundSolver::getInstance(ProgramCtx& ctx, const AnnotatedGroundProgram& p, InterpretationConstPtr frozen, bool minCheck)
{

    switch (ctx.config.getOption("GenuineSolver")) {
        case 1: case 2:          // internal grounder or Gringo + internal solver
        {
            DBGLOG(DBG, "Instantiating genuine solver with internal solver (min-check: " << minCheck << ")");
            GenuineGroundSolverPtr ptr(minCheck ? new InternalGroundDASPSolver(ctx, p) : new InternalGroundASPSolver(ctx, p));
            return ptr;
        }
        break;
        case 3: case 4:          // internal grounder or Gringo + clasp
        #ifdef HAVE_LIBCLASP
            {
                DBGLOG(DBG, "Instantiating genuine solver with clasp (min-check: " << minCheck << ")");
                // clasp 3 is always disjunctive
                GenuineGroundSolverPtr ptr(new ClaspSolver(ctx, p, frozen));
                return ptr;
            }
        #else
            throw GeneralError("No support for clasp compiled into this binary");
        #endif                   // HAVE_LIBCLASP
            break;
        default:
            assert(false);
            return GenuineGroundSolverPtr();
    }
}


GenuineGroundSolverPtr GenuineGroundSolver::getInstance(ProgramCtx& ctx, const OrdinaryASPProgram& p, InterpretationConstPtr frozen, bool minCheck)
{
    //DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid, "grounding (GenuineGroundS.::getInst)");

    switch (ctx.config.getOption("GenuineSolver")) {
        case 1: case 2:          // internal grounder or Gringo + internal solver
        {
            DBGLOG(DBG, "Instantiating genuine solver with internal solver (min-check: " << minCheck << ")");
            GenuineGroundSolverPtr ptr(minCheck ? new InternalGroundDASPSolver(ctx, AnnotatedGroundProgram(ctx, p)) : new InternalGroundASPSolver(ctx, AnnotatedGroundProgram(ctx, p)));
            return ptr;
        }
        break;
        case 3: case 4:          // internal grounder or Gringo + clasp
        #ifdef HAVE_LIBCLASP
            {
                DBGLOG(DBG, "Instantiating genuine solver with clasp (min-check: " << minCheck << ")");
                // clasp 3 is always disjunctive
                GenuineGroundSolverPtr ptr(new ClaspSolver(ctx, AnnotatedGroundProgram(ctx, p), frozen));
                return ptr;
            }
        #else
            throw GeneralError("No support for clasp compiled into this binary");
        #endif                   // HAVE_LIBCLASP
            break;
        default:
            assert(false);
            return GenuineGroundSolverPtr();
    }
}


GenuineSolverPtr GenuineSolver::getInstance(ProgramCtx& ctx, const OrdinaryASPProgram& p, InterpretationConstPtr frozen, bool minCheck)
{
    const OrdinaryASPProgram* gprog;
    GenuineGrounderPtr grounder;
    {
        DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidhexground, "HEX grounder time (GenuineSolver ctor)");
        grounder = GenuineGrounder::getInstance(ctx, p, frozen);
        gprog = &grounder->getGroundProgram();
    }

    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidhexsolve, "HEX solver time (GenuineSolver ctor)");
    GenuineGroundSolverPtr gsolver = GenuineGroundSolver::getInstance(ctx, *gprog, frozen, minCheck);
    return GenuineSolverPtr(new GenuineSolver(grounder, gsolver, grounder->getGroundProgram()));
}


std::string GenuineSolver::getStatistics()
{
    return solver->getStatistics();
}


const OrdinaryASPProgram& GenuineSolver::getGroundProgram()
{
    return gprog;
}


void GenuineSolver::setOptimum(std::vector<int>& optimum)
{
    solver->setOptimum(optimum);
}


InterpretationPtr GenuineSolver::getNextModel()
{
    DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidhexsolve, "HEX solver (GenuineSolver gNM)");
    return solver->getNextModel();
}


int GenuineSolver::getModelCount()
{
    return solver->getModelCount();
}


void GenuineSolver::addNogood(Nogood ng)
{
    solver->addNogood(ng);
}


void GenuineSolver::restartWithAssumptions(const std::vector<ID>& assumptions)
{
    solver->restartWithAssumptions(assumptions);
}


void GenuineSolver::addPropagator(PropagatorCallback* pb)
{
    solver->addPropagator(pb);
}


void GenuineSolver::removePropagator(PropagatorCallback* pb)
{
    solver->removePropagator(pb);
}


void GenuineSolver::addProgram(const AnnotatedGroundProgram& program, InterpretationConstPtr frozen)
{
    solver->addProgram(program, frozen);
}


void GenuineSolver::addNogoodSet(const NogoodSet& ns, InterpretationConstPtr frozen)
{
    solver->addNogoodSet(ns, frozen);
}


DLVHEX_NAMESPACE_END


// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
