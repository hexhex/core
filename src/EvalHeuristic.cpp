/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
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
 * 02110-1301 USA
 */


/**
 * @file   EvalHeuristic.cpp
 * @author Andreas Humenberger <e1026602@student.tuwien.ac.at>
 * 
 * @brief  Explicit instantiaton of evaluation heuristics.
 * 
 */

#include "EvalHeuristicEasy.cpp"
#include "EvalHeuristicGreedy.cpp"
#include "EvalHeuristicOldDlvhex.cpp"
#include "EvalHeuristicTrivial.cpp"
#include "EvalHeuristicMonolithic.cpp"
#include "EvalHeuristicFromFile.cpp"
#include "EvalHeuristicASP.cpp"
#include "EvalHeuristicFromHEXSourcecode.cpp"
#include "EvalHeuristicShared.cpp"

#define INST_CLASS(classname, tempname) \
  template class classname<tempname>;

#define EVAL_INST(type)                             \
  INST_CLASS(EvalHeuristic ## type, FinalEvalGraph) \
  INST_CLASS(EvalHeuristic ## type, HTEvalGraph)

DLVHEX_NAMESPACE_BEGIN

EVAL_INST(Easy)
EVAL_INST(Greedy)
EVAL_INST(OldDlvhex)
EVAL_INST(Trivial)
EVAL_INST(Monolithic)
EVAL_INST(FromFile)
EVAL_INST(ASP)
EVAL_INST(FromHEXSourcecode)

template void executeBuildCommands<FinalEvalGraph>(const CommandVector&, EvalGraphBuilder<FinalEvalGraph>&);
template void executeBuildCommands<HTEvalGraph>(const CommandVector&, EvalGraphBuilder<HTEvalGraph>&);

DLVHEX_NAMESPACE_END
