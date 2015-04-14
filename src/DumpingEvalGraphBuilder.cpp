/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2015 Peter Schüller
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
 * @file DumpingEvalGraphBuilder.cpp
 * @author Peter Schller
 *
 * @brief  Evaluation Graph builder that dumps its evaluation plan.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#include "dlvhex2/DumpingEvalGraphBuilder.h"
#include "dlvhex2/Logger.h"

#include <fstream>

DLVHEX_NAMESPACE_BEGIN

DumpingEvalGraphBuilder::DumpingEvalGraphBuilder(
ProgramCtx& ctx,
ComponentGraph& cg,
EvalGraphT& eg,
ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig,
const std::string& ofname):
EvalGraphBuilder(ctx, cg, eg, externalEvalConfig),
output(ofname.c_str(), std::ios::out | std::ios::trunc)
{
    throw std::runtime_error("TODO revitalize this functionality as follows: record indices of components here, cg.collapseComponents must record in the component which components get into a goal component, then createEvalUnit can dump which original component indices become which units (this is the clean and only useful way to do it)");
}


DumpingEvalGraphBuilder::~DumpingEvalGraphBuilder()
{
}


DumpingEvalGraphBuilder::EvalUnit
DumpingEvalGraphBuilder::createEvalUnit(
const std::list<Component>& comps, const std::list<Component>& ccomps)
{
    if( componentidx.empty() ) {
        ComponentGraph::ComponentIterator cit, cit_end;
        unsigned idx = 0;
        for(boost::tie(cit, cit_end) = cg.getComponents();
        cit != cit_end; ++cit, ++idx) {
            componentidx[*cit] = idx;
        }
    }

    std::vector<unsigned> icomps, iccomps;
    BOOST_FOREACH(const Component& comp, comps) {
        icomps.push_back(componentidx[comp]);
    }
    BOOST_FOREACH(const Component& comp, ccomps) {
        iccomps.push_back(componentidx[comp]);
    }

    output << "collapse" << printrange(icomps, " ", " ", " ");
    if( !iccomps.empty() )
        output << "share" << printrange(iccomps, " ", " ", " ");
    output << std::endl;

    DumpingEvalGraphBuilder::EvalUnit u(
        EvalGraphBuilder::createEvalUnit(comps, ccomps));

    #ifndef NDEBUG
    typedef EvalGraphBuilder::ComponentGraphRest CGRest;
    typedef boost::graph_traits<CGRest> CGRestTraits;
    const CGRest& cgrest = getComponentGraphRest();
    DBGLOG(DBG, "after createEvalUnit with result " << u);
    DBGLOG_SCOPE(DBG,"cgrest",false);
    CGRestTraits::vertex_iterator cit, cit_end;
    for(boost::tie(cit, cit_end) = boost::vertices(cgrest);
    cit != cit_end; ++cit) {
        DBGLOG(DBG,"component " << *cit << ": " << cgrest[*cit]);
    }
    #endif
    return u;
}


DLVHEX_NAMESPACE_END


// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
