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
 * @file   ParseContext.cpp
 * @author Markus Boegl
 * @date   Sun Jan 24 13:46:27 2010
 * 
 * @brief  Context element for Parsing the Input file
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "ParseContext.h"
#include "Global.h"
#include <ostream>


namespace dlvhex {
  namespace mcsdiagexpl {

   ParseContext::ParseContext(int num, std::string e, std::string p)
     : contextnum(num), extatom(e), param(p) {
   }//BridgeRuleEntry End

   ParseContext::ParseContext()
     : contextnum(0), extatom(""), param("") {
   }//BridgeRuleEntry End

   std::ostream&
   operator<< (std::ostream& out, const ParseContext& context) {
     const int cn = context.ContextNum();

     if( Global::getInstance()->isKR2010rewriting() )
     {
       // guess outputs
       out << "ma" << cn << "(X) v nma" << cn << "(X) :- o" << cn << "(X)." << std::endl;

       // check context with constraint
       out << ":- not &" << context.ExtAtom()
           << "[" << cn << ",ma" << cn << ",mb" << cn << ",o" << cn << ","
           << "\"" << context.Param() << "\"]()." << std::endl; 

       // mark context as existing
       out << "ctx(" << cn << ")." << std::endl;
     }
     else
     {
       // guess input and output beliefs (after previous context has been finished)

       // inputs
       out << "mb" << cn << "(X) v nmb" << cn << "(X) :- i" << cn << "(X)";
       out << ", ok(" << (cn-1) << ")";
       out << "." << std::endl;

       // outputs
       out << "ma" << cn << "(X) v nma" << cn << "(X) :- o" << cn << "(X)";
       out << ", ok(" << (cn-1) << ")";
       out << "." << std::endl;

       // context check
       out << "ok(" << cn << ") :- &" << context.ExtAtom() 
         << "[" << context.ContextNum()
         << ",ma" << context.ContextNum()
         << ",mb" << context.ContextNum() 
         << ",o" << context.ContextNum()
         << ",\"" << context.Param() << "\"]()";
       out << ", ok(" << (cn-1) << ")";
       out << "." << std::endl;

       // require that context check is successful
       out << ":- not ok(" << cn << ")." << std::endl;

       // verify guessed output with output calculated via bridge rules (if all contexts are ok)
       out << ":- mc" << cn << "(X), not mb" << cn << "(X), ok(all)." << std::endl;
       out << ":- not mc" << cn << "(X), mb" << cn << "(X), ok(all)." << std::endl;

       // mark context as existing
       out << "ctx(" << cn << ")." << std::endl;
     }
     return out;
   }

  } // namespace mcsdiagexpl
} // namespace dlvhex
