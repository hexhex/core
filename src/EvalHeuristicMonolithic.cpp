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
 * @file EvalHeuristicMonolithic.cpp
 * @author Christoph Redl
 *
 * @brief Implementation of a trivial evaluation heuristic which puts everything into one unit.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/EvalHeuristicMonolithic.h"
#include "dlvhex2/Logger.h"

DLVHEX_NAMESPACE_BEGIN

EvalHeuristicMonolithic::EvalHeuristicMonolithic():
  Base()
{
}

EvalHeuristicMonolithic::~EvalHeuristicMonolithic()
{
}

// trivial strategy:
// do a topological sort of the tree
// build eval units in that order
void EvalHeuristicMonolithic::build(EvalGraphBuilder& builder)
{
  typedef ComponentGraph::Component Component;
  const ComponentGraph& compgraph = builder.getComponentGraph();
  std::list<Component> comps(compgraph.getComponents().first, compgraph.getComponents().second), constraintcomps;
  EvalGraphBuilder::EvalUnit u = builder.createEvalUnit(comps, constraintcomps);
  LOG(ANALYZE,"got since eval unit " << u);
}

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
