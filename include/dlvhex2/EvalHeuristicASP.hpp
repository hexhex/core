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
 * @file   EvalHeuristicASP.hpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Evaluation heuristic that uses ASP to plan hex evaluation.
 */

#ifndef EVAL_HEURISTIC_ASP_HPP_INCLUDED__19112011
#define EVAL_HEURISTIC_ASP_HPP_INCLUDED__19112011

#include "dlvhex2/EvalHeuristicBase.hpp"
#include "dlvhex2/EvalGraphBuilder.hpp"

DLVHEX_NAMESPACE_BEGIN

class EvalHeuristicASP:
  public EvalHeuristicBase<EvalGraphBuilder>
{
  // types
public:
  typedef EvalHeuristicBase<EvalGraphBuilder> Base;

  // methods
public:
  EvalHeuristicASP(const std::string& scriptname);
  virtual ~EvalHeuristicASP();
  virtual void build(EvalGraphBuilder& builder);

  // data
protected:
  std::string scriptname;
};

DLVHEX_NAMESPACE_END

#endif // EVAL_HEURISTIC_ASP_HPP_INCLUDED__19112011
