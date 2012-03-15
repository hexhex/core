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
 * @file   EquilibriumPrintVisitor.h
 * @author Markus Boegl
 * @date   Sun Jan 24 13:44:22 2010
 * 
 * @brief  PrintVisitor to go throught the Answersets and write as Equilibria
 */
#ifndef _DLVHEX_MCSDIAGEXPL_EQUILIBRIUMPRINTVISITOR_H_
#define _DLVHEX_MCSDIAGEXPL_EQUILIBRIUMPRINTVISITOR_H_

#include "dlvhex2/PrintVisitor.h"

namespace dlvhex {
  namespace mcsdiagexpl {

    class EquilibriumPrintVisitor : public RawPrintVisitor {
      public:
        explicit
        EquilibriumPrintVisitor(std::ostream&);

        /// outputs the Equilibrium in '{(a,b,c), (cd,bx)}' form
        virtual void
        visit(const AtomSet* const);
    };
  } // END namespace mcsdiagexpl
} // END namespace dlvhex

#endif // _DLVHEX_MCSDIAGEXPL_EQUILIBRIUMPRINTVISITOR_H_
