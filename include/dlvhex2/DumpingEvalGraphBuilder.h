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
 * @file   DumpingEvalGraphBuilder.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Evaluation Graph builder that dumps its evaluation plan.
 */

#ifndef DUMPING_EVAL_GRAPH_BUILDER_HPP_INCLUDED__16112011
#define DUMPING_EVAL_GRAPH_BUILDER_HPP_INCLUDED__16112011

#include "dlvhex2/EvalGraphBuilder.h"
#include <fstream>

DLVHEX_NAMESPACE_BEGIN

template<typename EvalGraphT>
class DumpingEvalGraphBuilder:
	public EvalGraphBuilder<EvalGraphT>
{
public:
	typedef EvalGraphBuilder<EvalGraphT> Base;
	typedef typename Base::EvalUnit EvalUnit;
	typedef typename Base::Component Component;
protected:
	std::ofstream output;
  std::map<ComponentGraph::Component, unsigned> componentidx;

  //////////////////////////////////////////////////////////////////////////////
  // methods
  //////////////////////////////////////////////////////////////////////////////
public:
	DumpingEvalGraphBuilder(
      ProgramCtx& ctx, ComponentGraph& cg, EvalGraphT& eg,
      ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig,
			const std::string& outputfilename);
	virtual ~DumpingEvalGraphBuilder();

	// write to file how eval units were created
  virtual EvalUnit createEvalUnit(
			const std::list<Component>& comps, const std::list<Component>& ccomps);
};

DLVHEX_NAMESPACE_END

#endif
