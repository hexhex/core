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
 * @file   CAUAlgorithms.cpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Implementations related to Common Ancestor Units (CAUs).
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif                           // HAVE_CONFIG_H

#include "dlvhex2/CAUAlgorithms.h"

namespace CAUAlgorithms
{

    void logAPM(
    const AncestryPropertyMap& apm) {
        DBGLOG_SCOPE(DBG,"apm",false);
        DBGLOG(DBG,"logging AncestryPropertyMap:");
        std::vector<Ancestry>::const_iterator it;
        unsigned u = 0;
        for(it = apm.storage_begin();
        it != apm.storage_end(); ++it, ++u) {
            const Ancestry& anc = *it;
            DBGLOG(DBG,"unit " << u << ": " << printset(anc));
        }
    }

    void logJRPM(const JoinRelevancePropertyMap& jr) {
        DBGLOG_SCOPE(DBG,"jrpm",false);
        DBGLOG(DBG,"logging JoinRelevancePropertyMap:");
        std::vector<bool>::const_iterator it;
        unsigned u = 0;
        for(it = jr.storage_begin();
        it != jr.storage_end(); ++it, ++u) {
            DBGLOG(DBG,"unit " << u << ": " << ((*it == true) ?
                std::string("relevant") : std::string("irrelevant")) );
        }
    }

}                                // namespace CAUAlgorithms
