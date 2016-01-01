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
 * @file   EvalHeuristicBase.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Evaluation heuristic base class.
 */

#ifndef EVAL_HEURISTIC_BASE_HPP_INCLUDED__08112010
#define EVAL_HEURISTIC_BASE_HPP_INCLUDED__08112010

#include "dlvhex2/PlatformDefinitions.h"

DLVHEX_NAMESPACE_BEGIN

/**
 * \brief Base class for all evaluation heuristics.
 *
 * An evaluation heuristic gets an eval graph builder and shall build an eval graph
 * using methods of the eval graph builder only.
 */
template<typename EvalGraphBuilderT>
class EvalHeuristicBase
{
    public:
        /** \brief Constructor. */
        EvalHeuristicBase() {}
        /** \brief Destructor. */
        virtual ~EvalHeuristicBase() {}
        /** \brief Constructs the evaluation graph.
         *
         * @param builder EvalGraphBuilder to be used for constructing the evaluation graph. */
        virtual void build(EvalGraphBuilderT& builder) = 0;
};

DLVHEX_NAMESPACE_END
#endif                           // EVAL_HEURISTICBASEX_HPP_INCLUDED__03112010

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
