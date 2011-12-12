/* dlvhex-mcs-equilibrium-plugin
 * Calculate Equilibrium Semantics of Multi Context Systems in dlvhex
 *
 * Copyright (C) 2009,2010  Markus Boegl
 * 
 * This file is part of dlvhex-mcs-equilibrium-plugin.
 *
 * dlvhex-mcs-equilibrium-plugin is free software; 
 * you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dlvhex-mcs-equilibrium-plugin is distributed in the hope that it will
 *  be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dlvhex-dlplugin; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


/**
 * @file   ACC_ContextAtom.cpp
 * @author Markus Boegl
 * @date   Sun Jan 24 13:24:22 2010
 * 
 * @brief  Base Class for User implemented Context Atoms
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "ACC_ContextAtom.h"
#include "Timing.h"
#include <ostream>

namespace dlvhex {
  namespace mcsdiagexpl {

    void
    ACC_ContextAtom::retrieve(const Query& query, Answer& answer) throw (PluginError) {
      bool accept = false;
      std::set<std::string> oset,aset,bset,interset;
      std::set<std::set<std::string> > accset;

      /////////////////////////////////////////////////////////////////
      //
      // get the parameter out of the External Atom
      // get the belief set's out of the External Atom an fill in the
      // stringset's
      //
      /////////////////////////////////////////////////////////////////
      std::string param = query.getInputTuple()[4].getUnquotedString();
      convertQueryToStringSets(query,aset,bset,oset);

      /////////////////////////////////////////////////////////////////
      //
      // get accepted set of beliefsets
      //
      /////////////////////////////////////////////////////////////////

      if((Timing::getInstance())->isActive()) {
	(Timing::getInstance())->start(context_id);
      }
      accset = acc(param,bset);

      if((Timing::getInstance())->isActive()) {
	(Timing::getInstance())->stop(context_id);
      }
      /////////////////////////////////////////////////////////////////
      //
      // Iterate throw the accepted set's, 
      // build intersection with Output-beliefs
      // and compare to beliefs in Bridgerulebody
      // if there's at least one set equal, the answerset is accepted.
      //
      /////////////////////////////////////////////////////////////////
      for (std::set<std::set<std::string> >::iterator setit = accset.begin(); 
		((setit != accset.end()) && (!accept)); ++setit) {
	interset.clear();
	std::insert_iterator<std::set<std::string> > out_it(interset, interset.begin());
        set_intersection((*setit).begin(), (*setit).end(), oset.begin(), oset.end(), out_it);
        if (aset.size() == interset.size()) {
          if (equal(aset.begin(),aset.end(),interset.begin())) {
            accept = true;
          } 
        }
      }

      if (accept) {
        Tuple out;
	answer.addTuple(out);
      }
    } // end ACC_ContextAtom::retrieve()

  } // namespace mcsdiagexpl
} // namespace dlvhex
