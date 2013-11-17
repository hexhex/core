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
 * @file DumpingEvalGraphBuilder.cpp
 * @author Peter Schüller
 *
 * @brief  Evaluation Graph builder that dumps its evaluation plan.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "dlvhex2/DumpingEvalGraphBuilder.h"
#include "dlvhex2/Logger.h"

#include <fstream>

DLVHEX_NAMESPACE_BEGIN

template<typename EvalGraphT>
DumpingEvalGraphBuilder<EvalGraphT>::DumpingEvalGraphBuilder(
    ProgramCtx& ctx, 
		ComponentGraph& cg,
    EvalGraphT& eg,
    ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig,
		const std::string& ofname):
	EvalGraphBuilder<EvalGraphT>(ctx, cg, eg, externalEvalConfig),
	output(ofname.c_str(), std::ios::out | std::ios::trunc)
{
  throw std::runtime_error("TODO revitalize this functionality as follows: record indices of components here, cg.collapseComponents must record in the component which components get into a goal component, then createEvalUnit can dump which original component indices become which units (this is the clean and only useful way to do it)");
}

template<typename EvalGraphT>
DumpingEvalGraphBuilder<EvalGraphT>::~DumpingEvalGraphBuilder()
{
}

template<typename EvalGraphT>
typename DumpingEvalGraphBuilder<EvalGraphT>::EvalUnit
DumpingEvalGraphBuilder<EvalGraphT>::createEvalUnit(
		const std::list<Component>& comps, const std::list<Component>& ccomps)
{
  if( componentidx.empty() )
  {
    ComponentGraph::ComponentIterator cit, cit_end;
    unsigned idx = 0;
    for(boost::tie(cit, cit_end) = Base::cg.getComponents();
        cit != cit_end; ++cit, ++idx)
    {
      componentidx[*cit] = idx;
    }
  }

  std::vector<unsigned> icomps, iccomps;
  BOOST_FOREACH(const Component& comp, comps)
  {
    icomps.push_back(componentidx[comp]);
  }
  BOOST_FOREACH(const Component& comp, ccomps)
  {
    iccomps.push_back(componentidx[comp]);
  }

	output << "collapse" << printrange(icomps, " ", " ", " ");
	if( !iccomps.empty() )
		output << "share" << printrange(iccomps, " ", " ", " ");
	output << std::endl;

	EvalUnit u(Base::createEvalUnit(comps, ccomps));

	#ifndef NDEBUG
	typedef typename Base::ComponentGraphRest CGRest;
	typedef boost::graph_traits<CGRest> CGRestTraits;
	const CGRest& cgrest = Base::getComponentGraphRest();
	DBGLOG(DBG, "after createEvalUnit with result " << u);
	DBGLOG_SCOPE(DBG,"cgrest",false);
	typename CGRestTraits::vertex_iterator cit, cit_end;
	for(boost::tie(cit, cit_end) = boost::vertices(cgrest);
			cit != cit_end; ++cit)
	{
		DBGLOG(DBG,"component " << *cit << ": " << cgrest[*cit]);
	}
	#endif
  return u;
}

template class DumpingEvalGraphBuilder<FinalEvalGraph>;
template class DumpingEvalGraphBuilder<HTEvalGraph>;

DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
