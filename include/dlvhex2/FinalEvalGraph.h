/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schller
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
 * @file   FinialEvalGraph.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Definition of eval graph as used in real dlvhex evaluation.
 */

#ifndef FINAL_EVAL_GRAPH_HPP_INCLUDED__08112010
#define FINAL_EVAL_GRAPH_HPP_INCLUDED__08112010

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/Interpretation.h"
#include "dlvhex2/ModelGenerator.h"
#include "dlvhex2/EvalGraph.h"
//#include "dlvhex2/ComponentGraph.h"

#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

/** \brief Definition of eval graph as used in real dlvhex evaluation. */
struct FinalEvalUnitPropertyBase:
public EvalUnitProjectionProperties,
public EvalUnitModelGeneratorFactoryProperties<Interpretation>,
public ostream_printable<FinalEvalUnitPropertyBase>
{
    // XXX: storing it that way is easy but not nice, we SHOULD only store relevant information from ProgramCtx here in this local eval unit program ctx
    // XXX: better store such things in mgf (ModelGeneratorFactory)
    //ProgramCtx& programCtx;
    //ComponentGraph::ComponentInfo ci;
    //FinalEvalUnitPropertyBase(): ci() {}
    //FinalEvalUnitPropertyBase(ComponentGraph::ComponentInfo& ci): ci(ci) {}
    std::ostream& print(std::ostream& o) const
        { return o << static_cast<const EvalUnitModelGeneratorFactoryProperties<Interpretation> >(*this); }
};

typedef EvalGraph<FinalEvalUnitPropertyBase> FinalEvalGraph;
typedef boost::shared_ptr<FinalEvalGraph> FinalEvalGraphPtr;

DLVHEX_NAMESPACE_END
#endif                           // FINAL_EVAL_GRAPH_HPP_INCLUDED__08112010
