/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schüller
 * Copyright (C) 2011-2015 Christoph Redl
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
 * @file   CDNLSolver.hpp
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 *
 * @brief  Conflict-driven Nogood Learning Solver.
 */

#ifndef INTERNALGROUNDDASPSOLVER_HPP_INCLUDED__30102011
#define INTERNALGROUNDDASPSOLVER_HPP_INCLUDED__30102011

#include "dlvhex2/ID.h"
#include "dlvhex2/Interpretation.h"
#include "dlvhex2/ProgramCtx.h"
#include <vector>
#include <map>
#include <boost/foreach.hpp>
#include "dlvhex2/Printhelpers.h"
#include "dlvhex2/InternalGroundASPSolver.h"
#include "dlvhex2/UnfoundedSetChecker.h"
#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

#ifdef _MSC_VER
// suppresses warning C4250: 'dlvhex::InternalGroundASPSolver' : inherits 'dlvhex::CDNLSolver::dlvhex::CDNLSolver::addNogood' via dominance
// (there is a compiler bug in MSVC; the call of addNogood is actually _not_ ambigious because the method is pure virtual in GenuineGroundSolver)
#pragma warning (disable: 4250)
#endif
/** \brief Internal solver for disjunctive ASP without using third-party software. */
class InternalGroundDASPSolver : public InternalGroundASPSolver
{
    private:
        /** \brief Manager for unfounded set checking. */
        UnfoundedSetCheckerManager ufscm;

    public:
        /** \brief Constructor.
         * @param ctx ProgramCtx.
         * @param p Ground program with meta information.
         */
        InternalGroundDASPSolver(ProgramCtx& ctx, const AnnotatedGroundProgram& p);
        virtual InterpretationPtr getNextModel();

        typedef boost::shared_ptr<InternalGroundDASPSolver> Ptr;
        typedef boost::shared_ptr<const InternalGroundDASPSolver> ConstPtr;
};
#ifdef _MSC_VER
#pragma warning (default: 4250)
#endif

typedef InternalGroundDASPSolver::Ptr InternalGroundDASPSolverPtr;
typedef InternalGroundDASPSolver::ConstPtr InternalGroundDASPSolverConstPtr;

DLVHEX_NAMESPACE_END
#endif
