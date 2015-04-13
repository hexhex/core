/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) Peter Schüller
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
 * @file EvalHeuristicShared.cpp
 * @author Peter Schller
 *
 * @brief Implementation of shared evaluation heuristic functionality.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#include "dlvhex2/EvalHeuristicShared.h"

DLVHEX_NAMESPACE_BEGIN

namespace evalheur
{

    void executeBuildCommands(const CommandVector& commands, EvalGraphBuilder& builder) {
        const ComponentGraph& compgraph = builder.getComponentGraph();

        // collapse according to commands
        BOOST_FOREACH(const BuildCommand& cmd, commands) {
            LOG(ANALYZE,"BuildCommand collapses components " <<
                printvector(cmd.collapse) <<
                " and shared components " << printvector(cmd.share));
            std::list<Component> comps(cmd.collapse.begin(), cmd.collapse.end());
            std::list<Component> ccomps(cmd.share.begin(), cmd.share.end());
            EvalGraphBuilder::EvalUnit u = builder.createEvalUnit(comps, ccomps);
            LOG(ANALYZE,"yields eval unit " << u);
        }
    }

}


DLVHEX_NAMESPACE_END

// Local Variables:
// mode: C++
// End:
