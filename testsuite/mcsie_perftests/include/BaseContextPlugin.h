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
 * @file   BaseContextPlugin.h
 * @author Markus Boegl
 * @date   Sun Jan 24 13:26:52 2010
 * 
 * @brief  Base Context Plugin Element
 */
#ifndef _DLVHEX_MCSDIAGEXPL_BASECONTEXTPLUGIN_H
#define _DLVHEX_MCSDIAGEXPL_BASECONTEXTPLUGIN_H

#include <dlvhex/PluginInterface.h>

namespace dlvhex {
  namespace mcsdiagexpl {

    class BaseContextPlugin : public PluginInterface {
      private:
        AtomFunctionMap *a_int;

      public:
        BaseContextPlugin() {};

	template <class type> void registerAtom() {
           boost::shared_ptr<dlvhex::mcsdiagexpl::BaseContextAtom> atom(new type());
           (*a_int)[(atom.get())->getExtAtomName()] = atom;
	};

	// User-Defined Atoms are registered in this Function
	// use the registerAtom<AtomType>(); function above.
	virtual void registerAtoms() = 0;

	void getAtoms(AtomFunctionMap& a) {
          a_int = &a;
	  registerAtoms();
	};
    };
  } // namespace mcsdiagexpl
} // namespace dlvhex
#endif // _DLVHEX_MCSDIAGEXPL_GENERICCONTEXTATOM_H
