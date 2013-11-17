/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2013 Andreas Humenberger
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
 * @file   EvalContext.h
 * @author Andreas Humenberger
 *
 * @brief Evaluation context.
 *
 *
 */

#ifndef EVAL_CONTEXT_H
#define EVAL_CONTEXT_H

#include "dlvhex2/EvalGraphBuilder.h"
#include "dlvhex2/EvalHeuristicBase.h"

#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

template<typename EvalGraphT>
struct EvalContext
{
	// member types
	typedef typename EvalGraphT::EvalUnit EvalUnit;
	typedef boost::shared_ptr<EvalGraphT> EvalGraphPtr;
	typedef boost::shared_ptr<EvalHeuristicBase<EvalGraphBuilder<EvalGraphT> > > EvalHeuristicPtr;
	// factory for eval heuristics
	EvalHeuristicPtr heuristic;
	// eval graph
	EvalGraphPtr evalgraph;
	EvalUnit ufinal;

	typedef boost::shared_ptr<EvalContext<EvalGraphT> > Ptr;
};

DLVHEX_NAMESPACE_END

#endif // EVAL_CONTEXT_H
