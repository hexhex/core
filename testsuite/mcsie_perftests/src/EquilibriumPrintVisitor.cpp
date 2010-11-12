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
 * @file   EquilibriumPrintVisitor.cpp
 * @author Markus Boegl
 * @date   Sun Jan 24 13:43:34 2010
 * 
 * @brief  PrintVisitor to go throught the Answersets and write as Equilibria
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "EquilibriumPrintVisitor.h"

#include "dlvhex/AtomSet.h"
#include <iostream>
#include <sstream>

//#define DEBUG

namespace dlvhex {
  namespace mcsdiagexpl {
////////////////////////////////////////////////////////////////////////////
// 
//	PrintVisitorMethods
//
////////////////////////////////////////////////////////////////////////////
EquilibriumPrintVisitor::EquilibriumPrintVisitor(std::ostream& s)
: RawPrintVisitor(s)
{ }

void
EquilibriumPrintVisitor::visit(const AtomSet* const as)
{
  // diagnosis output
  //typedef std::map<std::string, int> DiagMap;
  std::map<int, std::string> cmap;
  std::multimap<int,std::string> outlist; 
  std::string pred;
  int id=0;

  #ifdef DEBUG
    std::cerr << "Answerset: ";
		::dlvhex::RawPrintVisitor dbgVisitor(std::cerr);
		as->accept(dbgVisitor);
		std::cerr << std::endl;
    //stream << "D1 size: " << d1.size() << "  / D2 size: " << d2.size() << " / normal: " << normal.size() << std::endl;
  #endif

  //stream << '(';
  if (!as->empty()) {
    for (AtomSet::atomset_t::const_iterator a = as->atoms.begin(); a != as->atoms.end(); ++a) {
	// get predicate (we are interested in o<i> a<i> d1 d2)
	std::string pred;
	{
		std::stringstream s; s << (*a)->getPredicate();
		pred = s.str();
	}
	assert(!pred.empty());

	// get argument
	std::string arg;
	{
        	const Tuple& arguments = (*a)->getArguments();
		assert(arguments.size() == 1);
		std::stringstream s; s << arguments[0];
		arg = s.str();
	}

	// process pred
	if( pred[0] == 'a' or pred[0] == 'b') {
          std::stringstream s;
          s << pred.substr(1,std::string::npos);
          s >> id;

          // if "a<i>", add belief to output container
          if( pred[0] == 'a' && id > 0 )
          {
                  outlist.insert(std::make_pair(id,arg));
                  cmap.insert(std::make_pair(id,""));
          }
	}

        if( pred == "ctx" )
        {
          std::stringstream s;
          s << arg;
          s >> id;
          assert(id != 0);
          cmap.insert(std::make_pair(id,""));
        }
    } // for-loop over AtomSet's

	// display equilibrium
	stream << "(";
    	std::multimap<int,std::string>::iterator oit;
    	std::pair<std::multimap<int,std::string>::iterator,std::multimap<int,std::string>::iterator> rangeit;

	//stream << "cmapsize: " << cmap.size();
	for(std::map<int, std::string>::const_iterator it = cmap.begin();
            it != cmap.end(); ++it) {
		if( it != cmap.begin() )
                  stream << ",";
		stream << "{";
		if( outlist.count(it->first) > 0 ) {
			rangeit = outlist.equal_range(it->first);
			for (oit = rangeit.first; oit != rangeit.second; ) {
				stream << oit->second;
				if (++oit != rangeit.second)
				  stream << ",";
			}
		}
		stream << "}";
	}
	stream << ')';

  } // if empty
}//EquilibriumPrintVisitor::visit(AtomSet* const as)

}//namespace mcsdiagexpl
}//namespace dlvhex
// vim:ts=8:
