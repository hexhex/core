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
 * @file   BaseContextAtom.h
 * @author Markus Boegl
 * @date   Sun Jan 24 13:25:02 2010
 * 
 * @brief  Base Context Plugin Atom Element
 */
#ifndef _DLVHEX_MCSDIAGEXPL_BASECONTEXTATOM_H
#define _DLVHEX_MCSDIAGEXPL_BASECONTEXTATOM_H

#include <dlvhex/PluginInterface.h>
#include <sstream>

namespace dlvhex {
  namespace mcsdiagexpl {

    class BaseContextAtom : public PluginAtom {

      private:
        std::string atom_name;

      protected:
	int context_id;

      public:
        BaseContextAtom(std::string name): atom_name(name), context_id(-1) {
          addInputConstant();
          addInputPredicate();
          addInputPredicate();
          addInputPredicate();
          addInputConstant();
          setOutputArity(0);
        }

        std::string 
        getExtAtomName() { 
          return atom_name; 
        }

        virtual void retrieve(const Query& query, Answer& answer) throw (PluginError) = 0;

      protected:
        void
        convertAtomSetToStringSet(AtomSet& as, std::set<std::string>& sset) {
          for (AtomSet::const_iterator ai = as.begin(); ai != as.end(); ++ai) {
            sset.insert(((*ai).getArgument(1)).getString());
          }
        }

        void
        convertQueryToStringSets( const Query& query,
                                  std::set<std::string>& aset, 
                                  std::set<std::string>& bset, 
                                  std::set<std::string>& oset) 
        throw (PluginError) {

          int cid = query.getInputTuple()[0].getInt();
	  context_id = cid;

          const std::string& ai_match = query.getInputTuple()[1].getString();
          const std::string& bi_match = query.getInputTuple()[2].getString();
          const std::string& oi_match = query.getInputTuple()[3].getString();

          AtomSet input = query.getInterpretation();

          AtomSet a;
          input.matchPredicate(ai_match,a);
          AtomSet b;
          input.matchPredicate(bi_match,b);
          AtomSet o;
          input.matchPredicate(oi_match,o);

          convertAtomSetToStringSet(a,aset);
          convertAtomSetToStringSet(b,bset);
          convertAtomSetToStringSet(o,oset);

        }

    };

  } // namespace mcsdiagexpl
} // namespace dlvhex

#endif // _DLVHEX_MCSDIAGEXPL_BASECONTEXTATOM_H
