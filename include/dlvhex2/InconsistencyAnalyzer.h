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
 * @file   InconsistencyAnalyzer.h
 * @author Christoph Redl
 * @date Wed April 20 2016
 *
 * @brief  Computes a reason for the inconsistency in a program unit.
 *
 */

#ifndef _DLVHEX_INCONSISTENCYANALYZER_HPP_
#define _DLVHEX_INCONSISTENCYANALYZER_HPP_

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/ID.h"
#include "dlvhex2/Error.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/OrdinaryASPProgram.h"
#include "dlvhex2/PredicateMask.h"
#include "dlvhex2/Nogood.h"
#include "dlvhex2/AnnotatedGroundProgram.h"

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include <vector>
#include <list>

DLVHEX_NAMESPACE_BEGIN

/**
 * \brief Computes a reason for the inconsistency in a program unit.
 */
class DLVHEX_EXPORT InconsistencyAnalyzer
{
private:
    /** \brief ProgramCtx reference. */
    ProgramCtx& ctx;

public:

    /** \brief Constructor.
      * @param ctx ProgramCtx.
      */
    InconsistencyAnalyzer(ProgramCtx& ctx);

    /** \brief Returns an inconsistency reason represented by a nogood.
      *
      * An inconsistency reason for a program P wrt. a domain D is a pair of atoms R=(R+,R-) such that
      * P \cup I is inconsistent for all sets I from D such that all of R+ but none of R- occur in I.
      *
      * @param mg Pointer to some model generator to be used for external atom evaluation.
      * @param explAtoms Atoms to compute the inconsistency explanation for.
      * @param innerEatoms Inner external atoms in this unit; for inconsistency analysis, <em>all</em> external atoms in the unit must be considered to be inner.
      * @param program Program to compute the inconsistency reason for.
      * @param annotatedGroundProgram Previous (optimized) grounding with annotations.
      * @param haveInconsistencyCause Contains the result whether an inconsistency reason has been found.
      * @return Inconsistency reason R=(R+,R-) in form of a nogood with R+ as positive and R- as negative literals.
      */
    Nogood getInconsistencyReason(BaseModelGenerator* mg, InterpretationConstPtr explAtoms, std::vector<ID>& innerEatoms, OrdinaryASPProgram& program, AnnotatedGroundProgram& annotatedOptimizedProgram, bool* haveInconsistencyReason);
};

DLVHEX_NAMESPACE_END
#endif                           // _DLVHEX_INCONSISTENCYANALYZER_HPP_


// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
