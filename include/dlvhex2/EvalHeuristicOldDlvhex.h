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
 * @file   EvalHeuristicOldDlvhex.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Evaluation heuristic corresponding to old DLVHEX strategy.
 */

#ifndef EVAL_HEURISTIC_OLD_DLVHEX_HPP_INCLUDED__03112010
#define EVAL_HEURISTIC_OLD_DLVHEX_HPP_INCLUDED__03112010

#include "dlvhex2/EvalHeuristicBase.h"
#include "dlvhex2/EvalGraphBuilder.h"

DLVHEX_NAMESPACE_BEGIN

template<typename EvalGraphT>
class EvalHeuristicOldDlvhex:
  public EvalHeuristicBase<EvalGraphBuilder<EvalGraphT> >
{
  // types
public:
  typedef EvalHeuristicBase<EvalGraphBuilder<EvalGraphT> > Base;

  // methods
public:
  EvalHeuristicOldDlvhex();
  virtual ~EvalHeuristicOldDlvhex();
  virtual void build(EvalGraphBuilder<EvalGraphT>& builder);
};

DLVHEX_NAMESPACE_END

#endif // EVAL_HEURISTIC_OLD_DLVHEX_HPP_INCLUDED__03112010
