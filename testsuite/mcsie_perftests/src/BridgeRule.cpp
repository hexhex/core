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
 * @file   BridgeRule.cpp
 * @author Markus Boegl
 * @date   Sun Jan 24 13:31:57 2010
 * 
 * @brief  BridgeRule element for Parsing the Input file
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "BridgeRule.h"
#include "Global.h"

#include <ostream>
#include <list>


namespace dlvhex {
  namespace mcsdiagexpl {

  BridgeRule::BridgeRule() {
     fact = false;
   }

  BridgeRule::BridgeRule(bool f) {
     fact = f;
   }

   void
   BridgeRule::setHeadRule(std::string rid, int cid, std::string f) {
     ruleid = rid;
     head = BridgeRuleEntry(cid,f);
   } // end of BridgeRule::setHeadRule

   void
   BridgeRule::addBodyRule(int id, std::string f, bool n){
     BridgeRuleEntry elem = BridgeRuleEntry(id,f,n);
     body.push_back(elem);
   } // end of BridgeRule::addBodyRule

   void 
   BridgeRule::writeProgram(std::ostream& o) {
     // write bridgerule in asp form
     std::list<int> ilist;

     // mark outputs: OUT_i via "o<i>(belief)"
     for (std::vector<BridgeRuleEntry>::iterator it = body.begin(); it != body.end(); ++it) {
       const BridgeRuleEntry& elem = *it;
       o << "o" << elem << "." << std::endl;
     }

     if ((Global::getInstance())->isKR2010rewriting())
     {
       if ((Global::getInstance())->isSet()) {
       // Only print equilibria
         // output diagnosis disjunction
         o << "normal(" << ruleid << ") v d1(" << ruleid << ") v d2(" << ruleid << ")." << std::endl;
         // output d2 rule
         o << "b" << head << " :- d2(" << ruleid << ")." << std::endl;
         // output d1 rule
         o << "b" << head << " :- not d1(" << ruleid << ")";
         if (fact)
           o << "." << std::endl;
         else
           o << ", ";
       } else {
         o << "b" << head;
         if (fact)
           o << "." << std::endl;
         else
           o << " :- ";
       }
     }
     else
     {
       // mark inputs: IN_i via "i<i>(belief)"
       o << "i" << head << "." << std::endl;

       // BR evaluation (and diagnosis guessing) (after all contexts ok, indicated by ok(all))
       if ((Global::getInstance())->isSet())
       {
         // diagnosis guessing
         o << "normal(" << ruleid << ") v d1(" << ruleid << ") v d2(" << ruleid << ") :- ok(all)." << std::endl;
         // d2 rule
         o << "c" << head << " :- d2(" << ruleid << "), ok(all)." << std::endl;
         // d1 rule
         o << "c" << head << " :- not d1(" << ruleid << "), ok(all)";
         if (fact)
           o << "." << std::endl;
         else
           o << ", ";
       }
       else
       {
         // else only print equilibria
         o << "c" << head;
         if (fact)
           o << "." << std::endl;
         else
           o << " :- ";
       }
     }

     // output bridge rule body
     for (std::vector<BridgeRuleEntry>::iterator it = body.begin(); it != body.end(); ++it) {
       const BridgeRuleEntry& elem = *it;
       if (elem.Neg())
         o << "n";
       o << "a" << elem;
       if (it+1 != body.end())
         o << ", ";
       else
         o << "." << std::endl;
     }
   }
  } // namespace mcsdiagexpl
} // namespace dlvhex

// vim:ts=8:
