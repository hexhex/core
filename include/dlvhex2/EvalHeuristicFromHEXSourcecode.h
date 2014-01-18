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
 * @file   EvalHeuristicFromHEXSourcecode.h
 * @author Andreas Humenberger <andreas.humenberger@student.tuwien.ac.at>
 * 
 * @brief  Evaluation heuristic....
 */

#ifndef EVAL_HEURISTIC_FROMHEXSOURCECODE
#define EVAL_HEURISTIC_FROMHEXSOURCECODE

#include "dlvhex2/EvalHeuristicBase.h"
#include "dlvhex2/EvalHeuristicShared.h"
#include "dlvhex2/EvalGraphBuilder.h"
#include "dlvhex2/ManualEvalHeuristicsPlugin.h"

DLVHEX_NAMESPACE_BEGIN

typedef ComponentGraph::Component Component;
typedef ComponentGraph::ComponentSet ComponentSet;
typedef ComponentGraph::ComponentInfo ComponentInfo;
typedef ComponentGraph::ComponentIterator ComponentIterator;
typedef evalheur::ComponentContainer ComponentContainer;
typedef ManualEvalHeuristicsPlugin::CtxData::InstructionList InstructionList;
typedef std::map<unsigned, std::list<Component> > UnitMap;
typedef std::map<Component, unsigned > UnitBackMap;

template<typename EvalGraphT>
class EvalHeuristicFromHEXSourcecode:
  public EvalHeuristicBase<EvalGraphBuilder<EvalGraphT> >
{
  // types
public:
  typedef EvalHeuristicBase<EvalGraphBuilder<EvalGraphT> > Base;

  // methods
public:
  EvalHeuristicFromHEXSourcecode() { }
  virtual ~EvalHeuristicFromHEXSourcecode() { }

  virtual void build(EvalGraphBuilder<EvalGraphT>& builder);

protected:
  virtual void preprocessComponents(EvalGraphBuilder<EvalGraphT>& builder);
};

DLVHEX_NAMESPACE_END

#endif // EVAL_HEURISTIC_FROMHEXSOURCECODE
