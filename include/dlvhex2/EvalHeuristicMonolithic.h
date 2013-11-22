/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * Copyright (C) 2011, 2012, 2013 Christoph Redl
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
 * @file   EvalHeuristicMonolithic.h
 * @author Christoph Redl <redl@kr.tuwien.ac.at>
 * 
 * @brief  Implementation of a trivial evaluation heuristic which puts everything into one unit.
 */

#ifndef EVAL_HEURISTIC_MONOLITHIC_HPP_
#define EVAL_HEURISTIC_MONOLITHIC_HPP_

#include "dlvhex2/EvalHeuristicBase.h"
#include "dlvhex2/EvalGraphBuilder.h"

DLVHEX_NAMESPACE_BEGIN

class EvalHeuristicMonolithic:
  public EvalHeuristicBase<EvalGraphBuilder>
{
  // types
public:
  typedef EvalHeuristicBase<EvalGraphBuilder> Base;

  // methods
public:
  EvalHeuristicMonolithic();
  virtual ~EvalHeuristicMonolithic();
  virtual void build(EvalGraphBuilder& builder);
};

DLVHEX_NAMESPACE_END

#endif // EVAL_HEURISTIC_TRIVIAL_HPP_INCLUDED__15112010
