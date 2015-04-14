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
 * @file   EvalHeuristicTrivial.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Evaluation heuristic that just uses the compgraph as eval graph.
 */

#ifndef EVAL_HEURISTIC_TRIVIAL_HPP_INCLUDED__15112010
#define EVAL_HEURISTIC_TRIVIAL_HPP_INCLUDED__15112010

#include "dlvhex2/EvalHeuristicBase.h"
#include "dlvhex2/EvalGraphBuilder.h"

DLVHEX_NAMESPACE_BEGIN

/** \brief Heuristics which creates a separate evaluation unit for each strongly connected component. */
class EvalHeuristicTrivial:
public EvalHeuristicBase<EvalGraphBuilder>
{
    // types
    public:
        typedef EvalHeuristicBase<EvalGraphBuilder> Base;

        // methods
    public:
        /** \brief Constructor. */
        EvalHeuristicTrivial();
        /** \brief Destructor. */
        virtual ~EvalHeuristicTrivial();
        virtual void build(EvalGraphBuilder& builder);
};

DLVHEX_NAMESPACE_END
#endif                           // EVAL_HEURISTIC_TRIVIAL_HPP_INCLUDED__15112010
// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
