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
 * @file   BridgeRuleEntry.cpp
 * @author Markus Boegl
 * @date   Sun Jan 24 13:30:12 2010
 * 
 * @brief  BridgeRuleEntry element as elements in BridgeRules
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "BridgeRuleEntry.h"
#include <ostream>

namespace dlvhex {
  namespace mcsdiagexpl {

   BridgeRuleEntry::BridgeRuleEntry(int id, std::string f, bool n)
    : contextid(id), fact(f), neg(n) {
   }//BridgeRuleEntry End

   BridgeRuleEntry::BridgeRuleEntry()
     : contextid(0), fact(""), neg(false) {
   }//BridgeRuleEntry End

   std::ostream&
   operator<< (std::ostream& out, const BridgeRuleEntry& ruleentry) {
     out << ruleentry.ContextID() << "(" << ruleentry.Fact() << ")";
     return out;
   }

  } // namespace mcsdiagexpl
} // namespace dlvhex
